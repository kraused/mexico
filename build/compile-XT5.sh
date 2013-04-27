#!/bin/bash -x


cat > mexico_config.hpp << EOF

/// vim: tabstop=4:expandtab:hlsearch

#ifndef MEXICO_CONFIG_HPP_INCLUDED
#define MEXICO_CONFIG_HPP_INCLUDED 1

/// Check for MPI
#define MEXICO_HAVE_MPI 1
#define MEXICO_HAVE_MPI_H 1

/// Check for GA
#define MEXICO_HAVE_GA 1
#define MEXICO_HAVE_GA_H 1
#define MEXICO_HAVE_MACDECLS_H 1

/// Check for SHMEM
#define MEXICO_HAVE_SHMEM 1
#define MEXICO_HAVE_MPP_SHMEM_H 1

#endif

EOF

flex  -o lexer.cpp ../*.lex
bison -d ../*.ypp

FLAGS="-I. -I$1 $CRAY_GA_INCLUDE_OPTS"

CC -c $FLAGS lexer.cpp -o lexer.cpp.o
CC -c $FLAGS parser.tab.cpp -o parser.tab.cpp.o
for i in $1/*.cpp
do
    o=`echo $i | sed s+$1/++g`.o
    CC -c $FLAGS $i -o $o
done
ar rus libmexico.a *.o

CC  -c $FLAGS -DUSE_GA=1    $1/examples/binning.cpp -o binning-ga.cpp.o
CC  -c $FLAGS -DUSE_SHMEM=1 $1/examples/binning.cpp -o binning-shmem.cpp.o
ftn -c $FLAGS               $1/examples/binning.f90 -o binning.f90.o

CC  -o binning-ga    binning-ga.cpp.o    binning.f90.o -L. -lmexico $CRAY_GA_POST_LINK_OPTS
CC  -o binning-shmem binning-shmem.cpp.o binning.f90.o -L. -lmexico $CRAY_GA_POST_LINK_OPTS

