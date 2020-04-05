CXX := clang++
CXXFLAGS := -Wall -Wextra -Wconversion -O3
LDFLAGS :=

# shm_open
# LDFLAGS += -lrt

# parellel support
CXXFLAGS += -fopenmp -D_GLIBCXX_PARALLEL -march=native
LDFLAGS += -fopenmp

SRC := main.cpp \
	common.h hex.h \
	query.cpp query.h \
	database.cpp database.h
OBJS := main.o query.o database.o
FILES := $(SRC) Makefile input output.ac

DEBUG := 1
ifeq ($(DEBUG), 1)
	CXXFLAGS += -g
else
	CXXFLAGS += -DNDEBUG
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

# preload: database-preload.cpp
# 	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

upload:
	# scp -P 9455 $(FILES) soyccan@bravo.nctu.me:/home/soyccan/Documents/dsa-ad
	scp $(FILES) b07902143@linux6.csie.ntu.edu.tw:/home/student/07/b07902143/dsa-ad

remote-run: upload
	ssh -p 9455 soyccan@bravo.nctu.me "cd /home/soyccan/Documents/dsa-ad ; make local-run DEBUG=$(DEBUG)"

local-run: demo
	# time ./demo Criteo_Conversion_Search/CriteoSearchData < input > output
	time ./demo /tmp2/dsahw2/CriteoSearchData < input > output

run:
ifneq ($(shell hostname), soyccan-vm-server)
	$(MAKE) remote-run
else
	$(MAKE) local-run
endif

show-data:
	awk '{gsub("\t"," / ",$$0); print $$0 "\n"}' /tmp2/dsahw2/CriteoSearchData

clean:
	rm -rf $(OBJS) demo preload