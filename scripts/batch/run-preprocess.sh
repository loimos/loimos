#!/usr/bin/bash

#SBATCH -N 1
#SBATCH -A nssac_students
#SBATCH -p bii
#SBATCH --mem 375000
#SBATCH -t 120


module load python
STATE=$1

#RAW_DIR=/project/bii_nssac/Loimos/dp-2.0/${STATE} 
#RAW_DIR=/home/hsm2v/population_data/dp_2_4_0/${STATE}
RAW_DIR=/project/biocomplexity/us_population_pipeline/population_data/usa_840/2017/ver_2_4_0/${STATE}
CMD="time ../preprocessing/preprocess.py ${STATE} ${RAW_DIR} -o /scratch/${USER}/loimos/data/populations/${STATE}_2.4 --activity-suffix .(\d+)-of-(\d+)$ -ai /sfs/qumulo/qhome/hsm2v/us_lower_48_activity_locations_v20_sorted.csv"
echo "$CMD"
$CMD
