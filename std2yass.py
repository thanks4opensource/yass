# yass: Yet Another Soma Solver
# Copyright (C) 2021 Mark R. Rubin aka "thanks4opensource"
#
# This file is part of yass.
#
# The yass program is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# The yass program is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# (LICENSE.txt) along with the yass program.  If not, see
# <https:#www.gnu.org/licenses/gpl.html>


#!/usr/bin/env python3

import argparse
import sys



def std2mrr(infile, outfile):
    height = None
    width  = None
    depth  = 0
    for line in infile:
        if not line or ';' in line or '/SOMA' in line:
            continue
        layers = line.strip()[1:].split('/')
        if height:
            if len(layers) != height:
                raise ValueError(  "Mismatched heights: %d vs %d"
                                 % (len(layers), height)        )
        else:
            height = len(layers)
            mrr = [[] for _ in range(height)]
        if width:
            if len(layers[0]) != width:
                raise ValueError(  "Mismatched widths: %d vs %d"
                                 % (len(layers), width)     )
        else:
            width = len(layers[0])
        for (z, layer) in enumerate(layers):
            piece_empty = ''.join(['.'
                                   if std in '.-0' else 'o'
                                   for std in layer       ])
            mrr[z].append(piece_empty)
        depth += 1
    # outfile.write("%d %d %d * -\n" % (width, depth, height))
    for layer in mrr[:-1]:
        outfile.write('\n'.join(layer) + '\n\n')
    outfile.write('\n'.join(mrr[-1]) + '\n')



def parse_commandline():
    parser = argparse.ArgumentParser(
                formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('infile',
                        nargs='?',
                        type=argparse.FileType('r'),
                        default=sys.stdin)

    parser.add_argument('outfile',
                        nargs='?',
                        type=argparse.FileType('w'),
                        default=sys.stdout)

    return parser.parse_args()



if __name__ == "__main__":
    args = parse_commandline()

    std2mrr(args.infile, args.outfile)

    args. infile.close()
    args.outfile.close()
