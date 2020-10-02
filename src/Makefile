.PHONY: diseaseModelProto
CXX=g++
CHARMC      = charmc

PROTOBUF_DIR=/usr/local/bin/vcpkg/installed/x64-osx
CXXFLAGS=-std=c++11 -Wall -I${PROTOBUF_DIR}/include

OBJS		= Main.o DiseaseModel.o People.o Locations.o Defs.o disease.pb.o distribution.pb.o
OBJS_DECL	= loimos.decl.h

loimos: $(OBJS) $(OBJS_DECL)
	$(CHARMC) -o loimos $(OBJS) -language charm++ \
	-module CkMulticast $(OPTS) \
	-L${PROTOBUF_DIR}/lib -lprotobuf

main.decl.h: main.ci
	$(CHARMC) main.ci

Main.o: diseaseModelProto Main.C Main.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o Main.o Main.C

People.o: People.C People.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o People.o People.C

Locations.o: Locations.C Locations.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o Locations.o Locations.C

DiseaseModel.o: DiseaseModel.h DiseaseModel.C 
	$(CHARMC) $(OPTS) $(INC) -o DiseaseModel.o DiseaseModel.C

diseaseModelProto: 
	mv disease_model/*.proto ../src/ ; protoc --cpp_out=. disease.proto distribution.proto ; mv *.proto disease_model/

distribution.pb.o: distribution.pb.cc distribution.pb.h
	${CXX} ${CXXFLAGS} $< -c -o $@

disease.pb.o: disease.pb.cc disease.pb.h
	${CXX} ${CXXFLAGS} $< -c -o $@

Defs.o: Defs.h
	$(CHARMC) $(OPTS) $(INC) -o Defs.o Defs.C

clean:
	rm -f *.decl.h *.def.h *.o charmrun loimos *.pb.cc *.pb.h

test: loimos
	./charmrun +p4 ./loimos 110 20 15 6 15 disease_model/H5N1.textproto ++local	
