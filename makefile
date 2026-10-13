CC = gcc
BISON = bison
FLEX = flex
CFLAGS = -Wall -Wextra -g
CPPFLAGS += -Iinclude -Ibuild/generated

TARGET = ccom
BUILD_DIR = build
GEN_DIR = $(BUILD_DIR)/generated
OBJ_DIR = $(BUILD_DIR)/obj

SOURCES = main ast symbol_table semantic diagnostics label_table codegen generate
OBJECTS = $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(SOURCES))) \
          $(OBJ_DIR)/parser.tab.o $(OBJ_DIR)/lex.yy.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Bison produces both files in one invocation (GNU Make >= 4.3).
$(GEN_DIR)/parser.tab.c $(GEN_DIR)/parser.tab.h &: grammar/parser.y | $(GEN_DIR)
	$(BISON) --defines=$(GEN_DIR)/parser.tab.h -o $(GEN_DIR)/parser.tab.c $<

$(GEN_DIR)/lex.yy.c: grammar/lexer.l $(GEN_DIR)/parser.tab.h | $(GEN_DIR)
	$(FLEX) -o $@ $<

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/parser.tab.o: $(GEN_DIR)/parser.tab.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/lex.yy.o: $(GEN_DIR)/lex.yy.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR) $(GEN_DIR) $(OBJ_DIR):
	mkdir -p $@

# Standalone arithmetic and array code-generation checks.
$(BUILD_DIR)/emitir_aritmetica: src/codegen_arith.c pruebas/aritmetica/emitir.c include/codegen_arith.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-arith: $(BUILD_DIR)/emitir_aritmetica
	python3 pruebas/aritmetica/probar.py $<

$(BUILD_DIR)/emitir_arreglos: src/codegen_array.c pruebas/arreglos/emitir.c include/codegen_array.h include/codegen_arith.h include/symbol_table.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-arrays: $(BUILD_DIR)/emitir_arreglos
	python3 pruebas/arreglos/probar.py $<

check: check-arith check-arrays

clean:
	rm -f $(TARGET) $(OBJECTS) $(OBJECTS:.o=.d)
	rm -f $(GEN_DIR)/parser.tab.c $(GEN_DIR)/parser.tab.h $(GEN_DIR)/lex.yy.c
	rm -f $(BUILD_DIR)/emitir_aritmetica $(BUILD_DIR)/emitir_arreglos

-include $(OBJECTS:.o=.d)

.PHONY: all clean check check-arith check-arrays
