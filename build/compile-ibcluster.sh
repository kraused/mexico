#!/bin/bash -x

# The source directory is assumed
# to be the same as the directory
# containing the script
dir=`dirname $(readlink -f $0)`

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
/* #undef MEXICO_HAVE_SHMEM */
/* #undef MEXICO_HAVE_MPP_SHMEM_H */

#endif

EOF

flex  -olexer.cpp ../*.lex
bison -d ../*.ypp

FLAGS="-I. -I$dir $GA_POST_COMPILE_OPTS"

c++ -c $FLAGS lexer.cpp -o lexer.cpp.o
c++ -c $FLAGS parser.tab.cpp -o parser.tab.cpp.o
for i in $dir/*.cpp
do
    o=`echo $i | sed s+$dir/++g`.o
    c++ -c $FLAGS $i -o $o
done
ar rus libmexico.a *.o

c++ -c $FLAGS -DUSE_GA=1 $dir/examples/binning.cpp -o binning.cpp.o
ftn -c $FLAGS            $dir/examples/binning.f90 -o binning.f90.o

c++  -o binning binning.cpp.o binning.f90.o -L. -lmexico $GA_POST_LINK_OPTS

