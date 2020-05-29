#include <fw_file.h>

fw_str_t *fw_file_detach_data(fw_file_t *file)
{
    fw_str_t *filedata = file->file;
    file->file = NULL;
    return filedata;
}

void fw_file_close(fw_file_t *file)
{
    if (file->file) {
        fw_free(file->file);
        file->file = NULL;
    }
    fw_free(file);
}

/*Drop file handler and only save the file content*/
fw_file_t *fw_file_read(const char *filename)
{
    long unsigned byteslen;
    FILE *file = NULL;
    fw_file_t *ret = NULL;
    fw_str_t *filedata = NULL;

    file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    byteslen = ftell(file);

    ret = fw_malloc(sizeof(fw_file_t));
    if (ret == NULL) {
        goto err;
    }

    filedata = fw_malloc(sizeof(fw_str_t) + byteslen);
    if (filedata == NULL) {
        goto err;
    }

    filedata->start = (uint8_t *)(filedata + 1);
    filedata->end  = filedata->start + byteslen;
    filedata->pos = filedata->start;
    filedata->last = filedata->end;

    fseek(file, 0, SEEK_SET);
    if (fread(filedata->start, 1, byteslen, file) != byteslen) {
        goto err;
    }

    ret->file = filedata;
    fclose(file);
    return ret;

err:
    if (ret) {
        fw_free(ret);
    }

    if (file) {
        fclose(file);
    }

    return NULL;
}

