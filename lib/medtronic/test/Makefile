test_programs = history_test schedule_test time_test
other_programs = decode_time read_history

programs = $(test_programs) $(other_programs)

all: $(programs)

.PHONY: all clean test

CFLAGS += -Wall -I . -I .. -I /usr/include/cjson
LDFLAGS += -lcjson

$(test_programs): %: %.c common.c json.c stringer.c ../history.c ../schedule.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(other_programs): %: %.c common.c stringer.c ../history.c ../schedule.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

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
