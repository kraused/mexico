
include Makefile.inc

# List of objects
OBJ = ast.o log.o runtime.o runtime_impl_ga_gs.o runtime_impl_mpi_rma.o comm.o memory.o runtime_impl.o runtime_impl_mpi_alltoall.o			\
	  runtime_impl_shmem.o instance.o mexico.o runtime_impl_ga.o runtime_impl_mpi_common.o utils.o job.o parser.o runtime_impl_ga_common.o	\
	  runtime_impl_mpi_pt2pt.o lexer.o parser.tab.o

default: libmexico.a examples/binning

%.o:%.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

lexer.cpp: parser.tab.cpp
	flex -olexer.cpp parser.lex

parser.tab.cpp:
	bison -d parser.ypp

libmexico.a: $(OBJ)
	$(AR) rus $@ $(OBJ)

examples/binning:
	$(FC)  $(FFLAGS) -I. -o examples/binning.f90.o -c examples/binning.f90
	$(CXX) $(CFLAGS) -I. -o examples/binning.cpp.o -c examples/binning.cpp
	$(CXX) -o examples/binning examples/binning.f90.o examples/binning.cpp.o -L. -lmexico $(LDFLAGS)

clean:
	rm -f $(OBJ) parser.tab.hpp parser.tab.cpp lexer.cpp examples/binning examples/binning.f90.o examples/binning.cpp.o

