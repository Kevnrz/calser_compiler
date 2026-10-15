CC = gcc
BISON = bison
FLEX = flex
CFLAGS = -Wall -Wextra -g
CPPFLAGS += -Iinclude -Ibuild/generated

TARGET = ccom
BUILD_DIR = build
GEN_DIR = $(BUILD_DIR)/generated
OBJ_DIR = $(BUILD_DIR)/obj

SOURCES = main ast symbol_table semantic diagnostics label_table codegen codegen_instruction codegen_schedule codegen_verify generate codegen_crypto codegen_arith codegen_array assembler
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

$(BUILD_DIR)/emitir_crypto: src/codegen.c src/codegen_instruction.c src/codegen_schedule.c src/codegen_verify.c src/codegen_crypto.c pruebas/crypto/emitir.c include/codegen.h include/codegen_schedule.h include/codegen_crypto.h include/ast.h include/symbol_table.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-crypto: $(BUILD_DIR)/emitir_crypto $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/crypto/probar.py $<
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/crypto/probar_integracion.py

check-ast: $(TARGET)
	python3 pruebas/ast/probar.py

check-asm: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/ensamblador/probar.py

check-runner:
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/ejecutor/probar.py

check-help: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/ayuda/probar.py

check-time: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/tiempo/probar.py

check-matrices: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/matrices/probar.py

check-programas: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/programas/probar.py

# Entregables de los tres ejercicios (enunciado CE1108 §10.4 y §10.5). No
# forma parte de check: es para generar los archivos de ejercicios/.
ejercicios: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 tools/ejercicios.py

check-simulador: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/simulador/probar.py

check-etiquetas: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/etiquetas/probar.py

$(BUILD_DIR)/probar_ir: src/codegen.c src/codegen_instruction.c src/codegen_schedule.c src/codegen_verify.c pruebas/bundles/probar_ir.c include/codegen.h include/codegen_schedule.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

$(BUILD_DIR)/revisar_ir: pruebas/bundles/revisar_ir.c $(filter-out $(OBJ_DIR)/main.o,$(OBJECTS)) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/emitir_organizador: src/codegen.c src/codegen_instruction.c src/codegen_schedule.c src/codegen_verify.c pruebas/bundles/emitir_organizador.c include/codegen.h include/codegen_schedule.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

$(BUILD_DIR)/probar_dependencias: src/codegen.c src/codegen_instruction.c src/codegen_schedule.c src/codegen_verify.c pruebas/bundles/probar_dependencias.c include/codegen.h include/codegen_schedule.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-bundles: $(TARGET) $(BUILD_DIR)/probar_ir $(BUILD_DIR)/revisar_ir $(BUILD_DIR)/emitir_organizador $(BUILD_DIR)/probar_dependencias
	$(BUILD_DIR)/probar_ir
	$(BUILD_DIR)/probar_dependencias
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/bundles/probar.py
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/bundles/probar_organizador.py

check-salidas: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/bundles/probar_salidas.py

validar-bundles: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 tools/validar_bundles.py

check: check-arith check-arrays check-crypto check-ast check-asm check-programas check-runner check-help check-time check-matrices check-etiquetas check-simulador check-bundles check-salidas validar-bundles

clean:
	rm -f $(TARGET) $(OBJECTS) $(OBJECTS:.o=.d)
	rm -f $(GEN_DIR)/parser.tab.c $(GEN_DIR)/parser.tab.h $(GEN_DIR)/lex.yy.c
	rm -f $(BUILD_DIR)/emitir_aritmetica $(BUILD_DIR)/emitir_arreglos $(BUILD_DIR)/emitir_crypto $(BUILD_DIR)/probar_ir $(BUILD_DIR)/revisar_ir $(BUILD_DIR)/emitir_organizador $(BUILD_DIR)/probar_dependencias

-include $(OBJECTS:.o=.d)

.PHONY: all clean ejercicios check check-arith check-arrays check-crypto check-ast check-asm check-programas check-runner check-help check-time check-matrices check-etiquetas check-simulador check-bundles check-salidas validar-bundles
