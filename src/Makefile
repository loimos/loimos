.PHONY: diseaseModelProto
CHARMC=charmc
CXX=g++

PROTOBUF_DIR=/usr/local/bin/vcpkg/installed/x64-osx
# PROTOBUF_DIR=/usr/local
CXXFLAGS=-std=c++11 -Wall -I$(PROTOBUF_DIR)/include

OBJS		= Main.o DiseaseModel.o People.o Locations.o Defs.o disease.pb.o distribution.pb.o Location.o Event.o Attribute.o
OBJS_DECL	= loimos.decl.h
INC = -I$(PROTOBUF_DIR)/include -L$(PROTOBUF_DIR)/lib -lprotobuf

loimos: $(OBJS) $(OBJS_DECL)
	$(CHARMC) -o loimos $(OBJS) -language charm++ \
	-module CkMulticast $(OPTS) \
	-pthread -I$(PROTOBUF_DIR)/include -L$(PROTOBUF_DIR)/lib -lprotobuf

main.decl.h: main.ci
	$(CHARMC) main.ci

Main.o: diseaseModelProto Main.C Main.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o Main.o Main.C

People.o: People.C People.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o People.o People.C

Locations.o: Locations.C Locations.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o Locations.o Locations.C

Location.o: Location.C Location.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o Location.o Location.C

Event.o: Event.C Event.h main.decl.h
	$(CHARMC) $(OPTS) $(INC) -o Event.o Event.C

DiseaseModel.o: DiseaseModel.h DiseaseModel.C 
	$(CHARMC) $(OPTS) $(INC) -o DiseaseModel.o DiseaseModel.C

Attribute.o: Attributes.C Attributes.h
	$(CHARMC) $(OPTS) $(INC) -o Attribute.o Attributes.C 

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

preprocessing: preprocessing/preprocess.C
	${CXX} ${CXXFLAGS} preprocessing/preprocess.C -o preprocess

preprocess: preprocessing
	./preprocess

# test: loimos
	# ./charmrun +p4 ./loimos 40 16 1 4 7 disease_model/H5N1.textproto ++local

test: loimos
	./charmrun +p16 ./loimos 41120 9166 200 100 7 disease_model/H5N1.textproto ++local