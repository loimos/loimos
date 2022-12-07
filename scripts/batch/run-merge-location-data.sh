#!/bin/bash
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --account=biocomplexity

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 \
  openmpi/3.1.6 python/3.8.8 #mvapich2/2.3.3 

echo ${SCRIPTS_DIR}/merge-location-data.py \
  ${OUT_DIR} \
  -p ${STATE}_gidi_person.csv \
  -a ${STATE}_activity_locations_final.csv \
  -r ${STATE}_residence_locations_final.csv \
  -v ${STATE}_activity_location_assignment_week_final.csv
echo ------------------------------------------------------------------------
time ${SCRIPTS_DIR}/merge-location-data.py \
  ${OUT_DIR} \
  -p ${STATE}_gidi_person.csv.gz \
  -a locations/${STATE}_activity_locations.csv \
  -r locations/${STATE}_residence_locations.csv \
  -v location_assignment_corrected/${STATE}_{adult,child}_activity_location_assignment_week.csv.gz \
  -n 16
