// yass: Yet Another Soma Solver
// Copyright (C) 2021 Mark R. Rubin aka "thanks4opensource"
//
// This file is part of yass.
//
// The yass program is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// The yass program is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// (LICENSE.txt) along with the yass program.  If not, see
// <https://www.gnu.org/licenses/gpl.html>


#include <assert.h>
#include <map>
#include <set>

#include "rotators.hxx"
#include "shape.hxx"

#include "piece.hxx"



namespace soma {

// public class data   ========================================================

#define P  Position
Piece
    Piece::corner(3, P( 1, 0, 0), P( 0, 1, 0), P( 0, 0, 1), 'c', 1),
    Piece::   pos(3, P( 1, 0, 0), P( 1, 1, 0), P( 0, 0, 1), 'p', 2),
    Piece::   neg(3, P(-1, 0, 0), P(-1, 1, 0), P( 0, 0, 1), 'n', 3),
    Piece::   zee(3, P( 1, 1, 0), P( 0, 1, 0), P(-1, 0, 0), 'z', 4),
    Piece::   tee(3, P( 1, 0, 0), P( 0, 1, 0), P(-1, 0, 0), 't', 5),
    Piece::   ell(3, P( 1, 1, 0), P( 1, 0, 0), P(-1, 0, 0), 'l', 6),
    Piece:: three(2, P( 1, 0, 0), P( 0, 1, 0), P( 0, 0, 0), '3', 7);
#undef P

const std::map<char, Piece*>    Piece::PIECE_NAMES = { {'c', &Piece::corner},
                                                       {'p', &Piece::pos   },
                                                       {'n', &Piece::neg   },
                                                       {'z', &Piece::zee   },
                                                       {'t', &Piece::tee   },
                                                       {'l', &Piece::ell   },
                                                       {'3', &Piece::three } };

                  // must match above
const char    Piece::CODE_TO_NAME[Piece::NUMBER_OF_PIECES + 1]
            = {'#', 'c', 'p', 'n', 'z', 't', 'l', '3'};


// See piece.hxx
Piece::Piece(
int                 number_of_cubes,
const Position      &cube_0        ,
const Position      &cube_1        ,
const Position      &cube_2        ,
char                name           ,
uint8_t             code           )
:   _number_of_cubes    (number_of_cubes ),
    _name               (name            ),
    _code               (code            ),
    _pre_placed         (false           ),
    _current_position   (-1              ),
    _current_orientation(0               )
#ifdef SOMA_STATISTICS
                                          ,
     _place_successes   (0               ),
     _place_failures    (0               ),
     _place_duplicates  (0               ),
     _place_orphans     (0               ),
     _total_valid_orients(0              )
#endif

{
    // GCC can't handle this in colon initializer list
    _cubes[0] = cube_0;
    _cubes[1] = cube_1;
    _cubes[2] = cube_2;
}   // Piece()



// See piece.hxx
void Piece::generate_orientations()
{
    // +1 because includes central cube
    std::array<Position, MAX_NUMBER_OF_CUBES + 1>   centered_cubes;

    // Initialize and center
    centered_cubes[0] = Position(0, 0, 0);  // central cube
    // other cubes
    for (unsigned ndx = 0 ; ndx < _number_of_cubes ; ++ndx)
        centered_cubes[ndx + 1] = _cubes[ndx];
    Position    maxes = Position::normalize<Position,
                                            std::array<Position,
                                                         MAX_NUMBER_OF_CUBES
                                                       + 1>>(
                        centered_cubes, MAX_NUMBER_OF_CUBES + 1);

    Position::center<Position, std::array<Position, MAX_NUMBER_OF_CUBES + 1>>
    (centered_cubes, MAX_NUMBER_OF_CUBES + 1, maxes, false);

    // Fill _orientations with only unique (not -rotated/mirrored duplicates)
    std::vector<unsigned>               rotators_mirrorers;
    std::set<std::set<Position>>        uniques;
    // Can't do range-based for on Rotators::rotations
    //   because has plain, z-, and x-mirrored versions
    for (unsigned     rotation_ndx = 0                                    ;
                      rotation_ndx < Rotators::MAX_NUMBER_OF_ORIENTATIONS ;
                    ++rotation_ndx                                         ) {
        const auto  rotation = Rotators::rotations[rotation_ndx];

        // A rotated/mirrored orientation
        Cubes       rotated_centereds;
        for (unsigned     cube_ndx = 0                    ;
                          cube_ndx < _number_of_cubes + 1 ;
                        ++cube_ndx                         ) {
              rotated_centereds[cube_ndx]
            = centered_cubes[cube_ndx].rotate(rotation);
        }

        // For testing against already-generated
        std::set<Position>  unordered;
        for (const Position &cube : rotated_centereds)
            unordered.insert(cube);

        // See if duplicate of already generated
        unsigned    before = uniques.size();
        uniques.insert(unordered);
        if (uniques.size() != before)
            rotators_mirrorers.push_back(rotation_ndx);
    }

    // Copy uniques into _orientations
    for (unsigned rotator : rotators_mirrorers) {
        Cubes   rotated;
        rotate(rotated                     ,
               _cubes                      ,
               Rotators::rotations[rotator],
               _number_of_cubes            );
        _orientations.push_back(rotated);
    }

    _current_orientation = 0;

}  // generate_orientations()



// See piece.hxx
void Piece::set_valid_orientations(
const unsigned  cubicle_ndx ,
const unsigned  piece_number)
{
    _valid_orientations[cubicle_ndx].clear();

    for (unsigned     orientation_ndx = 0                    ;
                      orientation_ndx < _orientations.size() ;
                    ++orientation_ndx                         )
        // Is valid only if fits into empty (no other pieces) shape
        if (_shape->place_piece(cubicle_ndx                          ,
                                this                                 ,
                                piece_number                         ,
                                _number_of_cubes                     ,
                                _orientations[orientation_ndx].data(),
                                true                                 )) {
            _valid_orientations[cubicle_ndx].push_back(orientation_ndx);
        }
#ifdef SOMA_STATISTICS
    _total_valid_orients += _valid_orientations[cubicle_ndx].size();
#endif
}



// See piece.hxx
bool Piece::place(
unsigned    piece_number    ,
bool        check_orphans   ,
bool        check_duplicates)
{
    // Do nothing if pre-placed, but still need to keep track of
    //   "placed" vs non for forward and backtracking in solution tree space.
    if (is_pre_placed()) {
        if (_current_orientation == 0) {
            ++_current_orientation;
            return true;
        }
        else {
            _current_orientation = 0;
            return false;
        }
    }


    // First or next position/orientation
    //
    if (is_placed()) {
        _shape->remove_piece(this, piece_number);
        if (!place_next()) {
            _current_position = -1;
            return false;
        }
    }
    else {
        _current_position = _shape->first_free();
        _current_orientation = 0;

        if (_current_position >= static_cast<int>(Shape::NUMBER_OF_CUBICLES)) {
            _current_position = -1;
            return false;
        }
    }


    // Try to place until success or know failure
    while (true) {
        // Repeatedly try to place until success or place_next() finished
        while (   _valid_orientations[_current_position].size() == 0
               || !_shape->place_piece(_current_position,
                                       this                                ,
                                       piece_number                        ,
                                       _number_of_cubes                    ,
                                       _orientations[
                                         _valid_orientations[
                                           _current_position][
                                             _current_orientation]].data())) {
            if (!place_next())
                return false;   // no more positions/orientations to try
        }

        // Has been placed, but might need to remove
        //

        // Either condition (if enabled and true) causes removal
        bool    is_duplicate = false,
                has_orphan   = false;

        // Do duplicate check first because is faster than orphan check
        if (check_duplicates) {
            if (!(is_duplicate = _shape->is_duplicate_solution(piece_number)))
                _shape->add_solution(piece_number);
#ifdef SOMA_STATISTICS
            else
                ++_place_duplicates;
#endif
        }

        // No need to check for orphans if already known to be duplicate
        if (!is_duplicate && check_orphans && _shape->has_orphan()) {
            has_orphan = true;
#ifdef SOMA_STATISTICS
            ++_place_orphans;
#endif
        }

        if (is_duplicate || has_orphan) {
            _shape->remove_piece(this, piece_number);
#ifdef SOMA_STATISTICS
            ++_place_failures;
#endif

            // See if any more positins/orientations
            if (!place_next())
                return false;
        }
        else
            break;
    }

#ifdef SOMA_STATISTICS
    ++_place_successes;
#endif

    return true;   // piece is now placed in shape

}   // place()



// Set next _current_position and/or _current_orientation
bool Piece::place_next()
{
    // Increment to next _current_orientation
    if (   ++_current_orientation
        >= _valid_orientations[_current_position].size()) {
        // At end of _valid_orientations, reset to first and
        //   go to next position
        _current_orientation = 0                                   ;
        _current_position    = _shape->next_free(_current_position);

        if (_current_position >= static_cast<int>(Shape::NUMBER_OF_CUBICLES)) {
            // No more valid positions, reset and return failure
            _current_position    = -1;
            _current_orientation =  0;
            return false;
        }
    }

    return true;

}   // place_next()

}  // namespace soma
