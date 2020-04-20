test_programs = time_test

programs = $(test_programs)

include ../../../mk/testing.mk

$(programs): %: %.c ../time.c $(COMMON_CODE)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
