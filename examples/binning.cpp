
/// vi:tabstop=4:expandtab

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

#ifdef MEXICO_HAVE_MPI_H
#include <mpi.h>
#endif
#ifdef MEXICO_HAVE_GA
#include <ga.h>
#include <macdecls.h>
#endif
#ifdef MEXICO_HAVE_MPP_SHMEM_H
#include <mpp/shmem.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mexico.hpp"


/// BinningJob: The job
class BinningJob : public mexico::Job
{

public:
    BinningJob(int num_particles, int num_cells, int i_payload, int o_payload);

    /// Implements the exec() function in
    /// mexico::Job
    void exec(void* i_buf, void* o_buf);

private:
    /// Number of particles
    int num_particles;
    /// Number of cells
    int num_cells;
    /// Payload
    int i_flts, o_ints;

};

BinningJob::BinningJob(int num_particles, int num_cells, int i_payload, int o_payload)
: num_particles(num_particles), num_cells(num_cells)
{
    i_N = 3*num_particles + i_payload/4;
    i_type = MPI_FLOAT;

    o_N = num_particles + o_payload/4;
    o_type = MPI_INT;

    i_flts = i_payload/4;
    o_ints = o_payload/4;
}

void BinningJob::exec(void* i_buf, void* o_buf)
{
    int i, ix, iy, iz;
    float x, y, z;

    float* i_flt_buf = (float* )i_buf;
    int* o_int_buf = (int* )o_buf;

    for(i = 0; i < num_particles; ++i)
    {
        x  = i_flt_buf[(3 + i_flts)*i  ];
        y  = i_flt_buf[(3 + i_flts)*i+1];
        z  = i_flt_buf[(3 + i_flts)*i+2];
        
        ix = (int )x;
        iy = (int )y;
        iz = (int )z;

        o_int_buf[(1 + o_ints)*i] = ix*num_cells*num_cells + iy*num_cells + iz;
        memset(&o_int_buf[(1 + o_ints)*i+1], 'B', o_ints*sizeof(int));
    }
};

/// Application: The main driver code
class Application
{

public:
    /// Must be called after MPI_Init and mexico::init
    Application();

    /// Parse the arguments given to the application. In fact,
    /// only the root process parses the arguments and broadcasts
    /// The result to all others.
    void parse_args(int, char**);

    /// Set up the job and the mexico instance
    void setup();

    /// Run the benchmark
    void run();

    /// Clean up
    void finish();

    /// Return true if the processing element is a worker
    bool pe_is_worker() const;

    /// Create the particles on worker processes
    void create_particles();

    /// Strategies for distributing particles over all available
    /// processing elements
    enum
    {
        REDISTRIB_CELLS_CYCLIC = 1, ///< Redistribute the cells cyclic onto
                                    ///  the available processing elements
        REDISTRIB_RAND = 2          ///< Redistribute the particles randomly
    };

    /// Redistribute the particles 
    void redistribute_particles();

    /// Compute the mapping from particles to
    /// target pes
    void compute_map_redistrib_cells_cyclic(int*, int);
    void compute_map_redistrib_rand(int*);

    /// Convenience alltoallv wrapper which automatically
    /// computes the displacements
    void alltoallv(float*, int*, float*, int*);
    void alltoallv(int*  , int*, int*  , int*);

    /// Prepare the buffers for dispatching them to the
    /// mexico::Instance
    void prepare(float**, int**, int**, int**, int**, int**);

    /// Check the result of the job
    void check(int* boxes);


    int myrank;
    int nprocs;

#undef  MAX_NUM_WORKERS
#define MAX_NUM_WORKERS 4096

    int num_worker;                 ///< Number of worker
    int worker[MAX_NUM_WORKERS];    ///< List of worker ranks 
    int num_cells;                  ///< Number of cells per pe in each spatial dimension
    int num_particles_in_cell;      ///< Number of particles in each cell
    int num_bytes_per_particle_i;   ///< Payload per particle (in)
    int num_bytes_per_particle_o;   ///< Payload per particle (out)
    
    int redistrib_strategy;         ///< Redistribution strategy
    int redistrib_cyclic_blk;       ///< Block size for the cyclic distribution

    BinningJob* job;                ///< Binning job instance
    mexico::Instance* ci;           ///< The mexico instance

    int     w_peid;                 ///< Worker process id
    long    w_num_particles;        ///< Number of particles on this worker
    float*  w_particles;            ///< Array of particle positions on the worker

    int     num_particles;          ///< Number of particles on the processing
                                    ///  element (after redistribution)
    float*  particles;              ///< Array of particles positions
    int*    sources;                ///< Source array maps particles to their origin worker
    int*    offsets;                ///< Particle offsets
};

Application::Application()
{
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if(0 == myrank)
    {
        printf( " +----------------------------------------+\n" );
        printf( " |         MEXICO BINNING BENCHMARK       |\n" );
        printf( " +----------------------------------------+\n" );
        printf( "\n");
    }
}

bool Application::pe_is_worker() const
{
    int i;

    for(i = 0; i < num_worker; ++i)
        if(worker[i] == myrank)
            return 1;
    return 0;
}

/// For simplicity we parse the "binning" namelist in Fortran
#undef  F90NAME
#define F90NAME(func)   func ## _
extern "C" void F90NAME(parse_binning_namelist)(int*, int*, int*, int*, int*, int*, int*, int*);

void Application::parse_args(int argc, char** argv)
{
    int i;

    for(i = 0; i < MAX_NUM_WORKERS; ++i)
        worker[i] = -1;    //< invalid

    F90NAME(parse_binning_namelist)(&num_worker, 
                                    worker, 
                                    &num_cells, 
                                    &num_particles_in_cell, 
                                    &num_bytes_per_particle_i,
                                    &num_bytes_per_particle_o,
                                    &redistrib_strategy,
                                    &redistrib_cyclic_blk);
}

void Application::create_particles()
{
    int i, j, k, l, u;

    srand(0);

    w_num_particles = num_cells*num_cells*num_cells*num_particles_in_cell;
    w_particles = new float[3*w_num_particles];

    u = 0;
    /// Loop over the cells and create particles
    for(i = 0; i < num_cells; ++i)
        for(j = 0; j < num_cells; ++j)
            for(k = 0; k < num_cells; ++k)
                for(l = 0; l < num_particles_in_cell; ++l)
                {
                    w_particles[u++] = (1.0*rand())/(1.0*RAND_MAX) + num_cells*w_peid + i;
                    w_particles[u++] = (1.0*rand())/(1.0*RAND_MAX) + j;
                    w_particles[u++] = (1.0*rand())/(1.0*RAND_MAX) + k;
                }
}

void Application::compute_map_redistrib_cells_cyclic(int* map, int blk)
{
    int i, i0, j, j0, k, k0, l, u, p;

    printf(" Distributing cells in a cyclic fashion.\n");

    u = 0;
    p = 0;
    /// Loop over the cells and create particles
    for(i = 0; i < num_cells; i += blk)
        for(j = 0; j < num_cells; j += blk)
            for(k = 0; k < num_cells; k += blk)
            {
                for(i0 = i; i0 < i + blk; ++i0)
                    for(j0 = j; j0 < j + blk; ++j0)
                        for(k0 = k; k0 < k + blk; ++k0)
                        {
                            for(l = 0; l < num_particles_in_cell; ++l)
                            {
                                map[u++] = p;
                            }
                        }
                p = (p + 1)%nprocs;
            }
}

void Application::compute_map_redistrib_rand(int* map)
{
    int i, j, k, l, u;

    srand(0);

    printf(" Distributing particles in a random fashion.\n");

    u = 0;
    /// Loop over the cells and create particles
    for(i = 0; i < num_cells; ++i)
        for(j = 0; j < num_cells; ++j)
            for(k = 0; k < num_cells; ++k)
                for(l = 0; l < num_particles_in_cell; ++l)
                {
                    map[u++] = rand()%nprocs;
                }
}

void Application::alltoallv(float* sendbuf, int* sendcnts, float* recvbuf, int* recvcnts)
{
    int* recvdispls;
    int* senddispls;
    float null;
    int i;

    recvdispls = new int[nprocs];
    senddispls = new int[nprocs];

    recvdispls[0] = 0;
    for(i = 1; i < nprocs; ++i)
        recvdispls[i] = recvdispls[i-1] + recvcnts[i-1];

    senddispls[0] = 0;
    for(i = 1; i < nprocs; ++i)
        senddispls[i] = senddispls[i-1] + sendcnts[i-1];

    if(!sendbuf)
        sendbuf = &null;        /// Some MPI implementations have problems with passing
                                /// NULL even if 

    MPI_Alltoallv(sendbuf, sendcnts, senddispls, MPI_FLOAT,
                  recvbuf, recvcnts, recvdispls, MPI_FLOAT, MPI_COMM_WORLD);
}

void Application::alltoallv(int* sendbuf, int* sendcnts, int* recvbuf, int* recvcnts)
{
    int* recvdispls;
    int* senddispls;
    int null, i;

    recvdispls = new int[nprocs];
    senddispls = new int[nprocs];

    recvdispls[0] = 0;
    for(i = 1; i < nprocs; ++i)
        recvdispls[i] = recvdispls[i-1] + recvcnts[i-1];

    senddispls[0] = 0;
    for(i = 1; i < nprocs; ++i)
        senddispls[i] = senddispls[i-1] + sendcnts[i-1];

    if(!sendbuf)
        sendbuf = &null;        /// Some MPI implementations have problems with passing
                                /// NULL even if 

    MPI_Alltoallv(sendbuf, sendcnts, senddispls, MPI_INT,
                  recvbuf, recvcnts, recvdispls, MPI_INT, MPI_COMM_WORLD);
}

void Application::redistribute_particles()
{
    int* w_map;                 ///< Map from particles to the target ranks
    int* num_particles_to_send; ///< Number of particles to send. This array
                                ///  contains only non-zero entries on worker
                                ///  pes
    int* num_particles_to_recv; ///< Number of particles to receive
    float* w_ordered_particles; ///< Particles ordered according to their map    
    int* w_sources;             ///< The sources (filled with myrank and workers)
    int* w_offsets;             ///< Offsets
    int* t;
    int i;

    num_particles_to_send = new int[nprocs]();
    num_particles_to_recv = new int[nprocs]();

    w_map = 0;
    w_ordered_particles = 0;
    w_sources = 0;
    w_offsets = 0;

    if(pe_is_worker())
    {
        w_map = new int[w_num_particles];
        w_ordered_particles = new float[3*w_num_particles];
        w_sources = new int[w_num_particles];
        w_offsets = new int[w_num_particles];

        /// Fill in w_map
        switch(redistrib_strategy)
        {
        case REDISTRIB_CELLS_CYCLIC:
            compute_map_redistrib_cells_cyclic(w_map, redistrib_cyclic_blk);
            break;
        case REDISTRIB_RAND:
            compute_map_redistrib_rand(w_map);
            break;
        default:
            printf(" ERROR: invalid strategy = %d\n", redistrib_strategy);
            MPI_Abort(MPI_COMM_WORLD, 128);
        }

        /// Compute the number of particles to send.
        for(i = 0; i < w_num_particles; ++i)
            num_particles_to_send[w_map[i]] += 1;

        
        /// Use num_particles_to_recv as a temporary array
        /// for storing the offsets
        t    = num_particles_to_recv;
        t[0] = 0;
        for(i = 1; i < nprocs; ++i)
            t[i] = t[i-1] + num_particles_to_send[i-1];

        for(i = 0; i < w_num_particles; ++i)
        {
            w_ordered_particles[3*t[w_map[i]]  ] = w_particles[3*i  ];
            w_ordered_particles[3*t[w_map[i]]+1] = w_particles[3*i+1];
            w_ordered_particles[3*t[w_map[i]]+2] = w_particles[3*i+2];

            w_sources[t[w_map[i]]] = myrank;
            w_offsets[t[w_map[i]]] = i;

            t[w_map[i]] += 1;
        }
    }

    MPI_Alltoall(num_particles_to_send, 1, MPI_INT, num_particles_to_recv, 1, MPI_INT, MPI_COMM_WORLD);

    /// Compute the number of particles
    num_particles = 0;
    for(i = 0; i < nprocs; ++i)
        num_particles += num_particles_to_recv[i];

    sources   = new int[num_particles];
    alltoallv(w_sources, num_particles_to_send, sources, num_particles_to_recv);

    offsets   = new int[num_particles];
    alltoallv(w_offsets, num_particles_to_send, offsets, num_particles_to_recv);

    particles = new float[3*num_particles];

    /// Three coordinates per particle
    for(i = 0; i < nprocs; ++i)
    {
        num_particles_to_send[i] *= 3;
        num_particles_to_recv[i] *= 3;
    }
    alltoallv(w_ordered_particles, num_particles_to_send, particles, num_particles_to_recv);

    delete[] num_particles_to_send;
    delete[] num_particles_to_recv;
    
    if(pe_is_worker())          /// Clean up
    {
        delete[] w_ordered_particles;
        delete[] w_map;
        delete[] w_sources;
        delete[] w_offsets;
        delete[] w_particles;   /// Not needed anymore. Set it to zero  
                                /// to be sure
        w_particles = 0;
    }
}

void Application::setup()
{
    FILE* fi;
    int i;

    /// Compute the worker id
    w_peid = -1;
    for(i = 0; i < num_worker; ++i)
        if(myrank == worker[i])
            w_peid = i;
   
    /// Create the particles on worker nodes. This fills in the w_particles
    /// array 
    if(pe_is_worker())
        create_particles();
    /// Now we distribute the particles to all other processing elements
    redistribute_particles();

    /// Create the job instance
    job = 0;
    if(pe_is_worker())
        job = new BinningJob(w_num_particles, num_cells, num_bytes_per_particle_i, num_bytes_per_particle_o);

    if(!(fi = fopen("binning.in", "r")))
    {
        printf(" ERROR: Cannot open \"binning.in\"\n");
        MPI_Abort(MPI_COMM_WORLD, 128);
    }

    ci  = new mexico::Instance(MPI_COMM_WORLD, num_worker, worker, job, fi);    
    fclose(fi);
}

void Application::prepare(float** i_buf, int** i_worker, int** i_offsets, int** o_buf, int** o_worker, int** o_offsets)
{
    int i;

    (*i_buf) = new float[(3 + num_bytes_per_particle_i/4)*num_particles];
    (*o_buf) = new int  [(1 + num_bytes_per_particle_o/4)*num_particles];

    (*i_worker)  = new int[num_particles];
    (*o_worker)  = new int[num_particles];

    (*i_offsets) = new int[num_particles];
    (*o_offsets) = new int[num_particles];

    for(i = 0; i < num_particles; ++i)
    {
        (*i_buf)[(3 + num_bytes_per_particle_i/4)*i  ] = particles[3*i  ];
        (*i_buf)[(3 + num_bytes_per_particle_i/4)*i+1] = particles[3*i+1];
        (*i_buf)[(3 + num_bytes_per_particle_i/4)*i+2] = particles[3*i+2];
        memset(&(*i_buf)[(3 + num_bytes_per_particle_i/4)*i+3], 'A', num_bytes_per_particle_i);

        (*i_worker)[i] = sources[i];
        (*o_worker)[i] = sources[i];

        (*i_offsets)[i] = offsets[i];
        (*o_offsets)[i] = offsets[i];
    }
}

void Application::check(int* boxes)
{
    int i, ix, iy, iz;
    float x, y, z;

    for(i = 0; i < num_particles; ++i)
    {
        x  = particles[3*i  ];
        y  = particles[3*i+1];
        z  = particles[3*i+2];

        ix = (int )x;
        iy = (int )y;
        iz = (int )z;
        
        if(boxes[(1 + num_bytes_per_particle_o/4)*i] != ix*num_cells*num_cells + iy*num_cells + iz)
        {
            printf(" ERROR: Check failed.\n");
            MPI_Abort(MPI_COMM_WORLD, 128);
        }
    }
}

void Application::run()
{
    float* i_buf;
    int* i_worker;
    int* i_offsets;
    int* o_buf;
    int* o_worker;
    int* o_offsets;
    int i;
    double t0, t1, timing;

#undef  NTIMES
#define NTIMES 10/*20*/

    prepare(&i_buf, &i_worker, &i_offsets, &o_buf, &o_worker, &o_offsets);

    timing = 0.0;
    for(i = 0; i < NTIMES; ++i)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        t0 = MPI_Wtime();

        ci->exec(i_buf,
                 3 + num_bytes_per_particle_i/4,
                 MPI_FLOAT,
                 num_particles,
                 1,
                 i_worker,
                 i_offsets,
                 o_buf,
                 1 + num_bytes_per_particle_o/4,
                 MPI_INT,
                 num_particles,
                 1,
                 o_worker,
                 o_offsets);

        MPI_Barrier(MPI_COMM_WORLD);
        t1 = MPI_Wtime();

        check(o_buf);

        timing += (t1 - t0);
    }

    timing = timing/NTIMES;
    
    printf(" TIMING: %.3e\n", timing);

    delete[] i_buf;
    delete[] i_worker;
    delete[] i_offsets;
    delete[] o_buf;
    delete[] o_worker;
    delete[] o_offsets;
}

void Application::finish()
{
    delete ci;
    delete job;
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

#if !defined(USE_GA) && !defined(USE_SHMEM)
    /// On XE6 systems, GA and SHMEM are mutually exclusive, i.e.
    /// you cannot use both in the same executable (linking works but
    /// only one library can be initialized)
    #define USE_GA    1
    #define USE_SHMEM 0
#endif

#if (1 == USE_GA) && defined(MEXICO_HAVE_GA)
    printf(" \nCALLING INIT GA\n");
    GA_Initialize();
    printf(" Initialization finished");
    if(MA_FALSE == MA_init(MT_CHAR, 1024*1024, 512*1024*1024))
        MPI_Abort(MPI_COMM_WORLD, 128);
#endif
#if (1 == USE_SHMEM) && defined(MEXICO_HAVE_SHMEM)
    printf(" \nCALLING INIT SHMEM\n");
    shmem_init();
#endif

    /// Need to be called before any other mexico
    /// function is called or any mexico class is
    /// instanciated
    mexico::init(&argc, &argv);

    Application app;
    app.parse_args(argc, argv);

    app.setup();
    app.run();
    app.finish();

    mexico::finalize();
#if (1 == USE_SHMEM) && defined(MEXICO_HAVE_SHMEM)
    shmem_finalize();
#endif
#if (1 == USE_GA) && defined(MEXICO_HAVE_GA)
    GA_Terminate();
#endif
    return MPI_Finalize();
}

