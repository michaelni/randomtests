
all: mooltitestwalker

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

mooltitestwalker: mooltitestwalker.o
	$(CC) $(LDFLAGS) -o $@ $< -lm
