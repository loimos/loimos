#!/bin/bash
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --tasks-per-node 37
#SBATCH -p bii
#SBATCH -t 60
#SBATCH --account=nssac_students

module load python

SCRIPTS_DIR="${HOME}/biocomplexity/loimos/loimos/scripts/preprocessing"
#OUT_DIR="${HOME}/biocomplexity/loimos/loimos/data/populations/coc_2.0"
OUT_DIR="/scratch/${USER}/loimos/data/populations/md_2.4"
TASKS_PER_NODE=37

echo ${SCRIPTS_DIR}/location_heuristics.py ${OUT_DIR} -n ${TASKS_PER_NODE} #-O
echo -------------------------------------------------------------------------
time ${SCRIPTS_DIR}/location_heuristics.py ${OUT_DIR} -n ${TASKS_PER_NODE} #-O
