#!/bin/bash
#SBATCH -p standard
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH -t 10
#SBATCH --account=biocomplexity

export PROJECT_ROOT=/home/arr2vg/biocomplexity/loimos
module load python

$PROJECT_ROOT/loimos/scripts/pipeline.py $PROJECT_ROOT/loimos/data/populations/wy/ -a wy_activity_locations_final.csv -r wy_residence_locations_final.csv -v wy_gidi_visits.csv -p wy_gidi_person.csv
