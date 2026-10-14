# Builds the DS frame decoder for the PC (used by stream/host/tests/test_c_decoder.py).
CC	?= cc
CFLAGS	?= -O2 -Wall -Wextra -fno-strict-aliasing

decode_test: decode_test.c ../src/decode.c ../src/decode.h
	$(CC) $(CFLAGS) -o $@ decode_test.c ../src/decode.c

clean:
	rm -f decode_test
