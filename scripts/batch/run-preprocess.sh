#!/usr/bin/bash

#SBATCH -N 1
#SBATCH -A nssac_students
#SBATCH -p bii
#SBATCH --mem 375000
#SBATCH -t 120

cd /home/arr2vg/biocomplexity/loimos/loimos/scripts/preprocessing

module load python
STATE=md

#RAW_DIR=/project/bii_nssac/Loimos/dp-2.0/${STATE} 
RAW_DIR=/home/hsm2v/population_data/dp_2_4_0/${STATE}
CMD="time ./preprocess.py ${STATE} ${RAW_DIR} -o /scratch/${USER}/loimos/data/populations/${STATE}_2.4 --activity-suffix .(\d+)-of-(\d+)$  -aai location_assignment/daily/adult/${STATE}_adult_activity_location_assignment_day.csv -aci location_assignment/daily/child/${STATE}_child_activity_location_assignment_day.csv -ai /sfs/qumulo/qhome/hsm2v/us_lower_48_activity_locations_v20_sorted.csv"
echo "$CMD"
$CMD
