CC=gcc
CFLAGS=-Os
LDFLAGS=

powerled: powerled.o
    $(CC) $(LDFLAGS) -o $@ $^

powerled.o: powerled.c
    $(CC) $(CFLAGS) -c -o $@ $<

clean:
    rm -f powerled.o powerled
