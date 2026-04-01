#!/bin/bash
#SBATCH -N 1
#SBATCH --ntasks 4
./charmrun +p4 ./loimos 1 100 100 50 50 5 5 5 32 30 test-syn.csv ../data/disease_models/covid19_onepath.textproto --mca opal_warn_on_missing_libcuda 0
