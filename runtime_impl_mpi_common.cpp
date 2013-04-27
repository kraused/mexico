
/// vi: tabstop=4:expandtab

/* Copyright 2010 University of Lugano. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of the University of Lugano.
 */

#include "mexico_config.hpp"
#include "runtime_impl_mpi_common.hpp"


mexico::RuntimeImpl_MPI_Common::RuntimeImpl_MPI_Common(Instance* ptr, const std::string& hints)
: RuntimeImpl(ptr)
{
}

mexico::RuntimeImpl_MPI_Common::~RuntimeImpl_MPI_Common()
{
}

MPI_Datatype mexico::RuntimeImpl_MPI_Common::create_struct_int_type(int cnt, MPI_Datatype type)
{
    MPI_Datatype newtype;
    int blocklengths[2];
    MPI_Aint displacements[2];
    MPI_Datatype types[2];

    blocklengths[0] = 1;
    displacements[0] = 0;
    types[0] = MPI_INT;

    blocklengths[1] = cnt;
    displacements[1] = sizeof(int);
    types[1] = type;

    MPI_Type_create_struct(2, blocklengths, displacements, types, &newtype);
    MPI_Type_commit(&newtype);

    return newtype;
}

