This testing suite uses GoogleTest and local Charm++ stubs (no Charm++ runtime required).

To run the tests you have to install 
https://github.com/google/googletest/tree/master/googletest
Download from source and build
then set the GTEST_HOME to be your root installation location.

To run the tests, cd to src/tests and run
  make clean
  ENABLE_UNIT_TESTING=1 make test
