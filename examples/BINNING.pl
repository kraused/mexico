#!/usr/bin/env perl

use strict;
use Cwd;
use File::Copy;
use List::Util qw[min max];


# The list of runtime implementations
my @rtimpl = ( "MPI Alltoall", "MPI RMA", "MPI Pt2Pt", "GA", "GA gs", "SHMEM" );

# The options for the runtime
my %rtopts = (
	"MPI Alltoall" => [ "", "pack", "exch_with_pt2pt", "pack,exch_with_pt2pt" ],
	"MPI RMA"	   => [ "coalesce" ],
	"MPI Pt2Pt"    => [ "" ],
	"GA"		   => [ "coalesce", "coalesce,use_irreg_distr" ],
	"GA gs"		   => [ "coalesce", "coalesce,use_irreg_distr" ],
	"SHMEM"	       => [ "coalesce" ]
);

# On cub
my $bindir = "$ENV{HOME}/Work/Multiscale/krause-multiscale/trunk/mexico/build";
# On palu
# my $bindir = 
# On rosa
# my $bindir = 

# prefix for the output
my $prefix = "RUN";

# The executable names
my %rtexe = (
    "MPI Alltoall" => "$bindir/binning",
    "MPI RMA"      => "$bindir/binning",
    "MPI Pt2Pt"    => "$bindir/binning",
    "GA"           => "$bindir/binning",
    "GA gs"        => "$bindir/binning",
    "SHMEM"        => "$bindir/binning",
);

# Go to the prefix directory
chdir $prefix or die $!;

foreach my $rti (@rtimpl)
{
	# Create working directory
	my $wdir = uc($rti);
	$wdir =~ s/ /_/;
	mkdir $wdir or print "Error creating directory \"$wdir\": $!\n";

	# Go into the working directory
	chdir $wdir or die $!;

	my @opts = @{ $rtopts{$rti} };
	for(my $i = 0; $i < @opts; ++$i)
	{
		my $o = $opts[$i];

		# Create a subdirectory for each option
		my $sdir = uc($o);
		$sdir =~ s/,/-/;
		if("" eq "$sdir")
		{
			$sdir = "NONE";
		}
		mkdir $sdir or print "Error creating directory \"$sdir\": $!\n";

		# Go into the subdirectory
		chdir $sdir or die $!;

		# Copy the executable
		# Don't use copy, it doesn't keep permissions: copy($rtexe{$rti}, "binning") or die $!;
		system "cp $rtexe{$rti} binning";

		# Create a subfolder for each number of cores
		# on cub
		foreach my $np ( "2", "8", "16", "32", "64" )
		{
			mkdir $np or print "Error creating directory \"$np\": $!\n";
			
			# Go to the subdirectory
			chdir $np or die $!;
			
			# Create a subfolder for each configuration
			foreach my $ncells ( "64", "32", "16" )
			{
				mkdir "C$ncells" or print "Error creating directory \"$np\": $!\n";

				# Go to the subdirectory
				chdir "C$ncells" or die $!;

				# Name of the current directory
				my $dir = getcwd;

				#
				# Experiment 1: BINNING 
				#
				open(FH, ">job.SC") or die $!;
				print FH << "EOF";
#!/bin/bash

cd $dir

# On cub
mpiexec -np $np -hostfile \$PBS_NODEFILE $dir/../../binning
EOF
				close(FH);

				# on cub
				my $num_worker = max(1, int($np/8));
				my $worker = "";
				for(my $j = 0; $j < $np; $j += 8)
				{
					$worker = "$worker $j"
				}

				my %np = (
					"64" => "32",
					"32" => "256",
					"16" => "2048"
				);
				my $num_particles_in_cell = $np{$ncells};

				open(FH, ">binning.in") or die $!;
				print FH << "EOF";
! vi: tabstop=4:expandtab

! Benchmark settings
&binning
    ! Number of worker in the execution. This number
    ! should be smaller than the number of MPI processes
    num_worker = $num_worker,
    ! List of worker ranks
    worker = $worker,
    ! Number of cells in each spatial dimension per worker
    ! processing element
    num_cells = $ncells,
    ! Number of particles per timestep
    num_particles_in_cell = $num_particles_in_cell,
    ! Number of extray payload bytes per particle.
    num_bytes_per_particle_i = 0,
    num_bytes_per_particle_o = 0,
    ! Redistribution strategy:
    ! 1 = cyclic by cells
    ! 2 = randomly
    redistrib_strategy = 1,
	redistrib_cyclic_blk = 1
/

! Input for the log instance
&log
    ! the debug level between 0 and 2
    debug = 1
/

! runtime settings, e.g., which runtime to use
&runtime
    ! choice of the implementation, some choices
    ! might not be available depending on the
    ! configuration
    implementation = '$rti',
    ! optimization hints
    hints = '$o'
/

! information about the job
&job
    ! whether or not the job performs communication
    ! internally
    no_comm = .TRUE.
/
EOF
				close(FH);

				# Submit the job
				# on cub
				my $nodes = max(1,int($np/8));
				system "qsub -lnodes=$nodes:ppn=8,vmem=16gb,pvmem=2gb,walltime=00:30:00 job.SC | tee jid.txt\n";
		
				# Go up
				chdir ".." or die $!;
			}

			# Go up
			chdir ".." or die $!;
		}

		# Go up
		chdir ".." or die $!;
	}

	# Go back
	chdir ".." or die $!;
}

