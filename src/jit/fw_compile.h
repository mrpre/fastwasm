#pragma once
int32_t fw_module_compile(fw_module_t *module, fw_import_t *imports);
fw_import_t *fw_imports_wrap_new(fw_module_t *module);
void  fw_imports_wrap_free(fw_import_t *imports);
fw_import_func_t *fw_imports_func_new(fw_import_t *imports, char *mname, char* nsname, void *faddr, uint8_t argc, uint8_t retc);
fw_import_glb_t *fw_imports_glb_new(fw_import_t *imports, char *mname, char* nsname, fw_value_t *val);

