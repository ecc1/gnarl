programs = decode_time time_test read_history history_test schedule_test
test_programs = $(filter %_test,$(programs))

all: $(programs)

.PHONY: all clean test

headers = testing.h ../medtronic.h ../pump_history.h

CFLAGS += -Wall -isystem . -I .. -g
LDFLAGS += -ljansson

$(programs): %: %.o common.o history.o json.o schedule.o stringer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(headers)
	$(CC) $(CFLAGS) -c -o $@ $<

history.o: ../history.c $(headers)
	$(CC) $(CFLAGS) -c -o $@ $<

schedule.o: ../schedule.c $(headers)
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(test_programs)
	@for prog in $(test_programs) ; do \
		if ./$$prog; then \
			echo $$prog: OK ; \
		else \
			echo $$prog: FAILED ; \
		fi \
	done

clean:
	rm -f *.o $(programs)
