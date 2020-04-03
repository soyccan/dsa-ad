CC := gcc
CFLAGS := -Wall -Wextra -Wconversion
CXX := g++
CXXFLAGS := -Wall -Wextra -Wconversion -O2
LD := ld
LDFLAGS := -lrt

SRC := main.cpp query.cpp query.h common.h database.cpp database.h database-preload.cpp
OBJS := main.o query.o database.o
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
ifneq ($(shell hostname), soyccanmac.local)
	$(MAKE) local-run
else
	$(MAKE) upload
endif

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

demo: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

preload: database-preload.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

upload:
	# scp -P 9455 $(FILES) soyccan@bravo.nctu.me:/home/soyccan/Documents/dsa-ad
	scp $(FILES) b07902143@linux2.csie.ntu.edu.tw:/home/student/07/b07902143/dsa-ad

remote-run: upload
	ssh -p 9455 soyccan@bravo.nctu.me "cd /home/soyccan/Documents/dsa-ad ; make local-run DEBUG=$(DEBUG)"

local-run: demo preload
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
	rm -rf $(OBJS) demo preload