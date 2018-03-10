CC=g++
CFLAGS=-I

n64sym:
	$(CC) $(CFLAGS) ./include -O3 main.cpp -o n64sym

clean:
	rm -f n64sym

.PHONY: n64sym clean
