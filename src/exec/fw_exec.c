#include <fw_core.h>
typedef uint64_t (*funcret)();

uint64_t _fw_module_run_host_func(fw_module_func_t *func, int32_t ret_cnt, int32_t argc, uint64_t *argv, uint64_t *rets)
{
    /*assume the host function will return value
     *for x86/x64 the return value just %eax/%rax
     */
    funcret f = func->host_addr;
    uint64_t ret = 0;

    switch(argc) {
        case 0:
            ret = f();
            break;
        case 1:
            ret = f(argv[0]);
            break;
        case 2:
            ret = f(argv[0], argv[1]);
            break;
        case 3:
            ret = f(argv[0], argv[1], argv[2]);
            break;
        case 4:
            ret = f(argv[0], argv[1], argv[2], argv[3]);
            break;
        case 5:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4]);
            break;
        case 6:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
            break;
        case 7:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
            break;
        case 8:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
            break;
        case 9:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
            break;
        case 10:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]);
            break;
        case 11:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10]);
            break;
        case 12:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11]);
            break;
        case 13:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12]);
            break;
        case 14:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13]);
            break;
        case 15:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14]);
            break;
        case 16:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14], argv[15]);
            break;
        case 17:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14], argv[15], argv[16]);
            break;
        case 18:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14], argv[15], argv[16], argv[17]);
            break;
        case 19:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14], argv[15], argv[16], argv[17], argv[18]);
            break;
        case 20:
            ret = f(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14], argv[15], argv[16], argv[17], argv[18], argv[19]);
            break;
        default:
            printf("Too maany args\n");
            return FW_ERR;
    }

    if (ret_cnt) {
        rets[0] = ret;
    }

    return FW_OK;
}
#if 0
fw_func_locals_t *fw_module_func_local_new(fw_module_func_t *func)
{
    fw_func_locals_t *ret = fw_calloc(sizeof(fw_func_locals_t) + func->local_cnt*sizeof(uint64_t));
    if (!ret) {
        return NULL;
    }
    ret->local_cnt = func->local_cnt;
    ret->local_type = func->local_type;
    return ret;
}
#endif
void fw_module_func_local_free(fw_func_locals_t *locals)
{
    fw_free(locals);
}

int32_t fw_module_run_host_func(fw_module_context_t *ctx, fw_module_func_t *func)
{
    fw_module_stack_t *stack;
    uint32_t i;
    uint64_t ret = FW_OK;
    uint64_t *args = NULL, *rets = NULL;
    fw_module_type_t  *ft = func->functype;
    uint8_t param_cnt = ft->param_cnt, res_cnt = ft->res_cnt;

    if (param_cnt) {
        args = fw_malloc(param_cnt * sizeof(uint64_t));
        if (!args) {
            goto err;
        }
    }

    if (res_cnt) {
        rets = fw_malloc(res_cnt * sizeof(uint64_t));
        if (!rets) {
            goto err;
        }
    }

    /*pop argument*/
    while (param_cnt--) {
        FW_POP(ctx, stack, 0);
        if (stack->type != ft->val[param_cnt]) {
            printf("function parament type not match\n");
            return FW_ERR;
        }
        args[param_cnt] = stack->val;
    }

    if (FW_OK != _fw_module_run_host_func(func, ft->res_cnt, ft->param_cnt, args, rets)) {
        goto err;
    }

    for (i = 0; i < res_cnt; i++) {
        FW_PUSH(ctx,  ft->val[ft->param_cnt], rets[i], NULL, NULL)
    }

ok:
    if (args) {
        fw_free(args);
    }

    if (rets) {
        fw_free(rets);
    }
    return ret;
err:
    ret = FW_ERR;
    goto ok;

}
int32_t _fw_module_run_block(fw_module_context_t *ctx, fw_module_func_t *func)
{
    return FW_OK;
}

int32_t _fw_module_run_func(fw_module_context_t *ctx, fw_module_func_t *func)
{
    int32_t i;
    uint8_t opcode;
    fw_func_locals_t *save_locals, *locals;
    fw_instr_t *instr;
    fw_str_t body = func->body;

    printf("_fw_module_run_func\n");
    /*It's an imported function*/
    if (func->host_addr) {
        printf("run host function\n");
        return fw_module_run_host_func(ctx, func);
    }

    /*init locals*/
    printf("function local %d param %d\n",func->local_cnt,func->functype->param_cnt);
    if (func->local_cnt + func->functype->param_cnt) {
        locals = fw_calloc(sizeof(fw_func_locals_t) + (func->local_cnt + func->functype->param_cnt) * sizeof(uint64_t));
        if (!locals) {
            goto err;
        }
        locals->local_cnt = func->local_cnt + func->functype->param_cnt;
    }
    /*https://www.w3.org/TR/wasm-core-1/#functions%E2%91%A0
    *The index of the first local is the smallest index not referencing a parameter.
    */
    i = func->functype->param_cnt;
    while(i-- > 0) {
        fw_module_stack_t *param;
        FW_STACK_GET(ctx, param, i);
        locals->locals[func->functype->param_cnt - i - 1] = param->val;
    }

    save_locals = ctx->locals;
    ctx->locals = locals;

    /*So the body must be consumed*/
    ctx->frame.end = body.last;
    ctx->frame.stack_deepth = ctx->pos;

    while(body.pos < body.last) {
        printf("run code %p\n", body.pos);

        fw_str_get8(&body, opcode);

        instr = fw_get_instr(opcode);
        if (instr) {
            instr->run(ctx, func, &body, instr);
        } else {
           goto err;
        }
    }

    /*recover locals*/
    if (ctx->locals) {
        fw_free(ctx->locals);
    }

    ctx->locals = save_locals;

    if (body.pos != body.last) {
        goto err;
    }
    printf("run function  end\n");
    return FW_OK;
err:
    if (save_locals) {
        fw_free(save_locals);
    }
str_err:
    printf("err\n");
    return FW_OK;
}

void fw_module_context_free(fw_module_context_t *ctx)
{
    if (ctx->stack) {
        fw_free(ctx->stack);
    }

    fw_free(ctx);
}

fw_module_context_t *fw_module_context_new(fw_module_t *module)
{
    fw_module_context_t *ret;

    ret = fw_calloc(sizeof(fw_module_context_t));
    if (!ret) {
        goto err;
    }

    ret->module = module;
    ret->stack = fw_malloc(FW_MAX_STACK_SIZE * sizeof(fw_module_stack_t));
    if (!ret->stack) {
        goto err;
    }

    ret->pos  = 0;
    ret->end = FW_MAX_STACK_SIZE;

    return ret;

err:
    if (ret) {
        fw_module_context_free(ret);
    }

    return NULL;
}

/*A callable function should be in export section*/
fw_module_func_t *fw_module_get_export_func(fw_module_t *module, char *funcname)
{
    fw_str_t tstr;
    fw_module_export_t *export;
    int32_t i;

    tstr.pos = (uint8_t *)funcname;
    tstr.last = tstr.pos + strlen(funcname);

    for_each_exports(i, module, export)
    {
        if (!fw_str_cmp(export->name, &tstr)) {
            return export->func;
        }
    }

    return NULL;
}

int32_t fw_module_run_func(fw_module_t *module, fw_module_func_t *func, int argc, char **args, fw_ret_t *rets)
{
    fw_module_context_t *ctx;
    fw_module_stack_t  *wrets;

    if (func->functype->param_cnt != argc) {

        printf("func expect %d paraments but %d is provided\n",
            func->functype->param_cnt, argc);

        if (func->functype->param_cnt > argc) {
            return FW_ERR;
        }
    }

    /*
    * used to maintain the context and the globals
    */
    ctx = fw_module_context_new(module);
    if (!ctx) {
        return FW_ERR;
    }

    argc = func->functype->param_cnt;
    while(argc > 0) {
        //printf("push arg %s\n", args[func->functype->param_cnt - argc]);
        FW_PUSH(ctx, FW_I32, atoi(args[func->functype->param_cnt - argc]), NULL, NULL);
        argc--;
    }
    //printf("stack num %d\n", ctx->pos);
    if (FW_OK != _fw_module_run_func(ctx, func)) {
        return FW_ERR;
    }

    /*get ret*/
    if (rets && func->functype->res_cnt) {
        FW_POP(ctx, wrets, 0);
        rets->val = wrets->val;
        printf("wasm ret %d\n", rets->val);
    }
    return FW_OK;
}

int32_t fw_module_run(fw_module_t *module, char *funcname, int argc, char **args, fw_ret_t *rets)
{
    fw_module_func_t *func;

    func = fw_module_get_export_func(module, funcname);
    if (!func) {
        printf("func %s not found in wasm", funcname);
        return FW_ERR;
    }

    return fw_module_run_func(module, func, argc, args, rets);
}

