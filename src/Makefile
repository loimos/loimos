# Copyright 2020-2023 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

include Makefile.include

OBJS   = Main.o DiseaseModel.o People.o Locations.o Location.o Person.o  \
         Defs.o Event.o readers/Preprocess.o \
				 readers/DataInterface.o \
				 contact_model/MinMaxAlphaModel.o contact_model/ContactModel.o \
				 intervention_model/AttributeTable.o \
				 intervention_model/VaccinationIntervention.o \
         protobuf/disease.pb.o protobuf/distribution.pb.o \
		     protobuf/data.pb.o protobuf/interventions.pb.o
DEFS   = loimos.def.h Defs.h readers/DataReader.h
DECLS  = loimos.decl.h

BIN   := loimos

ifdef ENABLE_SMP
BIN   :=$(BIN)-smp
endif
ifdef ENABLE_TRACING
BIN   :=$(BIN)-prj
endif
ifdef ENABLE_LB
BIN   :=$(BIN)-lb
endif

# Set the ENABLE_UNIT_TESTING environment variable to compile for unit testing
ifdef ENABLE_UNIT_TESTING
UNIT_TEST_OBJS = tests/DiseaseModelTest.o
endif

# Set the USE_HYPERCOMM environment variable to compile for Charm++'s in-built
# dynamic load balancing
ifdef USE_HYPERCOMM
OBJS += Aggregator.o
endif

# Set the ENABLE_LB environment variable to compile for Charm++'s in-built
# dynamic load balancing

# set the ENABLE_TRACING environment variable to compile for projections
ifdef ENABLE_TRACING
PROJECTION_FLAGS = -tracemode projections
else
PROJECTION_FLAGS =
endif

SUBDIRS = protobuf tests

.PHONY:all
all: all-sub $(BIN)

# Build the executable (and implicitly charmrun) from the object files
$(BIN): $(OBJS) $(UNIT_TEST_OBJS) $(DECLS)
	$(CHARMC) -o $@ $(OBJS) $(UNIT_TEST_OBJS) $(PROJECTION_FLAGS) \
		-language charm++ -module CkMulticast $(LIBS)

# Build .decl.h (and implicitly .def.h) files from the corresponding
# .ci files
$(DECLS): %.decl.h: %.ci
	$(CHARMC) $< $(OPTS)

# Build .o files from the corresponding .cpp and .h files
$(OBJS): %.o: %.cpp %.h $(DECLS) $(DEFS)
	$(CHARMC) $(CXXFLAGS) -c -o $@ $<

# Build .o files from just the .cpp files for unit tests
# (they don't have .h files)
$(UNIT_TEST_OBJS): %.o: %.cpp $(DECLS) $(DEFS)
	$(CHARMC) $(CXXFLAGS) -c -o $@ $<

# Subdirs will build the protobuf object files
.PHONY: all-sub
all-sub:
	@for d in $(SUBDIRS); do \
		$(MAKE) -C $$d all; \
	done

.PHONY:clean
clean:
	rm -f *.o charmrun $(BIN) $(OBJS) *.decl.h *.def.h
	@for d in $(SUBDIRS); do \
		$(MAKE) -C $$d clean; \
	done

# All test names start with "test-"
TEST_NAMES= -small -syn -large -validation -intervention -intervention-syn
TESTS=$(subst -,test-,$(TEST_NAMES))

# Run all tests with test rule
.PHONY:test $(TESTS)
test: $(TESTS)

test-style:
	cpplint --extensions=C,h,ci --root=loimos/src --recursive . &> linter-errors.log

test-small: all
	./charmrun +p4 ./loimos 0 100 735 2 2 30 7 test-small.csv ../data/disease_models/covid19.textproto ../data/populations/synthetic_small_city/ --min-max-alpha ++local

test-syn: all
	./charmrun +p4 ./loimos 1 100 100 50 50 5 5 5 32 30 test-syn.csv ../data/disease_models/covid19_onepath.textproto ++local

test-large: all
	if [ -f ../data/populations/coc/visits.csv ]; then \
		./charmrun +p4 ./loimos 0 41119 19203 60 40 30 7 test-large.csv ../data/disease_models/covid19.textproto ../data/populations/coc/ --min-max-alpha ++local;\
	else\
		echo CoC data not available;\
	fi

test-validation: all
	./charmrun +p4 ./loimos 0 25000 5001 60 40 30 7 test-validation.csv ../data/disease_models/validation_model.textproto ../data/populations/validation_set/ ++local

test-intervention: all
	if [ -f ../data/populations/coc/visits.csv ]; then \
		./charmrun +p4 ./loimos 0 41119 19203 60 40 180 7 test-intervention.csv ../data/disease_models/covid19_onepath.textproto ../data/populations/coc/ -i ../data/interventions/isolation_and_school_closures.textproto ++local\
	else\
		echo CoC data not available;\
	fi

test-intervention-syn: all
	./charmrun +p4 ./loimos 1 100 100 50 50 5 5 5 32 30 test-intervention-syn.csv ../data/disease_models/covid19_onepath.textproto -i ../data/interventions/vaccination.textproto ++local

clean-cache:
	rm ../data/populations/coc/*.cache ../data/populations/synthetic_small_city/*.cache
