CHARMC = charmc

OBJS   = Main.o People.o Locations.o Location.o Event.o Defs.o
DEFS   = Defs.h loimos.def.h
DECLS  = loimos.decl.h
BIN    = loimos

# Build the executable (and implicitly charmrun) from the object files
$(BIN): $(OBJS) $(DECLS)
	$(CHARMC) -o $@ $(OBJS) -language charm++ \
	-module CkMulticast $(OPTS)

# Build .decl.h (and implicitly .def.h) files from the corresponding
# .ci files
$(DECLS): %.decl.h: %.ci
	$(CHARMC) $<

# Build .o files from the corresponding .C and .h files
$(OBJS): %.o: %.C %.h $(DECLS) $(DEFS)
	$(CHARMC) $(OPTS) $(INC) -o $@ $<

# These are utility rules that don't actually build anything
.PHONY:clean test
clean:
	rm -f *.decl.h *.def.h *.o charmrun $(BIN)

test: loimos
	./charmrun +p4 ./$(BIN) 110 20 15 6 15 ++local	
	./charmrun +p4 ./loimos 110 20 15 6 15 disease_model/H5N1.textproto ++local	
