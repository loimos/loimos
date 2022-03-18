# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

include ../Makefile.include

PROTOS      = interventions.proto
TMP_SRCS    = $(subst .proto,.pb.cc,$(PROTOS)) 
SRCS        = $(subst .proto,.pb.C,$(PROTOS)) 
OBJS        = $(subst .proto,.pb.o,$(PROTOS)) 

PROTOC_FLAGS= --experimental_allow_proto3_optional

.PHONY:all
all: $(OBJS)

$(TMP_SRCS): %.pb.cc: $(PROTOS)
	$(PROTOC) $(PROTOC_FLAGS) --cpp_out=. $^

$(SRCS): %.pb.C: %.pb.cc
	mv $< $@

$(OBJS): %.pb.o: %.pb.C %.pb.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

.PHONY:clean
clean:
	rm -f *.pb.C *.pb.h *.pb.o
