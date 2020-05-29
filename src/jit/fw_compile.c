#include <fw_core.h>

int32_t fw_module_validate_inst(fw_module_func_t *func)
{
    uint8_t opcode;
    fw_instr_t *instr;
    fw_str_t body = func->body;

    while(body.pos < body.last)
    {
        fw_str_get8(&body, opcode);
        instr = fw_get_instr(opcode);

        if (instr) {
            instr->consume(func, &body, instr);
        }
    }

    if (body.pos != body.last) {
        goto err;
    }

    return FW_OK;
err:
str_err:
    printf("err\n");
    return FW_OK;
}

static int32_t fw_instance_globals(fw_module_t *module, fw_import_t *imports)
{
    //fw_module_instance_t *instance = module->instance;
    return FW_OK;
}
static int32_t fw_instance_funcs(fw_module_t *module, fw_import_t *imports)
{
    /*Adding import.func first*/
    uint32_t i, j;
    fw_module_import_t *inner;
    fw_import_func_t   *host;
    fw_module_instance_t *instance = module->instance;

    instance->funcs = fw_malloc(sizeof(fw_module_func_t) * (module->func_cnt + module->import_cnt[FW_IMPORT_KIND_FUNC]));
    if (!instance->funcs) {
        return FW_ERR;
    }

    for (i = 0; i < module->import_cnt[FW_IMPORT_KIND_FUNC]; i++) {
        /*resolve*/
        inner = &module->imports[FW_IMPORT_KIND_FUNC][i];
        host = imports->funcs.find(&imports->funcs, inner->mname, inner->nsname);
        if (!host) {
            printf("wasm need import %s:%s\n", inner->mname->pos, inner->nsname->pos);
            return FW_ERR;
        }

        if (inner->u.func.typeidx >= module->type_cnt) {
            printf("wasm import %s:%s's type index error(%d)\n",
                inner->mname->pos, inner->nsname->pos, inner->u.func.typeidx);

            return FW_ERR;
        }

        instance->funcs[i] = inner->u.func.f;
        instance->funcs[i]->host_addr = host->ptr;
        instance->funcs[i]->functype = module->type[inner->u.func.typeidx];
    }

    for (j = 0; j < module->func_cnt; j++) {
        instance->funcs[i + j] = &module->funcs[j];
    }

    return FW_OK;
}

int32_t fw_module_compile(fw_module_t *module, fw_import_t *imports)
{
    fw_module_func_t *func;
    fw_module_export_t *export;
    int32_t i;

    if (module->instance) {
        printf("Module has already been compiled\n");
        return FW_ERR;
    }

    module->instance = fw_malloc(sizeof(fw_module_instance_t));
    if (!module->instance) {
        printf("Creating Module instance error\n");
        return FW_ERR;
    }

    for_each_funcs(i, module, func)
    {
        if (FW_OK != fw_module_validate_inst(func)) {
            printf("validate inner func err\n");
            return FW_ERR;
        }
    }

    /*https://www.w3.org/TR/wasm-core-1/#syntax-importdesc
     *
     *Every import defines an index in the respective index space.
     *In each index space, the indices of imports go before the first
     *index of any definition contained in the module itself.
     */
    if (FW_OK != fw_instance_funcs(module, imports)) {
        printf("validate func err\n");
        return FW_ERR;
    }

    if (FW_OK != fw_instance_globals(module, imports)) {
        printf("validate global err\n");
        return FW_ERR;
    }

    for_each_exports(i, module, export)
    {
        if (export->idx >= module->func_cnt + module->import_cnt[FW_IMPORT_KIND_FUNC]) {
            printf("export function idx %d exceed %d\n",
                export->idx, module->func_cnt);
            return FW_ERR;
        }
        export->func = module->instance->funcs[export->idx];
    }
    return FW_OK;
}


/*****************Function*****************/
fw_import_func_t *fw_imports_func_new(fw_import_t *imports, char *mname, char* nsname,
    void *faddr, uint8_t argc, uint8_t retc)
{
    fw_import_func_t *ret = fw_malloc(sizeof(fw_import_func_t));
    if (!ret) {
        return NULL;
    }

    INIT_LIST_HEAD(&ret->node);

    ret->argc = argc;
    ret->retc = retc;
    ret->mname = fw_make_str((uint8_t*)mname, strlen(mname));
    ret->nsname = fw_make_str((uint8_t*)nsname, strlen(nsname));
    ret->ptr = faddr;

    if (FW_OK != imports->funcs.insert(&imports->funcs, ret)) {
        /*must be duplicate*/
    }

    return ret;
}

fw_import_func_t *fw_import_func_wrap_find(struct fw_import_func_wrap_t *funcs, fw_str_t *mname, fw_str_t *nsname)
{
    fw_import_func_t *func;
    uint32_t hash;

    hash = (nsname->pos[0] + mname->pos[0])&FW_MAX_IMPORT_HASH_MASK;

    list_for_each_entry(func, &funcs->funcs[hash], node)
    {
        if (fw_str_cmp(mname, func->mname)) {
            continue;
        }

        if (fw_str_cmp(nsname, func->nsname)) {
            continue;
        }

        return func;
    }

    return NULL;

}

int32_t fw_import_func_wrap_insert(fw_import_func_wrap_t *funcs, fw_import_func_t *func)
{
    /*hash it*/
    uint32_t hash = (func->nsname->pos[0] + func->mname->pos[0])&FW_MAX_IMPORT_HASH_MASK;

    list_add(&func->node, &funcs->funcs[hash]);
    return 0;
}


/*****************Global*****************/
fw_import_glb_t *fw_imports_glb_new(fw_import_t *imports, char *mname, char* nsname, fw_value_t *val)
{
    fw_import_glb_t *ret = fw_malloc(sizeof(fw_import_glb_t));
    if (!ret) {
        return NULL;
    }

    INIT_LIST_HEAD(&ret->node);

    ret->mname = fw_make_str((uint8_t*)mname, strlen(mname));
    ret->nsname = fw_make_str((uint8_t*)nsname, strlen(nsname));
    ret->val = *val;

    /*hash is not used currently*/
    list_add(&ret->node, &imports->glbs.glbs[0]);
    return ret;
}



fw_import_t *fw_imports_wrap_new(fw_module_t *module)
{
    int32_t i;

    fw_import_t *ret = fw_malloc(sizeof(fw_import_t));
    if (!ret) {
        return NULL;
    }

    fw_memset(ret, 0, sizeof(fw_import_t));

    ret->funcs.insert = fw_import_func_wrap_insert;
    ret->funcs.find = fw_import_func_wrap_find;

    for (i = 0; i < FW_MAX_IMPORT_HASH; i++) {
        INIT_LIST_HEAD(&ret->funcs.funcs[i]);
    }

    for (i = 0; i < FW_MAX_IMPORT_HASH; i++) {
        INIT_LIST_HEAD(&ret->tbls.tbls[i]);
    }

    for (i = 0; i < FW_MAX_IMPORT_HASH; i++) {
        INIT_LIST_HEAD(&ret->mems.mems[i]);
    }

    for (i = 0; i < FW_MAX_IMPORT_HASH; i++) {
        INIT_LIST_HEAD(&ret->glbs.glbs[i]);
    }

    ret->add_func = fw_imports_func_new;
    ret->add_global = fw_imports_glb_new;

    return ret;
}

void  fw_imports_wrap_free(fw_import_t *imports)
{
    int32_t i;

    fw_import_func_t *funcs, *tfuncs;
    fw_import_tbl_t  *tbls, *ttbls;
    fw_import_mem_t  *mems, *tmems;
    fw_import_glb_t  *glbs, *tglbs;

    for (i = 0; i < FW_MAX_IMPORT_HASH; i++) {

        list_for_each_entry_safe(funcs, tfuncs, &imports->funcs.funcs[i], node)
        {
            list_del(&funcs->node);
            fw_free(funcs->mname);
            fw_free(funcs->nsname);
            fw_free(funcs);
        }

        list_for_each_entry_safe(tbls, ttbls, &imports->tbls.tbls[i], node)
        {
            list_del(&tbls->node);
            fw_free(tbls->mname);
            fw_free(tbls->nsname);
            fw_free(tbls);
        }

        list_for_each_entry_safe(mems, tmems, &imports->mems.mems[i], node)
        {
            list_del(&mems->node);
            fw_free(mems->mname);
            fw_free(mems->nsname);
            fw_free(mems);
        }


        list_for_each_entry_safe(glbs, tglbs, &imports->glbs.glbs[i], node)
        {
            list_del(&glbs->node);
            fw_free(glbs->mname);
            fw_free(glbs->nsname);
            fw_free(glbs);
        }
    }
}

