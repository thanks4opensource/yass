yass: Yet Another Soma Solver
=================================

**yass** is an open-source program for solving Piet Hein's Soma puzzle.

* Solves arbitrary Soma puzzle shapes
* Finds all solutions, culling rotated/reflected duplicates (or optionally not)
* Solves separated shapes, with individual part rotation/reflection culling
* Supports pre-placement of Soma pieces (subset of all solutions)
* Very fast performance: 0.024 seconds to solve the basic 3x3x3 cube shape, 0.0017 seconds/solve average on 4000+ example figures [<sup>1</sup>](#footnote1), on mid-level hardware
* Runtime performance tuning options
* Text-based commandline executable with simple input file format, implemented as wrapper around independent C++ solver engine.




Contents  <a name="contents"></a>
--------
* [License](#license)
* [Introduction](#introduction)
* [Purpose](#purpose)
* [Compiling](#compiling)
    * [Compilation options](#compilation_options)
    * [Unittest](#unittest)
* [Program usage](#program_usage)
    * [Basic help](#basic_help)
    * [Extended help](#extended_help)
    * [Tuning options](#tuning_options)
    * [std2yass.py](#std2yass_py)
* [Algorithms and implementation](#algorithms_and_implementation)
    * [Basic algorithm](#basic_algorithm)
    * [Optimizations](#optimizations)
    * [Implementation](#implementation)
* [Autobio(soma)graphy](#autobio_soma_graphy)
* [Future work](#future_work)
* [Footnotes](#footnotes)



License  <a name="license"></a>
-------

yass: Yet Another Soma Solver

Copyright (C) 2021 Mark R. Rubin

This file is part of yass.

The yass program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

The yass program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the [GNU General Public License](LICENSE.txt) along with the yass program.  If not, see <https://www.gnu.org/licenses/gpl.html>



Introduction <a name="introduction"></a>
------------

The Soma Cube is arguably [Piet Hein's](https://en.wikipedia.org/wiki/Piet_Hein_(scientist)) greatest invention, although his popularization of [superellipses/superellipsoids](https://en.wikipedia.org/wiki/Superellipse)/[supereggs](https://en.wikipedia.org/wiki/Superegg) and his ["grooks"](https://en.wikipedia.org/wiki/Grook) (short poems) [<sup>2</sup>](#footnote2) are strong alternate contenders.

It's safe to assume that anyone reading this is already familiar with the Soma cube. In case not, there are many excellent online references including [Wikipedia](https://en.wikipedia.org/wiki/Soma_cube), and [Thorlief's](https://www.fam-bundgaard.dk/SOMA/SOMA.HTM) excellent and extensive (if somewhat disjointed) website.

Better yet, make ([Instructables](https://www.instructables.com/Make-a-Wooden-Soma-Cube/), [Thingiverse](https://www.thingiverse.com/search?q=soma+cube&type=things&sort=relevant)) or buy (Piet Hein's [posthumous website](https://piethein.com/shop/183-games-amp-puzzles/217-soma-888-cm---wood-invented-1933/), [Amazon](https://www.amazon.com/s?k=soma+cube&i=toys-and-games)) a Soma cube and experiment with it.  Then come back to this software and compare your results.



Purpose <a name="purpose"></a>
--------------

**yass** was written for investigating interesting [<sup>3</sup>](#footnote3) details about Soma figures and their solutions. Things like:

* Is there a solution to a given shape? One of the joys of Soma is that in addition to the coincidence that Piet Hein originally set out to prove (can the seven Soma pieces in fact be assembled into a 3x3x3 cube?) there exist so many other impressionistic/evocative shapes, the vast majority of which *are* solvable. See several thousand examples at [Thorlief's](https://www.fam-bundgaard.dk/SOMA/FIGURES/ALLFIGS.HTM) pages. 

* How many different solutions exist for a given figure? The program correctly finds the 240 unique solutions (11520 including rotations and reflections) for the basic 3x3x3 cube. The default output with rotations and reflections excluded greatly aids in understanding meaningful differences between the solutions.

* Are there any solutions given restrictions on how one or more pieces are placed? For example, the program can be used as a brute-force demonstration of the elegant proof listed in [Wikipedia](https://en.wikipedia.org/wiki/Soma_cube#Solutions)  that all cube solutions must have the "t" piece spanning two corners of the cube ("#" characters in the output indicate an unsolvable figure):

            $ ./soma figures/bad_t_face_cube.soma 
            ###
            ###
            ###

            ###
            ###
            ###

            #t#
            #tt
            #t#

            $ ./soma figures/bad_t_center_1_cube.soma 
            ###
            ###
            ###

            ###
            #t#
            ###

            #t#
            #t#
            #t#

            $ ./soma figures/bad_t_center_3_cube.soma
            ###
            #t#
            ###

            ###
            #tt
            ###

            ###
            #t#
            ###

* Or alternately, a correctly pre-placed "t" piece yields the full count of 240 cube solutions, proving that such placement is mandatory without manually checking the complete output of `soma -a figures/cube.soma` ("o" characters indicate normal, non-pre-placed, shape cubicles -- see [Extended help](#extended_help), and input file format and piece naming  [notes](#file_format_notes), below).

            $ cat figures/good_t_cube.soma 
            ooo
            ooo
            ooo

            ooo
            ooo
            ooo

            ooo
            oto
            ttt

            $ ./soma -c figures/good_t_cube.soma
            figures/good_t_cube.soma: 240 solutions

            $ cat figures/cube.soma
            ooo
            ooo
            ooo

            ooo
            ooo
            ooo

            ooo
            ooo
            ooo

            $ ./soma -c figures/cube.soma
            figures/cube.soma: 240 solutions

            $ ./soma -a figures/cube.soma
            solution #1
            3zz
            zzc
            ttt

            33c
            ncc
            ptl

            nnl
            npl
            ppl

            ... solutions #2 through #239 ...

            solution #240
            ccl
            czl
            3ll

            czn
            tzp
            33p

            tzn
            tnn
            tpp


* What about the disappointing [<sup>4</sup>](#footnote4) fact that not all interesting figures are solvable? For example, my own personal greatest disappointment, the "windowed cube":

            $ ./soma figures/internal_corner_hole_cube.soma 
            ### ..
            #.# ..
            ### ..

            ### ..
            #.. ..
            #.# #.

            ### ..
            ### #.
            ### ##

* What interesting separated shapes can be combined into more basic ones (such as the basic cube)?

            $ ./soma figures/disassemblable_cube.soma 
            ll. t..
            z.. ...
            ... ...

            zln tp.
            z.. tp.
            c.. ...

            zln tpp
            cnn 33.
            cc. 3..

* And many other possibilities for for confirming or contradicting hypotheses formed while playing with Soma by hand.



Compiling <a name="compiling"></a>
---------

The included source files (`./*.[ch]xx`) have been developed using GNU GCC (g++) version 7.5.0 with `-std=c++11`. Reports of incompatibilities with later releases or other compliant C++ compilers and/or pull requests with fixes for same are welcome [<sup>5</sup>](#footnote5).

#### Compilation options <a name="compilation_options"></a>

A [`Makefile`](Makefile) is provided. Commandline-overridable `make` variables allow easy setting of several compile-time switches controlling the implementation and features of the resulting executable. These include the following, although note that as per the runtime commandline [tuning options](#tuning_options) below, the performance gains or losses resulting from changing them are usually minimal.

##### Rotation

Makefile or `make` commandline: `ROTATION=matrix` or `ROTATION=lambda`
<br>Source files:  `#define SOMA_MATRIX_ROTATION` or `#define SOMA_LAMBDA_ROTATION`
<br>Default: `ROTATION=matrix` and `#define SOMA_MATRIX_ROTATION`

An important part of the program's execution involves rotating and/or reflecting the 7 Soma pieces and/or entire Soma figures. Two alternate implementations are provided: One which multiplies X,Y,Z coordinates by 3x3 rotation matrices, and one which directly swaps the components using variable assignment statements. See e.g.  [`rotators.cxx`](rotators.cxx) and the two versions of `static inline void rotate()` in [piece.hxx](piece.hxx).

It seems intuitively obvious that the `lambda` (swapping) version would be faster, but testing on typical modern CPUs shows the reverse, probably due to the massive hardware parallelism in such hardware. Again, the overall performance gain or loss between the two versions is very minimal.


##### std::set

Makefile or `make` commandline: `STD_SET=set` or `STD_SET=unordered`
<br>Source files: `#define SOMA_STD_SET_SET` or `#define SOMA_STD_SET_UNORDERED`
<br>Default: `STD_SET=unordered` and `#define SOMA_STD_SET_UNORDERED`

Another important implementation detail is use of the STL (C++ Standard Template Library) "set" metaclasses for checking duplicate Soma solutions, among  other things. A slight performance increase can be obtained by using `std::unordered_set` instead of `std:set`. Allowing the latter is included as it is more generally available than the former, although other parts of the code require C++11 or above which implies that `unordered_set` wil be present according to the C++ standards.


##### stats

Makefile or `make` commandline: `STATS=-D` or `STATS=-U`
<br>Source files: `#define SOMA_STATISTICS` or `#undef SOMA_STATISTICS`
<br>Default: `STATS=-U` and `#undef SOMA_STATISTICS`

Defining `SOMA_STATISTICS` will conditionally compile tracking of interesting [<sup>6</sup>](#footnote6) statistics regarding the algorithm's execution on given Soma figures. It also enables the runtime commandline `-s` option to output the statistics. Due to the slight performance decrease caused by gathering the statistics (whether or not `-s` is specified), they are turned off by default.


#### Unittest <a name="unittest"></a>

A unit test suite is provided, using example Soma shapes in the [`figures`](figures) subdirectory and corret results in the [`tests`](tests) directory. Execute `make test` to run the tests. A known incompatibility when using certain commandline options is handled via a text file containing the expected output.



Program usage <a name="program_usage"></a>
-------------

Run the program with one of its two (basic and extended) help options.

#### Basic help: <a name="basic_help"></a>

        $ ./soma -h
        Solve SOMA figure(s).

        Usage: ./soma [OPTIONS] <FILE> [FILES...]

        FILE:       filename or "-" for standard input
        FILES:      additional files

        OPTIONS:
          -a            all solutions (only unique solutions by default)
          -r            include rotated and reflected solutions (forces -D 0)
          -c            only count of solutions, not solution(s) themselves
          -t            print elapsed time to solve figures
          -n            print filename before solution(s)
          -o <FILE>     output to file instead of standard output
          -O <pieces>   orphans check:     1 to 7 numbers, each 1 thru 7,
                                           or single 0 (default: 123456)
          -D <pieces>   duplicates checks: as per -O (default: 17)
          -S <pieces>   symmetry checks:   as per -O (default: 0)
          -P <pieces>   piece order:       7 characters, exactly one each of
                                           "cpnztl3" (default: ztcpnl3)
          -h            this help text
          -H            extended help


#### Extended help: <a name="extended_help"></a>

        $ ./soma -H
        Extended help.

        Usage: ./soma [OPTIONS] <FILE> [FILES...]

        OPTIONS (partial list, see "-h" for "<pieces>" syntax)
          -O <pieces>   orphans   check pieces (default: 123456)
          -D <pieces>   duplicate check pieces (default: 17)
          -S <pieces>   symmetry  check pieces (default: 0)
          -P <pieces>   piece order            (default: ztcpnl3)
          -h            basic help text (full list of options)
          -H            this extended help

        Orphan check (-O option):
          Program implements a recursive tree search, attempting to insert
          each SOMA piece in turn into SOMA figure. Pieces are numbered
          1 through 7, with insertion order specified by -P option.

          If piece number is in -O option, code checks for "orphan" cubicles
          after the piece is inserted. Orphan cubicles are single, or two
          orthogonally joined, cubicle(s) that are empty and not orthogonally
          joined to other empty cubicles.

          Checking consumes solving time but if successful prunes potentially
          large amounts of search tree space thus improving solution
          performance. Check is less likely to find orphans at earlier piece
          numbers but if successful prunes larger amount of search tree.
          Default values produce best results on large example set of SOMA
          figures. Checking at piece 7 is not useful because no orphans can
          exist after last piece. Set -O 0 to turn off all orphan checking.

        Duplicate check (-D option):
          During recursive tree search, program will check for duplicate
          solutions (rotated and/or mirrored) after each piece insertion if
          piece number is in -D <pieces>. Numbers/pieces as per orphan check,
          above. Setting -D 0 (or -r option) turns off all checking and
          reports all solutions.

          Checks at 1 through 6 are for each individual piece.
          Check at piece 7 is for entire figure including individually
          rotated/mirrored separated SOMA shapes. Missing solutions can
          result if neither piece 7, nor all of 1 through 6, are set.
          Additionally, setting 1-6 without 7 may result in missing solutions
          for separated shapes; the program forces -D 7 for such cases.

          Except for checking at piece #1, the computation time required for
          duplicate checking generally results in an overall increase in
          solution time.

        Symmetry check (-S option):
          During recursive tree search, program will check for symmetric SOMA
          shape cubicles before inserting each piece if piece number is in -S
          <pieces>. As per -D option, computation time required for symmetry
          checking (except at piece #1) generally results in an overall
          increase in solution time.

          Symmetry checking without duplicate checking (at the same piece
          number) is not sufficient for culling all duplicated
          rotated/mirrored solutions. Duplicate checking is efficient enough
          that despite the potentially large percentage of symmetric cubicles
          (4 non-symmetric out of 27 total for the basic 3x3x3 SOMA cube shape)
          overall performance gains are usually not significant. Note that
          symmetry checking of the "p", "n", "z", or "l" pieces can produce
          specious results.

        Piece order (-P option):
          Order in which solver will attempt to place pieces into shape. Affects
          performance, but no universally-best order exists. In general "easier"
          pieces such as "l" and "3" should be at end of order. Pieces "p" and
          "n" must be contiguous, in that order, if -D option is enabled for
          either.

        File format:
          - Z slices of SOMA figure, separated by blank line(s)
          - Each slice: Y lines of X cubicles
          - SOMA shape defined by "." or " " characters for empty space,
            any other character for shape cubicle
          - Characters "c", "p", "n", "z", "t", "l", "3" to pre-place piece(s)
          - "#" character to end of line is comment, ignored

          Example, SOMA "battleship" figure (indented here by 4 spaces, but
          will still solve correctly):

            # The classic battleship figure
            ....o....    # top Z slice
            .........    # second Y line of slice

            ...ooo...
            .........

            ..coooo..    # pre-place "c" piece
            .........

            occoooooo    # bottom Z slice
            oocoooooo    # with remaining "c" piece cubicles

          More info:
          - Orientation unimportant, suggest minimal Z slices, ordered top-to-bottom
          - Figures with multiple separated shapes accepted (useful for forcing
            specific solutions, and for additional removal of reflections/rotations)
          - Pre-placed pieces checked for correct number of cubicles (3 for "3" piece,
            4 for all others) but not for geometric shape (if wrong will produce
            incorrect solutions)
          - Tab characters not allowed except after "#" comment character

<a name="file_format_notes"></a>
I realize this file format (much less the piece names) don't match what is likely [common practice](https://www.fam-bundgaard.dk/SOMA/NOTATION.HTM). See [std2yass.py](#std2yass_py), and particularly the [Autobio(soma)graphy](#autobio_soma_graphy), below. I've been writing Soma solver software for a very long time and it's too late to change now. It never occurred to me to call the pieces anything other than "corner", "positive", "negative", "zee", "tee", "ell", and "three". I see the logic behind naming the last one "vee", but basing that on a unique non-orthogonal rotation is too incongruous. Similarly, the "slice" file format seems as intuitively obvious now as it did decades ago when I first implemented it.


##### Tuning options <a name="tuning_options"></a>

Note that the performance tuning options (`-O`, `-D`, `-S`, and `-P`, [above](#extended_help)) are included mainly for completeness and experimentation. The default values perform perfectly adequately in almost all cases, and the program is more than fast enough for any conceivable interactive use with all but the most pathological of settings.


### `std2yass.py` <a name="std2yass_py"></a>

Additionally, a small Python script ([`std2yass.py`](std2yass.py)) is included for converting ["standard"](https://www.fam-bundgaard.dk/SOMA/NOTATION.HTM) figure notation to the file format used by yass.

        $ ./std2yass.py -h 
        usage: std2yass.py [-h] [infile] [outfile]

        positional arguments:
          infile
          outfile

        optional arguments:
          -h, --help  show this help message and exit

        $ cat snake.std
        /22......../.2......../.244....../.446......
        /........../........../...665..../...655....
        /........../........../.....537../.....333..
        /........../........../.......771/.......711

        $ ./std2yass.py snake.std
        oo........
        ..........
        ..........
        ..........

        .o........
        ..........
        ..........
        ..........

        .ooo......
        ...ooo....
        .....ooo..
        .......ooo

        .ooo......
        ...ooo....
        .....ooo..
        .......ooo

        $ ./std2yass.py snake.std | ./soma -    
        ll........
        ..........
        ..........
        ..........

        .l........
        ..........
        ..........
        ..........

        .lzz......
        ...ppn....
        .....ntc..
        .......cc3

        .zzp......
        ...pnn....
        .....ttt..
        .......c33



Algorithms and implementation <a name="algorithms_and_implementation"></a>
-----------------------------

### Basic algorithm <a name="basic_algorithm"></a>

The software is based on a simple recursive tree descent algorithm. Each Soma piece in turn is tested against all currently empty cube positions in the shape to be solved. This is done by aligning one pre-chosen cube of the piece with each empty shape position, and then orthogonally rotating the piece into each of its unique orientations. If all the remaining piece cubes (2 for the "3" piece, 3 for all others) are inside the shape and not conflicting with any previously placed piece, the algorithm proceeds to the next piece. If not, it backtracks to the previous piece and tries its next rotation and/or position in the shape.

When the last piece has been placed, a solution has been found. If all solutions have been requested (`-a` or `-c` option, see [above](#program_usage)) the code tries another orientation of the last piece and/or backtracks as previously described. Solving halts when backtracking returns to the first piece and it cannot be placed.

### Optimizations <a name="optimizations"></a>

See runtime documentation for `-O`, `-D`, `-S`, and `-P` options in [Extended help](#extended_help), above.

The experimentally-derived "optimal" default settings for these options are  counter-intuitive. "Big O" analysis would indicate that pruning large amounts of the solution search tree space would result in faster solution times, but the computational cost of doing the checks generally results in the opposite, except in specific cases such as doing symmetry and duplicate checking after the first piece (where pruning has the greatest impact and there is a higher chance of successful checks).

The code has other, always turned on, optimizations. These include:

* Pre-computation of unique piece rotations/orientations. Each piece can be rotated to 24 different orientations (the original Z axis rotated to the positive or negative X, Y, and Z axes and then 4 rotations around each), but given piece symmetries only the following are unique:

        c:  8 orientations
        p: 24 orientations
        n: 24 orientations
        z: 12 orientations
        t: 12 orientations
        l: 24 orientations
        3: 12 orientations

<a name="cull_impossible"></a>
* Prior to the executing the recursive descent algorithm the above piece orientations are further culled on a per shape cubicle basis. If a piece cannot be placed in a given position and orientation with no other pieces present there is no point in repeatedly checking that position+orientation during the recursive search.

* Also per-shape, a fairly large number of data structures are generated to optimize performance. For example, arrays of neighboring shape cubicles populated to avoid linear (through the 27 shape cubicles) or geometric (via X,Y,Z coordinate offsets) searches.


### Implementation <a name="implementation"></a>

The algorithms are implemented in code as a text-based commandline wrapper ([`main.cxx`](main.cxx)) around a C++ engine (`class Soma` in files [`soma.cxx`](soma.cxx) and [`soma.hxx`](soma.hxx)) and associated classes in the remaining `*.[ch]xx` files.

The code is extensively documented but will likely fail code quality analysis tools which check the ratio of comments to total source lines (832 to 4472 at a recent count). It is the author's opinion that code itself describes what it does (if well-written, including descriptive names for variables, methods, classes, etc.), while comments should explain *why* it does it what it does (background explanations and justifications). The code here is therefore notably lacking in such comments as:

        // loop through all the elements in the container
        for (element : container) {

and [<sup>7</sup>](#footnote7):

        ++counter;  // increment counter by one

In general, each class's public methods are documented at their declarations in `classname.hxx` files, and private methods at their definitions in `classname.cxx` files.

Example code for using `class Soma` via its `Soma::shape` (input) and `Soma::solution` (output) API instead of the text- and file-based `Soma::read` and `Soma::print` methods can be found in the `read_pieces_file` and `print_api` functions in file [`main.cxx`](main.cxx).

A possible reason why doing duplicate checking only after the last piece has been placed (see [Optimizations](#optimizations), above) is faster than doing it for each piece is that the checking implementation, despite being inherently *O(n<sup>2</sup>)*, is very efficient. Each solution is checked against known solutions stored in an `std::unordered_set` (optionally `std::set`) via fast `Signature::operator==` or `Signature::operator<` methods which merely compare two pairs of packed 64-bit integers. If the solution is new, all of its possible rotations and reflections are immediately generated and added to the set of known solutions. In this way the rotating/reflecting is done only once per unique solution (240 times for the basic 3x3x3 cube shape) instead of every time time a new, unknown solution is found (11520 times).



Autobio(soma)graphy <a name="autobio_soma_graphy"></a>
-------------------

I have a very long personal history with the Soma cube.

My father made two wooden Soma cubes, I assume inspired by and sometime after they were described in Martin Gardner's "Mathematical Games" column in the September 1958 issue of *Scientific American* magazine. I would have been far too young at the time to have been aware of the exact date -- my earliest memories are that the cubes were "always around" in my childhood.

I still have one of the two, the other having disappeared sometime after I left the family home. My father was a master craftsman, and despite not owning woodworking machinery such as table saws, jointers, and planers, instead using only hand saws and planes, the ones he made were more precise than most of the mass-produced wooden and plastic ones I purchased years later. Lack of parallelism, perpendicularity, and/or dimensional accuracy in a Soma cube impacts the tactile joy of playing with one as the pieces don't fit together well and solved figures show gaps and don't support themselves. Magnetic cubes alleviate some of these problems (see below).

I wrote my first Soma solver software in my teens, sometime around 1970. It ran on a mainframe computer which filled half of a basketball court sized room. I think the final version was written in assembly language although there may have been precursors in Fortran and/or Basic. It implemented the same simple recursive descent algorithm which is the basis of this repository's code, but without any of its rotation/reflection culling.  I deeply mourn the fact that the code no longer exists, the punch cards and/or paper tape containing the source and executable having long since been lost.

Approximately every 10 years one thing or another seems to bring me back to Soma. I'll come across a new cube for sale and/or throw together some code for a few weeks and then move on to other pursuits. (There are limits to recreational mathematics fetishism, even mine.) Sometime in the 1980's I found a wooden version for sale in a store, and similarly, in the 1990's the rather poorly made plastic "Block By Block" offering by the Binary Arts company.

In 1996 I was inspired to recreate my long lost mainframe code, largely as an excuse to see how well GCC would compile an object-oriented implementation of the algorithm. For some forgotten reason (likely another new cube purchase) I wanted to look at some Soma solutions in 2010, and as the old executable no longer linked with the then-current Linux shared libraries, I rewrote the code, updating the deprecated and rather strange `iostream` API from the pre-standardized 1996 `g++` libraries.

Finally, this current version began when I resurrected a broken 3D printer that had been given to me several years earlier and realized I could finally make a Soma cube to my desired standards. (I'm not quite the craftsman my father was, and I don't have access to the kind of  metal- and/or woodworking machinery my lack of skill requires me to have in order to achieve precise results.)

Before using the printer, I checked online for currently available commercial cubes, and found magnetic cubes being offered for sale. I was instantly puzzled: How could a Soma cube be magnetized? Any cube face of any piece can be arbitrarily placed against any other, so regardless of how the magnets are arranged it's highly likely that in at least some cases  two north or two south poles will be facing each other and repel the pieces instead of holding them together. An analogous problem exists if magnets and ferromagnetic plates are used instead (two magnets will repel, and two plates won't attract, each other).

Another 30 seconds of pondering produced the answer, although my solution is slightly different (in details, not concept) than that in the commercial products. I'll leave the answer to the reader's own thoughts and/or research. I did buy a magnetic cube to confirm my suppositions and to have one to play with while making my own.

As an aside, the Soma cube's younger, better known cousin, the Rubik's Cube, is trivial to magnetize. The inner faces of the 20 movable "cubies" and the 6 rotatable face pseudo-cubies always touch others of a single type. Corner and edge cubies' faces always touch each other, and edge cubies touch face pseudo-cubies, so installing magnets north pole outwards on the corners and faces and south on the edges (or vice-versa) works.

I find it very satisfying that magnetizing a Soma cube adds a mechanical component to the intrinsically mathematical puzzle. As Douglas Hofstadter pointed out in his "Metamagical Themas" column (the successor to Martin Gardner's "Mathematical Games" series) in the March 1981 *Scientific American* magazine, in addition to the problem of solving one it is not obvious how a Rubik's cube is mechanically held together in the first place. (Ironically, he stated that most people initially guess that magnets are used.) As another aside, I claim to have one of if not the first Rubik's cube ever sold in the USA. Along with many others I was smitten by the CGI rendering on the Scientific American cover and Hofstadter's article inside. A friend and I called our local "Toys 'R' Us" store every day for several weeks asking if they had them for sale, always getting the response, "Rubik's *what*? No, I told you I don't know what that is. Stop calling here!" until one day: "You again, I told you ... wait a second ... hey, Manny! What was that we got in this morning's delivery? Something 'Cube'? OK ... yeah, we got them, whatever they are."

We immediate ran over and bought two apiece. I scrambled one and then painstakingly spent three weeks developing "operators" on the other, being careful to write down and reverse my steps so I could restore it to its solved state where seeing what was going on was easier. (I only failed once and had to resort to disassembling the cube and putting it back together.) I finally worked out a small set of operators sufficient for a full solution, and eventually got solving down to approximately two minutes which was fast for that era  before widely available, efficient algorithms were published. At that point I lost interest, realizing that to get any better would require significant effort and practice, and as with card-counting blackjack in Las Vegas, any potential rewards weren't worth the trouble.

Back to Soma cubes ... in my opinion, magnets take them to the next level. They make solving figures much easier as the pieces hold together without balancing or using a flat surface, and some figures that aren't free standing without them become so with. Plus, there's something tactically and audibly satisfying about hearing and feeling the magnets flip and click together (there's a hint about the mechanical design). They've sparked this decade's interest in Soma for me.

I finally decided to add rotation and reflection culling to the code (which I'd wanted to do for years), along with several other features such as implementing the same for separated figures. I find it very interesting to have an accurate count of unique solutions after estimating the number when solving by hand. Also having indisputable confirmation that a figure is unsolvable after coming to that conclusion (but with nagging doubts) experimentally/intellectually.

This open-source repository is the end result. If others find it interesting and/or useful that's an additional benefit.



Future work <a name="future_work"></a>
-----------

All somewhat good things must come to an end, and it is time to wrap up development on this code for now.

A possible alternate implementation could entail placing and orienting the "p", "n", "z", and "l" pieces by pairs of orthogonally-connected shape cubicles instead of by single ones. (The "c", "t", and "3" pieces would be handled as they are now.) This would aid in symmetry searches which are currently hampered by the fact that `Shape::set_statuses` chooses an arbitrary cubicle from the set of symmetric ones, and that may or may not match the pre-chosen "central" cubicle of the pieces. See `-S` documentation in [Extended help](#extended_help), above. A pathological example case is [`figures/pieces.soma`](figures/pieces.soma) (simply the pieces themselves, separated) where choosing the wrong cubicle can result in the code not being able to place the piece in its obvious, unique position.

In some ways this would be an optimization. For example, the "l" piece, instead of having 24 unique orientations around its current single, central cube would have only 8 around its pair of two central cubes. But, depending on the Soma shape, the number of cubicle pairs to test for piece insertion would be larger. For the basic 3x3x3 cube shape this would go from 27 single cubicles to 108 pairs of cubicles. it would likely be less for other shapes, so the cube shape's overall loss (1/3 the orientations but 4 times the placements) might be an edge case. And culling of impossible positions/orientations (see [above](#cull_impossible)) further changes the equation.

If someone would like to try this approach (of course while respecting the [License](#license)) I'd be interested in the results.



Footnotes <a name="footnotes"></a>
---------
1. <a name="footnote1"></a> [SOMA Figures](https://www.fam-bundgaard.dk/SOMA/FIGURES/FIGURES.HTM) at "Thorlief's SOMA Page"
2. <a name="footnote2"></a> In particular: "Problems worthy of attack. Prove their worth by fighting back."
3. <a name="footnote3"></a> Interesting to a complete Soma cube nerd like me.
4. <a name="footnote4"></a> Or alternately the Zen-like meta-perfection that not everything in the world of Soma shapes is achievable.
5. <a name="footnote5"></a> Reports about non-compliant compilers or those with proprietary extensions not so much.
6. <a name="footnote6"></a> Interesting to a complete optimization nerd like me.
7. <a name="footnote7"></a> My favorite straw-man example.
