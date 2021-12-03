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


#include <iomanip>   // DEBUG

#include "piece.hxx"
#include "shape.hxx"

#include "soma.hxx"



namespace soma {

const std::string   Soma::DEFAULT_PIECE_ORDER("ztcpnl3");
const unsigned      Soma::DEFAULT_P_PIECE_NDX(3        ),  // must match above
                    Soma::DEFAULT_N_PIECE_NDX(4        );  //  "     "     "



// See soma.hxx
Soma::Soma(
const unsigned       orphan_checks   ,
const unsigned       duplicate_checks,
const unsigned       symmetry_checks ,
const std::string   &piece_order_str )
:   _orphan_checks   (orphan_checks      ),
    _duplicate_checks(duplicate_checks   ),
    _symmetry_checks (symmetry_checks    ),
    _dup_chks_adjstd (duplicate_checks   ),
    _sym_chks_adjstd (symmetry_checks    ),
    _active_piece    (0                  ),
    _p_piece_ndx     (DEFAULT_P_PIECE_NDX),
    _n_piece_ndx     (DEFAULT_N_PIECE_NDX)
{
    piece_order(piece_order_str);

    for (Piece *piece : _pieces) {
        piece->register_shape(&_shape);
        piece->generate_orientations();
    }
}



bool Soma::piece_order(
const std::string   &pieces_str)
{
    _piece_order = pieces_str;

    bool    error = (pieces_str.size() != Piece::NUMBER_OF_PIECES);
    if (!error)
        for (unsigned ndx = 0 ; ndx < Piece::NUMBER_OF_PIECES; ++ndx) {
            auto    piece = Piece::PIECE_NAMES.find(pieces_str[ndx]);
            if (piece == Piece::PIECE_NAMES.end()) {
                error = true;
                break;
            }
            _pieces[ndx] = piece->second;
            switch (piece->first) {
                case 'p': _p_piece_ndx = ndx; break;
                case 'n': _n_piece_ndx = ndx; break;
                default:                      break;
            }
        }

    if (error) {
        // set default order
        unsigned    ndx = 0;
        for (const char piece_char : DEFAULT_PIECE_ORDER)
                             // find() can't fail
            _pieces[ndx++] = Piece::PIECE_NAMES.find(piece_char)->second;
        _p_piece_ndx = DEFAULT_P_PIECE_NDX;
        _n_piece_ndx = DEFAULT_N_PIECE_NDX;
        return false;
    }

    return true;
}



// See soma.hxx
void Soma::reset()
{
    _shape.reset();

    for (Piece  *piece : _pieces)
        piece->reset();

    _active_piece = 0;
}



// See soma.hxx
bool Soma::read(
std::istream    &input ,
std::ostream    *errors)
{
    reset();

    if (!_shape.read(input, errors))
        return false;

    return init_shape(errors);
}



// See soma.hxx
bool Soma::shape(
const std::array<int, Shape::NUMBER_OF_CUBICLES * 3>     coords,
const std::string                                        pieces,
std::ostream                                            *errors)
{
    reset();

    if (!_shape.specify(coords, pieces, errors))
        return false;

    return init_shape(errors);
}



// See soma.hxx
bool Soma::solve()
{
    bool    is_last_piece = _active_piece == Piece::NUMBER_OF_PIECES - 1;

    if (is_last_piece)
        post_solve();

    // Traverse solution tree space (forward and recursing backward)
    // Returns true when first/next solution found,
    while (true) {  // explicit return statements inside loop
        is_last_piece = _active_piece == Piece::NUMBER_OF_PIECES - 1;

        // Checks to do for current piece
        bool    check_orphan   ,
                check_duplicate,
                check_symmetry ;

        // Need to do this because can't rely on Piece::place() to
        // do check_duplicate==true if last piece is pre-placed
        if (is_last_piece)
            check_orphan = check_duplicate = check_symmetry = false;
        else {
            // normal
            check_orphan    =   _orphan_checks & (1 << _active_piece),
            check_duplicate = _dup_chks_adjstd & (1 << _active_piece),
            check_symmetry  = _sym_chks_adjstd & (1 << _active_piece);
        }

        // Try to place piece
        if ( _pieces[_active_piece]->place(_active_piece  ,
                                           check_orphan   ,
                                           check_duplicate)) {
            // Found solution
            //
            if (is_last_piece) {
                if (!(_dup_chks_adjstd & (1 << (Piece::NUMBER_OF_PIECES - 1))))
                    // Not checking for duplicates
                    return true;
                if (!_shape.is_duplicate_solution(_active_piece)) {
                    // Is not duplicate
                    _shape.add_solution(_active_piece);
                    return true;
                }
                // Is duplicate, recurse to previous piece
                post_solve();
            }
            else {
                ++_active_piece;  // try next piece
                if (check_duplicate) {
                    // Clear possible existing duplicate solutions (from
                    //   backtracking in recursive solve)
                    //
                    // Special cases for mutually mirrored pieces p,n pieces
                    if (_active_piece == _p_piece_ndx) {
                        _shape.clear_solutions(_p_piece_ndx);
                        _shape.clear_solutions(_n_piece_ndx);
                    }
                    else if (_active_piece != _n_piece_ndx)
                        _shape.clear_solutions(_active_piece);
                }

                // For symmetry checking
                _shape.set_statuses(_active_piece                          ,
                                    _pieces[_active_piece]->name()         ,
                                    _sym_chks_adjstd & (1 << _active_piece));
            }
        }
        else {  // failed to place piece
            if (_active_piece == 0)
                // Have rewound search tree to beginning, no (more) solutions
                return false;

            // Decrement to Try next orientation/position of previous piece.
            // Reset symmetry checking.
            _shape.restore_statuses(--_active_piece);
        }
    }

}  // solve()



// After read() or shape() of new shape to solve.
bool Soma::init_shape(
std::ostream    *errors)
{
    if (!check_preplaced(errors))
        return false;

    // Valid orienatations are subset of each piece's orientations on
    //   per-shape-cubicle basis because no need to keep checking
    //   an orientation at each step of recursive solve if piece cannot
    //   fit into shape at a cubicle regardless of other pieces placed
    //   in shape or not.
    for (unsigned     piece_ndx = 0                       ;
                      piece_ndx < Piece::NUMBER_OF_PIECES ;
                    ++piece_ndx                            ) {
        if (!_pieces[piece_ndx]->is_pre_placed())
            for (unsigned     cubicle_ndx = 0                         ;
                              cubicle_ndx < Shape::NUMBER_OF_CUBICLES ;
                            ++cubicle_ndx                              )
                _pieces[piece_ndx]->set_valid_orientations(cubicle_ndx,
                                                           piece_ndx  );
    }

    // Need if checking either at any piece number
    if (_duplicate_checks != 0 || _symmetry_checks != 0) {
        if (!_shape.generate_rotator_reflectors(errors))
            return false;
    }

    // Have to handle edge cases separated shapes.
    if (_shape.num_children() == 1) {
        _dup_chks_adjstd = _duplicate_checks;
        _sym_chks_adjstd = _symmetry_checks ;
    }
    else {
        // turn off all but last piece
        _dup_chks_adjstd = 1 | 1 << 6;  // force like "-D 17" commandline option
        _sym_chks_adjstd = 0         ;  // turn off all
    }

    // For first piece
    _shape.set_statuses(0                                      ,
                        _pieces[0]->name()                     ,
                        static_cast<bool>(_sym_chks_adjstd & 1));

    return true;
}  // init_shape(std::ostream*)



// Simplistic check. Only confirms  correct number of cubes per pre-placed
//   pieces, not correct piece shape.
// Solve will succeed if incorrect piece shape but be meaningless.
//
bool Soma::check_preplaced(
std::ostream    *errors)
{
    bool    result = true;

    for (const Piece *piece : _pieces) {
        unsigned    count;
        if (   piece->is_pre_placed()
            && (count = _shape.num_piece_cubicles(piece)) != piece->size()) {
            if (errors)
                *errors << "Pre-placed piece '"
                        << piece->name()
                        << "' has "
                        << count
                        << " cubes instead of correct "
                        << piece->size()
                        << std::endl;
            result = false;
        }
    }

    return result;
}



// Backtrack in recursive solution tree
void Soma::post_solve()
{

    // Go back skipping over pre-placed pieces (if any)
    do {
        _shape.restore_statuses(_active_piece);
        _shape.reset_piece(_pieces[_active_piece], _active_piece);
    } while (   _pieces[_active_piece--]->is_pre_placed()
             && _active_piece > 0                        );
    _shape.restore_statuses(_active_piece);

}

}  // namespace soma
