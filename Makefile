
all: mooltitestwalker mooltitestcycler

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< -O3

mooltitestwalker: mooltitestwalker.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

mooltitestcycler: mooltitestcycler.o
	$(CC) $(LDFLAGS) -o $@ $< -lm
