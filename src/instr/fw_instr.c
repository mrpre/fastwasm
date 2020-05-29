#include <fw_core.h>

static fw_module_func_block_t *fw_instr_find_blk(fw_module_func_t *func, uint8_t *start)
{
    fw_module_func_block_t *pos;

    list_for_each_entry(pos, &func->blk, node)
    {
        if (pos->start == start) {
            return pos;
        }
    }

    return NULL;
}
static void fw_instr_insert_blk(fw_module_func_t *func, fw_module_func_block_t *blk)
{
    list_add_tail(&blk->node, &func->blk);
}

int32_t fw_instr_consume_64(fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    uint64_t idx;

    fw_str_leb_getu64(body, idx);

    return FW_OK;
str_err:
    return FW_OK;
}

int32_t fw_instr_consume_32(fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    uint32_t idx;

    fw_str_leb_getu32(body, idx);

    return FW_OK;
str_err:
    return FW_OK;
}

int32_t fw_instr_consume_rtype(fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    uint32_t res_type;

    fw_str_leb_getu32(body, res_type);
    return FW_OK;
str_err:
    return FW_ERR;
}

int32_t fw_instr_consume_none(fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    return FW_OK;
}

int32_t fw_instr_consume_brtbl(fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{

    uint32_t blknum, blk;

    fw_str_leb_getu32(body, blknum);

    while(blknum--) {
        fw_str_leb_getu32(body, blk);
    }

    fw_str_leb_getu32(body, blk);

    return FW_OK;
str_err:
    return FW_OK;
}

int32_t fw_instr_consume_block(fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_func_block_t *blk = fw_calloc(sizeof(fw_module_func_block_t));
    if (!blk) {
        return FW_ERR;
    }
    INIT_LIST_HEAD(&blk->node);
    list_add_tail(&blk->node, &func->blktmp);

    if (FW_OK != fw_instr_consume_rtype(func, body, instr)) {
        return FW_ERR;
    }

    blk->start = body->pos;
    blk->type  = instr->code;
    return FW_OK;
}

int32_t fw_instr_consume_if(fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_func_block_t *blk;

    if (FW_OK != fw_instr_consume_block(func, body, instr)) {
        return FW_ERR;
    }

    blk = list_entry(func->blktmp.prev, fw_module_func_block_t, node);
    INIT_LIST_HEAD(&blk->ifnode);
    list_add_tail(&blk->ifnode, &func->iftmp);

    return FW_OK;
}

int32_t fw_instr_consume_else(fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_func_block_t *blk;

    /*the else block belongs to latest if block*/
    blk = list_entry(func->iftmp.prev, fw_module_func_block_t, ifnode);
    list_del(&blk->ifnode);
    blk->els = body->pos;
    return FW_OK;
}

int32_t fw_instr_consume_end(fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_func_block_t *blk;

    if (list_empty(&func->blktmp)) {
        if (body->pos != body->last) {
            printf("unexcepted end of en function\n");
            return FW_ERR;
        }
        return FW_OK;
    }

    /*get the last added block*/
    blk = list_entry(func->blktmp.prev, fw_module_func_block_t, node);
    list_del(&blk->node);
    blk->end = body->pos;
    if (!blk->els) {
        /*if no else block*/
        blk->els = blk->end;
    }

    fw_instr_insert_blk(func, blk);

    return FW_OK;
}

int32_t fw_instr_run_loop(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    uint32_t res_type;

    /*use to br*/
    uint8_t *end_addr = body->pos;

    fw_str_leb_getu32(body, res_type);

    FW_PUSH(ctx, FW_INSTR_LOOP, res_type, end_addr, NULL);

str_err:
    return FW_ERR;
}

int32_t fw_instr_run_block(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    uint32_t res_type;
    fw_module_func_block_t *blk;

    fw_str_leb_getu32(body, res_type);

    blk = fw_instr_find_blk(func, body->pos);
    if (!blk) {
        printf("can't find blk\n");
        return FW_ERR;
    }

    ctx->current = blk;

    FW_PUSH(ctx, FW_INSTR_BLOCK, res_type, blk->end, (void*)blk);

str_err:
    return FW_ERR;
}

int32_t fw_instr_run_call(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    uint32_t idx;
    fw_module_func_t *ifunc;

    fw_str_leb_getu32(body, idx);

    ifunc = ctx->module->instance->funcs[idx];

    if (FW_OK != _fw_module_run_func(ctx, ifunc)) {
        printf("run call err\n");
        return FW_ERR;
    }

    return FW_OK;
str_err:
    printf("run_call str err\n");
    return FW_ERR;
}

int32_t fw_instr_run_drop(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_stack_t *drop;

    FW_POP(ctx, drop, 0);
    (void)drop;
    return FW_OK;
}

/*https://www.w3.org/TR/wasm-core-1/#-hrefsyntax-instr-parametricmathsfselect%E2%91%A0*/
int32_t fw_instr_run_select(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_stack_t *cond;
    fw_module_stack_t *val1;
    fw_module_stack_t *val2;

    FW_POP(ctx, cond, 0);
    FW_POP(ctx, val2, 0);
    FW_POP(ctx, val1, 0);

    //printf("select cond %ld\n", cond->val);
    if (cond->val != 0) {
    //    printf("select %ld\n", val1->val);
        FW_PUSH(ctx, val1->type, val1->val, val1->end, val1->private);
    } else {
    //    printf("select %ld\n", val2->val);
        FW_PUSH(ctx, val2->type, val2->val, val2->end, val2->private);
    }
    return FW_OK;
}

int32_t fw_instr_run_i32_const(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    uint32_t val;

    fw_str_leb_getu32(body, val);

    /*get but not pop the value from stack*/
    FW_PUSH(ctx, FW_I32, val, NULL, NULL);

    return FW_OK;
str_err:
    return FW_ERR;
}

int32_t fw_instr_run_f32_const(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    uint32_t val;

    fw_str_leb_getu32(body, val);

    /*get but not pop the value from stack*/
    FW_PUSH(ctx, FW_F32, val, NULL, NULL);

    return FW_OK;
str_err:
    return FW_ERR;
}


int32_t fw_instr_run_i64_const(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    uint64_t val;

    fw_str_leb_getu64(body, val);

    /*get but not pop the value from stack*/
    FW_PUSH(ctx, FW_I64, val, NULL, NULL);

    return FW_OK;
str_err:
    return FW_ERR;
}


int32_t fw_instr_run_f64_const(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    uint64_t val;

    fw_str_leb_getu64(body, val);

    /*get but not pop the value from stack*/
    FW_PUSH(ctx, FW_F64, val, NULL, NULL);

    return FW_OK;
str_err:
    return FW_ERR;
}

int32_t fw_instr_run_i32_ne(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_stack_t *c2, *c1;

    FW_POP(ctx, c2, 0);
    FW_POP(ctx, c1, 0);

    if (c1->type != FW_I32
        || c2->type != FW_I32) {
        printf("i32.ne type error\n");
        return FW_ERR;
    }

    if (c1->val != c2->val) {
        FW_PUSH(ctx, FW_I32, 1, NULL, NULL);
    } else {
        FW_PUSH(ctx, FW_I32, 0, NULL, NULL);
    }
    return FW_OK;
}


int32_t fw_instr_run_end(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    /*should not function end*/
    return FW_OK;
}

/*https://www.w3.org/TR/wasm-core-1/#-hrefsyntax-instr-controlmathsfreturn%E2%91%A0*/
int32_t fw_instr_run_return(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    /*
    *
    *stack_deepth-> local a
    *               local b
    *               ret1
    *               ret2
    *pos->
    */


    int32_t oldpos = ctx->frame.stack_deepth;
    int32_t i = ctx->pos - func->functype->res_cnt;

    /*move rets ahead*/
    if (i != oldpos) {
        while(i < ctx->pos) {
            FW_STACK_COPY(ctx, &ctx->stack[oldpos], &ctx->stack[i]);
            oldpos++;
            i++;
        }
        ctx->pos = oldpos;
    }

    /*mark function end*/
    body->pos = ctx->frame.end;
    return FW_OK;
}

int32_t fw_instr_run_else(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_stack_t *st;
    fw_module_func_block_t *blk;

    FW_POP(ctx, st, 0);

    if (FW_INSTR_IF != st->type) {
        printf("else without if\n");
        return FW_ERR;
    }

    blk = st->private;

    body->pos = blk->end;
    return FW_OK;
}

int32_t fw_instr_run_if(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_stack_t *cond;
    fw_module_func_block_t *blk;
    fw_str_t blockbody;
    fw_instr_t *blockinstr;

    uint32_t res_type;
    uint8_t opcode;

    fw_str_leb_getu32(body, res_type);
    blk = fw_instr_find_blk(func, body->pos);
    if (!blk) {
        printf("can't find if blk\n");
        return FW_ERR;
    }

    ctx->current = blk;

    FW_POP(ctx, cond, 0);

    if (cond->type != FW_I32) {
        printf("stack cond is not i32 while run if [%d]\n", cond->type);
        return FW_ERR;
    }

    if (body->pos != blk->start) {
        printf("if block start doesn't match %p %p\n",blk->start, body->pos);
        return FW_ERR;
    }

    /*run if or else block*/
    if (cond->val) {
        blockbody.pos   = blk->start;
        blockbody.last   = blk->els - 1;
    } else {
        blockbody.pos   = blk->els;
        blockbody.last   = blk->end;
    }

    while(blockbody.pos < blockbody.last)
    {
        fw_str_get8(&blockbody, opcode);

        blockinstr = fw_get_instr(opcode);
        if (blockinstr) {
            blockinstr->run(ctx, func, &blockbody, blockinstr);
        } else {
            printf("run if  block error\n");
            return FW_ERR;
        }
    }

    if (blockbody.pos > blockbody.last) {
        body->pos = blockbody.pos;
        return FW_OK;
    }
    body->pos = blk->end;

#if 0
    if (cond->val) {
        /*run if-block*/
        body->pos = blk->start;
        printf("run if bolck %p\n",blk->end);
    } else {
        if (blk->els) {
            printf("fw_instr_run_if else\n");
            /*run else-block*/
            body->pos = blk->els;
            FW_PUSH(ctx, FW_INSTR_IF, res_type, blk->end, (void*)blk);
        } else {
            /*just end of block*/
            body->pos = blk->end;
        }
    }
#endif
    return FW_OK;
str_err:
    return FW_ERR;
}

int32_t fw_instr_run_local_tee(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_stack_t *cur;
    uint32_t           idx;

    fw_str_leb_getu32(body, idx);

    /*get but not pop the value from stack*/
    FW_POP(ctx, cur, 1);

    ctx->locals->locals[idx] = cur->val;
    return FW_OK;
str_err:
    return FW_ERR;
}

int32_t fw_instr_run_local_get(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    /*get value from stack then pop it to stack*/
    uint32_t           idx;

    /*get x arg*/
    fw_str_leb_getu32(body, idx);

    /*push it to stack*/
    //FW_PUSH(ctx, FW_INSTR_LOCAL_GET, ctx->locals->locals[idx], NULL, NULL);
    FW_PUSH(ctx, FW_I32, ctx->locals->locals[idx], NULL, NULL);
    return FW_OK;
str_err:
    return FW_ERR;
}

int32_t fw_instr_run_local_set(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    /*get value from stack then pop it to stack*/
    uint32_t           idx;
    fw_module_stack_t *cur;

    FW_POP(ctx, cur, 0);

    /*get x arg*/
    fw_str_leb_getu32(body, idx);

    /*push it to stack*/
    ctx->locals->locals[idx] = cur->val;

    return FW_OK;
str_err:
    return FW_ERR;
}

int32_t fw_instr_run_i32_eqz(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_stack_t *c1;

    FW_POP(ctx, c1, 0);

    if (c1->val == 0) {
        FW_PUSH(ctx, FW_I32, 1, NULL, NULL);
    } else {
        FW_PUSH(ctx, FW_I32, 0, NULL, NULL);
    }

    return FW_OK;
}

int32_t fw_instr_run_i32_eq(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_stack_t *c1, *c2;

    FW_POP(ctx, c2, 0);
    FW_POP(ctx, c1, 0);

    if (c2->val == c1->val) {
        FW_PUSH(ctx, FW_I32, 1, NULL, NULL);
    } else {
        FW_PUSH(ctx, FW_I32, 0, NULL, NULL);
    }

    return FW_OK;
}

/*https://www.w3.org/TR/wasm-core-1/#-tmathsfhrefsyntax-relopmathitrelop%E2%91%A0*/
int32_t fw_instr_run_gt_s(fw_module_context_t *ctx, fw_module_func_t *func, fw_str_t *body, fw_instr_t *instr)
{
    fw_module_stack_t *c2, *c1;

    FW_POP(ctx, c2, 0);
    FW_POP(ctx, c1, 0);

    if (c1->val > c2->val) {
        FW_PUSH(ctx, FW_I32, 1, NULL, NULL);
    } else {
        FW_PUSH(ctx, FW_I32, 0, NULL, NULL);
    }
    return FW_OK;
}


fw_instr_t common_instr_handler[FW_INSTR_MAX] =
{
    [FW_INSTR_UNREACHABLE] = {"unreachable", FW_INSTR_UNREACHABLE, NULL, fw_instr_consume_none},
    [FW_INSTR_BLOCK] = {"block", FW_INSTR_BLOCK, fw_instr_run_block, fw_instr_consume_block},
    [FW_INSTR_LOOP] = {"loop", FW_INSTR_LOOP, fw_instr_run_loop, fw_instr_consume_block},
    [FW_INSTR_IF] = {"if", FW_INSTR_IF, fw_instr_run_if, fw_instr_consume_if},
    [FW_INSTR_ELSE] = {"else", FW_INSTR_ELSE, fw_instr_run_else, fw_instr_consume_else},
    [FW_INSTR_END] = {"end", FW_INSTR_END, fw_instr_run_end, fw_instr_consume_end},
    [FW_INSTR_BR] = {"br", FW_INSTR_BR, NULL, fw_instr_consume_32},
    [FW_INSTR_BR_IF] = {"br_if", FW_INSTR_BR_IF, NULL, fw_instr_consume_32},
    [FW_INSTR_BR_TBL] = {"br_table", FW_INSTR_BR_TBL, NULL, fw_instr_consume_brtbl},
    [FW_INSTR_RET] = {"ret", FW_INSTR_RET, fw_instr_run_return, fw_instr_consume_none},
    [FW_INSTR_CALL] = {"call", FW_INSTR_CALL, fw_instr_run_call, fw_instr_consume_32},
    [FW_INSTR_CALL_INDIRECT] = {"call_indirect", FW_INSTR_CALL_INDIRECT, NULL, fw_instr_consume_32},
    [FW_INSTR_DROP] = {"drop", FW_INSTR_DROP, fw_instr_run_drop, fw_instr_consume_none},
    [FW_INSTR_SELECT] = {"select", FW_INSTR_SELECT, fw_instr_run_select, fw_instr_consume_none},
    [FW_INSTR_LOCAL_GET] = {"local.get", FW_INSTR_LOCAL_GET, fw_instr_run_local_get, fw_instr_consume_32},
    [FW_INSTR_LOCAL_SET] = {"local.set", FW_INSTR_LOCAL_SET, fw_instr_run_local_set, fw_instr_consume_32},
    [FW_INSTR_LOCAL_TEE] = {"local.tee", FW_INSTR_LOCAL_TEE, fw_instr_run_local_tee, fw_instr_consume_32},
    [FW_INSTR_GLOBAL_GET] = {"global.get", FW_INSTR_GLOBAL_GET, NULL, fw_instr_consume_32},
    [FW_INSTR_GLOBAL_SET] = {"global.set", FW_INSTR_GLOBAL_SET, NULL, fw_instr_consume_32},

/*memarg*/
    [FW_INSTR_I32_LOAD]  = {"i32.load", FW_INSTR_I32_LOAD, NULL, fw_instr_consume_32},
    [FW_INSTR_I32_CONST] = {"i32.const", FW_INSTR_I32_CONST, fw_instr_run_i32_const, fw_instr_consume_32},
    [FW_INSTR_I64_CONST] = {"i64.const", FW_INSTR_I64_CONST, fw_instr_run_i64_const, fw_instr_consume_64},
    [FW_INSTR_F32_CONST] = {"f32.const", FW_INSTR_F32_CONST, fw_instr_run_f32_const, fw_instr_consume_32},
    [FW_INSTR_F64_CONST] = {"f64.const", FW_INSTR_F64_CONST, fw_instr_run_f64_const, fw_instr_consume_64},

    [FW_INSTR_I32_EQZ] = {"i32.eqz", FW_INSTR_I32_EQZ, fw_instr_run_i32_eqz, fw_instr_consume_none},
    [FW_INSTR_I32_EQ] = {"i32.eq", FW_INSTR_I32_EQ, fw_instr_run_i32_eq, fw_instr_consume_none},
    [FW_INSTR_I32_NE] = {"i32.ne", FW_INSTR_I32_NE, fw_instr_run_i32_ne, fw_instr_consume_none},
    [FW_INSTR_I32_LT_S] = {"i32.lt_s", FW_INSTR_I32_LT_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_LT_U] = {"i32.lt_u", FW_INSTR_I32_LT_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_GT_S] = {"i32.gt_s", FW_INSTR_I32_GT_S, fw_instr_run_gt_s, fw_instr_consume_none},
    [FW_INSTR_I32_GT_U] = {"i32.gt_u", FW_INSTR_I32_GT_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_LE_S] = {"i32.le_s", FW_INSTR_I32_LE_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_LE_U] = {"i32.le_u", FW_INSTR_I32_LE_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_GE_S] = {"i32.ge_s", FW_INSTR_I32_GE_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_GE_U] = {"i32.ge_u", FW_INSTR_I32_GE_U, NULL, fw_instr_consume_none},


    [FW_INSTR_I64_EQZ] = {"i64.eqz", FW_INSTR_I64_EQZ, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_EQ] = {"i64.eq", FW_INSTR_I64_EQ, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_NE] = {"i64.ne", FW_INSTR_I64_NE, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_LT_S] = {"i64.lt_s", FW_INSTR_I64_LT_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_LT_U] = {"i64.lt_u", FW_INSTR_I64_LT_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_GT_S] = {"i64.gt_s", FW_INSTR_I64_GT_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_GT_U] = {"i64.gt_u", FW_INSTR_I64_GT_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_LE_S] = {"i64.le_s", FW_INSTR_I64_LE_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_LE_U] = {"i64.le_u", FW_INSTR_I64_LE_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_GE_S] = {"i64.ge_s", FW_INSTR_I64_GE_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_GE_U] = {"i64.ge_u", FW_INSTR_I64_GE_U, NULL, fw_instr_consume_none},

    [FW_INSTR_F32_EQ] = {"f32.eq", FW_INSTR_F32_EQ, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_NE] = {"f32.ne", FW_INSTR_F32_NE, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_LT] = {"f32.lt", FW_INSTR_F32_LT, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_GT] = {"f32.gt", FW_INSTR_F32_GT, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_LE] = {"f32.le", FW_INSTR_F32_LE, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_GE] = {"f32.ge", FW_INSTR_F32_GE, NULL, fw_instr_consume_none},

    [FW_INSTR_F64_EQ] = {"f64.eq", FW_INSTR_F64_EQ, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_NE] = {"f64.ne", FW_INSTR_F64_NE, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_LT] = {"f64.lt", FW_INSTR_F64_LT, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_GT] = {"f64.gt", FW_INSTR_F64_GT, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_LE] = {"f64.le", FW_INSTR_F64_LE, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_GE] = {"f64.ge", FW_INSTR_F64_GE, NULL, fw_instr_consume_none},

    [FW_INSTR_I32_CLZ] = {"i32.clz", FW_INSTR_I32_CLZ, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_CTZ] = {"i32.ctz", FW_INSTR_I32_CTZ, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_POPCNT] = {"i32.popcnt", FW_INSTR_I32_POPCNT, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_ADD] = {"i32.add", FW_INSTR_I32_ADD, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_SUB] = {"i32.sub", FW_INSTR_I32_SUB, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_MUL] = {"i32.mul", FW_INSTR_I32_MUL, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_DIV_S] = {"i32.div_s", FW_INSTR_I32_DIV_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_DIV_U] = {"i32.div_u", FW_INSTR_I32_DIV_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_REM_S] = {"i32.rem_s", FW_INSTR_I32_REM_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_REM_U] = {"i32.rem_u", FW_INSTR_I32_REM_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_AND] = {"i32.and", FW_INSTR_I32_AND, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_OR] = {"i32.or", FW_INSTR_I32_OR, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_XOR] = {"i32.xor", FW_INSTR_I32_XOR, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_SHL] = {"i32.shl", FW_INSTR_I32_SHL, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_SHR_S] = {"i32.shr_s", FW_INSTR_I32_SHR_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_SHR_U] = {"i32.shr_u", FW_INSTR_I32_SHR_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_ROTL] = {"i32.rotl", FW_INSTR_I32_ROTL, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_ROTR] = {"i32.rotr", FW_INSTR_I32_ROTR, NULL, fw_instr_consume_none},

    [FW_INSTR_I64_CLZ] = {"i64.clz", FW_INSTR_I64_CLZ, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_CTZ] = {"i64.ctz", FW_INSTR_I64_CTZ, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_POPCNT] = {"i64.popcnt", FW_INSTR_I64_POPCNT, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_ADD] = {"i64.add", FW_INSTR_I64_ADD, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_SUB] = {"i64.sub", FW_INSTR_I64_SUB, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_MUL] = {"i64.mul", FW_INSTR_I64_MUL, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_DIV_S] = {"i64.div_s", FW_INSTR_I64_DIV_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_DIV_U] = {"i64.div_u", FW_INSTR_I64_DIV_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_REM_S] = {"i64.rem_s", FW_INSTR_I64_REM_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_REM_U] = {"i64.rem_u", FW_INSTR_I64_REM_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_AND] = {"i64.and", FW_INSTR_I64_AND, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_OR] = {"i64.or", FW_INSTR_I64_OR, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_XOR] = {"i64.xor", FW_INSTR_I64_XOR, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_SHL] = {"i64.shl", FW_INSTR_I64_SHL, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_SHR_S] = {"i64.shr_s", FW_INSTR_I64_SHR_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_SHR_U] = {"i64.shr_u", FW_INSTR_I64_SHR_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_ROTL] = {"i64.rotl", FW_INSTR_I64_ROTL, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_ROTR] = {"i64.rotr", FW_INSTR_I64_ROTR, NULL, fw_instr_consume_none},

    [FW_INSTR_F32_ABS] = {"f32.abs", FW_INSTR_F32_ABS, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_NEG] = {"f32.neg", FW_INSTR_F32_NEG, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_CEIL] = {"f32.ceil", FW_INSTR_F32_CEIL, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_FLOOR] = {"f32.floor", FW_INSTR_F32_FLOOR, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_TRUNC] = {"f32.trunc", FW_INSTR_F32_TRUNC, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_NEAREST] = {"f32.nearest", FW_INSTR_F32_NEAREST, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_SQRT] = {"f32.sqrt", FW_INSTR_F32_SQRT, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_ADD] = {"f32.add", FW_INSTR_F32_ADD, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_SUB] = {"f32.sub", FW_INSTR_F32_SUB, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_MUL] = {"f32.mul", FW_INSTR_F32_MUL, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_DIV] = {"f32.div", FW_INSTR_F32_DIV, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_MIN] = {"f32.min", FW_INSTR_F32_MIN, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_MAX] = {"f32.max", FW_INSTR_F32_MAX, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_COPYSIGN] = {"f32.copysign", FW_INSTR_F32_COPYSIGN, NULL, fw_instr_consume_none},

    [FW_INSTR_F64_ABS] = {"f64.abs", FW_INSTR_F64_ABS, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_NEG] = {"f64.neg", FW_INSTR_F64_NEG, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_CEIL] = {"f64.ceil", FW_INSTR_F64_CEIL, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_FLOOR] = {"f64.floor", FW_INSTR_F64_FLOOR, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_TRUNC] = {"f64.trunc", FW_INSTR_F64_TRUNC, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_NEAREST] = {"f64.nearest", FW_INSTR_F64_NEAREST, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_SQRT] = {"f64.sqrt", FW_INSTR_F64_SQRT, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_ADD] = {"f64.add", FW_INSTR_F64_ADD, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_SUB] = {"f64.sub", FW_INSTR_F64_SUB, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_MUL] = {"f64.mul", FW_INSTR_F64_MUL, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_DIV] = {"f64.div", FW_INSTR_F64_DIV, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_MIN] = {"f64.min", FW_INSTR_F64_MIN, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_MAX] = {"f64.max", FW_INSTR_F64_MAX, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_COPYSIGN] = {"f64.copysign", FW_INSTR_F64_COPYSIGN, NULL, fw_instr_consume_none},

    [FW_INSTR_I32_WRAP_I64] = {"i32.wrap_i64", FW_INSTR_I32_WRAP_I64, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_TRUNC_F32_S] = {"i32.trunc_f32_s", FW_INSTR_I32_TRUNC_F32_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_TRUNC_F32_U] = {"i32.trunc_f32_u", FW_INSTR_I32_TRUNC_F32_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_TRUNC_F64_S] = {"i32.trunc_f64_s", FW_INSTR_I32_TRUNC_F64_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_TRUNC_F64_U] = {"i32.trunc_f64_u", FW_INSTR_I32_TRUNC_F64_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_EXTEND_I32_S] = {"i64.extend_i32_s", FW_INSTR_I64_EXTEND_I32_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_EXTEND_I32_U] = {"i64.extend_i32_u", FW_INSTR_I64_EXTEND_I32_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_TRUNC_F32_S] = {"i64.trunc_f32_s", FW_INSTR_I64_TRUNC_F32_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_TRUNC_F32_U] = {"i64.extend_f32_u", FW_INSTR_I64_TRUNC_F32_U, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_TRUNC_F64_S] = {"i64.extend_f64_s", FW_INSTR_I64_TRUNC_F64_S, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_TRUNC_F64_U] = {"i64.extend_f64_u", FW_INSTR_I64_TRUNC_F64_U, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_CONVERT_I32_S] = {"f32.convert_i32_s", FW_INSTR_F32_CONVERT_I32_S, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_CONVERT_I32_U] = {"f32.convert_i32_u", FW_INSTR_F32_CONVERT_I32_U, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_CONVERT_I64_S] = {"f32.convert_i64_s", FW_INSTR_F32_CONVERT_I64_S, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_CONVERT_I64_U] = {"f32.convert_i64_u", FW_INSTR_F32_CONVERT_I64_U, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_DEMOTE_F64] = {"f32.demote_f64", FW_INSTR_F32_DEMOTE_F64, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_CONVERT_I32_S] = {"f64.convert_i32_s", FW_INSTR_F64_CONVERT_I32_S, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_CONVERT_I32_U] = {"f64.convert_i32_u", FW_INSTR_F64_CONVERT_I32_U, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_CONVERT_I64_S] = {"f64.convert_i64_s", FW_INSTR_F64_CONVERT_I64_S, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_CONVERT_I64_U] = {"f64.convert_i64_u", FW_INSTR_F64_CONVERT_I64_U, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_PROMOTE_F32] = {"f64.promote_f32", FW_INSTR_F64_PROMOTE_F32, NULL, fw_instr_consume_none},
    [FW_INSTR_I32_REINTERPRET_F32] = {"i32.preiniterpret_f32", FW_INSTR_I32_REINTERPRET_F32, NULL, fw_instr_consume_none},
    [FW_INSTR_I64_REINTERPRET_F64] = {"i64.preiniterpret_f64", FW_INSTR_I64_REINTERPRET_F64, NULL, fw_instr_consume_none},
    [FW_INSTR_F32_REINTERPRET_I32] = {"f32.preiniterpret_i32", FW_INSTR_F32_REINTERPRET_I32, NULL, fw_instr_consume_none},
    [FW_INSTR_F64_REINTERPRET_I64] = {"f64.preiniterpret_i64", FW_INSTR_F64_REINTERPRET_I64, NULL, fw_instr_consume_none},
};


fw_instr_t *fw_get_instr(uint8_t opcode)
{
    if (opcode >= FW_INSTR_MAX) {
        return NULL;
    }

    return &common_instr_handler[opcode];
}


