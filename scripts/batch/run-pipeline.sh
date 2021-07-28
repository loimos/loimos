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

  echo Running ${SCRIPT}
  sbatch --wait ${BATCH_DIR}/${BATCH_SCRIPT}
  
  EXIT_CODE=$?
  if [ ${EXIT_CODE} -eq 0 ]; then
    echo Done with ${SCRIPT}
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

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 mvapich2/2.3.3 \
  openmpi/3.1.6 python/3.8.8

# Create a directory to hold all the processed data, if none already exists
if [ ! -d ${OUT_DIR} ]; then
 mkdir ${OUT_DIR}
fi

echo Processing data for ${STATE}

run_script pop-prep.sh
run_script combine-household-data.sh

# From here on we no longer need any of all the raw data, so move all the
# processed data (which will be at the top level of the input dir) over to
# the otput dir
mv ${IN_DIR}/*.csv ${OUT_DIR}

run_script merge-location-data.py
run_script location-heuristics.py
