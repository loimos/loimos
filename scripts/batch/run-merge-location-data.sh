#!/bin/bash
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --account=biocomplexity

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 mvapich2/2.3.3 \
  openmpi/3.1.6 python/3.8.8

${SCRIPTS_DIR}/merge-location-data.py \
  ${OUT_DIR} \
  -p ${STATE}_gidi_person.csv \
  -a ${STATE}_activity_locations_final.csv \
  -r ${STATE}_residence_locations_final.csv \
  -v ${STATE}_activity_location_assignment_week_final.csv
