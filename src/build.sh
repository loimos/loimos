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
      for enable_agg in true false; do
        for enable_prj in true; do
          OPTS=""
          
          if $enable_smp; then
            export ENABLE_SMP=1
          fi

          if $enable_tracing; then
            export ENABLE_TRACING=1
          fi

          if $enable_load_balancing; then
            export ENABLE_LB=1
          fi

          if $enable_agg; then
            export ENABLE_AGGREGATION=1
          fi
          
          if $enable_prj; then
            export ENABLE_TRACING=1
          fi

          make clean
          make
          unset ENABLE_SMP
          unset ENABLE_TRACING
          unset ENABLE_LB
          unset ENABLE_AGGREGATION
        done
      done
    done
  done
done
