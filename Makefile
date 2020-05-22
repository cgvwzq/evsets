CC = clang
CFLAGS += -std=gnu11 -Wall -pedantic -Wextra -fPIC -O3
LDFLAGS += -lm

evsets_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
RPATH=-Wl,-R -Wl,${evsets_dir}

default: all

OBJS := list_utils.o hist_utils.o micro.o cache.o utils.o algorithms.o evsets_api.o

all: main.c libevsets.so
	${CC} ${CFLAGS} ${RPATH} ${LDFLAGS} $^ -o evsets

libevsets.so: ${OBJS}
	${CC} ${CFLAGS} -shared ${LDFLAGS} $^ -o libevsets.so

%.o: %.c %.h public_structs.h private_structs.h
	${CC} ${CFLAGS} -c -o $@ $<

counter: CFLAGS += -DTHREAD_COUNTER
counter: LDFLAGS += -pthread -lpthread
counter: all

clean:
	rm -f *.o libevsets.so evsets
