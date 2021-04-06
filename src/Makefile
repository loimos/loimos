# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

include Makefile.include

OBJS   = Main.o DiseaseModel.o People.o Locations.o Location.o Person.o \
         Defs.o Event.o Interaction.o readers/Preprocess.o Aggregator.o \
         disease_model/disease.pb.o disease_model/distribution.pb.o \
		     readers/data.pb.o contact_model/ContactModel.o \
				 contact_model/MinMaxAlphaModel.o readers/interventions.pb.o
DEFS   = Defs.h loimos.def.h readers/DataReader.h
DECLS  = loimos.decl.h
BIN    = loimos

# Set the ENABLE_UNIT_TESTING environment variable to compile for unit testing
ifdef ENABLE_UNIT_TESTING
UNIT_TEST_OBJS = tests/DiseaseModelTest.o
endif

# Set the ENABLE_LB environment variable to compile for Charm++'s in-built
# dynamic load balancing

# set the USE_PROJECTIONS environment variable to compile for projections
ifdef USE_PROJECTIONS
PROJECTION_FLAGS = -tracemode projections
else
PROJECTION_FLAGS = 
endif

SUBDIRS = disease_model contact_model readers tests

.PHONY:all
all: all-sub $(BIN)

# Build the executable (and implicitly charmrun) from the object files
$(BIN): $(OBJS) $(UNIT_TEST_OBJS) $(DECLS)
	$(CHARMC) -o $@ $(OBJS) $(UNIT_TEST_OBJS) $(PROJECTION_FLAGS) -language charm++ \
	-module CkMulticast $(LIBS)

# Build .decl.h (and implicitly .def.h) files from the corresponding
# .ci files
$(DECLS): %.decl.h: %.ci
	$(CHARMC) $<

# Build .o files from the corresponding .C and .h files
$(OBJS): %.o: %.C %.h $(DECLS) $(DEFS)
	$(CHARMC) $(CXXFLAGS) -c -o $@ $<

# Build .o files from just the .C files for unit tests 
# (they don't have .h files)
$(UNIT_TEST_OBJS): %.o: %.C $(DECLS) $(DEFS)
	$(CHARMC) $(CXXFLAGS) -c -o $@ $<

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

# All test names start with "test-"
TEST_NAMES= -small -syn -large -validation -intervention -intervention-syn
TESTS=$(subst -,test-,$(TEST_NAMES))

# Run all tests with test rule
.PHONY:test $(TESTS)
test: $(TESTS)

test-small: all
	./charmrun +p4 ./loimos 0 100 735 2 2 30 7 test-small.csv ../data/disease_models/covid19.textproto ../data/populations/synthetic_small_city/ ++local

test-syn: all
	./charmrun +p4 ./loimos 1 100 100 50 50 5 5 5 32 30 test-syn.csv ../data/disease_models/covid19_onepath.textproto ++local

test-large: all
	./charmrun +p4 ./loimos 0 41119 19203 60 40 30 7 test-large.csv ../data/disease_models/covid19.textproto ../data/populations/coc/ --min-max-alpha ++local

test-validation: all
	./charmrun +p4 ./loimos 0 25000 5001 60 40 30 7 test-validation.csv ../data/disease_models/validation_model.textproto ../data/populations/validation_set/ ++local

test-intervention: all
	./charmrun +p4 ./loimos 0 41119 19203 60 40 30 7 test-intervention.csv ../data/disease_models/covid19_onepath.textproto ../data/populations/coc/ -i ../data/interventions/isolation_and_school_closures.textproto ++local

test-intervention-syn: all
	./charmrun +p4 ./loimos 1 100 100 50 50 5 5 5 32 30 test-intervention-syn.csv ../data/disease_models/covid19_onepath.textproto -i ../data/interventions/only_isolation.text_proto ++local


clean-cache:
	rm ../data/populations/coc/*.cache ../data/populations/synthetic_small_city/*.cache
