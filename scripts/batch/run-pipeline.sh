#!/bin/bash
#SBATCH -p parallel
#SBATCH -q normal
#SBATCH -N 2
#SBATCH --ntasks-per-node=40
#SBATCH -t 120
#SBATCH --account=biocomplexity

function run_script() {
  SCRIPT=$1
  BATCH_SCRIPT=run-${SCRIPT/.py/.sh}
  TIME=$2
  QUEUE=$3
  export TASKS_PER_NODE=$4
  MEM=$5

  ARGS="--export=ALL -t ${TIME} -p ${QUEUE} --ntasks-per-node ${TASKS_PER_NODE} --wait"
  if [ ! -z ${MEM} ]; then
    ARGS="${ARGS} --mem ${MEM}"
  else
    ARGS="${ARGS} --exclusive"
  fi

  echo Running ${SCRIPT}
  echo sbatch ${ARGS} ${BATCH_DIR}/${BATCH_SCRIPT}
  sbatch ${ARGS} ${BATCH_DIR}/${BATCH_SCRIPT}

  # Make sure the user knows if the job fails or not, and stop here if it does
  # since the subsequent scripts won't work without the right input
  EXIT_CODE=$?
  if [ ${EXIT_CODE} -eq 0 ]; then
    echo Done with ${SCRIPT}
    echo
  else
    echo ${SCRIPT} failed with exit code ${EXIT_CODE}
    exit ${EXIT_CODE}
  fi
}

# Export these so that the suplimental batch scripts have acess to them
export PROJECT_ROOT=/home/arr2vg/biocomplexity/loimos
export SCRIPTS_DIR=${PROJECT_ROOT}/loimos/scripts
export BATCH_DIR=${SCRIPTS_DIR}/batch

export STATE=$1
export IN_DIR=/project/biocomplexity/us_population_pipeline/population_data/usa_840/2017/ver_1_9_0/${STATE}
export OUT_DIR=${PROJECT_ROOT}/loimos/data/populations/${STATE}

# Use a default runtime of 10 minutes
BASE_TIME=$2
if [ -z ${BASE_TIME} ]; then
  BASE_TIME=10
fi

BASE_MEMORY=$3

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 \
  openmpi/3.1.6 python/3.8.8 #mvapich2/2.3.3

# Create a directory to hold all the processed data, if none already exists
if [ ! -d ${OUT_DIR} ]; then
  mkdir ${OUT_DIR}
fi

#if [ ! -d ${OUT_DIR}/base_population ]; then
#  ll ${OUT_DIR}/base_population
#  ln -s ${IN_DIR}/base_population ${OUT_DIR}/base_population 
#fi
# Make simlinks to all important folders that might be needed later on,
# so that we can pass a sinlge input path to the scripts
for d in ${IN_DIR}/*; do
  d=$(basename $d)
  if [[ -d ${IN_DIR}/$d && ! -e ${OUT_DIR}/$d ]]; then
    ln -s ${IN_DIR}/$d ${OUT_DIR}/$d
  fi
done

echo Processing data for ${STATE}

run_script pop-prep.sh ${BASE_TIME} standard 1 ${BASE_MEMORY}
run_script combine-household-data.sh ${BASE_TIME} standard 1 ${BASE_MEMORY}

# From here on we no longer need any of all the raw data, so move all the
# processed data (which will be at the top level of the input dir) over to
# the otput dir
mv ${IN_DIR}/*.csv ${OUT_DIR}

run_script merge-location-data.py ${BASE_TIME} largemem 16 ${BASE_MEMORY}
run_script location-heuristics.py ${BASE_TIME} largemem 16 ${BASE_MEMORY}

# Copy the neccessary textproto files over
#cp ${PROJECT_ROOT}/loimos/data/textproto_templates/real_data_templates/*.textproto ${OUT_DIR}

#sbatch run-validate.sh ${STATE}
