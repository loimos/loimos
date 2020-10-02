CHARMC      = charmc

OBJS		= Main.o People.o Locations.o Location.o Event.o Defs.o
OBJS_DECL	= loimos.decl.h
BIN         = loimos

$(BIN): $(OBJS) $(OBJS_DECL)
	$(CHARMC) -o $@ $(OBJS) -language charm++ \
	-module CkMulticast $(OPTS)

# Build .decl.h (and implicitly .def.h) files from the corresponding
# .ci files
$(OBJS_DECL): %.decl.h: %.ci
	$(CHARMC) $<

# Build .o files from the corresponding .C and .h files
$(OBJS): %.o: %.C %.h $(OBJS_DECL)
	$(CHARMC) $(OPTS) $(INC) -o $@ $<

# These are utility rules that don't actually build anything
.PHONY:clean test
clean:
	rm -f *.decl.h *.def.h *.o charmrun $(BIN)

test: loimos
	./charmrun +p4 ./$(BIN) 110 20 15 6 15 ++local	
