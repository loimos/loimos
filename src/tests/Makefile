# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

include ../Makefile.include

# Unit test executables
UNIT_TEST_BINS = DiseaseModelTest InterventionModelTest ContactModelTest PartitionerTest

# Required object files from parent directory for unit tests
DISEASE_MODEL_DEPS = ../DiseaseModel.o ../Event.o ../readers/AttributeTable.o \
                     ../readers/Data.o \
                     ../protobuf/disease.pb.o ../protobuf/distribution.pb.o

INTERVENTION_MODEL_DEPS = ../intervention_model/InterventionModel.o \
                          ../intervention_model/VaccinationIntervention.o \
                          ../DiseaseModel.o ../Event.o ../readers/AttributeTable.o \
                          ../readers/Data.o ../readers/DataInterface.o \
                          ../protobuf/interventions.pb.o ../protobuf/disease.pb.o \
                          ../protobuf/distribution.pb.o

CONTACT_MODEL_DEPS = ../contact_model/ContactModel.o \
                     ../contact_model/MinMaxAlphaModel.o \
                     ../Location.o ../Event.o \
                     ../readers/AttributeTable.o ../readers/DataInterface.o \
                     ../readers/Data.o

PARTITIONER_DEPS = ../Partitioner.o ../readers/Preprocess.o \
                   ../readers/DataReader.o ../readers/AttributeTable.o \
                   ../readers/Data.o ../readers/Parse.o \
                   ../protobuf/data.pb.o

.PHONY:all
all: $(UNIT_TEST_BINS)

# Build unit test executables
DiseaseModelTest: DiseaseModelTest.o $(DISEASE_MODEL_DEPS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -lpthread $(LIBS)

InterventionModelTest: InterventionModelTest.o $(INTERVENTION_MODEL_DEPS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -lpthread $(LIBS)

ContactModelTest: ContactModelTest.o $(CONTACT_MODEL_DEPS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -lpthread $(LIBS)

PartitionerTest: PartitionerTest.o $(PARTITIONER_DEPS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -lpthread $(LIBS)

# Build test object files
DiseaseModelTest.o: DiseaseModelTest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

InterventionModelTest.o: InterventionModelTest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

ContactModelTest.o: ContactModelTest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

PartitionerTest.o: PartitionerTest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Run individual unit tests
.PHONY:test-disease-model test-intervention-model test-contact-model test-partitioner test-all

# TODO: install gtest on zaratan

test-disease-model: DiseaseModelTest
	./DiseaseModelTest

test-intervention-model: InterventionModelTest
	./InterventionModelTest

test-contact-model: ContactModelTest
	./ContactModelTest

test-partitioner: PartitionerTest
	./PartitionerTest

test-all: $(UNIT_TEST_BINS)
	@echo "Running all unit tests..."
	@for test in $(UNIT_TEST_BINS); do \
		echo ""; \
		echo "========================================"; \
		echo "Running $$test..."; \
		echo "========================================"; \
		./$$test || exit 1; \
	done
	@echo ""
	@echo "All tests passed!"

# Build unit test executables
DiseaseModelTest: DiseaseModelTest.o $(DISEASE_MODEL_DEPS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -lpthread $(LIBS)

InterventionModelTest: InterventionModelTest.o $(INTERVENTION_MODEL_DEPS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -lpthread $(LIBS)

# Build test object files
DiseaseModelTest.o: DiseaseModelTest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

InterventionModelTest.o: InterventionModelTest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Run individual unit tests
.PHONY:test-disease-model test-intervention-model test-all

test-disease-model: DiseaseModelTest
	./DiseaseModelTest

test-intervention-model: InterventionModelTest
	./InterventionModelTest

test-all: $(UNIT_TEST_BINS)
	./DiseaseModelTest
	./InterventionModelTest

.PHONY:clean
clean:
	rm -f *.o $(UNIT_TEST_BINS)
