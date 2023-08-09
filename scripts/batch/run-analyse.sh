#!/bin/bash
#SBATCH -q normal
#SBATCH -p bii
#SBATCH -t 30
#SBATCH -N 1
#SBATCH --account=nssac_students
#SBATCH --exclusive

DATASET=md_partitioned
IN_DIR=../../data/populations/${DATASET}

module load python

../analysis/analyze_visit_distribution.py ${IN_DIR} ${DATASET}
