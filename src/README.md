## Command Line Arguments:

Loimos can be run with either synthetic or real data. For synthetic data,
run with the command:

```./loimos 1 PGW PGH LGW LGH NV LPGW LPGH NPP ND O D```

Where
- `PGW` is the people grid width
- `PGH` is the people grid height
- `LGW` is the location grid width
- `LGH` is the location grid height
- `NV` is the average number of visits
- `LPGW` is the location parition grid width
- `LPGH` is the location parition grid height
- `NPP` is the number of people paritions
- `ND` is the number of days to simulate
- `O` is the path to the output file
- `D` is the path to the disease model 

For real data, run with the command:

```./loimos 0 NP NL NPP NLP ND NDV O D S [-m] [-i I]```

Where
- `NP` is the number of people
- `NL` is the number of locations
- `NPP` is the number of people paritions
- `NLP` is the number of location paritions
- `ND` is the number of days to simulate
- `NVD` is the number of days of visit data in the scenario (or equivalently, the number
  how often to repeat people's visit schedules)
- `O` is the path to the output file
- `D` is the path to the disease model
- `S` is the path to the directory containing the population data for the
  scenario
- `-m` or `--min-max-alpha` is an optional flag which indicates that the
  min-max-alpha contact mdoel should be used
- `-i` is an optional flag used when specifying an intervention. `I` should
  be the path to a textproto file specifying the intervention to be used
