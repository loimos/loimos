#!/bin/bash
#SBATCH -q normal
#SBATCH -N 4
#SBATCH --ntasks-per-node=37
#SBATCH --mem=375000
#SBATCH -p bii
#SBATCH -t 600
#SBATCH --account=nssac_students

module load python

SCRIPTS_DIR="${HOME}/biocomplexity/loimos/loimos/scripts/preprocessing"
OUT_DIR="/scratch/${USER}/loimos/data/populations/$1"
TASKS_PER_NODE=37
NODES=4

echo ${SCRIPTS_DIR}/location_heuristics.py ${OUT_DIR} -n $((${NODES}*${TASKS_PER_NODE}))
echo -------------------------------------------------------------------------
time ${SCRIPTS_DIR}/location_heuristics.py ${OUT_DIR} -n $((${NODES}*${TASKS_PER_NODE}))
