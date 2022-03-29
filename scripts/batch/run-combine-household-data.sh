#!/bin/bash
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --account=biocomplexity

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 mvapich2/2.3.3 \
  openmpi/3.1.6 python/3.8.8

#pip3 install pandas numpy

${SCRIPTS_DIR}/combine-household-data.py -s ${STATE} \
  -i ${OUT_DIR} -o ${OUT_DIR}
