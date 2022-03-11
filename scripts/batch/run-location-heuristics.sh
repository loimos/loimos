#!/bin/bash
#SBATCH -q normal
#SBATCH -N 4
#SBATCH --account=biocomplexity

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 mvapich2/2.3.3 \
  openmpi/3.1.6 python/3.8.8

${SCRIPTS_DIR}/location-heuristics.py ${OUT_DIR} -n $((4 * ${TASKS_PER_NODE})) -O
