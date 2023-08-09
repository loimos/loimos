#!/usr/bin/bash

#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -A nssac_students
#SBATCH -p bii
#SBATCH -t 60

cd /home/arr2vg/biocomplexity/loimos/loimos/scripts/preprocessing

module load python
STATE=va

time ./preprocess.py ${STATE} /project/bii_nssac/Loimos/dp-2.0/${STATE} -o ../../data/populations/${STATE}_2.0 -f
