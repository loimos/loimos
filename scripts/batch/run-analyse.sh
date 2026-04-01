#!/bin/bash
#SBATCH -p bii
#SBATCH -t 5
#SBATCH -N 1
#SBATCH --ntasks-per-node 1
#SBATCH --account nssac_students
#SBATCH --mem-per-cpu 300000

DATASET=md_2.0
IN_DIR=../../data/populations/${DATASET}

module load python

mkdir -p ${DATASET}

../analysis/analyze_visit_distribution.py ${IN_DIR} ${DATASET}
