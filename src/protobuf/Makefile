# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

include ../Makefile.include

PROTOS      = disease.proto distribution.proto data.proto interventions.proto
TMP_SRCS    = $(subst .proto,.pb.cc,$(PROTOS))
SRCS        = $(subst .proto,.pb.cpp,$(PROTOS))
OBJS        = $(subst .proto,.pb.o,$(PROTOS))
HEADERS     = $(subst .proto,.pb.h,$(PROTOS))

.PHONY:all
all: $(OBJS)

$(TMP_SRCS): %.pb.cc: $(PROTOS)
	$(PROTOC) --cpp_out=. $^

$(SRCS): %.pb.cpp: %.pb.cc
	mv $< $@

$(OBJS): %.pb.o: %.pb.cpp %.pb.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

.PHONY:clean
clean:
	rm -f $(TMP_SRCS) $(SRCS) $(OBJS) $(HEADERS)
