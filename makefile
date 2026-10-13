CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = ccom

all: $(TARGET)

$(TARGET): main.o ast.o parser.tab.o lex.yy.o symbol_table.o semantic.o diagnostics.o label_table.o
	$(CC) $(CFLAGS) -o $@ $^

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

parser.tab.o: parser.tab.c ast.h diagnostics.h
	$(CC) $(CFLAGS) -c parser.tab.c

lex.yy.o: lex.yy.c parser.tab.h ast.h diagnostics.h
	$(CC) $(CFLAGS) -c lex.yy.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

symbol_table.o: symbol_table.c symbol_table.h
	$(CC) $(CFLAGS) -c symbol_table.c

semantic.o: semantic.c semantic.h symbol_table.h ast.h diagnostics.h
	$(CC) $(CFLAGS) -c semantic.c

diagnostics.o: diagnostics.c diagnostics.h
	$(CC) $(CFLAGS) -c diagnostics.c

label_table.o: label_table.c label_table.h
	$(CC) $(CFLAGS) -c label_table.c

main.o: main.c semantic.h ast.h diagnostics.h label_table.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(TARGET) *.o parser.tab.c parser.tab.h lex.yy.c

.PHONY: all clean

# Pruebas de las rutinas aritmeticas de Gabriel
build/emitir_aritmetica: codegen_arith.c codegen_arith.h pruebas/aritmetica/emitir.c
	mkdir -p build
	$(CC) $(CFLAGS) -std=c11 -I. codegen_arith.c pruebas/aritmetica/emitir.c -o $@

check-arith: build/emitir_aritmetica
	python3 pruebas/aritmetica/probar.py $<

.PHONY: check-arith