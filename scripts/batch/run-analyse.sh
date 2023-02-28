#!/bin/bash
#SBATCH -q normal
#SBATCH -p bii
#SBATCH -t 30
#SBATCH -N 1
#SBATCH --account=nssac_students
#SBATCH --exclusive

IN_DIR=../../data/populations/md_partitioned

module load python

../analysis/analyze_visit_distribution.py ${IN_DIR} .
