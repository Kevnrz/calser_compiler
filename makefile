CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = ccom

all: $(TARGET)

$(TARGET): main.o ast.o parser.tab.o lex.yy.o symbol_table.o semantic.o
	$(CC) $(CFLAGS) -o $@ $^

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

parser.tab.o: parser.tab.c ast.h
	$(CC) $(CFLAGS) -c parser.tab.c

lex.yy.o: lex.yy.c parser.tab.h ast.h
	$(CC) $(CFLAGS) -c lex.yy.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

symbol_table.o: symbol_table.c symbol_table.h
	$(CC) $(CFLAGS) -c symbol_table.c

semantic.o: semantic.c semantic.h symbol_table.h ast.h
	$(CC) $(CFLAGS) -c semantic.c

main.o: main.c semantic.h ast.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(TARGET) *.o parser.tab.c parser.tab.h lex.yy.c

.PHONY: all clean
