#!/bin/bash
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --account=biocomplexity

${SCRIPTS_DIR}/pop-prep.sh -s ${STATE} -i ${IN_DIR} -o ${OUT_DIR} -n
