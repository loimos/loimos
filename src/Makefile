CHARMC = charmc

OBJS   = Main.o DiseaseModel.o People.o Locations.o Defs.o \
         Location.o Event.o \
	     disease_model/disease.pb.o disease_model/distribution.pb.o
DEFS   = Defs.h loimos.def.h
DECLS  = loimos.decl.h
BIN    = loimos

.PHONY:all
all: all-sub $(BIN)

# Build the executable (and implicitly charmrun) from the object files
$(BIN): $(OBJS) $(DECLS)
	$(CHARMC) -o $@ $(OBJS) -language charm++ \
	-module CkMulticast $(OPTS) \
	`pkg-config --cflags --libs protobuf`

# Build .decl.h (and implicitly .def.h) files from the corresponding
# .ci files
$(DECLS): %.decl.h: %.ci
	$(CHARMC) $<

# Build .o files from the corresponding .C and .h files
$(OBJS): %.o: %.cc %.h $(DECLS) $(DEFS)
	$(CHARMC) $(OPTS) $(INC) -o $@ $<

# Subdirs will build the protobuf object files
.PHONY: all-sub
all-sub:
	@for d in $(SUBDIRS); do \
	cd $d; $(MAKE) $(MFLAGS) all; done

.PHONY:clean test
clean:
	@for d in $(SUBDIRS); do \
	cd $d; $(MAKE) $(MFLAGS) clean; done
	rm -f *.decl.h *.def.h *.o charmrun $(BIN)

test: loimos
	./charmrun +p4 ./loimos 110 20 15 6 15 disease_model/H5N1.textproto ++local	
