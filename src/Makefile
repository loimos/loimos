# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

include Makefile.include

OBJS   = Main.o DiseaseModel.o People.o Locations.o Location.o Person.o Defs.o \
         Event.o Interaction.o readers/Preprocess.o \
         disease_model/disease.pb.o disease_model/distribution.pb.o \
		     readers/data.pb.o contact_model/ContactModel.o \
				 contact_model/MinMaxAlphaModel.o
DEFS   = Defs.h loimos.def.h readers/DataReader.h
DECLS  = loimos.decl.h
BIN    = loimos

SUBDIRS := disease_model contact_model readers
UNIT_TESTS = tests/DiseaseModelTest.o

.PHONY:all
all: all-sub $(BIN)

# Build the executable (and implicitly charmrun) from the object files
$(BIN): $(OBJS) $(DECLS)
	$(CHARMC) -o $@ $(OBJS) -language charm++ -module CkMulticast \
	-module NDMeshStreamer -lpthread -L$(PROTOBUF_HOME)/lib -lprotobuf

# Build .decl.h (and implicitly .def.h) files from the corresponding
# .ci files
$(DECLS): %.decl.h: %.ci
	$(CHARMC) $<

# Build .o files from the corresponding .C and .h files
$(OBJS): %.o: %.C %.h $(DECLS) $(DEFS)
	$(CHARMC) $(CXXFLAGS) -o $@ $<

# Subdirs will build the protobuf object files
.PHONY: all-sub
all-sub:
	@for d in $(SUBDIRS); do \
		cd $$d; \
		$(MAKE) all; \
		cd ..; \
	done

.PHONY:clean
clean:
	rm -f *.o charmrun $(BIN) *.decl.h *.def.h 
	@for d in $(SUBDIRS); do \
		cd $$d; \
		$(MAKE) clean; \
		cd ..; \
	done

# Unit testing.
disease_model_test: $(DECLS) $(DEFS) $(OBJS)
	$(CHARMC) $(CXXFLAGS) -o tests/DiseaseModelTest.o tests/DiseaseModelTest.C -I/Users/iancostello/lib/googletest-release-1.10.0/googletest/include -L/Users/iancostello/lib/googletest-release-1.10.0/lib -lgtest

unit_tests: all-sub $(OBJS) $(DECLS)
	$(CHARMC) -o $@ $(OBJS) $(UNIT_TESTS) -language charm++ -module CkMulticast \
	-module NDMeshStreamer -lpthread -L$(PROTOBUF_HOME)/lib -lprotobuf \ 
	-I$(GTEST_HOME)/include -L$(GTEST_HOME)/lib -lgtest 
	# \ 
	# -DENABLE_UNIT_TESTING

# All test names start with "test-"
TEST_NAMES= -small -syn -large -validation
TESTS=$(subst -,test-,$(TEST_NAMES))

# Run all tests with test rule
.PHONY:test $(TESTS)
test: $(TESTS)

test-small: all
	./charmrun +p4 ./loimos 0 100 735 2 2 7 test-small.csv ../data/disease_models/covid19.textproto ../data/populations/synthetic_small_city/ ++local

test-syn: all
	./charmrun +p4 ./loimos 1 100 100 50 50 5 5 5 32 30 test-syn.csv ../data/disease_models/covid19_onepath.textproto ++local

test-large: all
	./charmrun +p4 ./loimos 0 41119 19203 60 40 14 test-large.csv ../data/disease_models/covid19.textproto ../data/populations/coc/ --min-max-alpha ++local

test-validation: all
	./charmrun +p4 ./loimos 0 25000 5001 60 40 30 test-validation.csv ../data/disease_models/validation_model.textproto ../data/populations/validation_set/ ++local

clean-cache:
	rm ../data/populations/coc/*.cache ../data/populations/synthetic_small_city/*.cache
