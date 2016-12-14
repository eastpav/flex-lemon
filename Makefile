CC := gcc
CFLAGS := $(CFLAGS) -std=c99 -g
CFLAGS  += `pkg-config --cflags glib-2.0`
LDFLAGS := `pkg-config --libs glib-2.0`

filter: filter.o scanner.o grammar.o sttype-test.o syntax-tree.o proto.o semcheck.o
	$(CC)  filter.o scanner.o grammar.o sttype-test.o syntax-tree.o proto.o semcheck.o $(LDFLAGS) -o filter 

filter.o: filter.c grammar.h grammar.h scanner.h

sttype-test.o: sttype-test.h sttype-test.c

syntax-tree.o: syntax-tree.h syntax-tree.c

proto.o: proto.h proto.c
semcheck.o: semcheck.h semcheck.c

grammar.o: grammar.h grammar.c

grammar.h grammar.c: grammar.y
	lemon grammar.y

scanner.o: scanner.h

scanner.h: scanner.l
	flex --outfile=scanner.c --header-file=scanner.h scanner.l

# Prevent yacc from trying to build parsers.
# http://stackoverflow.com/a/5395195/79202
%.c: %.y


.PHONY: clean
clean:
	rm -f *.o
	rm -f scanner.c scanner.h
	rm -f grammar.c grammar.h grammar.out
	rm -f filter
