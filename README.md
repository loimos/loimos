# Loimos

Loimos is a parallel, agent-based simulator for modeling the spread of
infectious diseases. This simulator employs a combination of
time-stepping and discrete event simulations to capture
population dynamics in realistic social contact networks. These
networks are *digital twins* of various U.S. states built based on a
variety of sources, including census and survey data,
which are then used as input for various outbreak simulations.

## Installing Dependencies

Loimos has two major dependencies:
1. [Charm++](https://github.com/UIUC-PPL/charm), the parallel framework
and runtime in which Loimos was implemented.
2. Google's Protocol Buffers, or [Protobuf](https://github.com/protocolbuffers/protobuf),
which Loimos uses to format and parse many of its input files.

### Charm++

Charm++ may be installed from source as follows:
1. Clone the Charm++ git repo from GitHub here: [https://github.com/UIUC-PPL/charm](https://github.com/UIUC-PPL/charm).
2. Once the repo is cloned, `cd` into its top-level directory and check out the latest version (ex: `git checkout v7.0.0`).
3. Next, build Charm++. Charm++ has a variety of options and which are best to use varies by system. In general, the build line will look something like

    ```./build charm++ <version> smp --with-production --enable-tracing```
  
    Note that you may need to load some module files in order to properly build Charm++ on a cluster. Most clusters will require loading the system MPI, and   Cray clusters will generally require some form of `craype-hugepages`. See below for some examples, or consult the [Charm++ documentation](https://charm.  readthedocs.io/en/latest/charm%2B%2B/manual.html#sec-install).
  
    | System              | Version            | Module Line                                              |
    |---------------------|--------------------|----------------------------------------------------------|
    | Cori at NERSC       | `mpi-crayxc`       | `module load rca craype-hugepages8M`                     |
    | Perlmutter at NERSC | `mpi-crayshasta`   | `module load cpu craype-hugepages8M cray-pmi cray-mpich` |
    | Zaratan at UMD      | `mpi-linux-x86_64` | `module load openmpi`                                    |
    | Rivanna at UVA      | `mpi-linux-x86_64` | `module load gompi/9.2.0_3.1.6`                          |

    We recommend building Charm++ twice: once with the `smp` argument passed to `./build` and once without, as Charm++'s Shared Memory Parallelism (SMP) mode is helpful on some machines and node counts but not on others.

4. Lastly, set the `CHARM_HOME` environment variable so that Loimos is able to find your local installation of Charm++. We recommend adding the following line to either your `~/.bashrc` or `~/.bash_profile` (or similar configuration file for other terminals):
    
    ```export CHARM_HOME="/<full/path/to/install>/charm/<version>"```

    Note that you should *not* append `-smp` to this path in order to build Loimos with SMP; this will be handled by setting a separate environment variable at compile time.

### Protobuf

## Building the Source

### Compile-time Flags

## Running the Code

### Quick Start

### Configuring Batch Jobs for Your System