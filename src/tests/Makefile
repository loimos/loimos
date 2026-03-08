# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

include ../Makefile.include

# Local Charm++ stubs for unit tests (avoid Charm runtime dependency)
TEST_STUBS = $(CURDIR)/stubs
CXXFLAGS += -I$(TEST_STUBS) -DUNIT_TEST -DENABLE_UNIT_TESTING

# GoogleTest paths - GTEST_HOME should point to the googletest repo root
# e.g., /path/to/loimos/src/googletest
GTEST_INCLUDE = $(GTEST_HOME)/googletest/include
GTEST_LIB     = $(GTEST_HOME)/build/lib

# Add GoogleTest include/lib paths
CXXFLAGS += -I$(GTEST_INCLUDE)
LIBS     += -L$(GTEST_LIB)

# Currently working unit test executables
# WORKING_TEST_BINS excludes InterventionModelTest until API updates land
WORKING_TEST_BINS = DiseaseModelTest ContactModelTest PartitionerTest
UNIT_TEST_BINS = DiseaseModelTest InterventionModelTest ContactModelTest PartitionerTest


# NOTE: charmrun prob not necessary for unit tests, same for CHARMC 

# Unit tests use Charm++ stubs and avoid .decl.h/.def.h dependencies

# look out for includes of charm.h, pupstl.h -- 
# 	probably not needed for Scenario 

# Required object files from parent directory for unit tests
DISEASE_MODEL_DEPS = ../DiseaseModel.o ../Event.o ../readers/AttributeTable.o \
                     ../readers/DataReader.o \
                     ../protobuf/disease.pb.o ../protobuf/distribution.pb.o

INTERVENTION_MODEL_DEPS = ../intervention_model/InterventionModel.o \
                          ../intervention_model/VaccinationIntervention.o \
                          ../DiseaseModel.o ../Event.o ../readers/AttributeTable.o \
                          ../readers/DataInterface.o ../readers/DataReader.o \
                          ../protobuf/interventions.pb.o ../protobuf/disease.pb.o \
                          ../protobuf/distribution.pb.o ../protobuf/data.pb.o

CONTACT_MODEL_DEPS = ../contact_model/ContactModel.o \
                     ../contact_model/MinMaxAlphaModel.o \
                     ../Location.o ../Event.o \
                     ../readers/AttributeTable.o ../readers/DataInterface.o

PARTITIONER_DEPS = ../Partitioner.o ../readers/Preprocess.o \
                   ../readers/DataReader.o ../readers/AttributeTable.o \
                   ../readers/Parse.o \
                   ../protobuf/data.pb.o ../protobuf/distribution.pb.o

# functional Charm++ stubs implementations for unit tests 
# (provides CkAbort, CmiAbort, )
# NOTE: only recommended for unit-test specific functionality; others should be stubbed via headers (trivial implementations)
CHARM_STUBS = CharmStubs.o

.PHONY: all clean test-all test-disease-model test-intervention-model test-contact-model test-partitioner

all: $(UNIT_TEST_BINS)

# Build unit test executables
DiseaseModelTest: DiseaseModelTest.o $(DISEASE_MODEL_DEPS) $(CHARM_STUBS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -lpthread $(LIBS)

InterventionModelTest: InterventionModelTest.o $(INTERVENTION_MODEL_DEPS) $(CHARM_STUBS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -lpthread $(LIBS)

ContactModelTest: ContactModelTest.o $(CONTACT_MODEL_DEPS) $(CHARM_STUBS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -lpthread $(LIBS)

PartitionerTest: PartitionerTest.o $(PARTITIONER_DEPS) $(CHARM_STUBS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -lpthread $(LIBS)

# Build Charm++ stubs
CharmStubs.o: CharmStubs.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


# Build test object files
DiseaseModelTest.o: DiseaseModelTest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

InterventionModelTest.o: InterventionModelTest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

ContactModelTest.o: ContactModelTest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

PartitionerTest.o: PartitionerTest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Run individual unit tests (from src/ directory for data file paths)
test-disease-model: DiseaseModelTest
	cd .. && ./tests/DiseaseModelTest

test-intervention-model: InterventionModelTest
	cd .. && ./tests/InterventionModelTest

test-contact-model: ContactModelTest
	cd .. && ./tests/ContactModelTest

test-partitioner: PartitionerTest
	cd .. && ./tests/PartitionerTest

# Run all working tests (excludes InterventionModelTest until API is updated)
test-all: $(WORKING_TEST_BINS)
	@echo "Running all unit tests from src/ directory..."
	@cd .. && for test in $(WORKING_TEST_BINS); do \
		echo ""; \
		echo "========================================"; \
		echo "Running $$test..."; \
		echo "========================================"; \
		./tests/$$test || exit 1; \
	done
	@echo ""
	@echo "All tests passed!"

# Run tests from current directory (requires data files at ../data/)
test-local: $(WORKING_TEST_BINS)
	@echo "Running tests locally..."
	@for test in $(WORKING_TEST_BINS); do ./$$test || exit 1; done

# Code coverage support
# Usage: make coverage CXXFLAGS="-g -O0 -std=c++11 --coverage"
.PHONY: coverage coverage-report coverage-clean

coverage: CXXFLAGS += --coverage -fprofile-arcs -ftest-coverage
coverage: clean test-all
	@echo "Coverage data collected. Run 'make coverage-report' to generate HTML."

coverage-report:
	@echo "Generating coverage report..."
	cd .. && lcov --capture --directory . --ignore-errors mismatch --ignore-errors unused --output-file coverage.info
	cd .. && lcov --remove coverage.info '/usr/*' '*/protobuf/*' --output-file coverage.info
	cd .. && genhtml coverage.info --output-directory coverage_html
	@echo "Coverage report generated in src/coverage_html/index.html"

coverage-clean:
	find .. -name "*.gcda" -delete
	find .. -name "*.gcno" -delete
	rm -f ../coverage.info
	rm -rf ../coverage_html

clean: coverage-clean
	rm -f *.o $(UNIT_TEST_BINS)
