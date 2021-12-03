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


#include "rotators.hxx"


namespace soma {

// See rotators.hxx

#ifdef SOMA_MATRIX_ROTATION
const int       Rotators::rotations[][3][3] = {
    // normal
    { { 1,  0,  0},     // +Z up        // 0
      { 0,  1,  0},
      { 0,  0,  1} },
    { { 0, -1,  0},                     // 1
      { 1,  0,  0},
      { 0,  0,  1} },
    { {-1,  0,  0},                     // 2
      { 0, -1,  0},
      { 0,  0,  1} },
    { { 0,  1,  0},                     // 3
      {-1,  0,  0},
      { 0,  0,  1} },
    { {-1,  0,  0},     // -Z up        // 4
      { 0,  1,  0},
      { 0,  0, -1} },
    { { 0, -1,  0},                     // 5
      {-1,  0,  0},
      { 0,  0, -1} },
    { { 1,  0,  0},                     // 6
      { 0, -1,  0},
      { 0,  0, -1} },
    { { 0,  1,  0},                     // 7
      { 1,  0,  0},
      { 0,  0, -1} },
    { { 1,  0,  0},     // +Y up        // 8
      { 0,  0, -1},
      { 0,  1,  0} },
    { { 0,  0,  1},                     // 9
      { 1,  0,  0},
      { 0,  1,  0} },
    { {-1,  0,  0},                     // 10
      { 0,  0,  1},
      { 0,  1,  0} },
    { { 0,  0, -1},                     // 11
      {-1,  0,  0},
      { 0,  1,  0} },
    { { 1,  0,  0},     // -Y up        // 12
      { 0,  0,  1},
      { 0, -1,  0} },
    { { 0,  0, -1},                     // 13
      { 1,  0,  0},
      { 0, -1,  0} },
    { {-1,  0,  0},                     // 14
      { 0,  0, -1},
      { 0, -1,  0} },
    { { 0,  0,  1},                     // 15
      {-1,  0,  0},
      { 0, -1,  0} },
    { { 0,  0, -1},     // +X up        // 16
      { 0,  1,  0},
      { 1,  0,  0} },
    { { 0, -1,  0},                     // 17
      { 0,  0, -1},
      { 1,  0,  0} },
    { { 0,  0,  1},                     // 18
      { 0, -1,  0},
      { 1,  0,  0} },
    { { 0,  1,  0},                     // 19
      { 0,  0,  1},
      { 1,  0,  0} },
    { { 0,  0,  1},     // -X up        // 20
      { 0,  1,  0},
      {-1,  0,  0} },
    { { 0, -1,  0},                     // 21
      { 0,  0,  1},
      {-1,  0,  0} },
    { { 0,  0, -1},                     // 22
      { 0, -1,  0},
      {-1,  0,  0} },
    { { 0,  1,  0},                     // 23
      { 0,  0, -1},
      {-1,  0,  0} },

    // pre-mirrored in Z
    { { 1,  0,  0},     // +Z up        // 0
      { 0,  1,  0},
      { 0,  0, -1} },
    { { 0, -1,  0},                     // 1
      { 1,  0,  0},
      { 0,  0, -1} },
    { {-1,  0,  0},                     // 2
      { 0, -1,  0},
      { 0,  0, -1} },
    { { 0,  1,  0},                     // 3
      {-1,  0,  0},
      { 0,  0, -1} },
    { {-1,  0,  0},     // -Z up        // 4
      { 0,  1,  0},
      { 0,  0,  1} },
    { { 0, -1,  0},                     // 5
      {-1,  0,  0},
      { 0,  0,  1} },
    { { 1,  0,  0},                     // 6
      { 0, -1,  0},
      { 0,  0,  1} },
    { { 0,  1,  0},                     // 7
      { 1,  0,  0},
      { 0,  0,  1} },
    { { 1,  0,  0},     // +Y up        // 8
      { 0,  0,  1},
      { 0,  1,  0} },
    { { 0,  0, -1},                     // 9
      { 1,  0,  0},
      { 0,  1,  0} },
    { {-1,  0,  0},                     // 10
      { 0,  0, -1},
      { 0,  1,  0} },
    { { 0,  0,  1},                     // 11
      {-1,  0,  0},
      { 0,  1,  0} },
    { { 1,  0,  0},     // -Y up        // 12
      { 0,  0, -1},
      { 0, -1,  0} },
    { { 0,  0,  1},                     // 13
      { 1,  0,  0},
      { 0, -1,  0} },
    { {-1,  0,  0},                     // 14
      { 0,  0,  1},
      { 0, -1,  0} },
    { { 0,  0, -1},                     // 15
      {-1,  0,  0},
      { 0, -1,  0} },
    { { 0,  0,  1},     // +X up        // 16
      { 0,  1,  0},
      { 1,  0,  0} },
    { { 0, -1,  0},                     // 17
      { 0,  0,  1},
      { 1,  0,  0} },
    { { 0,  0, -1},                     // 18
      { 0, -1,  0},
      { 1,  0,  0} },
    { { 0,  1,  0},                     // 19
      { 0,  0, -1},
      { 1,  0,  0} },
    { { 0,  0, -1},     // -X up        // 20
      { 0,  1,  0},
      {-1,  0,  0} },
    { { 0, -1,  0},                     // 21
      { 0,  0, -1},
      {-1,  0,  0} },
    { { 0,  0,  1},                     // 22
      { 0, -1,  0},
      {-1,  0,  0} },
    { { 0,  1,  0},                     // 23
      { 0,  0,  1},
      {-1,  0,  0} },

    // pre-mirrored in X
    { {-1,  0,  0},     // +Z up        // 0
      { 0,  1,  0},
      { 0,  0,  1} },
    { { 0, -1,  0},                     // 1
      {-1,  0,  0},
      { 0,  0,  1} },
    { { 1,  0,  0},                     // 2
      { 0, -1,  0},
      { 0,  0,  1} },
    { { 0,  1,  0},                     // 3
      { 1,  0,  0},
      { 0,  0,  1} },
    { { 1,  0,  0},     // -Z up        // 4
      { 0,  1,  0},
      { 0,  0, -1} },
    { { 0, -1,  0},                     // 5
      { 1,  0,  0},
      { 0,  0, -1} },
    { {-1,  0,  0},                     // 6
      { 0, -1,  0},
      { 0,  0, -1} },
    { { 0,  1,  0},                     // 7
      {-1,  0,  0},
      { 0,  0, -1} },
    { {-1,  0,  0},     // +Y up        // 8
      { 0,  0, -1},
      { 0,  1,  0} },
    { { 0,  0,  1},                     // 9
      {-1,  0,  0},
      { 0,  1,  0} },
    { { 1,  0,  0},                     // 10
      { 0,  0,  1},
      { 0,  1,  0} },
    { { 0,  0, -1},                     // 11
      { 1,  0,  0},
      { 0,  1,  0} },
    { {-1,  0,  0},     // -Y up        // 12
      { 0,  0,  1},
      { 0, -1,  0} },
    { { 0,  0, -1},                     // 13
      {-1,  0,  0},
      { 0, -1,  0} },
    { { 1,  0,  0},                     // 14
      { 0,  0, -1},
      { 0, -1,  0} },
    { { 0,  0,  1},                     // 15
      { 1,  0,  0},
      { 0, -1,  0} },
    { { 0,  0, -1},     // +X up        // 16
      { 0,  1,  0},
      {-1,  0,  0} },
    { { 0, -1,  0},                     // 17
      { 0,  0, -1},
      {-1,  0,  0} },
    { { 0,  0,  1},                     // 18
      { 0, -1,  0},
      {-1,  0,  0} },
    { { 0,  1,  0},                     // 19
      { 0,  0,  1},
      {-1,  0,  0} },
    { { 0,  0,  1},     // -X up        // 20
      { 0,  1,  0},
      { 1,  0,  0} },
    { { 0, -1,  0},                     // 21
      { 0,  0,  1},
      { 1,  0,  0} },
    { { 0,  0, -1},                     // 22
      { 0, -1,  0},
      { 1,  0,  0} },
    { { 0,  1,  0},                     // 23
      { 0,  0, -1},
      { 1,  0,  0} },
};
#endif

#ifdef SOMA_LAMBDA_ROTATION
Position (*Rotators::rotations[])(const Position) = {
#define R(X, Y, Z) \
    [](const Position pos) { return Position(X(), Y(), Z()); },

// orientation: looking inward along axis towards origin
    // normal
    R( pos.x,  pos.y,  pos.z)   // +Z   // 0
    R(-pos.y,  pos.x,  pos.z)   // +Z   // 1
    R(-pos.x, -pos.y,  pos.z)   // +Z   // 2
    R( pos.y, -pos.x,  pos.z)   // +Z   // 3

    R(-pos.x,  pos.y, -pos.z)   // -Z   // 4
    R(-pos.y, -pos.x, -pos.z)   // -Z   // 5
    R( pos.x, -pos.y, -pos.z)   // -Z   // 6
    R( pos.y,  pos.x, -pos.z)   // -Z   // 7

    R( pos.x, -pos.z,  pos.y)   // +Y   // 8
    R( pos.z,  pos.x,  pos.y)   // +Y   // 9
    R(-pos.x,  pos.z,  pos.y)   // +Y   // 10
    R(-pos.z, -pos.x,  pos.y)   // +Y   // 11

    R( pos.x,  pos.z, -pos.y)   // -Y   // 12
    R(-pos.z,  pos.x, -pos.y)   // -Y   // 13
    R(-pos.x, -pos.z, -pos.y)   // -Y   // 14
    R( pos.z, -pos.x, -pos.y)   // -Y   // 15

    R(-pos.z,  pos.y,  pos.x)   // +X   // 16
    R(-pos.y, -pos.z,  pos.x)   // +X   // 17
    R( pos.z, -pos.y,  pos.x)   // +X   // 18
    R( pos.y,  pos.z,  pos.x)   // +X   // 19

    R( pos.z,  pos.y, -pos.x)   // -X   // 20
    R(-pos.y,  pos.z, -pos.x)   // -X   // 21
    R(-pos.z, -pos.y, -pos.x)   // -X   // 22
    R( pos.y, -pos.z, -pos.x)   // -X   // 23

    // pre-mirrored in Z
    R( pos.x,  pos.y, -pos.z)   // +Z   // 0
    R(-pos.y,  pos.x, -pos.z)   // +Z   // 1
    R(-pos.x, -pos.y, -pos.z)   // +Z   // 2
    R( pos.y, -pos.x, -pos.z)   // +Z   // 3

    R(-pos.x,  pos.y,  pos.z)   // -Z   // 4
    R(-pos.y, -pos.x,  pos.z)   // -Z   // 5
    R( pos.x, -pos.y,  pos.z)   // -Z   // 6
    R( pos.y,  pos.x,  pos.z)   // -Z   // 7

    R( pos.x, -pos.z, -pos.y)   // +Y   // 8
    R( pos.z,  pos.x, -pos.y)   // +Y   // 9
    R(-pos.x,  pos.z, -pos.y)   // +Y   // 10
    R(-pos.z, -pos.x, -pos.y)   // +Y   // 11

    R( pos.x,  pos.z,  pos.y)   // -Y   // 12
    R(-pos.z,  pos.x,  pos.y)   // -Y   // 13
    R(-pos.x, -pos.z,  pos.y)   // -Y   // 14
    R( pos.z, -pos.x,  pos.y)   // -Y   // 15

    R(-pos.z,  pos.y, -pos.x)   // +X   // 16
    R(-pos.y, -pos.z, -pos.x)   // +X   // 17
    R( pos.z, -pos.y, -pos.x)   // +X   // 18
    R( pos.y,  pos.z, -pos.x)   // +X   // 19

    R( pos.z,  pos.y,  pos.x)   // -X   // 20
    R(-pos.y,  pos.z,  pos.x)   // -X   // 21
    R(-pos.z, -pos.y,  pos.x)   // -X   // 22
    R( pos.y, -pos.z,  pos.x)   // -X   // 23

    // pre-mirrored in X
    R(-pos.x,  pos.y,  pos.z)   // +Z   // 0
    R( pos.y,  pos.x,  pos.z)   // +Z   // 1
    R( pos.x, -pos.y,  pos.z)   // +Z   // 2
    R(-pos.y, -pos.x,  pos.z)   // +Z   // 3

    R( pos.x,  pos.y, -pos.z)   // -Z   // 4
    R( pos.y, -pos.x, -pos.z)   // -Z   // 5
    R(-pos.x, -pos.y, -pos.z)   // -Z   // 6
    R(-pos.y,  pos.x, -pos.z)   // -Z   // 7

    R(-pos.x, -pos.z,  pos.y)   // +Y   // 8
    R(-pos.z,  pos.x,  pos.y)   // +Y   // 9
    R( pos.x,  pos.z,  pos.y)   // +Y   // 10
    R( pos.z, -pos.x,  pos.y)   // +Y   // 11

    R(-pos.x,  pos.z, -pos.y)   // -Y   // 12
    R( pos.z,  pos.x, -pos.y)   // -Y   // 13
    R( pos.x, -pos.z, -pos.y)   // -Y   // 14
    R(-pos.z, -pos.x, -pos.y)   // -Y   // 15

    R( pos.z,  pos.y,  pos.x)   // +X   // 16
    R( pos.y, -pos.z,  pos.x)   // +X   // 17
    R(-pos.z, -pos.y,  pos.x)   // +X   // 18
    R(-pos.y,  pos.z,  pos.x)   // +X   // 19

    R(-pos.z,  pos.y, -pos.x)   // -X   // 20
    R( pos.y,  pos.z, -pos.x)   // -X   // 21
    R( pos.z, -pos.y, -pos.x)   // -X   // 22
    R(-pos.y, -pos.z, -pos.x)   // -X   // 23

//  [](const Position pos) { return Position(-pos.y(), pos.x(), pos.z()); },
//  [](const Position pos) { return Position(-pos.y(), pos.x(), pos.z()); }

#undef R
};
#endif

}  // namespace soma
