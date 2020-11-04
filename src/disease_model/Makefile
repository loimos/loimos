CXX         = g++

# The environment variable PROTOBUF_HOME should be set to hold the path to the
# local installation of protobuf
CXXFLAGS=-std=c++11 -Wall -I$(PROTOBUF_HOME)/include

PROTOS      = disease.proto distribution.proto
TMP_SRCS    = $(subst .proto,.pb.cc,$(PROTOS)) 
SRCS        = $(subst .proto,.pb.C,$(PROTOS)) 
OBJS        = $(subst .proto,.pb.o,$(PROTOS)) 

.PHONY:all
all: $(OBJS)

$(TMP_SRCS): %.pb.cc: $(PROTOS)
	protoc --cpp_out=. $^

$(SRCS): %.pb.C: %.pb.cc
	mv $< $@

$(OBJS): %.pb.o: %.pb.C %.pb.h
	${CXX} ${CXXFLAGS} $< -c -o $@

.PHONY:clean
clean:
	rm *.pb.C *.pb.h *.pb.o
