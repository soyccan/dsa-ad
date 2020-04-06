CXX := clang++
CXXFLAGS := -Wall -Wextra -Wconversion -O3 -std=gnu++17
LDFLAGS :=

# shm_open
# LDFLAGS += -lrt

# parellel support
ifndef PAR
	PAR := 1
endif
ifeq ($(PAR), 1)
	CXXFLAGS += -fopenmp -D_GLIBCXX_PARALLEL -march=native
	LDFLAGS += -fopenmp
else
	CXXFLAGS += -DNOPAR
endif

SRC := main.cpp \
	common.h hex.h \
	query.cpp query.h \
	database.cpp database.h
OBJS := main.o query.o database.o
FILES := $(SRC) Makefile input output.ac

ifndef DEBUG
	DEBUG := 0
endif
ifeq ($(DEBUG), 1)
	CXXFLAGS += -g
else
	CXXFLAGS += -DNDEBUG
endif

ifndef VERBOSE
	VERBOSE := 0
endif
ifeq ($(VERBOSE), 1)
	Q :=
else
	Q := @
endif

.PHONY: upload clean run show-data pack

demo: $(OBJS)
	$(Q)$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(Q)$(CXX) $(CXXFLAGS) -c -o $@ $^

upload:
	# scp -P 9455 $(FILES) soyccan@bravo.nctu.me:/home/soyccan/Documents/dsa-ad
	scp $(FILES) b07902143@linux6.csie.ntu.edu.tw:/home/student/07/b07902143/dsa-ad

# remote-run: upload
# 	ssh -p 9455 soyccan@bravo.nctu.me "cd /home/soyccan/Documents/dsa-ad ; make local-run DEBUG=$(DEBUG)"

run: demo
	# time ./demo Criteo_Conversion_Search/CriteoSearchData < input > output
	time ./demo /tmp2/dsahw2/CriteoSearchData < input > output

show-data:
	awk '{gsub("\t"," / ",$$0); print $$0 "\n"}' /tmp2/dsahw2/CriteoSearchData

clean:
	rm -rf $(OBJS) demo

pack:
	mkdir b07902143_dsa_hw2
	cp $(SRC) Makefile b07902143_dsa_hw2
	zip -r b07902143_dsa_hw2.zip b07902143_dsa_hw2
