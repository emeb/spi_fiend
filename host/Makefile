src = $(wildcard *.c)
obj = $(src:.c=.o)

cdc_prog: $(obj)
	$(CC) -o $@ $^

.PHONY: clean

clean:
	rm -f $(obj) cdc_prog