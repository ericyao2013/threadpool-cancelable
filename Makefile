CFLAGS += -std=c99 -D_GNU_SOURCE -Wall -Wextra -pedantic
LDFLAGS += -lpthread

PROG = example
OBJS = \
  example.o \
  threadpool.o

.PHONY: clean all

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(PROG)
