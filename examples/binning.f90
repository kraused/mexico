
! vi: tabstop=4:expandtab

! Copyright 2010 University of Lugano. All rights reserved.
! 
! Redistribution and use in source and binary forms, with or without modification, are
! permitted provided that the following conditions are met:
! 
!    1. Redistributions of source code must retain the above copyright notice, this list of
!       conditions and the following disclaimer.
! 
!    2. Redistributions in binary form must reproduce the above copyright notice, this list
!       of conditions and the following disclaimer in the documentation and/or other materials
!       provided with the distribution.
! 
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS
! OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
! AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
! CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
! CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
! SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
! ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
! NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
! ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
! 
! The views and conclusions contained in the software and documentation are those of the
! authors and should not be interpreted as representing official policies, either expressed
! or implied, of the University of Lugano.

subroutine parse_binning_namelist(num_worker,               &
                                  worker,                   &
                                  num_cells,                &
                                  num_particles_in_cell,    &
                                  num_bytes_per_particle_i, &
                                  num_bytes_per_particle_o, &
                                  redistrib_strategy,       &
                                  redistrib_cyclic_blk)
    implicit none

    integer, intent(out) :: num_worker,                 &
                            worker(4096),               &
                            num_cells,                  &
                            num_particles_in_cell,      &
                            num_bytes_per_particle_i,   &
                            num_bytes_per_particle_o,   &
                            redistrib_strategy,         &
                            redistrib_cyclic_blk
    logical :: file_exists

    namelist /binning/ num_worker, worker, num_cells,   &
                       num_particles_in_cell,           &
                       num_bytes_per_particle_i,        &
                       num_bytes_per_particle_o,        &
                       redistrib_strategy,              &
                       redistrib_cyclic_blk

    inquire(file = "binning.in", exist = file_exists )
    if(file_exists) then
        open (55, file = 'binning.in', status = 'old')
        read (55, nml = binning)
        close(55)
    else
        write(*,*) 'cannot find file binning.in'
        stop
    end if

    ! The number of bytes should be a multiple of the
    ! size of float (= 4 on most systems)
    num_bytes_per_particle_i = 4*int(num_bytes_per_particle_i/4)
    num_bytes_per_particle_o = 4*int(num_bytes_per_particle_o/4)

    write(*, binning)

end subroutine

