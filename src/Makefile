# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

include Makefile.include

OBJS   = Main.o DiseaseModel.o People.o Locations.o Location.o Person.o Defs.o \
         Event.o Interaction.o ContactModel.o readers/Preprocess.o \
         disease_model/disease.pb.o disease_model/distribution.pb.o \
		 readers/data.pb.o readers/DataLoader.o
DEFS   = Defs.h loimos.def.h
DECLS  = loimos.decl.h
BIN    = loimos

SUBDIRS := disease_model readers

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

# All test names start with "test-"
TEST_NAMES= -small -syn -large
TESTS=$(subst -,test-,$(TEST_NAMES))

# Run all tests with test rule
.PHONY:test $(TESTS)
test: $(TESTS)

test-small: all
	./charmrun +p4 /usr/local/bin/valgrind --tool=memcheck --track-origins=yes --log-file="valgrind.out" -s ./loimos 100 735 2 2 7 ../data/disease_models/covid19.textproto ../data/populations/synthetic_small_city/ ++local

test-syn: all
	./charmrun +p4 ./loimos 1000 100 4 4 7 ../data/disease_models/covid19.textproto ++local

test-large: all
	./charmrun +p4 ./loimos 41119 19203 60 40 7 ../data/disease_models/covid19.textproto ../data/populations/coc/ ++local

clean-cache:
	rm ../data/populations/coc/*.cache ../data/populations/synthetic_small_city/*.cache


# ./charmrun +p4 ./loimos 200 2 4 2 30 ../data/disease_models/test_highly_contagious.textproto ../data/populations/validation_set/ ++local