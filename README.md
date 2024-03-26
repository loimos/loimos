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

These dependencies may be installed from source by following the instructions available on the project GitHub repositories, or through the Spack package manager (as `charmpp` and `protobuf`, respectively).
We recommend building or installing Charm++ twice: once with the `smp` argument passed to `./build` and once without, as Charm++'s Shared Memory Parallelism (SMP) mode is helpful on some machines and node counts but not on others.

Once both Charm++ and Protobuf are installed, several environment variables need to be set so that Loimos can properly locate your installations. We recommend adding the following lines to your `~/.bashrc`, `~/.bash_profile`, or equivalent configuration file:

```bash
export CHARM_HOME="/<full/path/to/install/dir>/charm/<version>"
export PROTOBUF_HOME="/<full/path/to/install/dir>"
export LD_LIBRARY_PATH="$PROTOBUF_HOME/lib:$LD_LIBRARY_PATH"
```

Note that `CHARM_HOME` should be set so that the file `$CHARM_HOME/bin/charmc` exists, or, if building Charm++ with SMP, so that the file `$CHARM_HOME-smp/bin/charmc` exisits. Likewise, `PROTOBUF_HOME` should be set so that the file `$PROTOBUF_HOME/bin/protoc` exists. If these files are not present at these locations Loimos will not be able to build properly.

## Building from Source

Once Loimos's dependencies have been installed as outlined above, it can be built from source as follows:
1. Clone this repo, such as with
    ```git clone git@github.com:loimos/loimos.git```
2. `cd` into `loimos/src` and run `make` to build the application from source. By default, the executable will be named `loimos`, although some compile-time options can change this.

### Compile-time Flags

Loimos has a number of compile-time options that can be used to produce executables tuned for different purposes. These are generally passed to the build system by setting various environment variables. For example, an SMP version of Loimos can be build with

```bash
ENABLE_SMP=1 make
```

We recommend running `make clean` before building Loimos with a different configuration. Loimos's various compile-time options are summarized below. Note that some options append a suffix to the executable.
When multiple such options are used, these suffixes will be added in the order in which the appear in the table below. For example, building with `ENABLE_SMP=1 make` will build the executable `loimos-smp`, whereas building with `ENABLE_SMP=1 ENABLE_LB=1 ENABLE_DEBUG=2 make` will build the executable `loimos-smp-lb`, with the debug level not impacting the executable name.

| Environment Variable    | Value | Executable Suffix | Explanation                                                                   |
|-------------------------|-------|-------------------|-------------------------------------------------------------------------------|
| `ENABLE_SMP`            | 1     | `-smp`            | Builds Loimos with Shared Memory Parallelism.                                 |
| `ENABLE_TRACING`        | 1     | `-prj`            | Enables collecting performance profiles using the built-in Charm++ profiler   |
|                         | 2     |                   | Additionally prints memory usage information                                  |
| `ENABLE_LB`             | 1     | `-lb`             | Enables Charm++ dynamic load balancing                                        |
| `ENABLE_AGGREGATION`    | 1     | `-agg`            | Enables Charm++ message aggregation                                           |
| `ENABLE_SC`             | 1     | `-sc`             | Enables short-circuit evaluation of discrete event simulation                 |
| `ENABLE_RANDOM_SEED`    | 1     |                   | If not passed, will use a the same seed for all pseudo-random number generators in each run         |
| `ENABLE_FORCE_FULL_RUN` | 1     |                   | Forces the simulation to run the full number of days, even if the outbreak dies out and no people are still infected   |
| `ENABLE_UNIT_TESTING`   | 1     |                   | Builds Loimos with unit tests enabled                                         |
| `ENABLE_DEBUG`          | 1     |                   | Basic debug information                                                       |
|                         | 2     |                   | Verbose debug information                                                     |
|                         | 3     |                   | Prints out counts of person-person edges for each location on each day        |
|                         | 4     |                   | Saves list of all person-person edges to output file                          |
|                         | 5     |                   | Chare-level debug information                                                 |
|                         | 6     |                   | People- and location-level debug information                                  |
| `OUTPUT_FLAGS`          |       |                   | Adjusts output format. If this flag is set, the `OF` commandline argument is treated as a directory, rather than a file path. All options listed below may be combined with a bitwise or (e.g. passing `OUTPUT_FLAGS=5` will cause both state transitions and visit overlaps to be recorded)|
|                         | 1     |                   | Writes out individual-level state transitions. Can be or-ed with other options|
|                         | 2     |                   | Writes out all exposures. Can be or-ed with other options                     |
|                         | 4     |                   | Writes out all visit overlaps. Can be or-ed with other options                |
Note that the values for `OUTPUT_FLAGS` . All output options for

## Running the Code

### Quick Start
A quick test run may be run to verify that the build was successful using `<compile options> make test-small` with the same compile-time options used to build the executable. This test should only take a couple seconds to run.
A more through test suite can be run using `<compile options> make test`, although this will take longer to run (generally under 5 minutes), and so we recommend either submitting them as a batch script or in an interactive allocation on your cluster of choice.
Note that we use the default executable, `loimos`, for all further example commands below. If you used any compile-time options that change the executable name, replace `loimos` with the appropriate executable name.

For a more substantial test, run the following command on a 64 task MPI allocation:

```bash
srun -n 64 ./loimos 1 2349 2349 1248 1248 5 8 8 64 16 on-the-fly-md-out.csv ../data/disease_models/covid19_onepath.textproto
```

This command will run Loimos on a purely synthetic population about the size of the state of Maryland (~5.5 million people and ~1.5 million locations) with 64 processes.

### Command Line Arguments
Loimos can either generate a purely synthetic population on-the-fly or load a pre-defined population. Note that while more realistic social contact networks (the digital twins mentioned previously) are provided to Loimos using the later syntax, these datasets are not currently released.

To generate a synthetic population on-the-fly, run Loimos with a command line in the following form:

```bash
./loimos 1 <PGW> <PGH> <LGW> <LGH> <NV> <LPGW> <LPGH> <NPP> <ND> <OF> <DF>
```

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

For pre-defined populations, run Loimos with the command:

```bash
./loimos 0 <NP> <NL> <NPP> <NLP> <ND> <NDV> <OF> <DF> <SD> [-m] [-i <IF>]
```

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
  scenario. These are usually found in [`loimos/data/populations`](https://github.com/loimos/loimos/blob/develop/data/populations).
- `-m` or `--min-max-alpha` is an optional flag which indicates that the
  min-max-alpha contact model should be used.
- `-i` is an optional flag used when specifying an intervention. `IF` should
  be the path to a `.textproto` file specifying the intervention to be used.
  These are generally found in [`loimos/data/interventions`](https://github.com/loimos/loimos/blob/develop/data/interventions).

## Authors

Many thanks go to Loimos's
[contributors](https://github.com/loimos/loimos/graphs/contributors).

## License

Loimos is distributed under the terms of the MIT license.

All contributions must be made under the MIT license. Copyrights in the Loimos project are retained by contributors. No copyright assignment is required to contribute to Loimos.

See [LICENSE](https://github.com/hatchet/loimos/blob/develop/LICENSE) for details.

SPDX-License-Identifier: MIT
