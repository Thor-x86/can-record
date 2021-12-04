all: clean can-record

can-record:
	$(CC) can-record.c -o can-record -lpthread

clean:
	rm -f can-record

.PHONY: all clean