# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

include Makefile.include

OBJS   = Main.o DiseaseModel.o People.o Locations.o Location.o Person.o \
         Defs.o Event.o Interaction.o readers/Preprocess.o \
         disease_model/disease.pb.o disease_model/distribution.pb.o \
		     readers/data.pb.o contact_model/ContactModel.o \
				 contact_model/MinMaxAlphaModel.o
DEFS   = Defs.h loimos.def.h readers/DataReader.h
DECLS  = loimos.decl.h
BIN    = loimos

# Set the ENABLE_UNIT_TESTING environment variable to compile for unit testing
ifdef ENABLE_UNIT_TESTING
UNIT_TEST_OBJS = tests/DiseaseModelTest.o
endif

ifdef ENABLE_PROJECTIONS
PROJECTION_FLAGS = -tracemode projections
endif

SUBDIRS = disease_model contact_model readers tests

.PHONY:all
all: all-sub $(BIN)

# Build the executable (and implicitly charmrun) from the object files
$(BIN): $(OBJS) $(UNIT_TEST_OBJS) $(DECLS)
	$(CHARMC) -o $@ $(OBJS) $(UNIT_TEST_OBJS) $(ENABLE_PROJECTIONS) -language charm++ \
	-module CkMulticast -module NDMeshStreamer $(LIBS)


# Build .decl.h (and implicitly .def.h) files from the corresponding
# .ci files
$(DECLS): %.decl.h: %.ci
	$(CHARMC) $<

# Build .o files from the corresponding .C and .h files
$(OBJS): %.o: %.C %.h $(DECLS) $(DEFS)
	$(CHARMC) $(CXXFLAGS) -o $@ $<

# Build .o files from just the .C files for unit tests 
# (they don't have .h files)
$(UNIT_TEST_OBJS): %.o: %.C $(DECLS) $(DEFS)
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
