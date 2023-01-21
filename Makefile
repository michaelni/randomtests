
all: mooltitestwalker

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< -O3

mooltitestwalker: mooltitestwalker.o
	$(CC) $(LDFLAGS) -o $@ $< -lm
