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


PROGRAM = soma

CC = g++
STD ?= c++11
OPTIMIZE ?= -O3
DEBUG_SYMS ?= -g
PROFILE ?=
WARN ?= -Wall -Wextra

ROTATION ?= matrix
ifeq ($(ROTATION), lambda)
MATRIX = -U
LAMBDA = -D
else ifeq ($(ROTATION), matrix)
MATRIX = -D
LAMBDA = -U
else
MATRIX = -U
LAMBDA = -U
$(error ROTATION must be either matrix or lambda)
endif

STD_SET ?= unordered
ifeq ($(STD_SET), set)
STD_SET_SET 	  := -D
STD_SET_UNORDERED := -U
else ifeq ($(STD_SET), unordered)
STD_SET_SET	  := -U
STD_SET_UNORDERED := -D
else
STD_SET_SET 	  := -U
STD_SET_UNORDERED := -U
$(error STD_SET must be either "set" or "unordered")
endif

OSTREAM_OPS ?= -U
STATS ?= -U


CC_OPTIONS = -std=$(STD)					\
	     $(OPTIMIZE) 					\
	     $(WARN)						\
	     $(DEBUG_SYMS) 					\
	     $(LAMBDA)SOMA_LAMBDA_ROTATION			\
	     $(MATRIX)SOMA_MATRIX_ROTATION			\
	     $(STD_SET_SET)SOMA_STD_SET_SET			\
	     $(STD_SET_UNORDERED)SOMA_STD_SET_UNORDERED		\
	     $(STATS)SOMA_STATISTICS				\
	     $(OSTREAM_OPS)SOMA_OSTREAM_OPERATORS

OBJECTS = main.o soma.o piece.o shape.o rotators.o



$(PROGRAM): $(OBJECTS)
	$(CC) $(PROFILE) $(OBJECTS) -o $(PROGRAM)

clean:
	rm -f $(OBJECTS) $(PROGRAM)

clean_test:
	rm -f test.opt_* test.cube ok_opt_cn.diff

test: test.cube test.opt_cn test.opt_n test.opt_an test.opt_crn

test.opt_crn: $(PROGRAM) figures/*.soma figures/*.api_test
	./soma -q -crnt -o test.opt_crn figures/*.soma figures/*.api_test
	diff -q tests/test.opt_crn test.opt_crn

test.opt_cn: $(PROGRAM) figures/*.soma figures/*.api_test
	./soma -q -cnt -o test.opt_cn figures/*.soma figures/*.api_test
	diff -q tests/test.opt_cn test.opt_cn
	./soma -q -cnt -D 123456 -o test.opt_cn figures/*.soma figures/*.api_test
	diff -q tests/test.opt_cn test.opt_cn
	for piece_order      in cpnztl3 tzcpnl3 ztcpnl3 ; do  \
	for orphan_checks    in 12345 12 1 2345 45 5 0  ; do  \
	for duplicate_checks in 7 17		        ; do  \
	for symmetry_checks  in 0 1                     ; do  \
		echo -cnt all_tests		\
		     -D $$duplicate_checks      \
		     -O $$orphan_checks         \
		     -S $$symmetry_checks       \
		     -P $$piece_order      ;    \
		./soma -q -cnt			\
		       -D $$duplicate_checks 	\
		       -O $$orphan_checks -cnt	\
		       -S $$symmetry_checks 	\
		       -P $$piece_order 	\
		       -o test.opt_cn		\
		       figures/*.soma 		\
		       figures/*.api_test ;  	\
		if ! cmp -s tests/test.opt_cn test.opt_cn ; then  	   \
		    diff tests/test.opt_cn test.opt_cn > ok_opt_cn.diff ;  \
		    if ! cmp tests/ok_opt_cn.diff ok_opt_cn.diff ;     	   \
		    then				   	       	   \
		        exit 1 ;				   	   \
		    fi 						   	   \
		fi 						   	   \
	done ; done ; done ; done

test.opt_n: $(PROGRAM) figures/*.soma
	./soma -q -nt -o test.opt_n figures/*.soma figures/*.api_test
	diff -q tests/test.opt_n test.opt_n

test.opt_an: $(PROGRAM) figures/*.soma figures/*.api_test
	./soma -q -ant -o test.opt_an figures/*.soma figures/*.api_test
	diff -q tests/test.opt_an test.opt_an

# test.opt_arn: $(PROGRAM) figures/*.soma figures/*.api_test
# 	./soma -q -arn -o test.opt_arn figures/*.soma figures/*.api_test
# 	diff -q tests/test.opt_arn test.opt_arn

test.cube: $(PROGRAM) figures/cube.soma
	./soma -q -cnt -o test.cube figures/cube.soma
	diff -q tests/test.cube test.cube
	for piece_order      in cpnztl3 tzcpnl3 ztcpnl3 ; do  \
	for orphan_checks    in 12345 12 1 2345 45 5 0  ; do  \
	for duplicate_checks in 7 17 123456	        ; do  \
	for symmetry_checks  in 0 1                     ; do  \
		echo -cnt cube.soma		\
		     -D $$duplicate_checks      \
		     -O $$orphan_checks         \
		     -S $$symmetry_checks       \
		     -P $$piece_order      ;    \
		./soma -q -cnt			\
		       -D $$duplicate_checks 	\
		       -O $$orphan_checks -cnt	\
		       -S $$symmetry_checks 	\
		       -P $$piece_order 	\
		       -o test.cube		\
		       figures/cube.soma ;	\
		if ! diff -q tests/test.cube test.cube ; then  \
		   exit 1 ; 				       \
		fi 					       \
	done ; done ; done ; done

SOMA_HXX      = soma.hxx piece.hxx shape.hxx
PIECE_HXX     = piece.hxx position.hxx rotators.hxx
ROTATORS_HXX  = rotators.hxx position.hxx
SHAPE_HXX     = shape.hxx piece.hxx position.hxx rotators.hxx signature.hxx
SOMA_HXX      = soma.hxx piece.hxx shape.hxx

main.o: main.cxx $(SOMA_HXX)
	$(CC) -c $(CC_OPTIONS) $(INCLUDES) main.cxx

soma.o: soma.cxx $(SOMA_HXX)
	$(CC) -c $(CC_OPTIONS) $(INCLUDES) soma.cxx

piece.o: piece.cxx $(PIECE_HXX) position.hxx $(ROTATORS_HXX) $(SHAPE_HXX) 
	$(CC) -c $(CC_OPTIONS) $(INCLUDES) piece.cxx

shape.o: shape.cxx $(SHAPE_HXX) $(PIECE_HXX) $(ROTATORS_HXX) $(signature.hxx)
	$(CC) -c $(CC_OPTIONS) $(INCLUDES) shape.cxx

rotators.o: rotators.cxx $(ROTATORS_hxx)
	$(CC) -c $(CC_OPTIONS) $(INCLUDES) rotators.cxx
