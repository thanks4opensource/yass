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


#include <algorithm>
#include <iostream>
#include <iomanip>
#include <limits>

#include "rotators.hxx"
#include "signature.hxx"

#include "shape.hxx"



namespace soma {

// Public ======================================================================

Shape::Shape(
unsigned    number_of_cubicles)
:
#ifdef SOMA_STD_SET_UNORDERED
    // pre-allocate hash table for efficiency
    _solutions_sets{SignatureSet(1<<14),
                    SignatureSet(1<<14),
                    SignatureSet(1<<14),
                    SignatureSet(1<<14),
                    SignatureSet(1<<14),
                    SignatureSet(1<<14),
                    SignatureSet(1<<14)},
#endif
    _num_cubicles(number_of_cubicles)
#ifdef SOMA_STATISTICS
    ,
    _statuses_uniques   {0},
    _statuses_duplicates{0}
#endif
{}



// See shape.hxx
void Shape::reset()
{
    _symmetries        .clear();
    _rotators_mirrorers.clear();
    _solutions         .clear();
    _solution_ps       .clear();
    _solution_ns       .clear();

    for (SignatureSet &solutions : _solutions_sets)
        solutions.clear();

    for (Shape *child : _children)
        delete child;
    _children.clear();

    for (Cubicle &cubicle : _cubicles) {
        cubicle.occupant = 0                     ;
        cubicle.parent   = &cubicle              ;
        cubicle.status   = Cubicle::Status::UNSET;
    }

}   // reset()



// See shape.hxx
bool Shape::read(
std::istream    &input ,
std::ostream    *errors)
{
    unsigned        x                ,
                    y = 0            ,
                    z = 0            ,
                    max_x = 0        ,
                    max_y = 0        ,
                    cubicle_ndx = 0  ;
    bool            z_pending = false,  // to handle multiple blank lines ...
                    started = false  ,  // ... between Z layers
                    blank            ;  // to separate Z layers
    std::string     line             ;  // input text

    while (std::getline(input, line)) {
        // truncate line if/at comment character
        std::string::size_type  special;
        if ((special = line.find("#")) != std::string::npos)
            line = line.substr(0, special);

        // Comment and/or pure whitespace lines also count as blank
        if (line.empty())
            blank = true;
        else if ((special = line.find_first_not_of("\t ")) == std::string::npos)
            blank = true;
        else
            blank = false;

        // New Z layer, handling of multiple blank lines
        if (blank) {
            if (!started) continue;
            y         = 0   ;
            z_pending = true;
            continue        ;
        }
        started = true;

        // Beginning of new Z layer
        if (z_pending) {
            z += 1           ;
            z_pending = false;
        }

        // Z layer consisting of multiple Y slices
        x = 0;
        for (char const &letter : line) {
            if (letter == '\t') {
                *errors << "Illegal tab character in file"
                        << std::endl;
                return false;
            }

            // shape cubicle or empty space
            if (letter != '.' && letter != ' ') {
                if (cubicle_ndx < NUMBER_OF_CUBICLES) {
                    _cubicles[cubicle_ndx](x, y, z);
                    _cubicles[cubicle_ndx].parent   = &_cubicles[cubicle_ndx];
                    _cubicles[cubicle_ndx].in_child = false                  ;

                    set_cubicle_piece(_cubicles[cubicle_ndx], letter);
                }
                ++cubicle_ndx;
            }
            if (x > max_x) max_x = x;
            ++x;
        }
        if (y > max_y) max_y = y;
        ++y;
    }

    if (cubicle_ndx != NUMBER_OF_CUBICLES) {
        if (errors)
            *errors << "Bad number of cubicles: "
                    << cubicle_ndx
                    << " instead of "
                    << NUMBER_OF_CUBICLES
                    << std::endl;
        return  false;
    }

    // save values
    _max_pos(max_x, max_y, z);
    _num_cubicles = NUMBER_OF_CUBICLES;

    // reverse Y and Z coords (were read in high-to-low so indexed 0-to-N)
    //   but want +Z and +Y up
    for (Cubicle &cubicle : _cubicles) {
        cubicle.y() = _max_pos.y() - cubicle.y();
        cubicle.z() = _max_pos.z() - cubicle.z();
    }

    return prepare_solve(errors);

}   // read(std::istream&, std::ostream&)



// See shape.hxx
bool Shape::specify(
const std::array<int, NUMBER_OF_CUBICLES * 3>    coords,
const std::string                                pieces,
      std::ostream                              *errors)
{
    unsigned    cubicle_ndx = 0;
    for (Cubicle &cubicle : _cubicles) {
        cubicle(coords[cubicle_ndx * 3   ],
                coords[cubicle_ndx * 3 + 1],
                coords[cubicle_ndx * 3 + 2]);

        cubicle.parent   = &cubicle;
        cubicle.in_child = false   ;

        if (cubicle_ndx < pieces.size())
            set_cubicle_piece(cubicle, pieces[cubicle_ndx]);
        else
            cubicle.occupant = 0;

        ++cubicle_ndx;
    }

    return prepare_solve(errors);

}   // specify(const std::array, const std::string, std::ostream*)



// See shape.hxx
unsigned Shape::num_piece_cubicles(
const Piece *piece)
{
    unsigned    count = 0;
    for (const Cubicle &cubicle : _cubicles)
        if (cubicle.occupant == piece->code())
            ++count;
    return count;
}   // num_piece_cubicles(const Piece*)



// See shape.hxx
void Shape::write(
std::ostream    &output)
const
{
    // space instead of '.' for all-empty Y columns (for separated shapes)
    //
    IntSet      full_xs;

    for (auto cubicle : _cubicles)
        full_xs.insert(cubicle.x());

    auto    cubicle = _cubicles.cbegin();
    // z and y high-to-low, x low-to-high
    for (int z_pos = _max_pos.z(); z_pos >= -_max_pos.z(); z_pos -= 2) {
        for (int y_pos = _max_pos.y(); y_pos >= -_max_pos.y(); y_pos -= 2) {
            for (int x_pos = -_max_pos.x(); x_pos <= _max_pos.x(); x_pos += 2) {
                if (*cubicle == Position(x_pos, y_pos, z_pos)) {
                    if (cubicle->occupant)
                        output << Piece::code2name(cubicle->occupant);
                    else
                        output << '#';
                    ++cubicle;
                }
                else if (full_xs.find(x_pos) != full_xs.end())
                    output << '.';
                else
                    output << ' ';
            }
            output << std::endl;
        }

        if (z_pos != -_max_pos.z())
            output << std::endl;
    }

}   // write(ostream&) const



// See shape.hxx
std::array<char, Shape::NUMBER_OF_CUBICLES> Shape::solution(
std::array<int, Shape::NUMBER_OF_CUBICLES * 3>  &coords)
const
{
    std::array<char, Shape::NUMBER_OF_CUBICLES>     pieces;

    for (unsigned ndx = 0; ndx < NUMBER_OF_CUBICLES; ++ndx) {
        pieces[ndx        ] = Piece::code2name(_cubicles[ndx].occupant);
        coords[ndx * 3    ] = _cubicles[ndx].x();
        coords[ndx * 3 + 1] = _cubicles[ndx].y();
        coords[ndx * 3 + 2] = _cubicles[ndx].z();
    }

    return pieces;

}  // solution(...)



// See shape.hxx
bool Shape::generate_rotator_reflectors(
std::ostream    *errors)
{
    if (_children.size() == 1)
        return generate_rotator_reflectors(this, errors);
    else
        for (Shape *child : _children)
            if (!generate_rotator_reflectors(child, errors))
                return false;

    return true;

}  // generate_rotator_reflectors(std::ostream*)



// See shape.hxx
bool Shape::is_duplicate_solution(
const unsigned  piece_number)
{
    Signature   signature;

    if (_children.size() == 1)
        generate_signature(signature);
    else {
        unsigned    offset = 0;
        for (Shape *child : _children) {
            child->generate_signature_child(signature, offset);
            offset += child->_num_cubicles;
        }
    }

    // Check if in already seen solutions (or their rotations/reflections)
    return    _solutions_sets[piece_number].find(signature)
           != _solutions_sets[piece_number].end (         );

}   // is_duplicate_solution(const unsigned)



// See shape.hxx
void Shape::add_solution(
const unsigned  piece_number)   // can be called after any piece is placed
{

    if (_children.size() == 1) {   // No need to combine/permute child solutions
        add_solution_no_children(piece_number);
        return;
    }

    // Generate all combinations of rotated/reflected child shapes
    // E.g. if child/num_rot_refects are: a/2 b/3 c/1
    //      generate: a0/b0/c0 a0/b1/c0 a0/b2/c0
    //                a1/b0/c0 a1/b1/c0 a1/b2/c0
    // But also make sure that no combination has multiple "p" or "n"
    //   shapes (which were generated when reflecting/mirror the
    //   child shapes>
    //

    for (Shape *child : _children) {
        child->_solutions  .clear();
        child->_solution_ps.clear();
        child->_solution_ns.clear();
        add_solution(child)      ;
    }

    // Indices of each child's rotated/mirrored solutions
    std::vector<unsigned>   combinations(_children.size(), 0);

    // Go through all solutions for each child
    while (  combinations[_children.size() - 1]
           <    _children[_children.size() - 1]->_solutions.size()) {

        // must be before goto
        Signature   solution        ;   // concatenated child signatures
        unsigned    solution_ndx = 0;

        // Only check for multiple "p" and "n" pieces if all pieces placed
        if (piece_number == Piece::NUMBER_OF_PIECES - 1) {
            unsigned    num_ps = 0,
                        num_ns = 0;
            for (unsigned ndx = 0 ; ndx < _children.size() ; ++ndx) {
                num_ps += _children[ndx]->_solution_ps[combinations[ndx]];
                num_ns += _children[ndx]->_solution_ns[combinations[ndx]];
            }

            // Must have exactly one each of "p" and "n" pieces
            if (num_ps != 1 || num_ns != 1)
                goto next_combination;
        }

        for (unsigned     child_ndx = 0                ;
                          child_ndx < _children.size() ;
                        ++child_ndx                     ) {
            for (unsigned     piece_ndx = 0                                   ;
                              piece_ndx < _children[child_ndx]->_num_cubicles ;
                            ++piece_ndx                                       ){
                  solution[solution_ndx++]
                =   _children[child_ndx]
                  ->_solutions[combinations[child_ndx]][piece_ndx];
            }
        }

        _solutions_sets[piece_number].insert(solution);

        next_combination:
        // increment to next permutation
        for (unsigned     permute_ndx = 0                ;
                          permute_ndx < _children.size() ;
                        ++permute_ndx                     ) {
            if (  ++combinations[permute_ndx]
                <       _children[permute_ndx]->_solutions.size())
                break;
            // rollover except last ("most significant digit") one
            if (permute_ndx < _children.size() - 1)
                combinations[permute_ndx] = 0;
        }
    }

}  // add_solution(const unsigned)



// See shape.hxx
bool Shape::place_piece(
const unsigned      cubicle_ndx    ,
Piece* const        piece          ,
const unsigned      piece_number   ,
const unsigned      number_of_cubes,
const Position      cubes[]        ,
const bool          just_test      )
{
    Cubicle *center = &_cubicles[cubicle_ndx];
    Cubicle     *peripherals[Piece::MAX_NUMBER_OF_CUBES];
    for (unsigned ndx = 0; ndx < number_of_cubes; ++ndx) {
        Cubicle     *peripheral = find_cubicle(center, cubes[ndx]);

        if (!peripheral || peripheral->occupant)
            return  false;

        peripherals[ndx] = peripheral;
    }

    if (just_test) return true;

    center->occupant = piece->code();

    // number_of_cubes doesn't include central one
    _piece_cubicles[piece_number][number_of_cubes] = center;

    for (unsigned ndx = 0; ndx < number_of_cubes; ++ndx) {
        peripherals[ndx]->occupant = piece->code();
        _piece_cubicles[piece_number][ndx] = peripherals[ndx];
    }

    return  true;

}   // place_piece(const unsigned  ,
    //             Piece* const    ,
    //             const unsigned  ,
    //             const unsigned  ,
    //             const Position[],
    //             const bool      )



// See shape.hxx
bool Shape::has_orphan()
const
{
    uint32_t    handled = 0;
    for (unsigned     cubicle_ndx = 0                  ;
                      cubicle_ndx < NUMBER_OF_CUBICLES ;
                    ++cubicle_ndx                       ) {
        const Cubicle   *cubicle = &_cubicles[cubicle_ndx];

        if (cubicle->occupant || (handled & (1 << cubicle_ndx)))
            continue;

        unsigned         num_empties = 0;
        const Cubicle   *twin           ;
        for (const Cubicle* adjacent : cubicle->ortho_adjacents)
            if (adjacent && !adjacent->occupant) {
                ++num_empties;
                twin = adjacent;
            }

        if (num_empties == 0)
            return true;
        else if (num_empties == 1) {
            num_empties = 0;
            const Cubicle   *only = 0;  // initialize to silence g++ warning
            for (const Cubicle* sibling : twin->ortho_adjacents)
                if (sibling && !sibling->occupant) {
                    ++num_empties;
                    only = sibling;
                }

            // num_empties can't be 0
            if (num_empties == 1 && only == cubicle)
                return true;

            handled |= 1 << (twin - &_cubicles[0]);
        }
    }

    return  false;

}   // has_orphan() const




// Protected ===================================================================

// Decode letter into Piece
// Used by read() and specify()
//
void Shape::set_cubicle_piece(
Cubicle     &cubicle,
const char   letter )
{
    switch (letter) {
        case 'c':
            cubicle.occupant = Piece::corner.code();
            Piece::corner.pre_place();
            break;

        case 'p':
            cubicle.occupant = Piece::pos.code();
            Piece::pos.pre_place();
            break;

        case 'n':
            cubicle.occupant = Piece::neg.code();
            Piece::neg.pre_place();
            break;

        case 'z':
            cubicle.occupant = Piece::zee.code();
            Piece::zee.pre_place();
            break;

        case 't':
            cubicle.occupant = Piece::tee.code();
            Piece::tee.pre_place();
            break;

        case 'l':
            cubicle.occupant = Piece::ell.code();
            Piece::ell.pre_place();
            break;

        case '3':
            cubicle.occupant = Piece::three.code();
            Piece::three.pre_place();
            break;

        default:
            cubicle.occupant = 0;
            break;
    }
}   // set_cubicle_piece();



bool Shape::prepare_solve(
std::ostream    *errors)
{
    normalize             ();
    center                ();
    generate_symmetries   ();
    find_adjacent_cubicles();

    if (!create_children()) {
        if (errors)
            *errors << "Has child shape with unsolvable number of cubicles"
                     << std::endl;
        return false;
    }

    return true;

}  // prepare_solve(std::ostream*)



// Initialize Cubicle::.adjacents[][][] for each _cubicle
// Fills in only needed 6 orthogonal elements of .adjacents
//   ((plus superfluous 0,0,0)
// Use of inefficient find_cubicle(Position&) because only called
//   once per solve from:
//   read()/specify() -> prepare_solve() ->
//   create_children() -> find_adjacent_cubicles()
// Then fills Cubicle::ortho_adjacents for subsequent use
// Must do after center() because find_cubicle(Position&) works
//   with 2*(x,y,z) indices. See Position::center(...)
//
void Shape::find_adjacent_cubicles()
{
    // find adjacent _cubicles
    // must do after centering because find_cubicle() works
    //   using 2*N indexes
    for (Cubicle &cubicle : _cubicles) {

        for (int z = 0; z < 3 ; ++z)
            for (int y = 0 ; y < 3 ; ++y)
                for (int x = 0 ; x < 3 ; ++x)
                    // Only orthogonals (and unneeded 0,0,0)
                    if ((x - 1) * (y - 1) * (z - 1) == 0) {
                        // See Position::center() for why 2*(x,y,z)-2
                          cubicle.adjacents[x][y][z]
                        = find_cubicle(  cubicle
                                       + Position(2 * x - 2,
                                                  2 * y - 2,
                                                  2 * z - 2));
                    }

        cubicle.ortho_adjacents[OrthAdj::UP   ] = cubicle.adjacents[1][1][2];
        cubicle.ortho_adjacents[OrthAdj::DOWN ] = cubicle.adjacents[1][1][0];
        cubicle.ortho_adjacents[OrthAdj::FRONT] = cubicle.adjacents[1][2][1];
        cubicle.ortho_adjacents[OrthAdj::BACK ] = cubicle.adjacents[1][0][1];
        cubicle.ortho_adjacents[OrthAdj::LEFT ] = cubicle.adjacents[0][1][1];
        cubicle.ortho_adjacents[OrthAdj::RIGHT] = cubicle.adjacents[2][1][1];
    }

}   // find_adjacent_cubicles()



// Create _children shapes, one for each separated part of shape
//
bool Shape::create_children()
{

    for (Cubicle &cubicle : _cubicles)
        if (!cubicle.in_child) {
            // For each unprocessed cubicle, populate_child() will
            //   traverse all orthogonally-connected cubicles
            //   and mark as processed.
            Shape   *child = new Shape(0);
            _children.push_back(child)     ;
            populate_child(child, &cubicle);
        }

    // init each child
    for (Shape  *child : _children) {
        child->normalize          ();
        child->undo_odd_even      ();
        child->center             ();
        child->generate_symmetries();

        // sort child cubicles
        std::sort(&child->_cubicles[0]                       ,
                  &child->_cubicles[0] + child->_num_cubicles);
    }

    // Check sanity of each child -- no need to attempt solve if
    //   any is unsolvable.
    // Must have 4*n or 4*n+3 cubicles, n>=0 && n<=6 (combination of
    //   zero or more 4-cubicle pieces with or without 3-cubicle piece)
    for (Shape  *child : _children)
        if (       child->_num_cubicles     <  3
            || (   child->_num_cubicles % 4 != 0
                && child->_num_cubicles % 4 != 3))
            return false;

    return true;

}   // create_children()



// Find raw symmetries of shape, based only on rectangular parallelpiped
//   bounding box of shape.
// Later, generate_rotator_reflectors() and check_add_symmetric() will
//   prune these taking into account actual shape.
//
void Shape::generate_symmetries()
{
    // not necessary because only called at initialization, but
    // amount of memory is trivial
    _symmetries.reserve(Rotators::MAX_NUMBER_OF_ORIENTATIONS);

    // always include identity so add_solution() doesn't have to
    //   special case newly found non-rotated solution
    _symmetries.push_back(Rotators::POSX_POSY_POSZ);

    // any centered parallelpiped is unchanged when rotated 180 around any axis
    _symmetries.push_back(Rotators::NEGX_NEGY_POSZ);
    _symmetries.push_back(Rotators::NEGX_POSY_NEGZ);
    _symmetries.push_back(Rotators::POSX_NEGY_NEGZ);


    // Check for valid 90 degree rotations
    //

    if (_max_pos.x() == _max_pos.y()) {
        _symmetries.push_back(Rotators::NEGY_POSX_POSZ);
     // _symmetries.push_back(Rotators::NEGX_NEGY_POSZ);
        _symmetries.push_back(Rotators::POSY_NEGX_POSZ);
     // _symmetries.push_back(Rotators::NEGX_POSY_NEGZ);
        _symmetries.push_back(Rotators::NEGY_NEGX_NEGZ);
     // _symmetries.push_back(Rotators::POSX_NEGY_NEGZ);
        _symmetries.push_back(Rotators::POSY_POSX_NEGZ);
    }

    if (_max_pos.x() == _max_pos.z()) {
     // _symmetries.push_back(Rotators::POSX_POSY_POSZ);
     // _symmetries.push_back(Rotators::NEGX_POSY_NEGZ);
     // _symmetries.push_back(Rotators::POSX_NEGY_NEGZ);
        _symmetries.push_back(Rotators::NEGZ_POSY_POSX);
        _symmetries.push_back(Rotators::POSZ_NEGY_POSX);
        _symmetries.push_back(Rotators::POSZ_POSY_NEGX);
        _symmetries.push_back(Rotators::NEGZ_NEGY_NEGX);
    }

    if (_max_pos.y() == _max_pos.z()) {
     // _symmetries.push_back(Rotators::POSX_POSY_POSZ);
     // _symmetries.push_back(Rotators::NEGX_POSY_NEGZ);
     // _symmetries.push_back(Rotators::POSX_NEGY_NEGZ);
        _symmetries.push_back(Rotators::POSX_NEGZ_POSY);
        _symmetries.push_back(Rotators::NEGX_POSZ_POSY);
        _symmetries.push_back(Rotators::POSX_POSZ_NEGY);
        _symmetries.push_back(Rotators::NEGX_NEGZ_NEGY);
    }

    if (_max_pos.x() == _max_pos.y() && _max_pos.y() == _max_pos.z()) {
        _symmetries.push_back(Rotators::POSZ_POSX_POSY);
        _symmetries.push_back(Rotators::NEGZ_NEGX_POSY);
        _symmetries.push_back(Rotators::NEGZ_POSX_NEGY);
        _symmetries.push_back(Rotators::POSZ_NEGX_NEGY);
        _symmetries.push_back(Rotators::NEGY_NEGZ_POSX);
        _symmetries.push_back(Rotators::POSY_POSZ_POSX);
        _symmetries.push_back(Rotators::NEGY_POSZ_NEGX);
        _symmetries.push_back(Rotators::POSY_NEGZ_NEGX);
    }

}   // generate_symmetries()



// Undo odd/even dimension indices 1,3,... vs 0,2,4... (see Position::center())
//   created by normalize() because center() needs 0,1,2,...
// Used for child shapes because full shape already had normalize()
//
void Shape::undo_odd_even()
{
    _max_pos >>= 1;

    for (unsigned ndx = 0 ; ndx < _num_cubicles ; ++ndx)
        _cubicles[ndx] >>= 1;

}   // undo_odd_even()



// Recursively find all orthogonally-connected cubicles from
//   starting cubicle (called from create_children()), and
//   add (marked as in_child in main shape) to child
//
void Shape::populate_child(
Shape       *child  ,
Cubicle     *cubicle)
{

    child->_cubicles[child->_num_cubicles] = *cubicle;
    cubicle->in_child = true;
    child->_cubicles[child->_num_cubicles++].parent = cubicle;

    for (Cubicle *adjacent : cubicle->ortho_adjacents)
        if (adjacent && !adjacent->in_child)
            populate_child(child, adjacent);

}   // populate_child()




// Prune basic list of valid shape rotations in _symmetries created
//   by generate_symmetries() based only on rectangular parallelpiped
//   bounding box leaving only truly valid ones based on actual shape.
// Also check for unsolvable one-dimensional shapes (two-dimensional is
//   OK because valid for child shapes)
// Calls check_add_symmetric() for each rotation in _symmetries.
//
bool Shape::generate_rotator_reflectors(
Shape* const     shape ,
std::ostream    *errors)
{

    // not necessary because only called at initialization, but
    // amount of memory is trivial
    _rotators_mirrorers.reserve(MAX_ROTATOR_REFLECTORS);

    // Check solvable
    unsigned    num_2_or_3d = 0;
    if (_max_pos.z() > 0) ++num_2_or_3d;
    if (_max_pos.y() > 0) ++num_2_or_3d;
    if (_max_pos.x() > 0) ++num_2_or_3d;
    if (num_2_or_3d < 2) {
        *errors << "Unsolvable one- or zero-dimensional shape or part of shape"
                << std::endl;
        return false;
    }

    for (unsigned symmetry : shape->_symmetries)
        for (unsigned mirror = 0 ; mirror < 2 ; ++mirror)
            // no error if not symmetric, just doesn't get added
            check_add_symmetric(shape, symmetry, mirror);

    return true;

}  // generate_rotator_reflectors(Shape*, std::ostream*)



// Called by generate_rotator_reflectors(), above, for
//   each rototator/reflector, both plain and mirrored
// Returns true if shape is symmetric after such transformation
//
void Shape::check_add_symmetric(
Shape* const    shape   ,
const unsigned  symmetry,
const bool      mirror  )
{
    std::array<Cubicle, NUMBER_OF_CUBICLES>     rotated          ;

    // Choose which set of mirrored Rotators to use
    unsigned    mirror_offset = 0;
    if (mirror) {
        if (_max_pos.z() > 0) mirror_offset = Rotators::Z_MIRRORED_OFFSET;
        else                  mirror_offset = Rotators::X_MIRRORED_OFFSET;
        // already checked 1D or 0D in generate_rotator_reflectors
    }

    // Rotate/mirror shape
    for (unsigned ndx = 0 ; ndx  < shape->_num_cubicles ; ++ndx) {
        Position    position = shape->_cubicles[ndx];
        rotated[ndx](position.rotate(  Rotators::rotator(symmetry
                                     + mirror_offset             )));
    }

    // Rotation has changed canonical linear ordering
    std::sort(rotated.begin(), rotated.begin() + shape->_num_cubicles);

    // Check each cubicle, and abort if any is not symmetric
    for (unsigned ndx = 0 ; ndx < shape->_num_cubicles ; ++ndx)
        if (rotated[ndx] != shape->_cubicles[ndx])
            return;

    // All were symmetric
    shape->_rotators_mirrorers.push_back(symmetry + mirror_offset);

}  // check_add_symmetric(Shape* const, const nusigned, const bool)


// Set Cubicle::status for each _cubicle
// For doing symmetry check.
//
void Shape::set_statuses(
Shape* const    child       ,
const unsigned  piece_number,
const char      piece_name  )
{

    if (child->reset_statuses() == child->_num_cubicles)
        return;  // all cubicles occupied by pieces, no need to continue

    // Unrotated signature
    Signature   signature;
    generate_signature_child(signature, 0);

    // Fill in for each rotation/reflection
    //
    child->_piece_rotators_mirrorers[piece_number].clear();
    for (unsigned     rot_mir_ndx = 1                                 ;
                      rot_mir_ndx < child->_rotators_mirrorers.size() ;
                    ++rot_mir_ndx                                     ) {
        const unsigned    rotator_mirrorer
                        = child->_rotators_mirrorers[rot_mir_ndx];

        // Check and don't mirror "p" and "n" pieces
        if (   (   piece_name == Piece::pos.name()
                || piece_name == Piece::neg.name())
            && rotator_mirrorer >= Rotators::Z_MIRRORED_OFFSET)
            break;

        Signature   rotated_signature;
        child->generate_rotated_signature(rotated_signature, rotator_mirrorer);

        // Add if this rotation/reflection is symmetric
        if (rotated_signature == signature)
            child->_piece_rotators_mirrorers[piece_number]
                   .push_back(rotator_mirrorer);
    }


    // Go through all _cubicles
    //

    unsigned
    num_unset = std::count_if(child->_cubicles.cbegin()                       ,
                              child->_cubicles.cbegin() + child->_num_cubicles,
                              [](Cubicle cubicle) {
                                 return    cubicle.parent->status
                                        == Cubicle::Status::UNSET;}           );

    unsigned    primary_ndx = 0;
    while (num_unset) {
        // Find a yet-unprocessed cubicle
        while (      child->_cubicles[primary_ndx].parent->status
                  != Cubicle ::Status::UNSET
               && primary_ndx < child->_num_cubicles             )
            ++primary_ndx;

        if (primary_ndx == child->_num_cubicles) {  // at end
            num_unset = 0;
            break;
        }

        // Mark as primary
        Cubicle     &primary = child->_cubicles[primary_ndx];
        primary.parent->status = Cubicle::Status::PRIMARY;
        --num_unset;

        // Find all symmetric rotations/reflections of primary cubicle
        for (const unsigned   rotator_mirrorer
                            : child->_piece_rotators_mirrorers[piece_number]) {

            for (unsigned     duplicate_ndx = primary_ndx          ;
                              duplicate_ndx < child->_num_cubicles ;
                            ++duplicate_ndx                        ) {
                Cubicle     &duplicate = child->_cubicles[duplicate_ndx];

                if (duplicate.parent->status != Cubicle::Status::UNSET)
                    continue;  // already processed

                Position      rotated
                            = duplicate.rotate(  Rotators
                                               ::rotator(rotator_mirrorer));

                if (rotated == primary) {
                    // Is rotation/reflection of primary
                    duplicate.parent->status = Cubicle::Status::DUPLICATE;
                    --num_unset;
                    break;
                }
            }
        }
    }

}  // set_statuses(Shape* const, const unsigned, const char)


// Slightly more efficient version of set_statuses(), above,
//
void Shape::set_statuses_no_children(
const unsigned  piece_number,
const char      piece_name  )
{
    if (reset_statuses() == NUMBER_OF_CUBICLES)
        return;

    Signature   signature;
    generate_signature(signature);

    _piece_rotators_mirrorers[piece_number].clear();
    for (unsigned     rot_mir_ndx = 1                                 ;
                      rot_mir_ndx < _rotators_mirrorers.size() ;
                    ++rot_mir_ndx                                     ) {
        const unsigned    rotator_mirrorer
                        = _rotators_mirrorers[rot_mir_ndx];

        if (   (   piece_name == Piece::pos.name()
                || piece_name == Piece::neg.name())
            && rotator_mirrorer >= Rotators::Z_MIRRORED_OFFSET)
            break;

        Signature   rotated_signature;
        generate_rotated_signature(rotated_signature, rotator_mirrorer);

        if (rotated_signature == signature)
            _piece_rotators_mirrorers[piece_number].push_back(rotator_mirrorer);
    }

    unsigned      num_unset
                = std::count_if(_cubicles.cbegin()    ,
                                _cubicles.cbegin() + _num_cubicles,
                                [](Cubicle cubicle) {
                                     return     cubicle.parent->status
                                             == Cubicle::Status::UNSET;});

    unsigned    primary_ndx = 0;
    while (num_unset) {
        while (      _cubicles[primary_ndx].parent->status
                  != Cubicle ::Status::UNSET
               && primary_ndx < _num_cubicles            )
            ++primary_ndx;

        if (primary_ndx == _num_cubicles) {
            num_unset = 0;
            break;
        }

        Cubicle     &primary = _cubicles[primary_ndx];

        primary.parent->status = Cubicle::Status::PRIMARY;
        --num_unset;

        for (const unsigned   rotator_mirrorer
                            : _piece_rotators_mirrorers[piece_number]) {

            for (unsigned     duplicate_ndx = primary_ndx          ;
                              duplicate_ndx < _num_cubicles ;
                            ++duplicate_ndx                        ) {
                Cubicle     &duplicate = _cubicles[duplicate_ndx];

                if (duplicate.parent->status != Cubicle::Status::UNSET)
                    continue;

                Position      rotated
                            = duplicate.rotate(  Rotators
                                               ::rotator(rotator_mirrorer));

                if (rotated == primary) {
                    duplicate.parent->status = Cubicle::Status::DUPLICATE;
                    --num_unset;
                    break;
                }
            }
        }
    }

}  // set_statuses_no_children(const unsigned, const char)



// Much simpler version of add_solution(unsigned) plus
//   add_solution(Shape*) because no need to do combinations
//   of child solutions.
// Slightly more efficient than using add_solution(Shape*) with
//   only child because has own inline version of
//   generate_rotated_signature() without need to dereference
//   Cubicle::_parent
//
void Shape::add_solution_no_children(
const unsigned      piece_number)
{
    // Add all rotations/reflections
    for (const unsigned rotator_mirrorer : _rotators_mirrorers) {
        std::array<Cubicle, NUMBER_OF_CUBICLES>     rotated         ;
        bool                                        mirrored = false;

        if (rotator_mirrorer >= Rotators::Z_MIRRORED_OFFSET)
            mirrored = true;

        for (unsigned ndx = 0 ; ndx  < _num_cubicles ; ++ndx) {
            Position    cubicle_position = _cubicles[ndx];

            rotated[ndx]( cubicle_position
                         .rotate(Rotators::rotator(rotator_mirrorer)));

            // Exchange "p" and "n" pieces if mirrored
            unsigned    piece = _cubicles[ndx].occupant;
            if (mirrored) {
                if (piece == Piece::pos.code())
                    piece =  Piece::neg.code();
                else if (piece == Piece::neg.code())
                    piece =       Piece::pos.code();
            }
            rotated[ndx].occupant = piece;
        }

        // Rotation has destroyed geometric sort order
        std::sort(rotated.begin(), rotated.begin() + _num_cubicles);

        Signature   rotated_signature;
        generate_cubicles_signature(rotated_signature, rotated, _num_cubicles);

        _solutions_sets[piece_number].insert(rotated_signature);
    }

}   // add_solution_no_children(const unsigned)



// Like add_solution_no_children(), above,  with additional
//   information (num_ps, num_ns) for checking combinations
//   of rotated/reflected children in add_solution((), above.
//
void Shape::add_solution(
Shape* const    child)
{
    unsigned    num_ps   = 0,
                num_ns   = 0;
    for (unsigned ndx = 0 ; ndx  < child->_num_cubicles ; ++ndx) {
        unsigned    piece = child->_cubicles[ndx].parent->occupant;
        if (piece == Piece::pos.code()) ++num_ps;
        if (piece == Piece::neg.code()) ++num_ns;
    }

    SignatureSet    child_solutions_set;
    for (const unsigned rotator_mirrorer : child->_rotators_mirrorers) {
        std::array<Cubicle, NUMBER_OF_CUBICLES>     rotated         ;

        // Do not use unless neither, or both, "p and "n" pieces
        //   so can mirror one into the other.
        if (   rotator_mirrorer >= Rotators::Z_MIRRORED_OFFSET
            && num_ps + num_ns  == 1                          )
            continue;

        Signature   rotated_signature;
        child->generate_rotated_signature(rotated_signature, rotator_mirrorer);

        // Only insert if not already seen
        unsigned    before = child_solutions_set.size();
        child_solutions_set.insert(rotated_signature);
        if (child_solutions_set.size() != before) {
            child->_solutions  .push_back(rotated_signature);
            child->_solution_ps.push_back(num_ps / 4);  // 4 cubicles / piece
            child->_solution_ns.push_back(num_ns / 4);  // "    "     /   "
        }
    }
}   // add_solution(Shape* const)



// Similar to inline code in add_solution_no_children(), above
//   except dereferences Cubicle::_parent pointer to get cubicle occupant
//
void Shape:: generate_rotated_signature(
      Signature     &signature       ,
const unsigned       rotator_mirrorer)
const
{
    std::array<Cubicle, NUMBER_OF_CUBICLES>     rotated         ;
    bool                                        mirrored = false;

    if (rotator_mirrorer >= Rotators::Z_MIRRORED_OFFSET)
        mirrored = true;

    for (unsigned ndx = 0 ; ndx  < _num_cubicles ; ++ndx) {
        Position    cubicle_position = _cubicles[ndx];

        rotated[ndx]( cubicle_position
                     .rotate(Rotators::rotator(rotator_mirrorer)));

        // occupant not copied from parent (inefficient, would have
        //   to be done in place_piece() every time)
        // so need to dereference pointer
        unsigned    piece = _cubicles[ndx].parent->occupant;

        // Exchange "p" and "n" pieces if mirrored
        if (mirrored) {
            if (piece == Piece::pos.code())
                piece =  Piece::neg.code();
            else if (piece == Piece::neg.code())
                piece =       Piece::pos.code();
        }

        rotated[ndx].occupant = piece;
    }

    // Rotation has destroyed geometric sort order
    std::sort(rotated.begin(), rotated.begin() + _num_cubicles);

    generate_cubicles_signature(signature, rotated, _num_cubicles);

}  // generate_rotated_signature(Signature&, const unsigned) const


}  // namespace soma
