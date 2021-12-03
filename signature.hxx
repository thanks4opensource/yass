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


#ifndef SIGNATURE_HXX
#define SIGNATURE_HXX

#include <cstdint>
#include <iostream>
#include <string>



namespace soma {

// Linear string of Piece::code()s (0-6) in sorted order of cubicles
//   in solved shape.
// Used for comparing rotated/reflected solutions with already found ones.
// Efficiently stored, two codes per byte, in 14 byte aliased (union)
//   to two 64 bit words so only two comparisons (==, <, etc)
//   needed between two Signatures
//
class Signature {
  public:
    Signature()
    :   _words{0}
    {}

    Signature(
    const Signature &other)
    {
        for (unsigned ndx = 0 ; ndx < NUMBER_OF_WORDS ; ++ndx)
            _words[ndx] = other._words[ndx];
    }

    Signature& operator=(
    const Signature     &other)
    {
        // hard-coded for NUMBER_OF_WORDS==2
        _words[0] = other._words[0];
        return *this;
    }

    uint8_t operator[](
    unsigned    index)
    const
    {
        if (index & 1)
            return _bytes[index >> 1] >> 4;
        else
            return _bytes[index >> 1] & 0x0f;
    }

    bool operator==(
    const Signature &other)
    const
    {
        // hard-coded for NUMBER_OF_WORDS==2
        return !(other._words[0] != _words[0] || other._words[1] != _words[1]);
    }

    bool operator!=(const Signature &other) const { return !operator==(other); }

    bool operator<(
    const Signature &other)
    const
    {
        if (_words[0] < other._words[0]) return true ;
        if (_words[0] > other._words[0]) return false;
        if (_words[1] < other._words[1]) return true ;
        if (_words[1] > other._words[1]) return false;
        return false;
    }

#ifdef SOMA_STD_SET_UNORDERED
    struct Hash {
        std::size_t operator()(
        const Signature     &signature)
        const
        {
            std::size_t     hash = signature._words[0];

            hash ^= signature._words[0] >> 32;
            hash ^= signature._words[1]      ;
            hash ^= signature._words[1] >> 32;

            return hash;
        }
    };

    struct Equal {
        bool operator()(
        const Signature     &left,
        const Signature     &right)
        const
        {
            return left.operator==(right);
        }
    };

    friend class Hash;
#endif



  protected:
    static const unsigned       NUMBER_OF_CUBICLES = 27;

    class Nibble {
      public:
        Nibble(
              uint8_t       &byte ,
        const uint8_t        mask ,
        const unsigned       shift)
        :   _byte (byte ),
            _mask (mask ),
            _shift(shift)
        {}

        operator uint8_t() const { return (_byte & _mask) >> _shift; }

        Nibble& operator=(
        const uint8_t   value)
        {
            _byte = (_byte & ~_mask) | ((value & 0x0f) << _shift);
            return *this;
        }

        Nibble& operator=(
        const Nibble&   other)
        {
            _byte =   (_byte & ~_mask)
                    | (static_cast<uint8_t>(other & 0x0f) << _shift);
            return *this;
        }


      protected:
              uint8_t   &_byte ;
        const uint8_t    _mask ,
                         _shift;
    };  // class Nibble

    static const unsigned   NUMBER_OF_BYTES = (NUMBER_OF_CUBICLES + 1) >> 1,
                            NUMBER_OF_WORDS = (NUMBER_OF_BYTES    + 8) >> 3;

    union {
        uint8_t     _bytes[NUMBER_OF_BYTES];
        uint64_t    _words[NUMBER_OF_WORDS];
    };



  public:
    // Must be after Nibble definition
    Nibble operator[](
    unsigned    index)
    {
        if (index & 1)
            return Nibble(_bytes[index >> 1], 0xf0, 4);
        else
            return Nibble(_bytes[index >> 1], 0x0f, 0);
    }

};  // class Signature

}  // namespace soma

#endif  // #ifndef SIGNATURE_HXX
