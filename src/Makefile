include Makefile.include

OBJS   = Main.o DiseaseModel.o People.o Locations.o Location.o Defs.o \
         Event.o Interaction.o ContactModel.o data/Preprocess.o \
         disease_model/disease.pb.o disease_model/distribution.pb.o
DEFS   = Defs.h loimos.def.h
DECLS  = loimos.decl.h
BIN    = loimos

SUBDIRS := $(wildcard */.)

.PHONY:all
all: all-sub $(BIN)

# Build the executable (and implicitly charmrun) from the object files
$(BIN): $(OBJS) $(DECLS)
	$(CHARMC) -o $@ $(OBJS) -language charm++ -module CkMulticast \
	-lpthread -L$(PROTOBUF_HOME)/lib -lprotobuf

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
		cd $$d; $(MAKE) all; \
	done

.PHONY:clean
clean:
	rm -f *.o charmrun $(BIN) *.decl.h *.def.h 
	@for d in $(SUBDIRS); do \
		cd $$d; $(MAKE) clean; \
	done

.PHONY:test test_large
test: loimos
	./charmrun +p4 ./loimos 40 16 1 4 7 ../scenarios/h5n1/ ++local
# See loimos folder for this scneario.
test_large: loimos
	./charmrun +p16 ./loimos 41120 9166 200 100 7 ../scenarios/h5n1_large/ ++local