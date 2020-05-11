test_programs = history_test schedule_test time_test utility_test
other_programs = decode_time read_history

programs = $(test_programs) $(other_programs)

include ../../../mk/testing.mk

$(programs): %: %.c common.c json.c stringer.c ../history.c ../schedule.c ../utility.c $(COMMON_CODE)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
