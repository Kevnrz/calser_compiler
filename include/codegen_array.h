#ifndef CODEGEN_ARRAY_H
#define CODEGEN_ARRAY_H
#include "codegen_arith.h"
#include "symbol_table.h"

/* K-4b reuses K-4's emission contract; it does not call its arithmetic routines.
 * All register arguments are ISA numbers 4..15. Call only after semantic success.
 * The caller evaluates/preserves index expressions and schedules RAW separation.
 * Return 0 on invalid contract or emitter failure; discard partial output. */
int cg_array_base(CGArithContext *ctx, const Symbol *array, int dst);

/* Preserve indices. dst/work[0]/work[1] distinct from each other and indices.
 * count must equal array->dimensions; 2D is row-major.
 * Only two scratch registers, no multiplication instruction or runtime call.
 * Dynamic bounds are not checked; caller must supply valid indices. */
int cg_array_address(CGArithContext *ctx, const Symbol *array,
                     const int *indices, int count, int dst, const int work[2]);

/* address and work clobbered; load writes value. load's value must not alias
 * indices/address/work. store's value may alias an index, but not address/work.
 * store rejects Vi destinations; initialization of Vi is the caller's job. */
int cg_array_load(CGArithContext *ctx, const Symbol *array, const int *indices,
                  int count, int address, int value, const int work[2]);
int cg_array_store(CGArithContext *ctx, const Symbol *array, const int *indices,
                   int count, int address, int value, const int work[2]);

/* Whole-array argument: write base ADDRESS into destination parameter's cell.
 * Forwarded array parameters dereference their pointer cell first. */
int cg_array_pass(CGArithContext *ctx, const Symbol *source,
                  const Symbol *parameter, int temp);

/* Whole-array assignment and default local initialization. copy requires equal
 * shape/type and writable destination. Exact aliases work; arrays must otherwise
 * be disjoint (Calser passes complete arrays, without slice/pointer arithmetic).
 * copy clobbers 4 distinct work registers. zero clobbers 2 distinct registers and
 * may initialize Vi declarations. Neither walks the AST initializer lists. */
int cg_array_copy(CGArithContext *ctx, const Symbol *destination,
                  const Symbol *source, const int work[4]);
int cg_array_zero(CGArithContext *ctx, const Symbol *array, const int work[2]);
#endif
