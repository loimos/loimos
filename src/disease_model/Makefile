CXX         = g++

PROTOBUF_DIR=/usr/local/bin/vcpkg/installed/x64-osx
CXXFLAGS=-std=c++11 -Wall -I${PROTOBUF_DIR}/include

PROTOS      = disease.proto distribution.proto
SRCS        = $(subst .proto,.pb.cc,$(PROTOS)) 
OBJS        = $(subst .proto,.pb.o,$(PROTOS)) 

.PHONY:all
all: $(OBJS)

$(SRCS): %.pb.cc: $(PROTOS)
	protoc --cpp_out=. $^

$(OBJS): %.pb.o: %.pb.cc %.pb.h
	${CXX} ${CXXFLAGS} $< -c -o $@

.PHONY:clean
	rm *.pb.cc *.pb.h
