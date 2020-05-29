#pragma once
#include <stdlib.h>
typedef struct {
    uint8_t  type;
    uint64_t val;
    uint8_t *end;
    void *private;
} fw_module_stack_t;


typedef struct {
    uint8_t *end;
    uint8_t stack_deepth;
} fw_module_frame;

typedef struct fw_module_context_t {
    fw_module_t *module;
    fw_module_stack_t *stack;
    uint32_t pos;
    uint32_t end;
    fw_func_locals_t *locals;
    fw_module_func_block_t *current;
    fw_module_frame frame;
} fw_module_context_t;

fw_module_func_t *fw_module_get_func(fw_module_t *module, char *funcname);
int32_t _fw_module_run_func(fw_module_context_t *ctx, fw_module_func_t *func);
int32_t fw_module_run_func(fw_module_t *module, fw_module_func_t *func, int argc, char **args, fw_ret_t *rets);
int32_t fw_module_run(fw_module_t *module, char *funcname, int argc, char **args, fw_ret_t *rets);

