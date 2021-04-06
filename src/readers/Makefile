# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

include ../Makefile.include

PROTOS      = data.proto interventions.proto
TMP_SRCS    = $(subst .proto,.pb.cc,$(PROTOS)) 
SRCS        = $(subst .proto,.pb.C,$(PROTOS)) 
OBJS        = $(subst .proto,.pb.o,$(PROTOS)) 

.PHONY:all
all: $(OBJS)

$(TMP_SRCS): %.pb.cc: $(PROTOS)
	$(PROTOC) --cpp_out=. $^

$(SRCS): %.pb.C: %.pb.cc
	mv $< $@

$(OBJS): %.pb.o: %.pb.C %.pb.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

.PHONY:clean
clean:
	rm -f *.pb.C *.pb.h *.pb.o
