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
and runtime in which Loimos is implemented.
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
    
    ```export CHARM_HOME="/<full/path/to/install/dir>/charm/<version>"```

    Note that you should *not* append `-smp` to this path in order to build Loimos with SMP; this will be handled by setting a separate environment variable at compile time.

### Protobuf

Protobuf can be installed as follows:
1. Create a separate `install` directory somewhere you have write permissions with `mkdir install`. This is to get around the fact that on most computing clusters, many users will not have sudo permissions. Placing this at the top level of your home directory is often convenient, in which case the full path is given by `$HOME/install`.
2. Download version 3.21.12 of Protobuf here: [https://github.com/protocolbuffers/protobuf/releases/tag/v21.12](https://github.com/protocolbuffers/protobuf/releases/tag/v21.12). We suggest using the C++ version of this release rather than cloning the full repo or using the most recent version as the C++ version has a much simpler build system without as many dependencies, and this is the last release which contains a separate C++ version. You can download this C++ version like so: `wget https://github.com/protocolbuffers/protobuf/releases/download/v21.9/protobuf-cpp-3.21.12.tar.gz`
3. Extract the downloaded files and `cd` into the resulting directory: `tar -xzf protobuf-cpp-3.21.12.tar.gz`.
4. Build and install Protobuf as follows:
    ```
    ./configure --prefix=/<full/path/to/install/dir>
    make
    make check
    make install
    ```
5. Lastly, set two environment variables so that Loimos is able to find your local installation of Protobuf. We recommend adding the following lines to either your `~/.bashrc` or `~/.bash_profile` (or similar configuration file for other terminals):
    
    ```
    export PROTOBUF_HOME="/<full/path/to/install/dir>"
    export LD_LIBRARY_PATH="$PROTOBUF_HOME/lib:$LD_LIBRARY_PATH"
    ```

## Building from Source

Once Loimos's dependencies have been installed as outlined above, it can be built from source as follows:
1. Clone this repo, such as with
    ```git clone git@github.com:loimos/loimos.git```
2. `cd` into `loimos/src` and run `make` to build the application from source. By default, the executable will be named `loimos`, although some compile-time options can change this.

### Compile-time Flags

Loimos has a number of compile-time options that can be used to produce executables tuned for different purposes. These are generally passed to the build system by setting various environment variables. For example, an SMP version of Loimos can be build with

```
ENABLE_SMP=1 make
```

We recommend running `make clean` before building Loimos with a different configuration. Loimos's various compile time options are summarized below. Note that some options append a suffix to the executable. When multiple such options are used, these suffixes will be added in the order in which the appear in the table below. For example, building with `ENABLE_SMP=1 make` will build the executable `loimos-smp`, whereas building with `ENABLE_SMP=1 ENABLE_LB=1 ENABLE_DEBUG=2 make` will build the executable `loimos-smp-lb`, with the debug level not impacting the executable name.

| Environment Variable  | Value | Executable Suffix | Explanation                                                                   |
|-----------------------|-------|-------------------|-------------------------------------------------------------------------------|
| `ENABLE_SMP`          | 1     | `-smp`            | Builds Loimos with Shared Memory Parallelism.                                 |
| `ENABLE_TRACING`      | 1     | `-prj`            | Enables collecting performance profiles using the built-in Charm++ profiler   |
| `ENABLE_LB`           | 1     | `-lb`             | Enables Charm++ dynamic load balancing                                        |
| `ENABLE_UNIT_TESTING` | 1     |                   | Builds Loimos with unit tests enabled                                         |
| `ENABLE_DEBUG`        | 1     |                   | Basic debug information                                                       |
|                       | 2     |                   | Verbose debug information                                                     |
|                       | 3     |                   | Chare-level debug information                                                 |
|                       | 4     |                   | People- and location-level debug information                                  

## Running the Code

### Quick Start
A quick test run may be run to verify that the build was successful using `<compile options> make test-small` with the same compile options used to build the executable. This test should only take a couple seconds to run. A more through test suite can be run using `<compile options> make test`, although this will take longer to run (generally under 5 minutes), and so we recommend either submitting them as a batch script or in an interactive allocation on your cluster of choice.

For a more substantial test, run the following command on a 64 task MPI allocation:

```srun -n 64 ./loimos 1 2349 2349 1248 1248 5 8 8 64 16 on-the-fly-md-out.csv ../data/disease_models/covid19_onepath.textproto```

This command will run Loimos on a purely synthetic population about the size of the state of Maryland (~5.5 million people and ~1.5 million locations) with 64 processes.

### Command Line Arguments
Loimos can either generate a purely synthetic population on-the-fly or load a pre-defined population. Note that while more realistic social contact networks (the digital twins mentioned previously) are provided to Loimos using the later syntax, these datasets are not currently released.

To generate a synthetic population on-the-fly, run Loimos with a command line in the following form:

```./loimos 1 <PGW> <PGH> <LGW> <LGH> <NV> <LPGW> <LPGH> <NPP> <ND> <OF> <DF>```

Where
- `PGW` is the people grid width.
- `PGH` is the people grid height.
- `LGW` is the location grid width, and should be a multiple of `LPGW`.
- `LGH` is the location grid height, and should be a multiple of `LPGH`.
- `NV` is the average number of visits per person per day.
- `LPGW` is the location partition grid width.
- `LPGH` is the location partition grid height.
- `NPP` is the number of people partitions, and should usually be equal to `LPGW * LPGH` and evenly divide the number of cores Loimos is run on.
- `ND` is the number of days to simulate.
- `OF` is the path to the output file.
- `DF` is the path to the disease model.

For pre-defined populations, run with the command:

```./loimos 0 NP NL NPP NLP ND NDV OF DF SD [-m] [-i I]```

Where
- `NP` is the number of people.
- `NL` is the number of locations.
- `NPP` is the number of people partitions, and should evenly divide the number of cores Loimos is run on.
- `NLP` is the number of location partitions, and should evenly divide the number of cores Loimos is run on.
- `ND` is the number of days to simulate.
- `NVD` is the number of days of visit data in the scenario (or, equivalently,
  how often to repeat each person's visit schedule).
- `OF` is the path to the output file.
- `DF` is the path to the disease model.
- `SD` is the path to the directory containing the population data for the
  scenario. These are usually found in `loimos/data/populations`.
- `-m` or `--min-max-alpha` is an optional flag which indicates that the
  min-max-alpha contact model should be used.
- `-i` is an optional flag used when specifying an intervention. `I` should
  be the path to a `.textproto` file specifying the intervention to be used.
  These are generally found in `loimos/data/interventions`.

### Configuring Batch Jobs for Your System