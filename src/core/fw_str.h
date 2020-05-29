#pragma once

#include <stdint.h>
#include <fw_mem.h>
typedef struct {
    uint8_t *start;
    uint8_t *last;
    uint8_t *end;//dynamic
    uint8_t *pos;//dynamic
} fw_str_t;

#define fw_str_pass(_str, _size)\
    do {\
        if ((_str)->pos + (_size) > (_str)->last) {\
            goto str_err;\
        }\
        (_str)->pos += (_size); \
    } while(0)\

#define fw_str_get8(_str, _res)\
    do {\
        if ((_str)->pos + 1 > (_str)->last) {\
            goto str_err;\
        }\
        (_res) = *(_str)->pos++; \
    } while(0)\

#define fw_str_get16(_str, _res)\
    do {\
        if ((_str)->pos + 2 >  (_str)->last) {\
            goto str_err;\
        }\
        (_res) = ((_str)->pos[1] <<8) | ((_str)->pos[0]); \
        (_str)->pos += 2;\
    } while(0)\

#define fw_str_get24(_str, _res)\
    do {\
        if ((_str)->pos + 3 >  (_str)->last) {\
            goto str_err;\
        }\
        (_res) = ((_str)->pos[2] << 16)\
                | ((_str)->pos[1] << 8)\
                | ((_str)->pos[0]);\
        (_str)->pos += 4;\
    } while(0)\

#define fw_str_get32(_str, _res)\
    do {\
        if ((_str)->pos + 4 >  (_str)->last) {\
            goto str_err;\
        }\
        (_res) = ((_str)->pos[3] <<24)\
                | ((_str)->pos[2] << 16)\
                | ((_str)->pos[1] << 8)\
                | ((_str)->pos[0]);\
        (_str)->pos += 4;\
    } while(0)\

/*https://www.w3.org/TR/wasm-core-1/#integers%E2%91%A4
 *
 *Unsigned integers are encoded in unsigned LEB128 format.
 *As an additional constraint, the total number of bytes encoding
 *a value of type uN must not exceed ceil(N/7) bytes.
 *
 *Signed integers are encoded in signed LEB128 format,
 *which uses a two’s complement representation. As an additional
 *constraint, the total number of bytes encoding a value of type sN
 *must not exceed ceil(N/7) bytes.
 */
#define fw_str_leb_gets(_str, _res, _maxbit) \
    do {\
        uint8_t _shift = 0, _round = 0, _byte;\
        (_res) = 0;\
        while(1) {\
            if ((_str)->pos >= (_str)->last) {\
                goto str_err;\
            }\
            _byte = (_str)->pos[_round++];\
            (_res) |= (_byte&0x7f) << _shift;\
            _shift += 7;\
            if (!(_byte&0x80)) {\
                break;\
            }\
        }\
        if (_round > ((_maxbit) + 7 - 1) / 7) {\
            goto str_err;\
        }\
        if (_byte&0x40) {\
            _res |= - (1 << _shift);\
        }\
        (_str)->pos += _round;\
    } while(0)

#define fw_str_leb_getu(_str, _res, _maxbit) \
    do {\
        uint8_t _shift = 0, _round = 0;\
        (_res) = 0;\
        while(1) {\
            if ((_str)->pos >= (_str)->last) {\
                goto str_err;\
            }\
            (_res) |= ((_str)->pos[_round]&0x7f) << _shift;\
            _shift += 7;\
            if (!((_str)->pos[_round++]&0x80)) {\
                break;\
            }\
        }\
        if (_round > ((_maxbit) + 7 - 1) / 7) {\
            goto str_err;\
        }\
        (_str)->pos += _round;\
    } while(0)

#define fw_str_leb_getu64(_str, _res) fw_str_leb_getu((_str), (_res), 64)
#define fw_str_leb_getu32(_str, _res) fw_str_leb_getu((_str), (_res), 32)
#define fw_str_leb_getu8(_str, _res) fw_str_leb_getu((_str), (_res), 8)

#define fw_str_leb_gets64(_str, _res) fw_str_leb_gets((_str), (_res), 64)
#define fw_str_leb_gets32(_str, _res) fw_str_leb_gets((_str), (_res), 32)
#define fw_str_leb_gets8(_str, _res) fw_str_leb_gets((_str), (_res), 8)

/*
 *Uninterpreted integers are encoded as signed integers.
 */
#define fw_str_leb_geti64 fw_str_leb_gets64
#define fw_str_leb_geti32 fw_str_leb_gets32

/*Only 0 or 1 will be returned*/
static inline int32_t fw_str_cmp(fw_str_t *a, fw_str_t *b)
{
    uint32_t alen, blen;

    alen = a->last - a->pos;
    blen  = b->last - b->pos;

    if (alen != blen) {
        return 1;
    }

    if (memcmp(a->pos, b->pos, alen)) {
        return 1;
    }

    return 0;
}

static inline fw_str_t* fw_make_str(uint8_t *str, size_t size)
{
    fw_str_t *ret = fw_malloc(sizeof(fw_str_t) + size + 1);
    if (!ret) {
        return NULL;
    }

    ret->start = (uint8_t*)(ret + 1);
    ret->pos = ret->start;

    ret->end = fw_cpymem(ret->start, str, size);
    *ret->end = '\0';
    ret->last = ret->end;
    return ret;
}
