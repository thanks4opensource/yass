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


//  SOMA figure solver
//
//  Command-line, text-based input and output, driver program
//
//  Uses Soma class engine and API, see ./soma.[ch]xx
//
//  See parse_arguments(), BRIEF_HELP_TEXT, and EXTENDED_HELP_TEXT, below
//  (and/or "-h"/"-H" options if running compiled binary) for options and
//  file formats. Additional API test and file format in read_pieces_file(),
//  below.
//
//  See ./figures/*.{soma,api_test} for examples. See ./Makefile for unittests:
//  `make test`, etc.



#include <fstream>
#include <functional>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <printf.h>
#include <sstream>
#include <string>
#include <sys/time.h>

#include "soma.hxx"



using namespace soma;



namespace {

// check Soma class version compatibility
static const unsigned   MAJOR_VERSION = 0,
                        MINOR_VERSION = 0,
                        MICRO_VERSION = 0;
static_assert(    Soma::MAJOR_VERSION == MAJOR_VERSION
              &&  Soma::MINOR_VERSION >= MINOR_VERSION,
              "main.cxx MAJOR_VERSION and MINOR_VERSION incompatible "
              "with Soma::MAJOR_VERSION and Soma::MINOR_VERSION"      );

// commandline argument defaults
static const char   *DEFAULT_ORPHANS_CHARS    = "123456",
                    *DEFAULT_DUPLICATES_CHARS = "17"    ,
                    *DEFAULT_SYMMETRIES_CHARS = "0"     ;


// see implementations, below
//
int         parse_arguments (      int               argc            ,
                                   char             *argv[]          ,
                                   std::string      &output_filename ,
                                   bool             &all_solutions   ,
                                   bool             &reflects_rotates,
                                   unsigned         &orphans         ,
                                   unsigned         &duplicates      ,
                                   unsigned         &symmetries      ,
                                   std::string      &piece_order     ,
#ifdef SOMA_STATISTICS
                                   bool             &statistics      ,
#endif
                                   bool             &print_time      ,
                                   bool             &count_only      ,
                                   bool             &print_name      );

bool        parse_steps     (      unsigned         &steps           ,
                             const std::string      &string          ,
                             const std::string      &option          );

bool        read_pieces_file(      Soma             &soma            ,
                                   std::istream     &input           ,
                                   std::ostream     &output          ,
                             const std::string      &filename        ,
                                   bool              print_name      );

double      solve           (const std::string      &input_filename  ,
                                   Soma             &soma            ,
                                   std::ostream     &output          ,
                                   bool              print_name      ,
                                   bool              count_only      ,
                                   bool              all_solutions   ,
#ifdef SOMA_STATISTICS
                                   unsigned         &total_solutions ,
#endif
                                   bool             print_time       );

#ifdef SOMA_STATISTICS
void        print_statistics(      Soma             &soma            ,
                                   unsigned          number_of_solves,
                                   unsigned          total_solutions );
#endif

void        print_api       (const Soma             &soma            ,
                                   std::ostream     &output          );

}  // namespace



int main(
int     argc  ,
char    **argv)
{
    bool            all_solutions   ,   // true: all      false: first solution
                    reflects_rotates,   // true: include  false: only uniques
#ifdef SOMA_STATISTICS
                    statistics      ,   // report solution algorithm info
#endif
                    print_time      ,   // time to solve all on commandline
                    count_only      ,   // do not print actual solutions
                    print_name      ;   // print filename before each solution
    unsigned        orphans         ,   // see EXTENDED_HELP_TEXT
                    duplicates      ,   //  "          "
                    symmetries      ;   //  "          "
    std::string     piece_order     ;   //  "          "
    int             first_filename  ;   // index into argv
    std::string     input_filename  ,
                    output_filename ;

    if (    (first_filename = parse_arguments(argc            ,
                                              argv            ,
                                              output_filename ,
                                              all_solutions   ,
                                              reflects_rotates,
                                              orphans         ,
                                              duplicates      ,
                                              symmetries      ,
                                              piece_order     ,
#ifdef SOMA_STATISTICS
                                              statistics      ,
#endif
                                              print_time      ,
                                              count_only      ,
                                              print_name      ))
        == -1                                                   )
        // error details already printed to stderr by parse_arguments()
        return 1;   // arbitrary non-zero shell error code

    // input and output streams
    // use intermediate pointer because can't reassign reference

    std::ostream    *output_ptr = &std::cout;

    if (output_filename != "-") {
        output_ptr = new std::ofstream(output_filename);

        if (!*output_ptr) {
            std::cerr << "Can't open file "
                      << output_filename
                      << " for output"
                      << std::endl;
            return 2;   // arbitrary non-zero shell error code
        }
    }
    std::ostream &output = *output_ptr;


    // solver engine
    Soma    soma(orphans, duplicates, symmetries, piece_order);


    // solve all commandline-specified figures
    //
    double      elapsed_time    = 0.0;
#ifdef SOMA_STATISTICS
    unsigned    total_solutions = 0  ;
#endif

    for (int arg_ndx = first_filename ; arg_ndx < argc ; ++arg_ndx) {
        elapsed_time += solve(argv[arg_ndx]  ,
                              soma           ,
                              output         ,
                              print_name     ,
                              count_only     ,
                              all_solutions  ,
#ifdef SOMA_STATISTICS
                              total_solutions,
#endif
                              print_time     );

        // blank spaces between files if necessary
        if (   !count_only
            && !print_name
            && argc - first_filename > 1 && arg_ndx < argc - 1)
            output << std::endl;
    }

    if (print_time) {
        std::cout << elapsed_time
                  << " seconds"
                  << std::endl;
    }

    if (all_solutions) {
        // check and warn about potential problems with optimization arguments

        if (duplicates != 0 && !(duplicates & 0x40)) {
            std::cerr << "Warning: No piece 7 in -D option. Will be "
                         "added for separated shapes."
                      << std::endl;
            if ((duplicates & 0x3f) != 0x3f)
                std::cerr << "Warning: Neither piece 7 nor all of 1-6 "
                             "in -D option. Possible failed or incorrect "
                             "number of solutions."
                          << std::endl;
        }

        std::string     bad_pieces = "";
        for (unsigned ndx = 0 ; ndx < Piece::NUMBER_OF_PIECES ; ++ndx)
            if (   (symmetries & (1 << ndx))
                &&    std::string("pnzl").find(piece_order[ndx])
                   != std::string::npos                               )
                bad_pieces.append(std::string(1, piece_order[ndx]));
        if (bad_pieces != "") {
            std::cerr << "Warning: One or more of \""
                      << bad_pieces
                      << "\" in \"-P "
                      << piece_order
                      << "\" match \"-S ";
            for (unsigned ndx = 0 ; ndx < Piece::NUMBER_OF_PIECES ; ++ndx)
                if (symmetries & (1 << ndx))
                    std::cerr << ndx + 1;
            std::cerr << "\" symmetry checks. "
                         "Possible failed or incorrect number of solutions."
                      << std::endl;
        }
    }

#ifdef SOMA_STATISTICS
    if (statistics)
        print_statistics(soma, argc - first_filename, total_solutions);
#endif

    return 0;

}   // main(int, char**)



namespace {

//  Solve single SOMA figure
//
double solve(
const std::string   &input_filename,
Soma                &soma            ,  // solver engine
std::ostream        &output          ,
bool                 print_name      ,
bool                 count_only      ,
bool                 all_solutions   ,
#ifdef SOMA_STATISTICS
unsigned            &total_solutions ,
#endif
bool                 print_time      )
{
    std::istream    *input_ptr          ;
    bool             is_api_test = false;   // input is special test file format

    if (input_filename == "-")
        input_ptr = &std::cin;
    else {
        input_ptr = new std::ifstream(input_filename);

        if (!*input_ptr) {
            output << "Can't open file "
                   << input_filename
                   << " for input"
                   << std::endl;
            if (!count_only)
                output << std::endl;
            return 0.0;
        }
    }
    std::istream    &input = *input_ptr;


    // determine file type from filename extension (cheap hack)
    //
    std::string::size_type  period = input_filename.find(".");

    if (   period != std::string::npos
        && input_filename.substr(period) == ".api_test")
        is_api_test = true;


    // try to read file with appropriate reader
    if (!is_api_test) {
        std::ostringstream      errors;
        if (!soma.read(input, &errors)) {
            if (print_name)
                output << input_filename
                       << ':'
                       << std::endl;
            output << errors.str();
            if (!count_only)
                output << std::endl;
            return 0.0;
        }
    }
    else if (!read_pieces_file(soma          ,
                               input         ,
                               output        ,
                               input_filename,
                               print_name    )) {
        if (!count_only)
            output << std::endl;
        return 0.0;
    }

    if (print_name && !count_only)
        output << input_filename << ':' << std::endl;


    // solve SOMA figure
    //

    double          elapsed_time = 0.0;
    struct timeval  begin_time        ;
    if (print_time) gettimeofday(&begin_time, 0);

    unsigned    number_of_solutions = 0;
    while (soma.solve()) {    // returns true until no more solutions
        ++number_of_solutions;

        if (!count_only) {
            if (all_solutions)
                output << (number_of_solutions == 1 ? "" : "\n")
                       << "solution #"
                       << number_of_solutions
                       << std::endl                             ;

            soma.print(output);  // print current solution

            if (is_api_test)
                print_api(soma, output);

            if (!all_solutions)
                break;   // only first solution
        }
    }

#ifdef SOMA_STATISTICS
    total_solutions += number_of_solutions;
#endif

    if (print_time) {
        timeval     end_time;

        gettimeofday(&end_time, 0);

        if (begin_time.tv_usec > end_time.tv_usec) {
            end_time.tv_usec += 1000000;
            end_time.tv_sec  -= 1      ;
        }

        elapsed_time =   static_cast<double>(  end_time.tv_sec
                                             - begin_time.tv_sec)
                       + (end_time.tv_usec - begin_time.tv_usec) / 1e6;

    }

    if (count_only) {
        output << input_filename
               << ": "
               << number_of_solutions
               << " solution"
               << (number_of_solutions == 1 ? "" : "s")
               << std::endl;
    }

    if (!count_only && number_of_solutions == 0) {
        // no solution was printed above so show unsolved figure
        soma.print(output);
    }

    if (print_name && !count_only)
        output << std::endl;

    // clean up open file if any
    if (input_ptr != &std::cin)
        delete input_ptr;

    return elapsed_time;

}  // solve(...)



#ifdef SOMA_STATISTICS
void print_statistics(
Soma        &soma            ,
unsigned     number_of_solves,
unsigned     total_solutions )
{
    static const unsigned   SPACING     = 10,
                            LABEL_WIDTH = 10;

    // functor to print generic data
    // called with lambda accessor to data
    // std::function inefficient but necessary for lamba-with-capture,
    //   and print_statistics() is debug and therefor not performance-critical
    struct {
        void operator()(
        const std::string                 description,
        std::function<unsigned(unsigned)> statistic  )
        {
            std::cout << std::setw(LABEL_WIDTH)
                      << std::left
                      << description
                      << std::right            ;
            unsigned    total = 0;
            for (unsigned     piece = 0                       ;
                              piece < Piece::NUMBER_OF_PIECES ;
                            ++piece                            ) {
                unsigned    count = statistic(piece);
                std::cout << "  "
                          << std::setw(SPACING)
                          << count;
                total += count;
            }
            std::cout << "  "
                      << std::setw(SPACING)
                      << total
                      << std::endl;
        }
    } print_pieces;

    std::cout << number_of_solves
              << " solves, "
              << total_solutions
              << " solutions"
              << std::endl;

    // column titles
    std::cout << std::setw(LABEL_WIDTH)
              << std::left
              << "piece:"
              << std::right            ;
    for (unsigned piece = 1 ; piece <= Piece::NUMBER_OF_PIECES ; ++piece)
        std::cout << "       #"
                  << piece
                  << '('
                  << soma.piece_name(piece - 1)
                  << ')';
    std::cout << "       total"
              << std::endl     ;


    // statistics
    //

    std::cout << "orientations:" << std::endl;
    print_pieces("total",
                 [&soma, number_of_solves](unsigned piece)
                 {
                    return   soma.num_orientations(piece)
                           * Shape::NUMBER_OF_CUBICLES
                           * number_of_solves            ;
                 }                                        );
    print_pieces("valid",
                 [&soma](unsigned piece)
                 { return soma.num_valid_orientations(piece); });

    std::cout << "placings:" << std::endl;
    print_pieces("total",
                 [&soma](unsigned piece)
                 {
                    return   soma.place_successes(piece)
                           + soma.place_failures (piece);
                 }                                       );
    print_pieces("placed",
                 [&soma](unsigned piece)
                 { return soma.place_successes(piece); });
    print_pieces("failed",
                 [&soma](unsigned piece)
                 { return soma.place_failures(piece); });
    print_pieces("duplicates",
                 [&soma](unsigned piece)
                 { return soma.place_duplicates(piece); });
    print_pieces("orphans",
                 [&soma](unsigned piece)
                 { return soma.place_orphans(piece); });

    std::cout << "symmetries:" << std::endl;
    print_pieces("uniques",
                 [&soma](unsigned piece)
                 { return soma.statuses_uniques(piece); });
    print_pieces("duplicates",
                 [&soma](unsigned piece)
                 { return soma.statuses_duplicates(piece); });

}  // print_statistics(Soma&, unsigned)
#endif  // #ifdef SOMA_STATISTICS



// Test Soma::solution() method
//
void print_api(
const Soma          &soma  ,
      std::ostream  &output)
{
    std::array<int,  Shape::NUMBER_OF_CUBICLES * 3> coords;
    std::array<char, Shape::NUMBER_OF_CUBICLES    > pieces;

    pieces = soma.solution(coords);

    for (unsigned ndx = 0 ; ndx < Shape::NUMBER_OF_CUBICLES ; ++ndx)
        output << '('
               << coords[ndx * 3   ]
               << ','
               << coords[ndx * 3 + 1]
               << ','
               << coords[ndx * 3 + 2]
               << "):"
               << pieces[ndx]
               << std::endl;
}  // print_api(const Soma&, std::ostream&)



// Read special input file format for testing Soma::shape() API
// For testing only. File format not intended for external use.
// See ./figures/*.api_test for examples.
// Minimal error checking.
//
bool read_pieces_file(
      Soma          &soma      ,
      std::istream  &input     ,
      std::ostream  &output    ,
const std::string   &filename  ,
      bool           print_name)
{
    // 'o' normal, others pre-placed pieces
    static std::string  PIECE_CHARS("ocpnztl3");

    // flat array of x0, y0, z0, x1, y1, z1 ... positoins
    std::array<int, Shape::NUMBER_OF_CUBICLES * 3>  coords;

    std::string     line   ,  // raw input
                    pieces ;  // one-to-one with positions, name or unspecified
    unsigned        ndx = 0;  // indices into flat array

    while (std::getline(input, line)) {
        std::istringstream      parser(line);
        int                     x, y, z     ;
        std::string             piece       ;

        parser >> x
               >> y
               >> z
               >> piece;

        // check for valid piece (or unspecified) name
        if (PIECE_CHARS.find(piece) == std::string::npos) {
            if (print_name)
                output << filename
                       << ':'
                       << std::endl;
            output << "Bad piece character code in line: "
                   << line
                   << std::endl;
            return false;
        }

        pieces.append(piece);

        // flat array
        coords[ndx * 3    ] = x;
        coords[ndx * 3 + 1] = y;
        coords[ndx * 3 + 2] = z;
        ++ndx                  ;
    }

    if (ndx < Shape::NUMBER_OF_CUBICLES) {
        if (print_name)
            output << filename
                       << ':'
                       << std::endl;
        output << "Less than 27 cubicles ("
               << ndx
               << ") in .api_test file"
                << std::endl         ;
        return false;
    }


    // use Soma class API
    std::ostringstream      errors;
    if (!soma.shape(coords, pieces, &errors)) {
        if (print_name)
            output << filename
                   << ':'
                   << std::endl;
        output << errors.str();
        return false;
    }

    return true;

}  // read_pieces_file(...)



// commandline help text strings
//

const char  *BRIEF_HELP_TEXT = R"END_OF_TEXT(Solve SOMA figure(s).

Usage: %s [OPTIONS] <FILE> [FILES...]

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
                                   or single 0 (default: %s)
  -D <pieces>   duplicates checks: as per -O (default: %s)
  -S <pieces>   symmetry checks:   as per -O (default: %s)
  -P <pieces>   piece order:       7 characters, exactly one each of
                                   "cpnztl3" (default: %s)
  -h            this help text
  -H            extended help
  -w            print warranty
  -q            don't print version and copyright notice
)END_OF_TEXT"
#ifdef SOMA_STATISTICS
"  -s            report solution statistics\n"
#endif
"\n"
;



static const char   *EXTENDED_HELP_TEXT = R"END_OF_TEXT(Extended help (use "-h" for basic help).

Usage: %s [OPTIONS] <FILE> [FILES...]

OPTIONS (partial list, see "-h" for "<pieces>" syntax)
  -O <pieces>   orphans   check pieces (default: %s)
  -D <pieces>   duplicate check pieces (default: %s)
  -S <pieces>   symmetry  check pieces (default: %s)
  -P <pieces>   piece order            (default: %s)
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
  result if niether piece 7, nor all of 1 through 6, are set.
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

)END_OF_TEXT";

static const char   *COPYRIGHT_TEXT = R"END_OF_TEXT(soma %d.%d.%d
Copyright 2021 Mark R. Rubin aka "thanks4opensource".
This is free software with ABSOLUTELY NO WARRANTY.
Use "-w" option for full details.

)END_OF_TEXT";

static const char   *WARRANTY_TEXT = R"END_OF_TEXT(This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License , or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, write to

   The Free Software Foundation, Inc.
   51 Franklin Street, Fifth Floor
   Boston, MA 02110-1335  USA

)END_OF_TEXT";



// Using ancient C getopt library
//
// Returns index into argv of first filename (or "-" for standard input)
//
int parse_arguments(
int           argc            ,
char        **argv            ,
std::string  &output_filename ,
bool         &all_solutions   ,
bool         &reflects_rotates,
unsigned     &orphans         ,
unsigned     &duplicates      ,
unsigned     &symmetries      ,
std::string  &piece_order     ,
#ifdef SOMA_STATISTICS
bool         &statistics          ,
#endif
bool         &print_time      ,
bool         &count_only      ,
bool         &print_name      )
{
    int             option_letter;
    std::string     orphans_chars   (DEFAULT_ORPHANS_CHARS   ),
                    duplicates_chars(DEFAULT_DUPLICATES_CHARS),
                    symmetries_chars(DEFAULT_SYMMETRIES_CHARS);
    bool            help          = false,
                    extended_help = false,
                    copyright     = true ,
                    warranty      = false;

    piece_order = Soma::DEFAULT_PIECE_ORDER;

    // defaults (orphans, duplicates, and symmetries set below)
    all_solutions    = false;
    reflects_rotates = false;
#ifdef SOMA_STATISTICS
    statistics       = false;
#endif
    print_time       = false;
    count_only       = false;
    print_name       = false;
    output_filename  = "-"  ;

    while (  (option_letter = getopt(argc, argv, "arl:L:tcno:O:D:S:P:hHsqw"))
           != EOF                                                          )
        switch (option_letter) {
            case 'a': all_solutions     = true    ; break;
            case 'r': reflects_rotates  = true    ; break;
            case 't': print_time        = true    ; break;
            case 'n': print_name        = true    ; break;
            case 'o': output_filename   = ::optarg; break;
            case 'O': orphans_chars     = ::optarg; break;
            case 'D': duplicates_chars  = ::optarg; break;
            case 'S': symmetries_chars  = ::optarg; break;
#ifdef SOMA_STATISTICS
            case 's': statistics        = true    ; break;
#endif
            case 'c':
                count_only    = true;
                all_solutions = true;
                break;

            case 'P':
                piece_order= ::optarg;
                if (piece_order.size() != 7) {
                    std::cerr << "-P option string must be exactly 7 chars long"
                              << std::endl;
                    return -1;
                }
                for (char piece : Soma::DEFAULT_PIECE_ORDER)
                    if (   std::count(piece_order.begin(),
                                      piece_order.end  (),
                                      piece              )
                        != 1                              ) {
                        std::cerr << "Must be exactly 1 of each "
                                  << Soma::DEFAULT_PIECE_ORDER
                                  << " in -P option string ('"
                                  << piece
                                  << "' missing or duplicated)"
                                  << std::endl;
                        return -1;
                    }
                if (piece_order.find('n') < piece_order.find('p'))
                    // neither can be std::string::npos because check above
                    std::cerr << "Warning: -P option string has 'n' "
                                 "before 'p'. Incorrect number of "
                                 "solutions possible if -D option "
                                 "anything other than 7"
                              << std::endl;
                break;

            case 'q':
                copyright = false;
                break;

            case 'w':
                warranty = true;
                break;

            case 'H':
                extended_help = true;
                break;

            case 'h':
            case '?':
            default:
                help = true;
        }

    if (copyright)
        fprintf(stdout,
                COPYRIGHT_TEXT,
                MAJOR_VERSION ,
                MINOR_VERSION ,
                MICRO_VERSION );

    if (warranty)
        fprintf(stdout, WARRANTY_TEXT);

    if (help)
        fprintf(stdout                           ,
                BRIEF_HELP_TEXT                  ,
                argv[0]                          ,
                orphans_chars            .c_str(),
                duplicates_chars         .c_str(),
                symmetries_chars         .c_str(),
                Soma::DEFAULT_PIECE_ORDER.c_str());

    if (extended_help)
        fprintf(stdout                           ,
                EXTENDED_HELP_TEXT               ,
                argv[0]                          ,
                orphans_chars   .c_str()         ,
                duplicates_chars.c_str()         ,
                symmetries_chars.c_str()         ,
                Soma::DEFAULT_PIECE_ORDER.c_str());

    if (warranty || help || extended_help)
        return -1;

    if (!parse_steps(orphans   , orphans_chars   , "-O")) return -1;
    if (!parse_steps(duplicates, duplicates_chars, "-D")) return -1;
    if (!parse_steps(symmetries, symmetries_chars, "-S")) return -1;

    if (reflects_rotates) symmetries = duplicates = 0;

    return  ::optind;

}   // parse_arguments(...)



// Helper function for -O, -D, and -S options
bool parse_steps(   //
unsigned            &steps ,
const std::string   &string,
const std::string   &option)
{
    static std::string  good_chars("1234567");

    steps = 0;

    if (string == "0")
        return true;

    unsigned    count = 0;
    for (char letter : string) {
        if (good_chars.find(letter) == std::string::npos) {
            std::cerr << "Bad step number "
                      << letter
                      << " in option "
                      << option
                      << std::endl;
            return false;
        }
        else
            steps |= (1 << (letter - '1'));

        if (++count > Piece::NUMBER_OF_PIECES) {
            std::cerr << "Too many step numbers ("
                      << count
                      << ", limit is "
                      << Piece::NUMBER_OF_PIECES
                      << " in option "
                      << option
                      << std::endl;
            return false;
        }
    }

    return true;
}

}  // namespace
