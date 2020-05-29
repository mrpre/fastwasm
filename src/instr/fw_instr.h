#pragma once

/*
 *https://www.w3.org/TR/wasm-core-1/#a7-index-of-instructions
 */
typedef enum
{
    /*https://www.w3.org/TR/wasm-core-1/#control-instructions%E2%91%A6*/
    FW_INSTR_UNREACHABLE = 0x00,
    FW_INSTR_NOP,
    FW_INSTR_BLOCK,
    FW_INSTR_LOOP,
    FW_INSTR_IF,
    FW_INSTR_ELSE,

    FW_INSTR_END = 0x0b,
    FW_INSTR_BR = 0x0c,
    FW_INSTR_BR_IF,
    FW_INSTR_BR_TBL,
    FW_INSTR_RET,
    FW_INSTR_CALL,
    FW_INSTR_CALL_INDIRECT,

    FW_INSTR_DROP = 0x1a,
    FW_INSTR_SELECT = 0x1b,

    /*https://www.w3.org/TR/wasm-core-1/#parametric-instructions%E2%91%A6*/
    FW_INSTR_LOCAL_GET = 0x20,
    FW_INSTR_LOCAL_SET,
    FW_INSTR_LOCAL_TEE,
    FW_INSTR_GLOBAL_GET,
    FW_INSTR_GLOBAL_SET,


    /*https://www.w3.org/TR/wasm-core-1/#memory-instructions%E2%91%A6*/
    FW_INSTR_I32_LOAD     = 0x28,
    FW_INSTR_I64_LOAD,
    FW_INSTR_F32_LOAD,
    FW_INSTR_F64_LOAD,
    FW_INSTR_I32_LOAD8_S,
    FW_INSTR_I32_LOAD8_U,
    FW_INSTR_I32_LOAD16_S,
    FW_INSTR_I32_LOAD16_U,
    FW_INSTR_I64_LOAD8_S,
    FW_INSTR_I64_LOAD8_U,
    FW_INSTR_I64_LOAD16_S,
    FW_INSTR_I64_LOAD16_U,
    FW_INSTR_I64_LOAD32_S,
    FW_INSTR_I64_LOAD32_U = 0x35,
    FW_INSTR_I32_STORE    = 0x36,
    FW_INSTR_I64_STORE,
    FW_INSTR_F32_STORE,
    FW_INSTR_F64_STORE,
    FW_INSTR_I32_STORE8,
    FW_INSTR_I32_STORE16,
    FW_INSTR_I64_STORE8,
    FW_INSTR_I64_STORE16,
    FW_INSTR_I64_STORE32,

    FW_INSTR_MEM_SIZE,
    FW_INSTR_MEM_GLOW = 0x40,

    /*https://www.w3.org/TR/wasm-core-1/#numeric-instructions%E2%91%A6*/
    FW_INSTR_I32_CONST = 0x41,
    FW_INSTR_I64_CONST,
    FW_INSTR_F32_CONST,
    FW_INSTR_F64_CONST,

    FW_INSTR_I32_EQZ,
    FW_INSTR_I32_EQ,
    FW_INSTR_I32_NE,
    FW_INSTR_I32_LT_S,
    FW_INSTR_I32_LT_U,
    FW_INSTR_I32_GT_S,
    FW_INSTR_I32_GT_U,
    FW_INSTR_I32_LE_S,
    FW_INSTR_I32_LE_U,
    FW_INSTR_I32_GE_S,
    FW_INSTR_I32_GE_U = 0x4f,

    FW_INSTR_I64_EQZ = 0x50,
    FW_INSTR_I64_EQ,
    FW_INSTR_I64_NE,
    FW_INSTR_I64_LT_S,
    FW_INSTR_I64_LT_U,
    FW_INSTR_I64_GT_S,
    FW_INSTR_I64_GT_U,
    FW_INSTR_I64_LE_S,
    FW_INSTR_I64_LE_U,
    FW_INSTR_I64_GE_S,
    FW_INSTR_I64_GE_U,

    FW_INSTR_F32_EQ = 0x5b,
    FW_INSTR_F32_NE,
    FW_INSTR_F32_LT,
    FW_INSTR_F32_GT,
    FW_INSTR_F32_LE,
    FW_INSTR_F32_GE,

    FW_INSTR_F64_EQ = 0x61,
    FW_INSTR_F64_NE,
    FW_INSTR_F64_LT,
    FW_INSTR_F64_GT,
    FW_INSTR_F64_LE,
    FW_INSTR_F64_GE,

    FW_INSTR_I32_CLZ = 0x67,
    FW_INSTR_I32_CTZ,
    FW_INSTR_I32_POPCNT,
    FW_INSTR_I32_ADD,
    FW_INSTR_I32_SUB,
    FW_INSTR_I32_MUL,
    FW_INSTR_I32_DIV_S,
    FW_INSTR_I32_DIV_U,
    FW_INSTR_I32_REM_S,
    FW_INSTR_I32_REM_U,
    FW_INSTR_I32_AND,
    FW_INSTR_I32_OR,
    FW_INSTR_I32_XOR,
    FW_INSTR_I32_SHL,
    FW_INSTR_I32_SHR_S,
    FW_INSTR_I32_SHR_U,
    FW_INSTR_I32_ROTL,
    FW_INSTR_I32_ROTR,

    FW_INSTR_I64_CLZ = 0x79,
    FW_INSTR_I64_CTZ,
    FW_INSTR_I64_POPCNT,
    FW_INSTR_I64_ADD,
    FW_INSTR_I64_SUB,
    FW_INSTR_I64_MUL,
    FW_INSTR_I64_DIV_S,
    FW_INSTR_I64_DIV_U,
    FW_INSTR_I64_REM_S,
    FW_INSTR_I64_REM_U,
    FW_INSTR_I64_AND,
    FW_INSTR_I64_OR,
    FW_INSTR_I64_XOR,
    FW_INSTR_I64_SHL,
    FW_INSTR_I64_SHR_S,
    FW_INSTR_I64_SHR_U,
    FW_INSTR_I64_ROTL,
    FW_INSTR_I64_ROTR = 0x8A,

    FW_INSTR_F32_ABS = 0x8B,
    FW_INSTR_F32_NEG,
    FW_INSTR_F32_CEIL,
    FW_INSTR_F32_FLOOR,
    FW_INSTR_F32_TRUNC,
    FW_INSTR_F32_NEAREST,
    FW_INSTR_F32_SQRT,
    FW_INSTR_F32_ADD,
    FW_INSTR_F32_SUB,
    FW_INSTR_F32_MUL,
    FW_INSTR_F32_DIV,
    FW_INSTR_F32_MIN,
    FW_INSTR_F32_MAX,
    FW_INSTR_F32_COPYSIGN,

    FW_INSTR_F64_ABS = 0x99,
    FW_INSTR_F64_NEG,
    FW_INSTR_F64_CEIL,
    FW_INSTR_F64_FLOOR,
    FW_INSTR_F64_TRUNC,
    FW_INSTR_F64_NEAREST,
    FW_INSTR_F64_SQRT,
    FW_INSTR_F64_ADD,
    FW_INSTR_F64_SUB,
    FW_INSTR_F64_MUL,
    FW_INSTR_F64_DIV,
    FW_INSTR_F64_MIN,
    FW_INSTR_F64_MAX,
    FW_INSTR_F64_COPYSIGN,

    FW_INSTR_I32_WRAP_I64 = 0xa7,
    FW_INSTR_I32_TRUNC_F32_S,
    FW_INSTR_I32_TRUNC_F32_U,
    FW_INSTR_I32_TRUNC_F64_S,
    FW_INSTR_I32_TRUNC_F64_U,
    FW_INSTR_I64_EXTEND_I32_S,
    FW_INSTR_I64_EXTEND_I32_U,
    FW_INSTR_I64_TRUNC_F32_S,
    FW_INSTR_I64_TRUNC_F32_U,
    FW_INSTR_I64_TRUNC_F64_S,
    FW_INSTR_I64_TRUNC_F64_U,
    FW_INSTR_F32_CONVERT_I32_S,
    FW_INSTR_F32_CONVERT_I32_U,
    FW_INSTR_F32_CONVERT_I64_S,
    FW_INSTR_F32_CONVERT_I64_U,
    FW_INSTR_F32_DEMOTE_F64,
    FW_INSTR_F64_CONVERT_I32_S,
    FW_INSTR_F64_CONVERT_I32_U,
    FW_INSTR_F64_CONVERT_I64_S,
    FW_INSTR_F64_CONVERT_I64_U,
    FW_INSTR_F64_PROMOTE_F32,
    FW_INSTR_I32_REINTERPRET_F32,
    FW_INSTR_I64_REINTERPRET_F64,
    FW_INSTR_F32_REINTERPRET_I32,
    FW_INSTR_F64_REINTERPRET_I64 = 0xbf,

    FW_INSTR_MAX,
} FW_INSTR;


typedef struct {
    union {
        uint8_t *addr;
        uint32_t val;
    } u;
} fw_imm_t;

struct fw_instr_t;
struct fw_module_context_t;

typedef int32_t (*fw_instr_r_p)(struct fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, struct fw_instr_t *instr);
typedef int32_t (*fw_instr_c_p)(fw_module_func_t *func, fw_str_t *body, struct fw_instr_t *instr);

typedef struct fw_instr_t {
    char      *name;
    uint8_t    code;
    fw_instr_r_p run;
    fw_instr_c_p consume;
} fw_instr_t;

typedef struct {
    fw_module_func_t *func;
    fw_instr_t       *operate;
    fw_imm_t          imm;
} fw_instr_instance_t;


fw_instr_t *fw_get_instr(uint8_t opcode);

#define FW_STACK_COPY(_ctx, _stdst, _stsrc)\
    do {\
        memcpy((_stdst),(_stsrc),sizeof(*_stdst));\
    } while(0);

#define FW_PUSH(_ctx, _type, _val, _end, _private) \
    do {\
        fw_module_stack_t *_cur;\
        if ((_ctx)->pos >= (_ctx)->end) {\
            exit(-1);\
        }\
        _cur = &((_ctx)->stack[(_ctx)->pos]);\
        _cur->type = (_type);\
        _cur->val = (_val);\
        _cur->end = (_end);\
        _cur->private = (_private);\
        (_ctx)->pos++;\
    } while(0);

#define FW_POP(_ctx, _cur, _peek) \
    do {\
        if ((_ctx)->pos <= 0) {\
            exit(-1);\
        }\
        _cur = &((_ctx)->stack[(_ctx)->pos - 1]);\
        if (!(_peek)) (_ctx)->pos--;\
    } while(0);

#define FW_STACK_GET(_ctx, _cur, _idx) \
    do {\
        if ((_ctx)->pos <= (_idx)) {\
            exit(-1);\
        }\
        _cur = &((_ctx)->stack[(_ctx)->pos - (_idx + 1)]);\
    } while(0);
