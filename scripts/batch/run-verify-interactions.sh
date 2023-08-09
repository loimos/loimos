#!/usr/bin/bash

#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -A nssac_students
#SBATCH -p bii
#SBATCH -t 60

cd /home/arr2vg/biocomplexity/loimos/loimos/scripts/validation

module load python

./verify_interactions.py ../../data/populations/md_2.0
