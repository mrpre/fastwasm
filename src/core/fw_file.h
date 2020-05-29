#pragma once

#include <fw_mem.h>
#include <fw_str.h>
#include <stdio.h>
typedef struct {
    int32_t fd;
    fw_str_t *file;
} fw_file_t;

fw_file_t *fw_file_read(const char *filename);
void fw_file_close(fw_file_t *file);
fw_str_t *fw_file_detach_data(fw_file_t *file);

