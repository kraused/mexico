 
## Multiscale Execution Code (MEXICO)


Table of contents
=================

 o Introduction
 o Requirements
 o Installation
 o Known Problems


Introduction
============

Mexico is communication library initially designed for
supporting a multiscale simulation code with a complicated
communication pattern.


Requirements
============

C++ and Fortran compiler are needed. An MPI-2 conforming
(mostly) implementation is required. Additionally, support
for Global Array and SHMEM can be enabled if possible.


Installation
============

Currently no nice way for building the code are available.
Two possible ways exist:

a) Adapt one of the build/compile.[].sh script to your needs. 
   This should be fairly straight-forward. The script supports 
   out-of-source builds.

b) Modify "mexico_config.hpp" to enable/disable GA and SHMEM. 
   The Makefile.inc file should be modified to set the correct
   compilers and flags. If all settings are correct, a simple

% make

   should suffice.

In both cases the library "libmexico.a" is build along with
the binning benchmark.


Known Problems
==============

No known bugs so far.


Relevant Publications
=====================

If you use the provided code for your research please consider
citing the following publications:

- *Coupling Molecular Dynamics and Continua with Weak Constraints*.  
  Konstantin Fackeldey, Dorian Krause, Rolf Krause, Christoph Lenzen.  
  SIAM Multiscale Modeling & Simulation 2011; 9(4):1459-1494.

- *Parallel Scale-Transfer in Multiscale MD-FE Coupling using 
  Remote Memory Access.*  
  Dorian Krause and Rolf Krause.  
  IEEE 7th International Conference on E-Science, e-Science 2011, 
  Workshop Proceedings, IEEE Computer Society (2001).

