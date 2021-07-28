#!/bin/bash
#SBATCH -p standard
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH -t 10
#SBATCH --account=biocomplexity

${SCRIPTS_DIR}/pop-prep.sh -s ${STATE} -i ${IN_DIR} -o ${IN_DIR}
