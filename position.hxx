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


#ifndef POSITION_H
#define POSITION_H

#include <algorithm>  // DEBUG (if sorting in center())
#include <cstdint>


#if SOMA_MATRIX_ROTATION + SOMA_LAMBDA_ROTATION != 1
#error #define one and only one of SOMA_MATRIX_ROTATION or SOMA_LAMBDA_ROTATION
#endif



namespace soma {

// Position (aka point, vector, coordinate) class
// Stores 3 int8_t coordinate values accessable as x(), y(), z()
//   or via operator[], returning values or references depending on
//   RHS/LHS, const/non-const)
//
class Position {
  public:
    // constructors and destructors
    constexpr Position() : _word(0) {}

    constexpr
    Position(
    int8_t      in_x,
    int8_t      in_y,
    int8_t      in_z)
    :   _coords{in_x, in_y, in_z, 0}
    {}


    // See inline, below
    template<typename Type, typename Container>
    static Position normalize(
    Container   &positions,
    unsigned     size     );


    // See inline, below
    template<typename Type, typename Container>
    static void center(
          Container     &positions,
          unsigned       size    ,
    const Position       maxes   ,
          bool           sort    );

    Position& operator()(
    int     x,
    int     y,
    int     z)
    {
        _coords[0] = x;
        _coords[1] = y;
        _coords[2] = z;
        return *this;
    }

    Position operator=(
    const Position  &other)
    {
        _coords[0] = other.x();
        _coords[1] = other.y();
        _coords[2] = other.z();
        return *this;
    }

    Position operator+=(
    const Position  &add)
    {
        _coords[0] += add.x();
        _coords[1] += add.y();
        _coords[2] += add.z();
        return *this;
    }

    Position operator-=(
    const Position  &sub)
    {
        _coords[0] -= sub.x();
        _coords[1] -= sub.y();
        _coords[2] -= sub.z();
        return *this;
    }

    Position operator>>=(
    const unsigned  right_shift)
    {
        _coords[0] >>= right_shift;
        _coords[1] >>= right_shift;
        _coords[2] >>= right_shift;
        return *this;
    }


#ifdef SOMA_MATRIX_ROTATION
    Position operator*(
    const int       rotation[3][3])
    const
    {
        Position    result;

        result._coords[0] =  rotation[0][0] * _coords[0]
                           + rotation[0][1] * _coords[1]
                           + rotation[0][2] * _coords[2];

        result._coords[1] =  rotation[1][0] * _coords[0]
                           + rotation[1][1] * _coords[1]
                           + rotation[1][2] * _coords[2];

        result._coords[2] =  rotation[2][0] * _coords[0]
                           + rotation[2][1] * _coords[1]
                           + rotation[2][2] * _coords[2];

        return  result;

    }
#endif

    bool operator==(
    const Position  &check)
    const
    {
        return _word == check._word;
    }

    bool operator!=(const Position  check) const { return !(*this == check); }

    bool operator<(
    const Position  &other)
    const
    {
        if (z() > other.z()) return true ;
        if (z() < other.z()) return false;
        if (y() > other.y()) return true ;
        if (y() < other.y()) return false;
        if (x() < other.x()) return true ;
        return false;
    }

    Position operator+(
    const Position  &add)
    const
    {
        return Position(_coords[0] + add._coords[0],
                        _coords[1] + add._coords[1],
                        _coords[2] + add._coords[2]);
    }

    Position operator*(
    const unsigned  multiplier)
    {
        Position    result;
        result._coords[0] = _coords[0] * multiplier;
        result._coords[1] = _coords[1] * multiplier;
        result._coords[2] = _coords[2] * multiplier;
        return result;
    }

    Position operator-(
    const Position  &other)
    {
        Position    result;
        result._coords[0] = _coords[0] - other.x();
        result._coords[1] = _coords[1] - other.y();
        result._coords[2] = _coords[2] - other.z();
        return result;
    }

    int8_t  operator[](int ndx)const {return  _coords[ndx]; }
    int8_t &operator[](int ndx)      {return  _coords[ndx]; }

    int8_t x() const { return   _coords[0]; }
    int8_t y() const { return   _coords[1]; }
    int8_t z() const { return   _coords[2]; }

    int8_t &x() { return _coords[0]; }
    int8_t &y() { return _coords[1]; }
    int8_t &z() { return _coords[2]; }

#ifdef SOMA_MATRIX_ROTATION
    Position rotate(
    const int   matrix[3][3])
    {
        return *this * matrix;
    }
#endif

#ifdef SOMA_LAMBDA_ROTATION
    Position rotate(
    Position(*rotator)(const Position))
    {
        return rotator(*this);
    }
#endif


  private:
    // instance data
    union {
        int8_t      _coords[4];
        uint32_t    _word;
    };

};   // class Position



// Translate a collection of Positions to (0,0,0)
//   so that all x,y,z values range from 0 to some
//   positive value
//  Returns maximum x,y,z cordinate.
//
template<typename Type, typename Container>
Position Position::normalize(
Container   &positions,
unsigned     size     )
{
    int     min_x = std::numeric_limits<int>::max(),
            min_y = std::numeric_limits<int>::max(),
            min_z = std::numeric_limits<int>::max(),
            max_x = std::numeric_limits<int>::min(),
            max_y = std::numeric_limits<int>::min(),
            max_z = std::numeric_limits<int>::min();

    for (unsigned ndx = 0 ; ndx < size ; ++ndx) {
        Type    &position = positions[ndx];
        min_x = std::min(min_x, static_cast<int>(position.x()));
        min_y = std::min(min_y, static_cast<int>(position.y()));
        min_z = std::min(min_z, static_cast<int>(position.z()));
        max_x = std::max(max_x, static_cast<int>(position.x()));
        max_y = std::max(max_y, static_cast<int>(position.y()));
        max_z = std::max(max_z, static_cast<int>(position.z()));
    }

    Position    zeroer (min_x, min_y, min_z),
                max_pos(max_x, max_y, max_z);

    max_pos -= zeroer;

    for (unsigned ndx = 0 ; ndx < size ; ++ndx)
        positions[ndx] -= zeroer;

    return max_pos;

}   // normalize()



// Center a collection of Positions around origin.
// Collection must already be in range Position(0,0,0) to Position maxes
//   (passed-in argument, typically generated by normalize(), above)
//
// if max N in x,y,z is even coords are: -N,-N+2,...-2,0,2,...,N-2,N
// "   "  " "  ","," "  odd    "     " : -N,-N+2,...-1,  1,...,N-2,N
// examples:
// 0,1          -> -1,1
// 0,1,2        -> -2,0,2
// 0,1,2,3      -> -3,-1,1,3
// 0,1,2,3,4    -> -4,-2,0,2,4
// 0,1,2,3,4,5  -> -5,-3,-1,1,3,5
// Therefor orthogonal distance between any two positions is 2,
//   regardless of odd or even
// Can optionally sort Positions after centering
//
template<typename Type, typename Container>
void Position::center(
      Container     &positions,
      unsigned       size    ,
const Position       maxes   ,
      bool           sort    )
{
    // formula: centered = 2 * original - N
    for (unsigned ndx = 0; ndx < size; ++ndx) {
        Type    &position = positions[ndx];
        position.x() = 2 * position.x() - maxes.x();
        position.y() = 2 * position.y() - maxes.y();
        position.z() = 2 * position.z() - maxes.z();
    }

    if (sort)
        std::sort(&positions[0], &positions[size]);

}   // center()

}  // namespace soma

#endif   // ifndef POSITION_H
