# Fase 4 — contrato común (Kevin, Saddy, Gabriel)

**Grupo 6 · 12 oct 2026 · Entrega: 14 oct, 13:00**
**Rama base:** `integracion`. Cada uno trabaja en su rama, que sale de
`integracion`, y la une de vuelta solo con las pruebas pasando.

Este documento fija lo que los tres usan igual. **Si algo no cuadra, se avisa,
no se cambia en silencio.** Fuentes: `isa.md` (§6 codificación, §9 separación,
§10 saltos, §11 sintaxis), `memoria_decisiones.md`, `crypto_para_ccom.md`,
`coordinacion_pendientes.md` §1.2, `microarchitecture.md` §1.5.

---

## 1. Cómo se emite código: `codegen.h` (lo crea Kevin primero)

La Fase 4 no escribe texto directamente: arma en memoria una lista de líneas
(etiquetas y bundles) y al final la escribe al `.asm` o la pasa al ensamblado.

```c
/* Slots del bundle (isa.md §3). */
typedef enum { SLOT_ALU0 = 0, SLOT_ALU1 = 1, SLOT_BRU = 2, SLOT_LSU = 3 } Slot;

void cg_init(void);
void cg_free(void);

/* Etiqueta: vale el índice del bundle que la sigue (isa.md §11.1). */
void cg_label(const char *name);

/* Bundle completo. NULL en un slot = nopi. Las cadenas se copian. */
void cg_bundle(const char *s0, const char *s1, const char *s2, const char *s3);

/* Una sola operación en su slot, y a continuación un bundle vacío de
   separación (versión segura, isa.md §9.1). Formato tipo printf. */
void cg_op(Slot slot, const char *fmt, ...);

/* Bundle vacío. */
void cg_gap(void);

/* Escribe todo en formato isa.md §11 (un bundle por línea, 4 slots). */
void cg_write(FILE *out);

/* Acceso para el ensamblado: cantidad de líneas y cada línea ya en texto. */
int         cg_line_count(void);
const char *cg_line(int i);
```

**Regla de la versión segura:** `cg_op` deja siempre un bundle vacío después.
Así nunca se viola la separación de 1 bundle, aunque se gasten bundles.
Quien use `cg_bundle` directamente (las plantillas de cifrado, rutinas
calendarizadas a mano) es responsable de su propia separación.

## 2. Convenciones de código

| Tema | Regla | Fuente |
|---|---|---|
| Registros | `rz`=0, `rl`=1 (enlace), `rx`=2, `rg`=3, `t0`–`t11` = 4–15 | `isa.md` §8 |
| Acceso a variables | Base `rz` y dirección absoluta: `lw t0, 16(rz)` | `memoria_decisiones.md` §2 |
| Separación | 1 bundle entre escribir un registro y leerlo, también para saltos. `sw` → `lw` a la misma dirección no la necesita | `isa.md` §9 |
| Saltos | El bundle con el salto se ejecuta completo; el siguiente se descarta | `isa.md` §10 |
| Un `lw`/`sw` por bundle | Solo hay una LSU (slot 3) | `isa.md` §3 |
| Constantes | Si caben en 18 bits con signo: `mov`. Si no: secuencia de `isa.md` §4.3 | `isa.md` §4.3 |
| Comparaciones | `slt`, `sle`, `blt`, `bge` son con signo | `isa.md` C-03, C-04 |

## 3. Memoria y funciones

| Tema | Regla |
|---|---|
| Área de una función | +0 `rl` guardado (no en `main`), +4 retorno, +8… parámetros, luego locales, **al final 12 casillas de temporales** (`temp_base`, cambio 12 de Saddy) |
| Orden de emisión | **`main` primero**, así arranca en el bundle 0. Luego las demás funciones y al final las rutinas |
| Prólogo (no en `main`) | `sw rl, A+0(rz)`; `sw rz, A+4(rz)` (si devuelve valor) |
| Etiquetas de función | `.L_func_<nombre>` al inicio; `.L_body_<nombre>` después del prólogo |
| Llamada | Calcular argumentos → `sw` en las casillas de los parámetros (arreglos: su dirección) → guardar en `temp_base` los temporales vivos → `jas rl, .L_func_f` → recuperar temporales → `lw` del retorno en `A_f+4` |
| `E` | `lw rl, A+0(rz)` → bundle vacío → `jefR rz, rl` |
| `E` de `main` | Termina el programa: `fin_main: { nopi \| nopi \| jef fin_main \| nopi }` |
| `R` | Calcular **todos** los argumentos en registros → `sw` en los parámetros → `jef .L_body_f`. No toca `rl` ni el retorno |
| Locales | Se ponen en 0 o en su inicializador donde está su declaración, cada vez |
| Globales | Las de valor inicial constante van en la imagen de datos (§4). Las de valor no constante, código al inicio de `main` |

## 4. Imagen de datos

La Fase 4 llena un arreglo de palabras de 32 bits que representa 0x0000–0x0FFF,
en cero por defecto, con los valores iniciales constantes de las globales
(incluidos los arreglos globales con lista). Expone:

```c
int             cg_data_size(void);   /* bytes usados, múltiplo de 4 */
const uint32_t *cg_data(void);        /* palabras desde la dirección 0 */
```

## 5. Archivos de salida (los escribe Gabriel)

| Archivo | Contenido |
|---|---|
| `.asm` (`-s`) | Lo de `cg_write` |
| `.bin` | Encabezado de 4 palabras de 32 bits *little endian*: cantidad de bundles, punto de entrada (0), tamaño de datos en bytes, dirección inicial de datos (0). Luego cada bundle en 16 bytes (slot 0 primero, cada palabra *little endian*). Luego los datos |
| `.hex` | Un bundle por línea, `slot3_slot2_slot1_slot0` en hexadecimal (`microarchitecture.md` §1.5) |
| `.mem` | `@0` y una palabra por línea en hexadecimal (`microarchitecture.md` §1.5) |

## 6. Quién hace qué

| Persona | Documento |
|---|---|
| Kevin | `fase4_kevin.md` |
| Saddy | `docs/fase4_saddy.md` |
| Gabriel | `fase4_gabriel.md` |
