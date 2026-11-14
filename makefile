CC = gcc
BISON = bison
FLEX = flex
CFLAGS = -Wall -Wextra -g
CPPFLAGS += -Iinclude/comun -Iinclude/isa -Iinclude/analisis -Iinclude/representacion -Iinclude/optimizador -Iinclude/generador_o0 -Iinclude/generador_o1 -Iinclude/rutinas -Iinclude/ensamblado -Iinclude/servicios_ide -Isrc/representacion -Isrc/generador_o1 -I$(GEN_DIR)
vpath %.c src src/comun src/isa src/analisis src/representacion src/optimizador src/generador_o0 src/generador_o1 src/rutinas src/ensamblado src/servicios_ide

TARGET = ccom
BUILD_DIR = build
GEN_DIR = $(BUILD_DIR)/generated
OBJ_DIR = $(BUILD_DIR)/obj

SOURCES = main json_text codegen_common tac_passes tac_pass_unroll tac_pass_rename tac_pass_dce tac_pass_reorder tac_metrics tac_lower tac_lower_value tac_lower_control tac_intervals tac_allocate tac_allocate_sites tac_emit tac_emit_pseudo tac tac_loop_marks tac_live tac_locations tac_loops tac_edit tac_cfg tac_validate tac_isa tac_pseudo tac_print tac_build tac_build_expr tac_build_stmt tac_build_flow tac_build_init ast ast_constant symbol_table semantic semantic_scope semantic_calls semantic_expr semantic_crypto data_area diagnostics label_table isa_table isa_constant codegen codegen_instruction codegen_schedule codegen_verify generate generate_support generate_expr generate_stmt generate_init codegen_crypto codegen_arith codegen_array assembler clones repairs syntax_service suggestions diagnostics_json
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

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/parser.tab.o: $(GEN_DIR)/parser.tab.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/lex.yy.o: $(GEN_DIR)/lex.yy.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR) $(GEN_DIR) $(OBJ_DIR):
	mkdir -p $@

# Standalone arithmetic and array code-generation checks.
$(BUILD_DIR)/emitir_aritmetica: src/rutinas/codegen_arith.c pruebas/rutinas/aritmetica/emitir.c include/rutinas/codegen_arith.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-arith: $(BUILD_DIR)/emitir_aritmetica
	python3 pruebas/rutinas/aritmetica/probar.py $<

$(BUILD_DIR)/emitir_arreglos: src/rutinas/codegen_array.c pruebas/rutinas/arreglos/emitir.c include/rutinas/codegen_array.h include/rutinas/codegen_arith.h include/analisis/symbol_table.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-arrays: $(BUILD_DIR)/emitir_arreglos
	python3 pruebas/rutinas/arreglos/probar.py $<

$(BUILD_DIR)/emitir_crypto: src/ensamblado/codegen.c src/ensamblado/codegen_instruction.c src/isa/isa_table.c src/ensamblado/codegen_schedule.c src/ensamblado/codegen_verify.c src/rutinas/codegen_crypto.c src/rutinas/codegen_common.c pruebas/rutinas/crypto/emitir.c include/ensamblado/codegen.h include/isa/isa_table.h include/ensamblado/codegen_schedule.h include/rutinas/codegen_crypto.h include/rutinas/codegen_common.h include/analisis/ast.h include/analisis/symbol_table.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-crypto: $(BUILD_DIR)/emitir_crypto $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/rutinas/crypto/probar.py $<
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/rutinas/crypto/probar_integracion.py

check-ast: $(TARGET)
	python3 pruebas/analisis/ast/probar.py

check-asm: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/ensamblado/ensamblador/probar.py

check-runner:
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/conductor/ejecutor/probar.py

check-help: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/conductor/ayuda/probar.py

check-time: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/conductor/tiempo/probar.py

check-matrices: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/generador_o0/matrices/probar.py

check-programas: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/generador_o0/programas/probar.py

check-diagnosticos-generacion: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/generador_o0/diagnosticos_generacion/probar.py

# Evaluación constante sin dependencias del generador ni del emisor.
$(BUILD_DIR)/probar_constantes: src/analisis/ast.c src/comun/json_text.c include/comun/json_text.h src/analisis/ast_constant.c pruebas/analisis/constantes/probar.c include/analisis/ast.h include/analisis/ast_constant.h include/analisis/symbol_table.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-constantes: $(BUILD_DIR)/probar_constantes
	$<

# Materialización de constantes sin AST, registros concretos ni emisión.
$(BUILD_DIR)/probar_constantes_isa: src/isa/isa_constant.c pruebas/isa/constantes_isa/probar.c include/isa/isa_constant.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-constantes-isa: $(BUILD_DIR)/probar_constantes_isa
	$<

# Entregables de los tres ejercicios (enunciado CE1108 §10.4 y §10.5). No
# forma parte de check: es para generar los archivos de ejercicios/.
ejercicios: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 tools/ejercicios.py

check-simulador: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/ensamblado/simulador/probar.py

check-etiquetas: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/generador_o0/etiquetas/probar.py

$(BUILD_DIR)/probar_ir: src/ensamblado/codegen.c src/ensamblado/codegen_instruction.c src/isa/isa_table.c src/ensamblado/codegen_schedule.c src/ensamblado/codegen_verify.c pruebas/ensamblado/bundles/probar_ir.c include/ensamblado/codegen.h include/isa/isa_table.h include/ensamblado/codegen_schedule.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

$(BUILD_DIR)/revisar_ir: pruebas/ensamblado/bundles/revisar_ir.c $(filter-out $(OBJ_DIR)/main.o,$(OBJECTS)) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/emitir_organizador: src/ensamblado/codegen.c src/ensamblado/codegen_instruction.c src/isa/isa_table.c src/ensamblado/codegen_schedule.c src/ensamblado/codegen_verify.c pruebas/ensamblado/bundles/emitir_organizador.c include/ensamblado/codegen.h include/isa/isa_table.h include/ensamblado/codegen_schedule.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

$(BUILD_DIR)/probar_dependencias: src/ensamblado/codegen.c src/ensamblado/codegen_instruction.c src/isa/isa_table.c src/ensamblado/codegen_schedule.c src/ensamblado/codegen_verify.c pruebas/ensamblado/bundles/probar_dependencias.c include/ensamblado/codegen.h include/isa/isa_table.h include/ensamblado/codegen_schedule.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

# Semántica ISA compartida, independiente del parseo textual y del organizador.
$(BUILD_DIR)/probar_semantica_isa: src/isa/isa_table.c pruebas/ensamblado/bundles/probar_semantica_isa.c include/isa/isa_table.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-semantica-isa: $(BUILD_DIR)/probar_semantica_isa
	$<

check-bundles: check-semantica-isa $(TARGET) $(BUILD_DIR)/probar_ir $(BUILD_DIR)/revisar_ir $(BUILD_DIR)/emitir_organizador $(BUILD_DIR)/probar_dependencias
	$(BUILD_DIR)/probar_ir
	$(BUILD_DIR)/probar_dependencias
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/ensamblado/bundles/probar.py
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/ensamblado/bundles/probar_organizador.py

check-salidas: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/ensamblado/bundles/probar_salidas.py

validar-bundles: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 tools/validar_bundles.py

# Representación intermedia de tres direcciones independiente del compilador.
TAC_SOURCES = src/representacion/tac.c src/representacion/tac_loop_marks.c src/representacion/tac_edit.c src/representacion/tac_cfg.c src/representacion/tac_validate.c src/representacion/tac_isa.c src/representacion/tac_pseudo.c src/representacion/tac_print.c src/isa/isa_table.c
TAC_HEADERS = src/representacion/tac_analysis_internal.h include/representacion/tac.h include/representacion/tac_loops.h src/representacion/tac_internal.h include/isa/isa_table.h include/analisis/symbol_table.h

$(BUILD_DIR)/probar_tac: $(TAC_SOURCES) pruebas/representacion/tac/probar.c $(TAC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

$(BUILD_DIR)/probar_tac_isa: $(TAC_SOURCES) pruebas/representacion/tac/probar_isa.c $(TAC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

$(BUILD_DIR)/probar_tac_pseudo: $(TAC_SOURCES) pruebas/representacion/tac/probar_pseudo.c $(TAC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-tac: $(BUILD_DIR)/probar_tac $(BUILD_DIR)/probar_tac_isa $(BUILD_DIR)/probar_tac_pseudo $(TARGET)
	$(BUILD_DIR)/probar_tac > $(BUILD_DIR)/tac_obtenido.txt
	diff -u pruebas/representacion/tac/esperado.txt $(BUILD_DIR)/tac_obtenido.txt
	$(BUILD_DIR)/probar_tac_isa > $(BUILD_DIR)/tac_obtenido_isa.txt
	diff -u pruebas/representacion/tac/esperado_isa.txt $(BUILD_DIR)/tac_obtenido_isa.txt
	$(BUILD_DIR)/probar_tac_pseudo
	./$(TARGET) pruebas/representacion/tac/ejemplo.cls -o $(BUILD_DIR)/tac_ejemplo.bin
	@echo "check-tac: 6 casos de alto nivel, ISA virtual/fisica, consultas y validacion: OK"

# Construcción desde AST: prueba de memoria, CLI y listados esperados; sin intérprete.
$(BUILD_DIR)/probar_tac_build: pruebas/representacion/tac_build/probar.c $(filter-out $(OBJ_DIR)/main.o,$(OBJECTS)) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(LDFLAGS) -o $@ $^ $(LDLIBS)

check-tac-build: $(TARGET) $(BUILD_DIR)/probar_tac_build
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/representacion/tac_build/probar_cli.py

# El intérprete solo se enlaza con este auxiliar de pruebas, nunca con ccom.
$(BUILD_DIR)/interpretar_tac: pruebas/representacion/tac_build/ejecutar.c pruebas/representacion/tac_build/interpretar.c pruebas/representacion/tac_build/interpretar.h $(filter-out $(OBJ_DIR)/main.o,$(OBJECTS)) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(LDFLAGS) -o $@ $(filter %.c %.o,$^) $(LDLIBS)

check-tac-values: $(BUILD_DIR)/interpretar_tac
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/representacion/tac_build/probar_valores.py --binario $(BUILD_DIR)/interpretar_tac

# E enlaza la selección B con ccom; estos auxiliares conservan pruebas directas.
TAC_LOWER_SOURCES = src/generador_o1/tac_lower.c src/generador_o1/tac_lower_value.c src/generador_o1/tac_lower_control.c
TAC_LOWER_HEADERS = include/generador_o1/tac_lower.h src/generador_o1/tac_lower_internal.h include/isa/isa_constant.h include/analisis/ast.h

$(BUILD_DIR)/bajar_tac: pruebas/generador_o1/tac_lower/desde_fuente.c $(TAC_LOWER_HEADERS) $(filter-out $(OBJ_DIR)/main.o,$(OBJECTS)) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(LDFLAGS) -o $@ $(filter %.c %.o,$^) $(LDLIBS)

$(BUILD_DIR)/probar_tac_lower: pruebas/generador_o1/tac_lower/probar.c $(TAC_LOWER_SOURCES) $(TAC_SOURCES) src/isa/isa_constant.c src/analisis/ast.c src/comun/json_text.c include/comun/json_text.h $(TAC_LOWER_HEADERS) $(TAC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

check-tac-lower: $(BUILD_DIR)/bajar_tac $(BUILD_DIR)/probar_tac_lower
	$(BUILD_DIR)/probar_tac_lower
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/generador_o1/tac_lower/probar.py

# C1: análisis reutilizables; sin intervalos, asignación ni derrame.
TAC_ANALYSIS_SOURCES = src/representacion/tac_live.c src/representacion/tac_locations.c src/representacion/tac_loops.c
TAC_ANALYSIS_HEADERS = include/representacion/tac_live.h include/representacion/tac_loops.h src/representacion/tac_live_internal.h src/representacion/tac_analysis_internal.h pruebas/representacion/tac_analysis/fixture.h

$(BUILD_DIR)/probar_tac_live: pruebas/representacion/tac_live/probar.c $(TAC_ANALYSIS_SOURCES) $(TAC_SOURCES) $(TAC_HEADERS) $(TAC_ANALYSIS_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

$(BUILD_DIR)/probar_tac_loops: pruebas/representacion/tac_loops/probar.c $(TAC_ANALYSIS_SOURCES) $(TAC_SOURCES) $(TAC_HEADERS) $(TAC_ANALYSIS_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

$(BUILD_DIR)/analizar_tac: pruebas/representacion/tac_analysis/desde_fuente.c $(TAC_LOWER_HEADERS) $(TAC_ANALYSIS_HEADERS) $(filter-out $(OBJ_DIR)/main.o,$(OBJECTS)) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(LDFLAGS) -o $@ $(filter %.c %.o,$^) $(LDLIBS)

check-tac-live: $(BUILD_DIR)/probar_tac_live $(BUILD_DIR)/analizar_tac
	$(BUILD_DIR)/probar_tac_live
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/representacion/tac_analysis/probar.py live

check-tac-loops: $(BUILD_DIR)/probar_tac_loops $(BUILD_DIR)/analizar_tac
	$(BUILD_DIR)/probar_tac_loops
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/representacion/tac_analysis/probar.py loops

# C2: intervalos/asignación compartidos por ccom -O1 y las pruebas directas.
TAC_ALLOCATE_SOURCES = src/representacion/tac_intervals.c src/generador_o1/tac_allocate.c src/generador_o1/tac_allocate_sites.c
TAC_ALLOCATE_HEADERS = include/analisis/data_area.h include/representacion/tac_intervals.h include/generador_o1/tac_allocate.h src/generador_o1/tac_allocate_internal.h

$(BUILD_DIR)/probar_tac_intervals: pruebas/representacion/tac_intervals/probar.c src/representacion/tac_intervals.c $(TAC_ANALYSIS_SOURCES) $(TAC_SOURCES) $(TAC_HEADERS) $(TAC_ANALYSIS_HEADERS) $(TAC_ALLOCATE_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

$(BUILD_DIR)/probar_tac_allocate: pruebas/generador_o1/tac_allocate/probar.c $(TAC_ALLOCATE_SOURCES) $(TAC_LOWER_SOURCES) src/isa/isa_constant.c src/analisis/ast.c src/comun/json_text.c include/comun/json_text.h src/analisis/data_area.c $(TAC_ANALYSIS_SOURCES) $(TAC_SOURCES) $(TAC_HEADERS) $(TAC_ANALYSIS_HEADERS) $(TAC_ALLOCATE_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(filter %.c,$^) -o $@

$(BUILD_DIR)/asignar_tac: pruebas/generador_o1/tac_allocate/desde_fuente.c $(TAC_ALLOCATE_HEADERS) $(TAC_LOWER_HEADERS) $(filter-out $(OBJ_DIR)/main.o,$(OBJECTS)) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(LDFLAGS) -o $@ $(filter %.c %.o,$^) $(LDLIBS)

check-tac-intervals: $(BUILD_DIR)/probar_tac_intervals
	$<

check-tac-allocate: $(BUILD_DIR)/probar_tac_allocate $(BUILD_DIR)/asignar_tac
	$(BUILD_DIR)/probar_tac_allocate
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/generador_o1/tac_allocate/probar.py

# D/E: emisión directa y comparación observable por la CLI -O1.
TAC_EMIT_SOURCES = src/generador_o1/tac_emit.c src/generador_o1/tac_emit_pseudo.c
TAC_EMIT_HEADERS = include/generador_o1/tac_emit.h src/generador_o1/tac_emit_internal.h include/rutinas/codegen_common.h

$(BUILD_DIR)/probar_tac_emit: pruebas/generador_o1/tac_emit/probar.c $(TAC_EMIT_HEADERS) $(TAC_ALLOCATE_HEADERS) $(filter-out $(OBJ_DIR)/main.o,$(OBJECTS)) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(LDFLAGS) -o $@ $(filter %.c %.o,$^) $(LDLIBS)

check-tac-emit: $(BUILD_DIR)/probar_tac_emit
	$<

check-o1-equivalence: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/generador_o1/tac_emit/comparar.py

$(BUILD_DIR)/probar_tac_passes: pruebas/optimizador/tac_passes/probar.c $(filter-out $(OBJ_DIR)/main.o,$(OBJECTS)) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(LDFLAGS) -o $@ $^ $(LDLIBS)

check-o1: $(TARGET) $(BUILD_DIR)/probar_tac_passes
	$(BUILD_DIR)/probar_tac_passes
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/optimizador/tac_passes/probar_cli.py

check: check-pasadas check-pasadas-isa check-ide-options check-o1 check-tac-emit check-o1-equivalence check-tac-intervals check-tac-allocate check-tac-live check-tac-loops check-tac check-tac-build check-tac-values check-tac-lower check-arith check-arrays check-crypto check-ast check-asm check-programas check-diagnosticos-generacion check-constantes check-constantes-isa check-runner check-help check-time check-matrices check-etiquetas check-simulador check-bundles check-salidas validar-bundles check-recovery check-clones-json check-repairs-json check-suggestions-json check-tokens-json check-success check-analysis check-analysis-json

clean:
	rm -f $(BUILD_DIR)/probar_tac_passes
	rm -f $(BUILD_DIR)/probar_tac_emit
	rm -f $(BUILD_DIR)/probar_tac_intervals $(BUILD_DIR)/probar_tac_allocate $(BUILD_DIR)/asignar_tac
	rm -f $(BUILD_DIR)/probar_tac_live $(BUILD_DIR)/probar_tac_loops $(BUILD_DIR)/analizar_tac
	rm -f $(BUILD_DIR)/bajar_tac $(BUILD_DIR)/probar_tac_lower
	rm -f $(BUILD_DIR)/interpretar_tac
	rm -f $(BUILD_DIR)/probar_tac_build
	rm -f $(BUILD_DIR)/probar_constantes
	rm -f $(BUILD_DIR)/probar_constantes_isa
	rm -f $(BUILD_DIR)/probar_semantica_isa
	rm -f $(BUILD_DIR)/tac_obtenido.txt $(BUILD_DIR)/tac_obtenido_isa.txt
	rm -f $(BUILD_DIR)/probar_tac_isa $(BUILD_DIR)/tac_ejemplo.bin $(BUILD_DIR)/tac_ejemplo.hex $(BUILD_DIR)/tac_ejemplo.mem
	rm -f $(BUILD_DIR)/probar_tac_pseudo
	rm -f $(TARGET) $(OBJECTS) $(OBJECTS:.o=.d)
	rm -f $(GEN_DIR)/parser.tab.c $(GEN_DIR)/parser.tab.h $(GEN_DIR)/lex.yy.c
	rm -f $(BUILD_DIR)/probar_tac $(BUILD_DIR)/emitir_aritmetica $(BUILD_DIR)/emitir_arreglos $(BUILD_DIR)/emitir_crypto $(BUILD_DIR)/probar_ir $(BUILD_DIR)/revisar_ir $(BUILD_DIR)/emitir_organizador $(BUILD_DIR)/probar_dependencias

-include $(OBJECTS:.o=.d)

.PHONY: check-o1 check-tac-emit check-o1-equivalence check-tac-intervals check-tac-allocate check-tac-live check-tac-loops all clean ejercicios check check-tac-lower check-tac-values check-tac-build check-semantica-isa check-tac check-arith check-arrays check-crypto check-ast check-asm check-programas check-diagnosticos-generacion check-constantes check-constantes-isa check-runner check-help check-time check-matrices check-etiquetas check-simulador check-bundles check-salidas validar-bundles

# Prototipo gráfico CALSIDE (PySide6).
PYTHON ?= python3
.PHONY: ide check-ide check-parser
ide:
	$(PYTHON) -m ide.main

check-ide:
	PYTHONDONTWRITEBYTECODE=1 QT_QPA_PLATFORM=offscreen $(PYTHON) -m unittest discover -s ide/pruebas -v

check-parser:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) -m unittest discover -s ide/pruebas -p test_parser.py -v

# Front-end sin generación para el análisis automático del IDE.
.PHONY: check-analysis
check-analysis: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) pruebas/analisis/check/probar.py

# Confirmación textual y rutas de operaciones correctas de ccom.
.PHONY: check-success
check-success: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) pruebas/conductor/exito/probar.py

# Protocolo estructurado del front-end real para CALSIDE (M1).
.PHONY: check-analysis-json
check-analysis-json: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) pruebas/servicios_ide/analisis_json/probar.py

# Tokens del lexer real y presentación del IDE, sin parser ni artefactos.
.PHONY: check-tokens-json
check-tokens-json: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) pruebas/servicios_ide/tokens_json/probar.py

.PHONY: check-suggestions-json
check-suggestions-json: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) pruebas/servicios_ide/sugerencias_json/probar.py

.PHONY: check-repairs-json
check-repairs-json: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) pruebas/servicios_ide/reparaciones_json/probar.py

.PHONY: check-clones-json
check-clones-json: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) pruebas/servicios_ide/clones_json/probar.py

# Recorrido gráfico aislado con todos los proveedores C y el simulador reales.
.PHONY: check-recorrido-ide
check-recorrido-ide: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 QT_QPA_PLATFORM=offscreen $(PYTHON) tools/probar_recorrido_ide.py

.PHONY: check-recovery
check-recovery: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) pruebas/analisis/recuperacion/probar.py

# Integración C6: las consultas no aceptan opciones de compilación.
.PHONY: check-ide-options
check-ide-options: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) pruebas/servicios_ide/integracion_ide/probar.py

# Pasadas vacías hoy; contratos de equivalencia de §9.11 y Aho §9.1.
check-pasadas: $(BUILD_DIR)/interpretar_tac
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/optimizador/tac_passes/probar_equivalencia.py

check-pasadas-isa: $(TARGET)
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/generador_o1/tac_emit/comparar.py --opt=reorder
	PYTHONDONTWRITEBYTECODE=1 python3 pruebas/generador_o1/tac_emit/comparar.py --opt=spill

.PHONY: check-pasadas check-pasadas-isa
