#!/bin/bash
#SBATCH -p standard
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH -t 10
#SBATCH --account=biocomplexity

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 mvapich2/2.3.3 \
  openmpi/3.1.6 python/3.8.8

pip3 install pandas

${SCRIPTS_DIR}/combine-household-data.py -s ${STATE} \
  -i ${IN_DIR} -o ${IN_DIR}
