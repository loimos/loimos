#!/usr/bin/bash

#SBATCH -N 1
#SBATCH -A nssac_students
#SBATCH -p bii
#SBATCH --mem 375000
#SBATCH -t 180

cd /home/arr2vg/loimos/loimos/scripts/preprocessing

module load python
DATASET=${1}
if [ ! -z ${2} ]; then
  NEW_DATASET=${2}
else
  NEW_DATASET=${DATASET}_partitioned
fi

time ./partition.py /scratch/$USER/loimos/data/populations/${DATASET} 13824 -o /scratch/$USER/loimos/data/populations/${NEW_DATASET} -nv 10000000
