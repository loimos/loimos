#!/usr/bin/bash

#SBATCH -N 1
#SBATCH -A nssac_students
#SBATCH -p bii
#SBATCH --mem 375000
#SBATCH -t 180

cd /home/arr2vg/loimos/loimos/scripts/preprocessing

module load python
DATASET=${1}

time ./partition.py /scratch/$USER/loimos/data/populations/${DATASET} 13824 -o /scratch/$USER/loimos/data/populations/${DATASET}_partitioned -nv 10000000
