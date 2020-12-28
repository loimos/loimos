.PHONY: diseaseModelProto
CHARMC=charmc
CXX=g++

PROTOBUF_DIR=/usr/local/bin/vcpkg/installed/x64-osx
# PROTOBUF_DIR=/usr/local
CXXFLAGS=-std=c++11 -Wall -I$(PROTOBUF_DIR)/include

OBJS		= Main.o DiseaseModel.o People.o Locations.o Defs.o disease.pb.o distribution.pb.o Location.o Event.o Attribute.o
OBJS_DECL	= loimos.decl.h
INC = -I$(PROTOBUF_DIR)/include -L$(PROTOBUF_DIR)/lib -lprotobuf

test: ../DataReader.C DataReadTest.C ../DataReader.h
	${CXX} ${CXXFLAGS} $(INC) -O0 -g ../data.pb.cc ../DataReader.C ../../Person.C DataReadTest.C -o main `pkg-config --cflags --libs protobuf`