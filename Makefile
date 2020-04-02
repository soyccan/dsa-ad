CC := gcc
CFLAGS := -Wall -Wextra -Wconversion
CXX := g++
CXXFLAGS := -Wall -Wextra -Wconversion
LD := ld

SRC := main.cpp query.cpp query.h common.h
OBJS := main.o query.o
FILES := $(SRC) Makefile input output.ac

DEBUG := 1
ifeq ($(DEBUG), 1)
	CC := clang
	CXX := clang++
	CFLAGS += -g
	CXXFLAGS += -g
else
	CFLAGS += -O2 -DNDEBUG
	CXXFLAGS += -O2 -DNDEBUG
endif

.PHONY: all upload clean run
all:
ifeq ($(shell hostname), soyccan-vm-server)
	$(MAKE) local-run
else
	$(MAKE) upload
endif

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

demo: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

upload:
	scp -P 9455 $(FILES) soyccan@bravo.nctu.me:/home/soyccan/Documents/dsa-ad

remote-run: upload
	ssh -p 9455 soyccan@bravo.nctu.me "cd /home/soyccan/Documents/dsa-ad ; make local-run DEBUG=$(DEBUG)"

local-run: demo
	time ./demo < input > output

run:
ifneq ($(shell hostname), soyccan-vm-server)
	$(MAKE) remote-run
else
	$(MAKE) local-run
endif

show-data:
	awk '{gsub("\t"," / ",$$0); print $$0 "\n"}' Criteo_Conversion_Search/CriteoSearchData

clean:
	rm -rf $(OBJS) demo