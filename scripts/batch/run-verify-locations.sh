#!/usr/bin/bash

#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -A nssac_students
#SBATCH -p bii
#SBATCH -t 15

cd /home/arr2vg/biocomplexity/loimos/loimos/scripts/validation

module load python

STATE=va

./verify_locations.py ../../data/populations/${STATE}_2.0 /project/bii_nssac/Loimos/dp-2.0/${STATE}/lid-measures.csv
