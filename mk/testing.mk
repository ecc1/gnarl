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
INC_DIRS = $(TEST_DIR) . .. ../../../include /usr/include/cjson
INC_FLAGS = $(foreach dir,$(INC_DIRS),-I $(dir))

CFLAGS += -Wall -Wempty-body -Wpedantic -Wunused $(INC_FLAGS)
LDFLAGS += -lcjson -lm

COMMON_CODE = $(TEST_DIR)/common.c
