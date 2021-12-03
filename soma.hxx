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


#ifndef SOMA_HXX
#define SOMA_HXX

#include <array>
#include <iostream>

#include "piece.hxx"
#include "shape.hxx"


namespace soma {


//  SOMA solver engine
//
class Soma {
  public:
    // Versioning
    // Client code should check:
    //    static_assert(   Soma::MAJOR_VERSION == CLIENT_MAJOR_VERSION
    //                  && Soma::MINOR_VERSION >= CLIENT_MINOR_VERSION,
    //                  "<error message>"                             );
    static const unsigned   MAJOR_VERSION = 0,
                            MINOR_VERSION = 0,
                            MICRO_VERSION = 0;

    // Client code can override
    static const std::string    DEFAULT_PIECE_ORDER;

    // Arguments set performance optimizations
    // See EXTENDED_HELP_TEXT in file main.cxx
    Soma(const unsigned        orphan_checks,   // bits 0..6 match piece numbers
         const unsigned     duplicate_checks,   //  "   ".."   "     "      "
         const unsigned      symmetry_checks,   //  "   ".."   "     "      "
         const std::string  &piece_order_str);  // Piece::name()

    ~Soma() {}

    // Client must call between solves of different SOMA figures
    // of new SOMA figure
    void    reset();

    // Read a  SOMA figure from stream
    // See EXTENDED_HELP_TEXT in file main.cxx for file format
    bool    read(std::istream   &input     ,
                 std::ostream   *errors = 0);  // optional error message output

    // API to set SOMA figure shape before solve()
    //  coords:  flat array of x0,y0,z0,x1,...,x26,y26,z26 coordinates
    //  pieces:  1-to-1 with first pieces.size() coordinates, 'o' for
    //           default normal to-be-solved cubicles, one of "cpnztl3"
    //           for pre-placed pieces, empty string OK
    //  errors:  error message output if non-NULL
    bool    shape(const std::array<int, Shape::NUMBER_OF_CUBICLES * 3>  coords,
                  const std::string                                     pieces,
                  std::ostream                                     *errors = 0);

    // Returns true on successful solve.
    // Client can call repeatedly for multiple solutions of same SOMA figure.
    bool    solve();

    // Output solution(s) to user
    void    print(std::ostream  &output) { _shape.write(output); }

    // For API use (vs. main.cxx/commandline program)
    // See print_api() in main.cxx
    std::array<char, Shape::NUMBER_OF_CUBICLES>
    solution(std::array<int, Shape::NUMBER_OF_CUBICLES * 3> &coords)
    const
    {
        return _shape.solution(coords);
    }

    // Get current configuration
    unsigned    orphans    () const { return _orphan_checks   ; }
    unsigned    duplicates () const { return _duplicate_checks; }
    unsigned    symmetries () const { return _symmetry_checks ; }
    std::string piece_order() const { return _piece_order     ; }

    // Change configuration of existing object.
    // Only change before or immediately after reset() (or initial object
    //   construction), not between read() or shape() and solve(), or
    //   between repeated calls to solve().
    void    orphans    (const unsigned setting) { _orphan_checks    = setting; }
    void    duplicates (const unsigned setting) { _duplicate_checks = setting; }
    void    symmetries (const unsigned setting) { _symmetry_checks  = setting; }
    bool    piece_order(const std::string&    );

#ifdef SOMA_STATISTICS
    char piece_name(
    unsigned piece_number)
    const
    {
        return _pieces[piece_number]->name();
    }

#define SOMA_PRINT_SHAPE_STATS(NAME)        \
    unsigned NAME(                          \
    unsigned    piece_number)               \
    const                                   \
    {                                       \
        return _shape.NAME(piece_number);   \
    }

    SOMA_PRINT_SHAPE_STATS(statuses_uniques   )
    SOMA_PRINT_SHAPE_STATS(statuses_duplicates)
#undef SOMA_PRINT_SHAPE_STATS

#define SOMA_PRINT_PIECE_STATS(NAME)            \
    unsigned NAME(                              \
    unsigned    piece_number)                   \
    const                                       \
    {                                           \
        return _pieces[piece_number]->NAME();   \
    }

    SOMA_PRINT_PIECE_STATS(num_orientations      )
    SOMA_PRINT_PIECE_STATS(num_valid_orientations)
    SOMA_PRINT_PIECE_STATS(place_successes       )
    SOMA_PRINT_PIECE_STATS(place_failures        )
    SOMA_PRINT_PIECE_STATS(place_duplicates      )
    SOMA_PRINT_PIECE_STATS(place_orphans         )
#undef SOMA_PRINT_SHAPE_STATS
#undef SOMA_PRINT_PIECE_STATS
#endif



  protected:
    // defined in soma.cxx to be next to and match DEFAULT_PIECE_ORDER
    static const unsigned       DEFAULT_P_PIECE_NDX,
                                DEFAULT_N_PIECE_NDX;


    // See implementations in file soma.cxx
    //

    bool        init_shape(std::ostream *errors = 0);

    bool        check_preplaced(std::ostream    *errors = 0);

    void        post_solve();

    std::array<Piece*, Piece::NUMBER_OF_PIECES>     _pieces;
    Shape       _shape           ;
    unsigned    _orphan_checks   ,  // see EXTENDED_HELP_TEXT
                _duplicate_checks,  //   in file main.cxx
                _symmetry_checks ,
                _dup_chks_adjstd ,  // forced to just 7 if shape has children
                _sym_chks_adjstd ;  //   "    "   "   0 "    "    "     "
    std::string _piece_order     ;  // see EXTENDED_HELP_TEXT in main.cxx
    unsigned    _active_piece    ,  // state of recursive tree solve
                _p_piece_ndx     ,  // for special case duplicate checks of
                _n_piece_ndx     ;  //   these two mutually-mirrored pieces
};

}  // namespace soma

#endif  // ifndef SOMA_HXX
