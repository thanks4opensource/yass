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


#ifndef SHAPE_H
#define SHAPE_H

#include <array>
#include <iostream>
#ifdef SOMA_STD_SET_SET
#include <set>
#endif
#ifdef SOMA_STD_SET_UNORDERED
#include <unordered_set>
#endif
#include <vector>

#include "piece.hxx"
#include "position.hxx"
#include "rotators.hxx"
#include "signature.hxx"

#if SOMA_STD_SET_SET + SOMA_STD_SET_UNORDERED == 0
#warning using #define SOMA_STD_SET_SET (not #define SOMA_STD_SET_UNORDERED)
#define SOMA_STD_SET_SET
#endif

#if SOMA_STD_SET_SET + SOMA_STD_SET_UNORDERED != 1
#error #define one and only one of SOMA_STD_SET_SET or SOMA_STD_SET_UNORDERED
#endif



namespace soma {


class Piece;



class Shape {

  public:
    // Architecturally belongs here, but piece.hxx needs for member
    //   variable template paramater and is included here (and
    //   not vice-versa) so cannot use if singly declared/defined here.
    static const unsigned         NUMBER_OF_CUBICLES
                                = Piece::NUMBER_OF_SHAPE_CUBICLES;

    // Constants for accessing Cubicle::ortho_adjacents
    // Pseudo-namespace and unsigned instead of enum class to avoid
    //   need to cast when using.
    struct OrthAdj {
        static const unsigned   UP    = 0,
                                DOWN  = 1,
                                FRONT = 2,
                                BACK  = 3,
                                LEFT  = 4,
                                RIGHT = 5;
    };

    Shape(unsigned  number_of_cubicles = NUMBER_OF_CUBICLES);

    ~Shape() { reset(); }

    // Reset for new SOMA shape solve
    void    reset();

    // See EXTENDED_HELP_TEXT in file main.cxx for file format
    bool        read(std::istream   &input     ,
                     std::ostream   *errors = 0);

    // See Soma::shape() as called from read_pieces_file() in main.cxx
    bool        specify(const std::array<int, NUMBER_OF_CUBICLES * 3> coords,
                        const std::string                             pieces,
                              std::ostream                       *errors = 0);

    // For simplistic check of pre-placed pieces in Soma::check_preplaced()
    unsigned    num_piece_cubicles(const Piece  *piece);

    // Human-readable output to stream
    void        write(std::ostream&) const;

    // API for client use (instead of write())
    // Returns 1-to-1 matching arrays, pieces[ndx] <-> coords[ndx*3+0,1,2]
    std::array<char, Shape::NUMBER_OF_CUBICLES>
    solution(std::array<int, Shape::NUMBER_OF_CUBICLES * 3> &coords) const;

    unsigned    num_children() const { return _children.size(); }

    // See _rotators_mirrorers
    // Optionally called by Soma::init_shape() if culling
    //   rotated/mirrored solutions
    bool    generate_rotator_reflectors(std::ostream    *errors);

    // Check against already found solutions in _solution_sets[piece_number]
    // Partial solutions if piece_number < 6, full solutions if == 6
    // See _solution_sets
    bool    is_duplicate_solution(const unsigned    piece_number);

    // Add all valid rotations/reflections of current solution
    //   to _solution_sets[piece_number]
    // If shape has separated child shapes (_children.size() > 1) child
    //   solutions are calculated individually and concatenated in
    //   all possible combinations.
    // Can be called after all or any subset of pieces have been
    //   place in shape (piece_number 0 to 6)
    void    add_solution(const unsigned     piece_number);


    // Used by Soma::solve() when backtracking in solution tree space.
    void clear_solutions(
    const unsigned   piece_number)
    {
        _solutions_sets[piece_number].clear();
    }

    // Attempt to place piece, center cubicle of piece at cubicle_ndx
    // Used by Piece::place()
    bool    place_piece(const unsigned          cubicle_ndx      ,
                              Piece*   const    piece            ,
                        const unsigned          piece_number     ,
                        const unsigned          number_of_cubes  ,
                        const Position          cubes[]          ,
                        const bool              just_test = false);

    // Return index of first/next in _cubicles that does not have
    //   piece placed in it.
    // Used by Piece::place() and Piece::place_next()
    //
    unsigned first_free()
    const
    {
        unsigned    first = 0;
        while (   first < NUMBER_OF_CUBICLES
               && _cubicles[first].status != Cubicle::Status::PRIMARY)
             ++first;
        return first;
    }
    //
    unsigned next_free(
    unsigned    current)
    const
    {
        while (            ++current < NUMBER_OF_CUBICLES
               &&  _cubicles[current].status != Cubicle::Status::PRIMARY)
           ;
        return current;
    }

    // Undo place_piece().
    // Used by Piece::place() after place_piece() but detecting
    //   is_duplicate_solution() or has_orphan(), or when resuming
    //   checks of piece after backtracking in recursive solution
    //   tree and returning forward to piece.
    void remove_piece(
    const Piece* const      piece       ,
    const unsigned          piece_number)
    {
        for (unsigned ndx = 0 ; ndx < piece->size() ; ++ndx)
            _piece_cubicles[piece_number][ndx]->occupant = 0;
    }

    // See EXTENDED_HELP_TEXT in main.cxx or run compiled program
    //   with -H option.
    // Used by Piece::place()
    bool    has_orphan() const;


    // Used by Soma::post_solve() when recursively backtracking in
    //   solution tree space
    void reset_piece(
    Piece* const    piece       ,
    const unsigned  piece_number)
    {
        if (!piece->is_pre_placed())
            remove_piece(piece, piece_number);
        piece->reset_position_orientation();
    }

    // Mark _cubicles[].status with Cubicle::Status flags to place
    //   pieces only in cubicles that are not rotated/mirrored symmetric
    //   in shape.
    // Sets to default values if check_symmetry==false, or calls
    //   set_statuses(Shape*, unsigned, char) or
    //   set_statuses_no_children(unsigned) otherwise.
    // Called by Soma::solve() and Soma::init_shape() for Nth and
    //   first piece to solve, respectively.
    void set_statuses(
    const unsigned  piece_number  ,
    const char      piece_name    ,
    const bool      check_symmetry)
    {
        if (!check_symmetry) {
            for (unsigned ndx = 0 ; ndx < NUMBER_OF_CUBICLES ; ++ndx)
                if (_cubicles[ndx].occupant) {
                    _cubicles[ndx].status        = Cubicle::Status::OCCUPIED;
                    _statuses[piece_number][ndx] = Cubicle::Status::OCCUPIED;
                }
                else {
                    _cubicles[ndx].status        = Cubicle::Status::PRIMARY;
                    _statuses[piece_number][ndx] = Cubicle::Status::PRIMARY;
                }
            return;
        }

        reset_statuses();
        if (_children.size() == 1)
            set_statuses_no_children(piece_number, piece_name);
        else
            for (Shape *child : _children)
                set_statuses(child, piece_number, piece_name);

        for (unsigned ndx = 0 ; ndx < NUMBER_OF_CUBICLES ; ++ndx) {
            _statuses[piece_number][ndx] = _cubicles[ndx].status;
#ifdef SOMA_STATISTICS
            if (_cubicles[ndx].status == Cubicle::Status::PRIMARY)
                ++_statuses_uniques[piece_number];
            else if (_cubicles[ndx].status == Cubicle::Status::DUPLICATE)
                ++_statuses_duplicates[piece_number];
#endif
        }
    }

    // Used by Soma::solve() and Soma::post_solve() when backtracking
    //   in solution tree space.
    // Avoids having to recompute already-computed statuses for piece
    //   at point in solution tree.
    void restore_statuses(
    const unsigned  piece_number)
    {
        for (unsigned ndx = 0 ; ndx < NUMBER_OF_CUBICLES ; ++ndx)
            _cubicles[ndx].status = _statuses[piece_number][ndx];
    }

#ifdef SOMA_STATISTICS
    unsigned statuses_uniques(
    const unsigned  piece_number)
    const
    {
        return _statuses_uniques[piece_number];
    }

    unsigned statuses_duplicates(
    const unsigned  piece_number)
    const
    {
        return _statuses_duplicates[piece_number];
    }
#endif



  protected:
    // datatypes
    //

#ifdef SOMA_STD_SET_SET
    using SignatureSet = std::set<Signature>;
    using IntSet       = std::set<int      >;
#endif
#ifdef SOMA_STD_SET_UNORDERED
    using SignatureSet = std::unordered_set<Signature       ,
                                            Signature::Hash ,
                                            Signature::Equal>;
    using IntSet       = std::unordered_set<int             >;
#endif

    // *2 because normal and mirrored versions of each _rotators_mirrorers
    static const unsigned     MAX_ROTATOR_REFLECTORS
                            = Rotators::MAX_NUMBER_OF_ORIENTATIONS * 2;


    struct Cubicle : public Position {
        unsigned                     occupant          ;  // Piece::code()
        Cubicle                     *adjacents[3][3][3],  // only orthogonals
                                    *parent            ;  // for child shapes
        std::array<Cubicle*, 6>      ortho_adjacents   ;  // efficient access
        bool                         in_child          ;

        // symmetry codes
        enum class Status {
            UNSET     = 0,
            OCCUPIED     ,
            PRIMARY      ,
            DUPLICATE    ,
        };
        Status           status;

        void operator()(
        int     x,
        int     y,
        int     z)
        {
            this->x() = x;
            this->y() = y;
            this->z() = z;
        }

        void operator()(
        const Position  &position)
        {
            this->x() = position.x();
            this->y() = position.y();
            this->z() = position.z();
        }
    };


    // Slow linear search, but only used at initialization:
    //   read()/specify() -> prepare_solve() ->
    //   create_children() -> find_adjacent_cubicles()
    Cubicle* find_cubicle(
    const Position  &position)
    const
    {
        for (const Cubicle &cubicle : _cubicles)
            if (cubicle == position)
                return const_cast<Cubicle*>(&cubicle);
        return  0;
    }

    // Fast lookup of neighboring cubicle
    Cubicle* find_cubicle(
    const Cubicle   *const cubicle,
    const Position  &position)
    const
    {
        // +1 because position x,y,z are -1,0,1
        return  cubicle->adjacents[position.x() + 1]
                                  [position.y() + 1]
                                  [position.z() + 1];
    }

    // Generate linear version of currently placed pieces.
    // Used for duplicate checking.
    //
    // Normal version, whole shape
    void generate_signature(
    Signature   &signature)
    const
    {
        for (unsigned ndx = 0 ; ndx < NUMBER_OF_CUBICLES ; ++ndx)
            signature[ndx] = _cubicles[ndx].occupant;
    }
    // For rotated cubicles
    static void generate_cubicles_signature(
          Signature                                 &signature,
    const std::array<Cubicle, NUMBER_OF_CUBICLES>   &cubes    ,
    const unsigned                                   num_cubes)
    {
        for (unsigned ndx = 0 ; ndx < num_cubes ; ++ndx)
            signature[ndx] = cubes[ndx].occupant;
    }
    // For concatenating rotated children
    void generate_signature_child(
    Signature       &signature,
    const unsigned   offset   )
    const
    {
        for (unsigned ndx = 0 ; ndx < _num_cubicles ; ++ndx)
            signature[ndx + offset] = _cubicles[ndx].parent->occupant;
    }

    // Translate shape to (0,0,0)
    void normalize()
    {
        _max_pos = Position::normalize<Cubicle                                ,
                                       std::array<Cubicle, NUMBER_OF_CUBICLES>>
                   (_cubicles, _num_cubicles);
    }

    // Center shape around (0,0,0).
    // See Position::center() regarding cubicle coordinates for
    //   odd or even x,y,z sizes.
    void center()
    {
        Position::center<Cubicle, std::array<Cubicle, NUMBER_OF_CUBICLES>>
        (_cubicles, _num_cubicles, _max_pos, true);
    }

    // Reset to default (no symmetry analysis) values.
    unsigned reset_statuses()
    {
    unsigned    number_occupied = 0;
        for (Cubicle &cubicle : _cubicles)
            if (cubicle.occupant) {
                cubicle.status = Cubicle::Status::OCCUPIED;
                ++number_occupied;
            }
            else
                cubicle.status = Cubicle::Status::UNSET;
        return number_occupied;
    }



    // See implementations in file shape.cxx
    //

    void    set_cubicle_piece(Cubicle       &_cubicle,
                              const char     letter  );

    bool    prepare_solve(std::ostream  *errors);

    void    find_adjacent_cubicles();
    bool    create_children       ();
    void    generate_symmetries   ();
    void    undo_odd_even         ();

    void    populate_child(Shape*, Cubicle*);

    void    check_add_symmetric(Shape* const        shape       ,
                                const unsigned      rotation_ndx,
                                const bool          mirror      );

    bool    generate_rotator_reflectors(Shape* const     child ,
                                        std::ostream    *errors);



    void    set_statuses(Shape* const       child       ,
                         const unsigned     piece_number,
                         const char         piece_name  );

    void    set_statuses_no_children(const unsigned     piece_number,
                                     const char         piece_name  );

    void    add_solution_no_children(const unsigned     piece_number);
    void    add_solution            (Shape* const       child       );

    void generate_rotated_signature(Signature       &signature       ,
                                    const unsigned   rotator_mirrorer) const;



    // data members
    //

    // shape definition
    std::array<Cubicle, NUMBER_OF_CUBICLES>     _cubicles;

    // Absolute value. Same as x,y,z maximums when normalized to
    //   0..max due to *2 coordinates for odd/even x,y,z sizes.
    // See Position::center().
    Position    _max_pos ;

    // Saved position of placed piece for fast remove_piece().
    // (Eliminates need for linear search through _cubicles).
    std::array<std::array<
               Cubicle*,
               Piece::MAX_NUMBER_OF_CUBES + 1>, // +1 for central cube
               Piece::NUMBER_OF_PIECES        > _piece_cubicles;

    // Indices into Rotators::rotations[]
    //
    // Raw rectangular parallelpiped bounding box of shape
    std::vector<unsigned>   _symmetries;
    // Valid subset of above, taking account of actual shape
    std::vector<unsigned>   _rotators_mirrorers;

    // For detecting duplicate solutions
    //
    // Finished solutions
    std::vector<Signature>  _solutions;
    // After Nth piece has been placed
    std::array<SignatureSet, Piece::NUMBER_OF_PIECES>   _solutions_sets;

    // To ensure sets of concatenated rotated/mirrored child shapes
    //   contain only one each of "p" and "n" piece.
    // See add_solution(const unsigned)
    std::vector<unsigned>   _solution_ps ,
                            _solution_ns ;

    // Saved for efficient restore_statuses() without recomputing when
    //   backtracking in solution tree space
    Cubicle::Status     _statuses[Piece::NUMBER_OF_PIECES][NUMBER_OF_CUBICLES];

    // Saved to avoid recomputing
    std::array<std::vector<unsigned>, Piece::NUMBER_OF_PIECES>
        _piece_rotators_mirrorers;

    // One for each separated (not orthogonally contiguous) sub-shape.
    // Just one if no sub-shapes.
    std::vector<Shape*>     _children;

    // != NUMBER_OF_CUBICLES in child shapes if multiple ones
    unsigned    _num_cubicles;// variable in child shapes

#ifdef SOMA_STATISTICS
    unsigned    _statuses_uniques   [Piece::NUMBER_OF_PIECES],
                _statuses_duplicates[Piece::NUMBER_OF_PIECES];
#endif



  public:

#ifdef SOMA_OSTREAM_OPERATORS
    Position cubicle_position(
    const unsigned ndx)
    const
    {
        if (ndx < NUMBER_OF_CUBICLES)
            return _cubicles[ndx];
        else
            return Position(0x7f, 0x7f, 0x7f);
    }
#endif

};   // class Shape

}  // namespace soma

#endif   // ifndef SHAPE_H
