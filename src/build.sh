#/usr/bin/sh

PROJECT_ROOT=/home/arr2vg/biocomplexity/loimos

function build-loimos () {
  BIN=${1}

  echo "${OPTS}" make
  make clean
  make
  mv loimos ${BIN}
}

for enable_smp in true false; do
  for enable_tracing in true false; do
    for enable_lb in true false; do
      for enable_hypercomm in true false; do
        BIN="loimos"
        OPTS=""
        
        if $enable_smp; then
          BIN="$BIN-smp"
          export CHARM_HOME="$PROJECT_ROOT/charm/mpi-linux-x86_64-smp"
        else
          export CHARM_HOME="$PROJECT_ROOT/charm/mpi-linux-x86_64"
        fi

        if $enable_tracing; then
          BIN="$BIN-prj"
          OPTS="$OPTS ENABLE_TRACING=1"
          export ENABLE_TRACING=1
        fi

        if $enable_load_balancing; then
          BIN="$BIN-lb"
          OPTS="$OPTS ENABLE_LB=1"
          export ENABLE_LB=1
        fi

        if $enable_hypercomm; then
          BIN="$BIN-hc"
          OPTS="$OPTS USE_HYPERCOMM=1"
          export USE_HYPERCOM=1
        fi

        build-loimos ${BIN} "${OPTS}"
      done
    done
  done
done
