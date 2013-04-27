
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

#include "runtime.hpp"
#include "parser.hpp"
#include "log.hpp"

#ifdef MEXICO_HAVE_GA
#include "runtime_impl_ga.hpp"
#include "runtime_impl_ga_gs.hpp"
#endif
#ifdef MEXICO_HAVE_SHMEM
#include "runtime_impl_shmem.hpp"
#endif
#ifdef MEXICO_HAVE_MPI
#include "runtime_impl_mpi_alltoall.hpp"
#include "runtime_impl_mpi_rma.hpp"
#include "runtime_impl_mpi_pt2pt.hpp"
#endif


mexico::Runtime::Runtime(Instance* ptr)
: Pointers(ptr)
{
    std::string hints;

    parser->print_namelist("runtime");

    implementation = parser->find_by_name_str("runtime", "implementation");
    hints          = parser->find_by_name_str("runtime", "hints");

    /// Factory
#ifdef MEXICO_HAVE_GA
    if(implementation == "GA")
    {
        impl = new RuntimeImpl_GA(ptr, hints);
    }
    else
    if(implementation == "GA gs")
    {
        impl = new RuntimeImpl_GA_gs(ptr, hints);
    }
    else
#endif
#ifdef MEXICO_HAVE_SHMEM 
    if(implementation == "SHMEM")
    {
        impl = new RuntimeImpl_SHMEM(ptr, hints);
    }
    else
#endif
#ifdef MEXICO_HAVE_MPI
    if(implementation == "MPI Alltoall")
    {
        impl = new RuntimeImpl_MPI_Alltoall(ptr, hints);
    }
    else
    if(implementation == "MPI RMA")
    {
        impl = new RuntimeImpl_MPI_RMA(ptr, hints);
    }
    else
    if(implementation == "MPI Pt2Pt")
    {
        impl = new RuntimeImpl_MPI_Pt2Pt(ptr, hints);
    }
#endif
    else
        MEXICO_FATAL("Found no constructor for this implementation");
}

mexico::Runtime::~Runtime()
{
    delete impl;
}

