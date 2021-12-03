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


#ifndef PIECE_H
#define PIECE_H

#include <limits>
#include <map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "position.hxx"
#include "rotators.hxx"

#if SOMA_MATRIX_ROTATION + SOMA_LAMBDA_ROTATION != 1
#error #define one and only one of SOMA_MATRIX_ROTATION or SOMA_LAMBDA_ROTATION
#endif



namespace soma {


class   Shape;



class Piece {

  public:
                                            // in addition to central cube
    static const unsigned                   MAX_NUMBER_OF_CUBES  = 3,
                                            NUMBER_OF_PIECES     = 7;
    static const std::map<char, Piece*>     PIECE_NAMES             ;

    // Used to set Shape::NUMBER_OF_CUBICLES.
    // Architecturally belongs in Shape class, but shape.hxx includes
    //   piece.hxx and files can't circularly include each other.
    // Needed here for _valid_orientations std::array template parameter.
    static const unsigned   NUMBER_OF_SHAPE_CUBICLES = 27;

    static Piece    corner,
                    pos   ,
                    neg   ,
                    zee   ,
                    tee   ,
                    ell   ,
                    three ;

    Piece(int                number_of_cubes,   // other than central cube
          const Position    &cube_0         ,   // non-central cube
          const Position    &cube_1         ,   //  " -   "     "
          const Position    &cube_2         ,   //  " -   "     "
          const char         name           ,   // client code / user visible
          const uint8_t      code           );  // internal use


    // See _orientations and _valid_orientations member variables
    void set_valid_orientations(const unsigned  cubicle_ndx ,
                                const unsigned  piece_number);

    // full reset, for new Shape
    void reset()
    {
        reset_position_orientation();
        _pre_placed = false;
    }

    // partial reset when backtracking failed placement in solution tree
    void reset_position_orientation()
    {
        _current_position    = -1;
        _current_orientation =  0 ;
    }

    // Initialize unique-considering-symmetries _orientations
    // Can't be done in or called from constructor because
    //   #ifdef SOMA_LAMBDA_ROTATION version of Rotators::rotations
    //   needs pre-main static initialization as do static Piece instances
    //   and former may be done after latter
    void    generate_orientations();

    // See _shape member variable
    void    register_shape(Shape *shape) {_shape = shape; }

    // For user-facing Shape::write()
    static char code2name(
    const unsigned code)
    {
        return CODE_TO_NAME[code];
    }

    constexpr char      name() const {return _name               ; }
    constexpr unsigned  code() const {return _code               ; }
    constexpr unsigned  size() const {return _number_of_cubes + 1; }

    // See member variables
    void pre_place()
    {
        _current_orientation    = 0   ;
        _pre_placed             = true;
    }

    bool     is_pre_placed      () const {return _pre_placed            ; }
    bool     is_placed          () const { return _current_position >= 0; }

    // Main solver algorithm
    // Repeatedly calls Shape::place_piece() using own internal state
    //   _current_position and _current_orientation.
    // If successful, piece has been placed in shape, and returns true.
    // Otherwise uses place_next() until no more valid positions/orientations
    //   and returns false.
    bool    place(unsigned  piece_number    ,
                  bool      check_orphans   ,
                  bool      check_duplicates);

#ifdef SOMA_STATISTICS
    unsigned    num_orientations   () const { return _orientations.size() ; }
    unsigned    place_successes    () const { return _place_successes     ; }
    unsigned    place_failures     () const { return _place_failures      ; }
    unsigned    place_duplicates   () const { return _place_duplicates    ; }
    unsigned    place_orphans      () const { return _place_orphans       ; }
    unsigned num_valid_orientations() const { return _total_valid_orients ; }
#endif



  protected:
    static const char       CODE_TO_NAME[NUMBER_OF_PIECES + 1];

    // +1 is for generate_orientation() with central cube
    using Cubes = std::array<Position, MAX_NUMBER_OF_CUBES + 1>;

    // See piece.cxx
    bool    place_next();


    // Rotate piece
    //
#ifdef SOMA_MATRIX_ROTATION
    static inline void rotate(
          Cubes     &rotated_cubes  ,
    const Cubes     &original_cubes ,
    const int        matrix[3][3]   ,
    const unsigned   number_of_cubes)
    {
        for (unsigned ndx = 0; ndx < number_of_cubes; ndx++)
            rotated_cubes[ndx] = original_cubes[ndx] * matrix;
    }
#endif

#ifdef SOMA_LAMBDA_ROTATION
    static void rotate(
          Cubes      &rotated_cubes           ,
    const Cubes      &original_cubes          ,
          Position  (*rotator)(const Position),
    const unsigned    number_of_cubes         )
    {
        for (unsigned ndx = 0; ndx < number_of_cubes; ndx++)
            rotated_cubes[ndx] = rotator(original_cubes[ndx]);
    }
#endif

    // Member variables
    //

    // Does not include central cube (center of rotations/orientations,
    //   and location in shape at _current_position
    unsigned             _number_of_cubes    ;   // besides central cube
    Cubes                _cubes              ;   // other than central cube

    const char           _name               ;   // user/client visible
    const uint8_t        _code               ;   // internal use

    // Unique (non-rotated/mirrored symmetric) rotations
    std::vector<Cubes>   _orientations;

    // Subset of _orientations, per-cubicle in shape (no need to
    //   repeatedly try other orientations during recursive tree solving
    //   if piece will not fit in shape regardless of other, previously
    //   placed pieces).
    std::array<std::vector<unsigned>, NUMBER_OF_SHAPE_CUBICLES>
                         _valid_orientations;

    // For communication with Shape
    Shape               *_shape;

    // Piece is in fixed, user-specified position and orientation
    bool                 _pre_placed;

    // Current state of piece (if currently) in shape
    // int and -1 flag for _current_position instead of unsigned and
    //   static const int to use fast  machine instruction negative test
    int                  _current_position   ;    // cubicle 0 to 26, or -1
    unsigned             _current_orientation;    // index into _orientations

#ifdef SOMA_STATISTICS
    unsigned             _place_successes    ,
                         _place_failures     ,
                         _place_duplicates   ,
                         _place_orphans      ,
                         _total_valid_orients;
#endif

};   // class Piece

}  // namespace soma

#endif   // ifndef PIECE_H
