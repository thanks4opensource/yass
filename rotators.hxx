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


#ifndef ROTATORS_H
#define ROTATORS_H

#if SOMA_MATRIX_ROTATION + SOMA_LAMBDA_ROTATION != 1
#error #define one and only one of SOMA_MATRIX_ROTATION or SOMA_LAMBDA_ROTATION
#endif

#include "position.hxx"



namespace soma {


// Utility for rotating Position objects around X,Y,Z coordinate system axes
// Two compile-time chosen techniques:
//    1) 3x3 matrix multiply of X,Y,Z values
//    2) Lambda functions to permute X,Y,Z components
// 24 different rotations: Original +Z coordinate axis rotated to
//   one of +X, -X, +Y, -Y, +Z, -Z axes, and then 4 possible
//   90 degree rotations around that axis
//
class Rotators {
  public:
    enum {
        POSX_POSY_POSZ = 0,
        NEGY_POSX_POSZ = 1,
        NEGX_NEGY_POSZ = 2,
        POSY_NEGX_POSZ = 3,

        NEGX_POSY_NEGZ = 4,
        NEGY_NEGX_NEGZ = 5,
        POSX_NEGY_NEGZ = 6,
        POSY_POSX_NEGZ = 7,

        POSX_NEGZ_POSY = 8,
        POSZ_POSX_POSY = 9,
        NEGX_POSZ_POSY = 10,
        NEGZ_NEGX_POSY = 11,

        POSX_POSZ_NEGY = 12,
        NEGZ_POSX_NEGY = 13,
        NEGX_NEGZ_NEGY = 14,
        POSZ_NEGX_NEGY = 15,

        NEGZ_POSY_POSX = 16,
        NEGY_NEGZ_POSX = 17,
        POSZ_NEGY_POSX = 18,
        POSY_POSZ_POSX = 19,

        POSZ_POSY_NEGX = 20,
        NEGY_POSZ_NEGX = 21,
        NEGZ_NEGY_NEGX = 22,
        POSY_NEGZ_NEGX = 23,

        MAX_NUMBER_OF_ORIENTATIONS,
    };

    static const unsigned   Z_MIRRORED_OFFSET = MAX_NUMBER_OF_ORIENTATIONS    ,
                            X_MIRRORED_OFFSET = MAX_NUMBER_OF_ORIENTATIONS * 2;


#ifdef SOMA_MATRIX_ROTATION
using RotatorGetter = const int(*)[3];
    static RotatorGetter rotator(
    const unsigned index)
    {
        return rotations[index];
    }
#endif

#ifdef SOMA_LAMBDA_ROTATION
using RotatorGetter = Position(*)(Position);
    static RotatorGetter rotator(
    const unsigned  index)
    {
        return rotations[index];
    }
#endif


#ifdef SOMA_MATRIX_ROTATION
    static const int    rotations[MAX_NUMBER_OF_ORIENTATIONS * 3][3][3];
#endif

#ifdef SOMA_LAMBDA_ROTATION
    static Position (*rotations[MAX_NUMBER_OF_ORIENTATIONS * 3])(const Position);
#endif

};  // class Rotators

}  // namespace soma

#endif   // ifndef ROTATORS_H
