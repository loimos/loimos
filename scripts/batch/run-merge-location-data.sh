#!/bin/bash
#SBATCH -q normal
#SBATCH -N 2
#SBATCH --account=biocomplexity

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 mvapich2/2.3.3 \
  openmpi/3.1.6 python/3.8.8

time ${SCRIPTS_DIR}/merge-location-data.py \
  ${OUT_DIR} \
  -p ${STATE}_gidi_person.csv.gz \
  -a locations/${STATE}_activity_locations.csv \
  -r locations/${STATE}_residence_locations.csv \
  -v location_assignment_corrected/${STATE}_{adult,child}_activity_location_assignment_week.csv.gz \
  -n 80
