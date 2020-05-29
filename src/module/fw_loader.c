#include <fw_core.h>
#include <fw_module.h>


/*
 *https://www.w3.org/TR/wasm-core-1/#numeric-instructions%E2%91%A6
 */
static int32_t fw_module_parse_expr(fw_str_t *data)
{
    uint8_t literal, iend;
    uint32_t vi32;
    uint64_t vi64;

    fw_str_get8(data, literal);
    switch (literal) {
        case 0x41:
            fw_str_leb_geti32(data, vi32);
        break;
        case 0x42:
            fw_str_leb_geti64(data, vi64);
        break;
        case 0x43:
            fw_str_pass(data, 4);
        break;
        case 0x44:
            fw_str_pass(data, 8);
        break;
        case 0x23:
            fw_str_leb_geti32(data, vi32);
        break;
        default:
            goto err;
    }

    fw_str_get8(data, iend);
    if (iend != 0x0b) {
        goto err;
    }
    return FW_OK;
str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}

void fw_module_free_imports_func(struct fw_module_t *module, struct fw_module_import_t *import)
{
    fw_free(import->u.func.f);
}

void fw_module_section_func_free(struct fw_module_t *module, struct fw_module_section_t *section)
{
    uint32_t i;
    fw_module_func_t *func;
    fw_module_func_block_t *blk;

    for (i = 0; i < module->func_cnt; i++) {
        func = &module->funcs[i];
        if (func->local_type) {
            fw_free(func->local_type);
        }

        if (func->locals) {
            fw_free(func->locals);
        }

        while(!list_empty(&func->blk)) {
            blk = list_entry(func->blk.next, fw_module_func_block_t, node);
            list_del(&blk->node);
            fw_free(blk);
        }
    }

    fw_free(module->funcs);
}

void fw_module_section_imports_free(struct fw_module_t *module, struct fw_module_section_t *section)
{
    uint32_t i, j;
    fw_module_import_t *import;

    for (i = 0; i < FW_IMPORT_KIND_MAX; i++) {

        import = module->imports[i];

        for (j = 0; j < module->import_cnt[i]; j++) {
            if (import[j].free) {
                import[j].free(module, &import[j]);
            }
            fw_free(import[j].mname);
            fw_free(import[j].nsname);
        }

        if (import) {
            fw_free(import);
        }
        module->imports[i] = NULL;

    }
}

void fw_module_section_table_free(struct fw_module_t *module, struct fw_module_section_t *section)
{
    if (module->tables) {
        fw_free(module->tables);
    }
}

void fw_module_section_memory_free(struct fw_module_t *module, struct fw_module_section_t *section)
{
    if (module->memories) {
        fw_free(module->memories);
    }
}

void fw_module_section_global_free(struct fw_module_t *module, struct fw_module_section_t *section)
{
    if (module->globals) {
        fw_free(module->globals);
    }
}

void fw_module_section_export_free(struct fw_module_t *module, struct fw_module_section_t *section)
{
    uint32_t export_cnt, i;

    export_cnt = module->export_cnt;

    for (i = 0; i < export_cnt; i++) {
        fw_free(module->exports[i].name);
    }

    if (module->exports) {
        fw_free(module->exports);
    }
}

void fw_module_section_elem_free(struct fw_module_t *module, struct fw_module_section_t *section)
{
    uint32_t func_cnt, i;

    func_cnt = module->func_cnt;

    for (i = 0; i < func_cnt; i++) {
        fw_free(module->elems[i].funcs);
    }

    if (module->elems) {
        fw_free(module->elems);
    }
}
void fw_module_section_data_free(struct fw_module_t *module, struct fw_module_section_t *section)
{
    if (module->datas) {
        fw_free(module->datas);
    }
}


/*
 *https://www.w3.org/TR/wasm-core-1/#binary-datasec
 */
FW_DECLARE_SECTION_METHOD(data)
{
    uint32_t data_cnt, i;

    fw_module_data_t *datas;

    fw_str_leb_getu32(&section->data, data_cnt);

    if (data_cnt) {
        datas = fw_malloc(sizeof(fw_module_data_t) * data_cnt);
        if (!datas) {
            goto err;
        }

        module->data_cnt = data_cnt;
        module->datas = datas;
        section->free  = fw_module_section_data_free;

        for (i = 0; i < data_cnt; i++, datas++) {
            fw_str_leb_getu32(&section->data, datas->mem_idx);

            if (FW_OK != fw_module_parse_expr(&section->data)) {
                goto err;
            }

            fw_str_leb_getu32(&section->data, datas->data_len);
            datas->data = section->data.pos;
            fw_str_pass(&section->data, datas->data_len);
        }

    }

    /*All data in section should be consumed*/
    if (section->data.pos != section->data.last) {
        goto err;
    }

    return FW_OK;
str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}

/*
 *https://www.w3.org/TR/wasm-core-1/#binary-elemsec
 */
FW_DECLARE_SECTION_METHOD(elem)
{
    uint32_t elem_cnt, i, j;
    uint32_t table_idx, func_cnt, func_idx;
    fw_module_elem_t *elems;

    fw_str_leb_getu32(&section->data, elem_cnt);

    if (elem_cnt) {
        elems = fw_malloc(sizeof(fw_module_elem_t) * elem_cnt);
        if (!elems) {
            goto err;
        }

        module->elem_cnt = elem_cnt;
        module->elems = elems;
        section->free  = fw_module_section_elem_free;

        for (i = 0; i < elem_cnt; i++, elems++) {
            fw_str_leb_getu32(&section->data, table_idx);
            elems->table_idx = table_idx;
            if (FW_OK != fw_module_parse_expr(&section->data)) {
                goto err;
            }

            fw_str_leb_getu32(&section->data, func_cnt);
            elems->func_cnt = func_cnt;
            if (func_cnt) {
                elems->funcs = fw_malloc(sizeof(uint32_t) * func_cnt);
                if (!elems->funcs) {
                    goto err;
                }

                fw_str_leb_getu32(&section->data, func_idx);
                for (j = 0; j < func_cnt; j++) {
                    elems->funcs[j] = func_idx;
                }
            }
        }
    }

    /*All data in section should be consumed*/
    if (section->data.pos != section->data.last) {
        goto err;
    }

    return FW_OK;
str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}

/*
 *https://www.w3.org/TR/wasm-core-1/#binary-startsec
 */
FW_DECLARE_SECTION_METHOD(start)
{
    uint32_t func_idx;
    fw_str_leb_getu32(&section->data, func_idx);

    if (func_idx) {
        if (func_idx > module->func_cnt/* + module->import_func_cnt*/) {
            goto err;
        }
        module->start_func = func_idx;
    }

    /*All data in section should be consumed*/
    if (section->data.pos != section->data.last) {
        goto err;
    }

    return FW_OK;
str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}

/*
 *https://www.w3.org/TR/wasm-core-1/#export-section%E2%91%A0
 */
FW_DECLARE_SECTION_METHOD(export)
{
    uint32_t export_cnt, i;
    uint32_t name_len;
    fw_module_export_t *exports;

    fw_str_leb_getu32(&section->data, export_cnt);
    if (export_cnt) {

        exports = fw_malloc(sizeof(fw_module_export_t) * export_cnt);
        if (!exports) {
            goto err;
        }

        module->export_cnt = export_cnt;
        module->exports = exports;
        section->free  = fw_module_section_export_free;

        for (i = 0; i < export_cnt; i++, exports++) {
            /*name exportdesc*/
            fw_str_leb_getu32(&section->data, name_len);
            exports->name = fw_make_str(section->data.pos, name_len);
            fw_str_pass(&section->data, name_len);

            /*kind*/
            fw_str_get8(&section->data, exports->kind);
            /*idx*/
            fw_str_leb_getu32(&section->data, exports->idx);

            switch (exports->kind) {
                case FW_IMPORT_KIND_FUNC:
                case FW_IMPORT_KIND_TABLE:
                case FW_IMPORT_KIND_MEMORY:
                case FW_IMPORT_KIND_GLOBAL:
                default:
                break;
            }

        }
    }
    /*All data in section should be consumed*/
    if (section->data.pos != section->data.last) {
        goto err;
    }

    return FW_OK;
str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}

/*
 *https://www.w3.org/TR/wasm-core-1/#binary-globalsec
 */
FW_DECLARE_SECTION_METHOD(global)
{
    uint32_t global_cnt, i;
    fw_module_global_t *globals;

    fw_str_leb_getu32(&section->data, global_cnt);

    if (global_cnt) {

        globals = fw_malloc(sizeof(fw_module_global_t) * global_cnt);
        if (!globals) {
            goto err;
        }

        module->global_cnt = global_cnt;
        module->globals = globals;
        section->free  = fw_module_section_global_free;

        for (i = 0; i < global_cnt; i++, globals++) {
            fw_str_leb_getu8(&section->data, globals->type);
            fw_str_leb_getu8(&section->data, globals->mutable);

            /*
             *each global is initialized with an init value
             *given by a constant initializer expression
             */
             if(FW_OK != fw_module_parse_expr(&section->data))
                 goto err;
        }
    }

    /*All data in section should be consumed*/
    if (section->data.pos != section->data.last) {
        goto err;
    }

    return FW_OK;

str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}

FW_DECLARE_SECTION_METHOD(memory)
{
    uint32_t memory_cnt, limits, i;
    fw_module_memory_t *memories;

    fw_str_leb_getu32(&section->data, memory_cnt);

    if (memory_cnt) {
        module->memory_cnt = memory_cnt;

        memories = fw_malloc(sizeof(fw_module_memory_t) * memory_cnt);
        if (memories) {
            goto err;
        }
        module->memories = memories;
        section->free  = fw_module_section_memory_free;

        for (i = 0; i < memory_cnt; i++, memories++) {

            fw_str_leb_getu8(&section->data, limits);
            fw_str_leb_getu32(&section->data, memories->min);
            if (limits == 0) {
                fw_str_leb_getu32(&section->data, memories->max);
            } else {
                memories->max = 0x100;
            }
        }
    }

    /*All data in section should be consumed*/
    if (section->data.pos != section->data.last) {
        goto err;
    }

    return FW_OK;

str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}

/*
 *https://www.w3.org/TR/wasm-core-1/#binary-tablesec
 */
FW_DECLARE_SECTION_METHOD(table)
{
    uint8_t  type;
    uint32_t table_cnt, limits, i;
    fw_module_table_t *tables;

    fw_str_leb_getu32(&section->data, table_cnt);

    if (table_cnt) {
        module->table_cnt = table_cnt;
        tables = fw_calloc(sizeof(fw_module_table_t) * table_cnt);
        if (!tables) {
            goto err;
        }

        module->tables = tables;
        section->free  = fw_module_section_table_free;

        for (i = 0; i < table_cnt; i++, tables++) {

            fw_str_leb_getu8(&section->data, type);
            if (type != 0x70) {
                goto err;
            }

            fw_str_leb_getu32(&section->data, limits);
            fw_str_leb_getu32(&section->data, tables->min);
            if (limits == 0x01) {
                fw_str_leb_getu32(&section->data, tables->max);
            } else {
                tables->max = 0x100;
            }
        }
    }

    /*All data in section should be consumed*/
    if (section->data.pos != section->data.last) {
        goto err;
    }

    return FW_OK;

str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}

FW_DECLARE_SECTION_METHOD(code)
{
    /*
     * code section will be(has been) parsed while parsing func section.
     */
    return FW_OK;
}
/*
 *https://www.w3.org/TR/wasm-core-1/#binary-funcsec
 */
FW_DECLARE_SECTION_METHOD(func)
{
    uint32_t i, j, k;
    uint32_t func_cnt, typeidx;
    uint32_t code_cnt, codesize;
    uint32_t local_cnt, count, total_cnt, local_cnt_idx;
    uint8_t *code_start, *code_end, valuetype;

    fw_module_func_t *funcs;

    fw_str_leb_getu32(&section->data, func_cnt);
    fw_str_leb_getu32(&module->code_ref->data, code_cnt);

    if (code_cnt != func_cnt) {
        goto err;
    }

    if (func_cnt) {
        //printf("func_cnt %d\n",func_cnt);
        module->func_cnt = func_cnt;
        funcs = fw_calloc(sizeof(fw_module_func_t) * func_cnt);
        if (!funcs) {
            goto err;
        }

        module->funcs = funcs;
        section->free = fw_module_section_func_free;

        /*
         *It decodes into a vector of type indices that represent
         *the type fields of the functions in the funcs component of a module.
         */
        for (i = 0; i < func_cnt; i++, funcs++) {
            fw_str_leb_getu32(&section->data, typeidx);
            //printf("func typeidx %d\n",typeidx);
            if (typeidx > module->type_cnt) {
                goto err;
            }

            INIT_LIST_HEAD(&funcs->blk);
            INIT_LIST_HEAD(&funcs->blktmp);
            INIT_LIST_HEAD(&funcs->iftmp);

            funcs->functype = module->type[typeidx];

            /*
             *The locals and body fields of the respective functions are
             *encoded separately in the code section
             */

            /*
             *https://www.w3.org/TR/wasm-core-1/#binary-codesec
             *
             *The encoding of each code entry consists of
             *--(1)the u32 size of the function code in bytes,
             *--(2)the actual function code, which in turn consists of
             *----(2.1)the declaration of locals,
             *----(2.2)the function body as an expression.
             *Local declarations are compressed into a vector whose
             *entries consist of
             *(2.1.1)a u32 count
             *(2.1.2)a value type,
             */

            /*(1)*/
            fw_str_leb_getu32(&module->code_ref->data, codesize);
            code_end = module->code_ref->data.pos + codesize;
            //printf("codesize %d\n",codesize);

            /*vector*/
            fw_str_leb_getu32(&module->code_ref->data, local_cnt);

            /*Save the pos to make it parsed twice*/
            code_start = module->code_ref->data.pos;

            /*first calc the total counts of valuetype*/
            total_cnt = 0;
            for (j = 0; j < local_cnt; j++) {
                /*(2.1.1)*/
                fw_str_leb_getu32(&module->code_ref->data, count);
                /*(2.1.2)*/
                fw_str_leb_getu8(&module->code_ref->data, valuetype);
                total_cnt += count;
            }

            funcs->body.start = module->code_ref->data.pos;
            funcs->body.pos = funcs->body.start;
            funcs->body.end = code_end;
            funcs->body.last = code_end;


            /*Create one liner memory to save valuetype*/
            funcs->local_type = fw_malloc(total_cnt);
            if (!funcs->local_type) {
                goto err;
            }
            funcs->local_cnt = total_cnt;

            /*Recover the position to parse again*/
            module->code_ref->data.pos = code_start;

            /*save all locals of this func into local_type*/
            local_cnt_idx = 0;
            for (j = 0; j < local_cnt; j++) {
                /*(2.1.1)*/
                fw_str_leb_getu32(&module->code_ref->data, count);
                /*(2.1.2)*/
                fw_str_leb_getu8(&module->code_ref->data, valuetype);

                for (k = 0; k < count; k++) {
                    funcs->local_type[local_cnt_idx++] = valuetype;
                }
            }
            /*now is the expression*/
            module->code_ref->data.pos = code_end;
        }
    }

    /*All data in section should be consumed*/
    if (section->data.pos != section->data.last) {
        goto err;
    }

    return FW_OK;

str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}

/*
 *https://www.w3.org/TR/wasm-core-1/#binary-importsec
 *   |  import_func  |  import_global  |  ...  |
 *
 */
FW_DECLARE_SECTION_METHOD(import)
{
    uint32_t import_cnt, mname_len, nsname_len, i;
    uint32_t typeidx, limits, lmin, lmax;
    uint8_t  kind, elemtype, valtype, mut;
    fw_module_func_t *func;
    fw_module_import_t *imports[FW_IMPORT_KIND_MAX] = {NULL};
    fw_str_t *mname;
    fw_str_t *nsname;
    fw_str_t body;

    fw_str_leb_getu32(&section->data, import_cnt);

    /*We need to classify all imports*/
    if (import_cnt) {
        body = section->data;
        /*Just parse import simply and get num for each import item*/
        for (i = 0; i < import_cnt; i++) {

            fw_str_leb_getu32(&body, mname_len);
            fw_str_pass(&body, mname_len);

            fw_str_leb_getu32(&body, nsname_len);

            fw_str_pass(&body, nsname_len);

            fw_str_leb_getu8(&body, kind);

            switch (kind) {
                case FW_IMPORT_KIND_FUNC:
                {
                    module->import_cnt[FW_IMPORT_KIND_FUNC]++;
                    fw_str_leb_getu32(&body, typeidx);
                    break;
                }
                case FW_IMPORT_KIND_TABLE:
                {
                    module->import_cnt[FW_IMPORT_KIND_TABLE]++;
                    fw_str_leb_getu8(&body, elemtype);
                    break;
                }
                case FW_IMPORT_KIND_MEMORY:
                {
                    module->import_cnt[FW_IMPORT_KIND_MEMORY]++;
                    fw_str_leb_getu32(&body, limits);
                    fw_str_leb_getu32(&body, lmin);
                    if (limits == 0x01) {
                        fw_str_leb_getu32(&body, lmax);
                    }
                    break;
                }
                case FW_IMPORT_KIND_GLOBAL:
                {
                    module->import_cnt[FW_IMPORT_KIND_GLOBAL]++;
                    fw_str_leb_getu8(&body, valtype);
                    fw_str_leb_getu8(&body, mut);
                    break;
                }

            }
        }

        /*alloc space for each import type*/
        for (i = 0; i < FW_IMPORT_KIND_MAX; i++) {
            if (module->import_cnt[i]) {
                module->imports[i] = fw_calloc(sizeof(fw_module_import_t) * module->import_cnt[i]);
                if (!module->imports[i]) {
                    goto err;
                }
                /*Get the start*/
                imports[i] = module->imports[i];
            }
        }

        section->free = fw_module_section_imports_free;

        /*realy parse*/
        for (i = 0; i < import_cnt; i++) {

            /*
             *https://www.w3.org/TR/wasm-core-1/#binary-name
             */
            fw_str_leb_getu32(&section->data, mname_len);
            mname = fw_make_str(section->data.pos, mname_len);
            fw_str_pass(&section->data, mname_len);

            fw_str_leb_getu32(&section->data, nsname_len);
            nsname = fw_make_str(section->data.pos, nsname_len);
            fw_str_pass(&section->data, nsname_len);

            fw_str_leb_getu8(&section->data, kind);
            imports[kind]->mname = mname;
            imports[kind]->nsname = nsname;

             switch (kind) {
                case FW_IMPORT_KIND_FUNC:
                {
                    /*Refer to type section*/
                    fw_str_leb_getu32(&section->data, typeidx);
                    func = fw_calloc(sizeof(fw_module_func_t));
                    if (!func) {
                        goto err;
                    }

                    imports[kind]->u.func.f = func;
                    imports[kind]->type = FW_IMPORT_KIND_FUNC;
                    imports[kind]->u.func.typeidx = typeidx;
                    imports[kind]->free = fw_module_free_imports_func;
                    imports[kind] = imports[kind] + 1;
                    break;
                }

                case FW_IMPORT_KIND_TABLE:
                {
                    /*
                     *https://www.w3.org/TR/wasm-core-1/#binary-tabletype
                     */
                    fw_str_leb_getu8(&section->data, elemtype);
                    if (elemtype != 0x70) {
                        /*Anyfunc*/
                    }
                    printf("elemtype %x\n", elemtype);
                    fw_str_leb_getu32(&section->data, limits);
                    fw_str_leb_getu32(&section->data, lmin);
                    printf("limits %x lmin %x\n", limits, lmin);
                    if (limits == 0x01) {
                        fw_str_leb_getu32(&section->data, lmax);
                    } else {
                        lmax = 0x1000;/*todo*/
                    }

                    imports[kind]->type = FW_IMPORT_KIND_TABLE;
                    imports[kind]->u.tb.type = elemtype;
                    imports[kind]->u.tb.min = lmin;
                    imports[kind]->u.tb.max = lmax;
                    imports[kind] = imports[kind] + 1;
                    break;
                }

                case FW_IMPORT_KIND_MEMORY:
                {
                    /*
                     *https://www.w3.org/TR/wasm-core-1/#binary-memtype
                     */
                    fw_str_leb_getu32(&section->data, limits);
                    fw_str_leb_getu32(&section->data, lmin);
                    if (limits == 0x01) {
                        fw_str_leb_getu32(&section->data, lmax);

                    } else {
                        lmax = 0x1000;/*todo*/
                    }
                    imports[kind]->type = FW_IMPORT_KIND_MEMORY;
                    imports[kind]->u.mm.min = lmin;
                    imports[kind]->u.mm.max = lmax;
                    imports[kind] = imports[kind] + 1;
                    break;
                }

                case FW_IMPORT_KIND_GLOBAL:
                {
                    /*
                     *https://www.w3.org/TR/wasm-core-1/#binary-globaltype
                     */
                    fw_str_leb_getu8(&section->data, valtype);
                    fw_str_leb_getu8(&section->data, mut);
                    imports[kind]->type = FW_IMPORT_KIND_GLOBAL;
                    imports[kind]->u.gl.type = valtype;
                    imports[kind]->u.gl.mutable = mut;
                    imports[kind] = imports[kind] + 1;
                    break;
                }

                default:
                    goto err;
                break;
             }
         }
    }

    /*All data in section should be consumed*/
    if (section->data.pos != section->data.last) {
        goto err;
    }


    return FW_OK;

str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}
/*
 *https://www.w3.org/TR/wasm-core-1/#binary-typesec
 *typesec:section(vec(functype))
 *https://www.w3.org/TR/wasm-core-1/#binary-functype
 *functype: 0x60 vec(valtype) vec(valtype)
 *https://www.w3.org/TR/wasm-core-1/#binary-valtype
 *i32: 7f, i64: 7e, f32: 7d, f64: 7c
 */
FW_DECLARE_SECTION_METHOD(type)
{
    /*
     *https://www.w3.org/TR/wasm-core-1/#binary-vec
     *Vectors are encoded with their u32 length followed by the encoding of their element sequence.
     */
    uint32_t type_cnt, i, param_cnt, res_cnt;
    uint8_t prefix;
    uint8_t *tparam, *tres;

    fw_str_leb_getu32(&section->data, type_cnt);
    module->type_cnt = type_cnt;

    if (type_cnt) {
        module->type = fw_malloc(type_cnt * sizeof(fw_module_type_t*));
        if (!module->type) {
            goto err;
        }

        for (i = 0; i < type_cnt; i++) {
            fw_str_get8(&section->data, prefix);
            if (prefix != 0x60) {
                goto err;
            }

            /*To alloc one liner memory we have to parse cnt first*/
            fw_str_leb_getu32(&section->data, param_cnt);
            tparam = section->data.pos;
            fw_str_pass(&section->data, param_cnt);
            fw_str_leb_getu32(&section->data, res_cnt);
            tres = section->data.pos;
            fw_str_pass(&section->data, res_cnt);

            /*We have param_cnt paraments and res_cnt results*/
            module->type[i] = fw_malloc(sizeof(fw_module_type_t) + param_cnt + res_cnt);
            if (!module->type[i]) {
                goto err;
            }

            module->type[i]->param_cnt = param_cnt;
            module->type[i]->res_cnt = res_cnt;
            fw_memcpy(module->type[i]->val, tparam, param_cnt);
            fw_memcpy(&module->type[i]->val[param_cnt], tres, res_cnt);
        }

    }

    if (section->data.pos != section->data.last) {
        goto err;
    }

    return FW_OK;

str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("err in %s\n", __func__);
    return FW_ERR;
}

FW_DECLARE_SECTION_METHOD(dummy)
{
    return FW_OK;
}

void fw_module_free(fw_module_t *module)
{
    fw_module_section_t *section = module->section.next;
    fw_module_section_t *tmp;
    uint32_t i;

    while(section) {
        /*Free inner*/
        if (section->free) {
            section->free(module, section);
        }

        /*It's no need to clean section->data now*/
        tmp = section;
        section = section->next;
        fw_free(tmp);
    }

    for (i = 0; i < module->type_cnt; i++) {
        fw_free(module->type[i]);
    }
    fw_free(module->type);

    if (module->filedata) {
        fw_free(module->filedata);
    }

    fw_free(module);
}

/*
 *https://www.w3.org/TR/wasm-core-1/#sections%E2%91%A0
 */
fw_module_section_handler common_section_handler[FW_SECTION_TYPE_MAX] = {
    [FW_SECTION_TYPE_CUSTOM] = FW_NAME_SECTION_METHOD(dummy),
    [FW_SECTION_TYPE_TYPE]   = FW_NAME_SECTION_METHOD(type),
    [FW_SECTION_TYPE_IMPORT] = FW_NAME_SECTION_METHOD(import),
    [FW_SECTION_TYPE_FUNC]   = FW_NAME_SECTION_METHOD(func),
    [FW_SECTION_TYPE_TABLE]  = FW_NAME_SECTION_METHOD(table),
    [FW_SECTION_TYPE_MEMORY] = FW_NAME_SECTION_METHOD(memory),
    [FW_SECTION_TYPE_GLOBAL] = FW_NAME_SECTION_METHOD(global),
    [FW_SECTION_TYPE_EXPORT] = FW_NAME_SECTION_METHOD(export),
    [FW_SECTION_TYPE_START]  = FW_NAME_SECTION_METHOD(start),
    [FW_SECTION_TYPE_ELEM]   = FW_NAME_SECTION_METHOD(elem),
    [FW_SECTION_TYPE_CODE]   = FW_NAME_SECTION_METHOD(code),
    [FW_SECTION_TYPE_DATA]   = FW_NAME_SECTION_METHOD(data),
};

/*parse binary*/
fw_module_t *_fw_module_load(fw_str_t *data)
{
    uint8_t s_type;
    uint32_t magic, version, s_size;
    fw_module_t *ret = NULL;
    fw_module_section_t *section, *nsection;

    fw_str_get32(data, magic);

    if (magic != FW_MAGIC_NUMBER) {
        goto err;
    }

    fw_str_get32(data, version);
    //printf("version %x\n", version);
    if (version != FW_VERSION) {
        goto err;
    }

    ret = fw_calloc(sizeof(fw_module_t));
    if (!ret) {
        goto err;
    }

    section = &ret->section;
    section->next = NULL;
    section->data.pos = NULL;

    /*
     *https://www.w3.org/TR/wasm-core-1/#sections%E2%91%A0
     *Each section consists of
     *1. a one-byte section id,
     *2. the u32 size of the contents, in bytes,
     *3. the actual contents, whose structure is depended on the section id.
     */
    while (data->pos < data->last) {

        fw_str_get8(data, s_type);
        if (s_type >= FW_SECTION_TYPE_MAX) {
            goto err;
        }

        fw_str_leb_getu32(data, s_size);

        nsection = fw_malloc(sizeof(fw_module_section_t));
        if (!nsection) {
            goto err;
        }

        nsection->free = NULL;
        nsection->data.pos = data->pos;
        nsection->data.last = data->pos + s_size;
        nsection->type = s_type;
        nsection->next = NULL;

        section->next = nsection;
        section = nsection;

        if (s_type == FW_SECTION_TYPE_CODE) {
            ret->code_ref = section;
        }

        data->pos += s_size;
    }

    if (!ret->code_ref) {
        printf("no code section\n");
        goto err;
    }

    /*Should be at the end of file*/
    if (data->pos != data->last) {
        goto err;
    }

    ret->section_handler = common_section_handler;
    return ret;

str_err:
    printf("str_err in %s\n", __func__);
err:
    printf("parse err in %s\n", __func__);
    if (ret) {
        fw_module_free(ret);
    }
    return NULL;
}

int32_t fw_module_parse(fw_module_t *module)
{
    fw_module_section_t *section = module->section.next;

    while(section) {
        /*We have validate the type while loading*/
        if (FW_OK != module->section_handler[section->type](module, section)) {
            return FW_ERR;
        }
        section = section->next;
    }

    return FW_OK;
}

/*fw_module_load will not hold the file handler*/
fw_module_t *fw_module_load(const char *filename)
{
    fw_module_t *module;
    fw_file_t   *file = fw_file_read(filename);
    if (file == NULL) {
        return NULL;
    }

    module = _fw_module_load(file->file);
    if (module == NULL) {
        fw_file_close(file);
        return NULL;
    }

    fw_module_parse(module);

    module->filedata = fw_file_detach_data(file);
    fw_file_close(file);
    return module;
}

