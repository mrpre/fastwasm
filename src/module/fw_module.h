#pragma once

#include <stdint.h>



#define FW_MAGIC_NUMBER 0x6d736100
#define FW_VERSION      1

#define FW_MAX_IMPORT_HASH 8
#define FW_MAX_IMPORT_HASH_MASK (FW_MAX_IMPORT_HASH - 1)

#define FW_MAX_STACK_SIZE 4096

typedef enum {
    FW_IMPORT_GLOBAL,
} FW_IMPORT_TYPE;


typedef enum {
    FW_F64 = 0x7C,
    FW_F32 = 0x7D,
    FW_I64 = 0x7E,
    FW_I32 = 0x7F,
} FW_VALUE_TYPE;

typedef struct fw_value_t {
    union {
        uint32_t i32;
        uint64_t i64;
        float f32;
        double f64;
    } u;
    FW_VALUE_TYPE type;
} fw_value_t;

typedef enum {
    FW_SECTION_TYPE_CUSTOM = 0,
    FW_SECTION_TYPE_TYPE,
    FW_SECTION_TYPE_IMPORT,
    FW_SECTION_TYPE_FUNC,
    FW_SECTION_TYPE_TABLE,
    FW_SECTION_TYPE_MEMORY,
    FW_SECTION_TYPE_GLOBAL,
    FW_SECTION_TYPE_EXPORT,
    FW_SECTION_TYPE_START,
    FW_SECTION_TYPE_ELEM,
    FW_SECTION_TYPE_CODE,
    FW_SECTION_TYPE_DATA,
    FW_SECTION_TYPE_MAX,
} FW_SECTION_TYPE;

typedef enum {
    FW_IMPORT_KIND_FUNC = 0,
    FW_IMPORT_KIND_TABLE,
    FW_IMPORT_KIND_MEMORY,
    FW_IMPORT_KIND_GLOBAL,
    FW_IMPORT_KIND_MAX,
} FW_IMPORT_KIND;

typedef enum {
    FW_EXPORT_KIND_FUNC = 0,
    FW_EXPORT_KIND_TABLE,
    FW_EXPORT_KIND_MEMORY,
    FW_EXPORT_KIND_GLOBAL,
    FW_EXPORT_KIND_MAX,
} FW_EXPORT_KIND;

struct fw_module_t;
struct fw_module_section_t;
struct fw_module_import_t;
struct fw_module_type_t;
struct fw_module_func_t;
struct fw_module_table_t;

typedef struct fw_module_type_t {
    uint8_t  param_cnt;
    uint8_t  res_cnt;
    /*
     *param_cnt: val[0..param_cnt-1]
     *res:       val[param_cnt..res_cnt-1]
     */
    uint8_t val[0];
} fw_module_type_t;

typedef void (*fw_module_section_free_hander)(struct fw_module_t *module, struct fw_module_section_t *section);
typedef struct fw_module_section_t {
    struct fw_module_section_t *next;
    fw_str_t data;
    uint8_t  type;
    fw_module_section_free_hander free;
} fw_module_section_t;


typedef void (*fw_module_import_free_handler)(struct fw_module_t *module, struct fw_module_import_t *import);
typedef struct fw_module_import_t {
    uint8_t  type;
    fw_str_t *mname;
    fw_str_t *nsname;
    union {
        struct function {
            struct fw_module_func_t *f;
            uint32_t typeidx;
        } func;

        struct table {
            uint32_t type;
            uint32_t min;
            uint32_t max;
        } tb;
        struct memory {
            uint32_t min;
            uint32_t max;
        } mm;
        struct global {
            uint32_t type;
            uint32_t mutable;
        } gl;
    } u;

    fw_module_import_free_handler free;
} fw_module_import_t;


typedef struct {
    struct list_head node;
    struct list_head ifnode;
    uint8_t *start;
    uint8_t *els;
    uint8_t *end;
    uint8_t  type;
} fw_module_func_block_t;

typedef struct fw_module_func_t {
    uint8_t *local_type;
    uint32_t local_cnt;
    fw_module_type_t *functype;
    fw_str_t body;
    void *host_addr;
    uint64_t *locals;/*including param*/

    /*describe the block boundary using fw_module_func_block_t
     *while beeing validated which will be removed into blkhash
     */
    struct list_head blktmp;
    struct list_head iftmp;
    struct list_head blk;
} fw_module_func_t;

typedef struct fw_module_table_t {
    uint8_t type;
    uint32_t min;
    uint32_t max;
} fw_module_table_t;

typedef struct fw_module_memory_t {
    uint32_t min;
    uint32_t max;
} fw_module_memory_t;

typedef struct fw_module_global_t {
    uint8_t type;
    uint8_t mutable;
} fw_module_global_t;

typedef struct fw_module_export_t {
    fw_str_t *name;
    FW_EXPORT_KIND kind;
    uint32_t idx;
    fw_module_func_t *func;/*set by compile*/
} fw_module_export_t;

typedef struct fw_module_elem_t {
    uint32_t table_idx;
    uint32_t func_cnt;
    uint32_t *funcs;
} fw_module_elem_t;

typedef struct fw_module_data_t {
    uint32_t data_len;
    uint32_t mem_idx;
    uint8_t  *data;
} fw_module_data_t;

typedef struct fw_module_instance_t {
    fw_module_func_t **funcs;
} fw_module_instance_t;

typedef int32_t (*fw_module_section_handler)(struct fw_module_t *module, fw_module_section_t *section);

typedef struct fw_module_t {
    fw_str_t *filedata;
    fw_module_section_t section;

    fw_module_import_t *imports[FW_IMPORT_KIND_MAX];
    uint32_t            import_cnt[FW_IMPORT_KIND_MAX];

    fw_module_func_t   *funcs;
    uint32_t            func_cnt;
    fw_module_table_t  *tables;
    uint32_t            table_cnt;
    fw_module_memory_t *memories;
    uint32_t            memory_cnt;

    fw_module_global_t *globals;
    uint32_t            global_cnt;

    fw_module_export_t *exports;
    uint32_t            export_cnt;

    fw_module_elem_t   *elems;
    uint32_t            elem_cnt;

    fw_module_data_t   *datas;
    uint32_t            data_cnt;

    fw_module_section_t *code_ref;/*just ref*/
    fw_module_section_handler *section_handler;
    fw_module_type_t **type;
    uint32_t           type_cnt;
    uint32_t           start_func;

    fw_module_instance_t *instance;
} fw_module_t;


typedef struct fw_import_func_t {
    struct list_head node;
    fw_str_t *mname;
    fw_str_t *nsname;
    uint8_t argc;
    uint8_t retc;
    void *ptr;
} fw_import_func_t;

typedef struct fw_import_tbl_t {
    struct list_head node;
    fw_str_t *mname;
    fw_str_t *nsname;
    void *ptr;
} fw_import_tbl_t;

typedef struct fw_import_mem_t {
    struct list_head node;
    fw_str_t *mname;
    fw_str_t *nsname;
    void *ptr;
} fw_import_mem_t;

typedef struct fw_import_glb_t {
    struct list_head node;
    fw_str_t *mname;
    fw_str_t *nsname;

    fw_value_t val;
} fw_import_glb_t;

typedef struct fw_import_func_wrap_t {
    struct list_head funcs[FW_MAX_IMPORT_HASH];
    fw_import_func_t *(*find)(struct fw_import_func_wrap_t *, fw_str_t *, fw_str_t *);
    int32_t (*insert)(struct fw_import_func_wrap_t *, fw_import_func_t *);
} fw_import_func_wrap_t;

typedef struct fw_import_tbl_wrap_t {
    struct list_head tbls[FW_MAX_IMPORT_HASH];
} fw_import_tbl_wrap_t;

typedef struct fw_import_mem_wrap_t {
    struct list_head mems[FW_MAX_IMPORT_HASH];
} fw_import_mem_wrap_t;

typedef struct fw_import_glb_wrap_t {
    struct list_head glbs[FW_MAX_IMPORT_HASH];
    fw_import_glb_t *(*find)(struct fw_import_func_wrap_t *, fw_str_t *, fw_str_t *);
    fw_import_glb_t *(*insert)(struct fw_import_glb_wrap_t*, fw_str_t *, fw_str_t *, fw_value_t *);
} fw_import_glb_wrap_t;

typedef struct fw_import_t {
    fw_import_func_wrap_t funcs;
    fw_import_tbl_wrap_t tbls;
    fw_import_mem_wrap_t mems;
    fw_import_glb_wrap_t glbs;

    fw_import_glb_t *(*add_global)(struct fw_import_t *, char *, char  *, fw_value_t *);
    fw_import_func_t *(*add_func)(struct fw_import_t *, char *, char *, void *, uint8_t , uint8_t);
} fw_import_t;

typedef struct fw_ret_t {
    uint32_t val;
} fw_ret_t;

typedef struct fw_func_locals_t {
    uint8_t *local_type;
    uint32_t local_cnt;
    uint64_t locals[0];
} fw_func_locals_t;

fw_module_t *fw_module_load(const char *filename);
void fw_module_free(fw_module_t *module);

#define FW_NAME_SECTION_METHOD(_section_name)   \
    fw_module_section_##_section_name

#define FW_DECLARE_SECTION_METHOD(_section_name)    \
    int32_t FW_NAME_SECTION_METHOD(_section_name)(struct fw_module_t *module, fw_module_section_t *section)

#define for_each_funcs(_i, _m, _f)  \
    for((_i) = 0; ((_i) < (_m)->func_cnt) && ((_f) = &(_m)->funcs[(_i)]); (_i)++)
#define for_each_exports(_i, _m, _e)  \
            for((_i) = 0; ((_i) < (_m)->export_cnt) && ((_e) = &(_m)->exports[(_i)]); (_i)++)


