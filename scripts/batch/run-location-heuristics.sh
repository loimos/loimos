#!/bin/bash
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -p bii
#SBATCH -t 30
#SBATCH --account=nssac_students

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 \
  openmpi/3.1.6 python/3.8.8 #mvapich2/2.3.3 

SCRIPTS_DIR="${HOME}/biocomplexity/loimos/loimos/scripts/preprocessing"
OUT_DIR="${HOME}/biocomplexity/loimos/loimos/data/populations/coc_2.0"
TASKS_PER_NODE=16

echo ${SCRIPTS_DIR}/location-heuristics.py ${OUT_DIR} -n ${TASKS_PER_NODE} -O
echo -------------------------------------------------------------------------
time ${SCRIPTS_DIR}/location-heuristics.py ${OUT_DIR} -n ${TASKS_PER_NODE} -O
