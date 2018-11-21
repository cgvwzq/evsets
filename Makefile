CC = clang
CFLAGS += -std=gnu11 -Wall -pedantic -Wextra -fPIC -O3
LDFLAGS += -lm

default: all

OBJS := list_utils.o hist_utils.o micro.o cache.o utils.o algorithms.o

all: main.c ${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} $^ -o evsets

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

counter: CFLAGS += -DTHREAD_COUNTER
counter: LDFLAGS += -pthread -lpthread
counter: all

clean:
	rm *.o
	test -f evsets && rm evsets
