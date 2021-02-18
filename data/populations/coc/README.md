## Creating a larger City of Charlottesville, VA dataset

The (large) datafiles are intentionally left out of the repository. You need to
run the following script in the scripts/ folder.

### How to download the data. 

1. Download the raw datafiles from the Loimos Team Drive/nssac_population_data/coc. 
2. Preprocess the files by running the python script in the scripts/ folder
python coc_pipeline.py ../data/populations/coc
3. Enter the confirmation.
4. Run make test_large to make sure that it is working as intended.
