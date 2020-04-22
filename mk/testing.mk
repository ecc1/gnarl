all: $(programs)

.PHONY: all clean test

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

TEST_DIR = ../../../test

CFLAGS += -Wall -Wempty-body -Wpedantic -Wunused -I . -I .. -I $(TEST_DIR) -I /usr/include/cjson
LDFLAGS += -lcjson

COMMON_CODE = $(TEST_DIR)/common.c
