programs = decode_time time_test read_history history_test
headers = testing.h ../medtronic.h ../pump_history.h

CFLAGS += -Wall -isystem . -I .. -g
LDFLAGS += -ljansson

all: $(programs)

$(programs): %: %.o common.o history.o json.o stringer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(headers)
	$(CC) $(CFLAGS) -c -o $@ $<

history.o: ../history.c
	$(CC) $(CFLAGS) -c -o history.o ../history.c

clean:
	rm -f *.o $(programs)
