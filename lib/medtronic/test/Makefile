test_programs = history_test time_test

all: $(test_programs)

CFLAGS += -Wall -isystem . -I .. -g

history_test: history_test.c ../history.c

time_test: time_test.c ../history.c

clean:
	rm -f *.o $(test_programs)
