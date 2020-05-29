#include <stdio.h>
#include <fw_core.h>
int func1(int v)
{
    return 0;
}
int host_func3(int v)
{
    return v;
}

int host_func2()
{
    return 2;
}


int main()
{
    fw_ret_t rets;
    char *inargs[2] = {"3", "2"};
    fw_value_t import_membase;

    fw_module_t *module = fw_module_load("test.wasm");
    if (!module) {
        fw_module_free(module);
        return 0;
    }

    fw_import_t *imports = fw_imports_wrap_new(module);

    //imports->add_func(imports, "env", "_host_func3", host_func3, 1, 1);
    //imports->add_func(imports, "env", "_host_func2", host_func2, 0, 1);
    imports->add_func(imports, "env", "_func1", func1, 1, 0);
    imports->add_func(imports, "env", "_func2", func1, 1, 0);


    import_membase.type = FW_I32;
    import_membase.u.i32 = 0;

    imports->add_global(imports, "env", "__memory_base", &import_membase);

    if(FW_OK != fw_module_compile(module, imports)) {
        fw_module_free(module);
        return 0;
    }

    printf("fw_module_run\n");
    if(FW_OK != fw_module_run(module, "_gmain", 2, inargs, &rets)) {
        fw_module_free(module);
        return 0;
    }

    fw_imports_wrap_free(imports);

    return 1;
}
