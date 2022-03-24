#!/bin/bash
#SBATCH -q normal
#SBATCH -p largemem
#SBATCH -t 10
#SBATCH -N 1
#SBATCH --account=biocomplexity

STATE=${1}

INPUT_DIR=../../data/populations/${STATE}

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 mvapich2/2.3.3 \
  openmpi/3.1.6 python/3.8.8

time ../validate.py  ${INPUT_DIR}

#for dir in ${INPUT_DIR}* ; do
#  echo Testing ${dir}
#  time ../validate.py  ${dir}
#done
