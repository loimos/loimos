#!/usr/bin/env python3

import numpy as np
import pandas as pd
import sys
import os

out_filename = sys.argv[1]
#store = pd.HDFStore(out_filename)

for in_filename in sys.argv[2:]:
    in_prefix, _ = os.path.splitext(in_filename)
    in_basename = os.path.basename(in_prefix)
    #out_filename = f'{in_prefix}.h5'
    #out_filename = 'md_synthetic.h5.gz'

    df = pd.read_csv(in_filename)
    #store.append(in_basename, df)

    df.to_hdf(out_filename, in_basename, append=True, complevel=9)#, format='table')
    #df.to_hdf(out_filename, in_basename, mode='w')#, format='table')

#store.close()
