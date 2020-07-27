CHARMC      = /Users/bhatele/work/apps/charm/mpi-darwin-x86_64/bin/charmc 

OBJS		= Main.o People.o Locations.o
OBJS_DECL	= loimos.decl.h

loimos: $(OBJS) $(OBJS_DECL)
	$(CHARMC) -o loimos $(OBJS) -language charm++ \
	-module CkMulticast $(OPTS)

main.decl.h: main.ci
	$(CHARMC) main.ci

Main.o: Main.C Main.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o Main.o Main.C

People.o: People.C People.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o People.o People.C

Locations.o: Locations.C Locations.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o Locations.o Locations.C

clean:
	rm -f *.decl.h *.def.h *.o charmrun loimos
