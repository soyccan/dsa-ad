CC := gcc
CFLAGS := -Wall -Wextra -Wconversion
LD := ld

SRC := main.c
OBJS := main.o
FILES := $(OBJS) $(SRC) Makefile

ifdef $(DEBUG)
	CC := clang
	CFLAGS += -g
else
	CFLAGS += -O2 -DNDEBUG
endif

.PHONY: all
all: demo

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

demo: $(OBJS)
	$(LD) -o $@ $^

upload:
	scp -P 9455 $(FILES) soyccan@bravo.nctu.me:/home/soyccan/Documents/

run:
	true

clean:
	rm -rf $(OBJS) demo