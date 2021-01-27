include Makefile.include

OBJS   = Main.o DiseaseModel.o People.o Locations.o Location.o Defs.o \
         Event.o Interaction.o ContactModel.o \
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
		cd $$d; $(MAKE) all; \
	done

.PHONY:clean
clean:
	rm -f *.o charmrun $(BIN) *.decl.h *.def.h 
	@for d in $(SUBDIRS); do \
		cd $$d; $(MAKE) clean; \
	done

.PHONY:test
test: all
	./charmrun +p4 ./loimos 110 20 15 6 15 disease_model/H5N1.textproto ++local	
