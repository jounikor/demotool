#ifndef _IFFHANDLE_H_INCLUDED
#define _IFFHANDLE_H_INCLUDED

#include <stdint.h>

struct IFFHandle;
typedef struct  IFFHandle IFFHandle_t;
struct  IFFHandle {
    int32_t (*open)(IFFHandle_t *ctx, char* name, int32_t mode);
    int32_t (*close)(IFFHandle_t *ctx);
    int32_t (*read)(IFFHandle_t *ctx, void *buffer, int32_t length);
    int32_t (*write)(IFFHandle_t *ctx, void *buffer, int32_t length);
    int32_t (*putnum)(IFFHandle_t *ctx, uint32_t num, int32_t length);
    int32_t (*putc)(IFFHandle_t *ctx, uint8_t ch);
    int32_t (*seek)(IFFHandle_t *ctx, int32_t position, int32_t mode);
    int32_t (*ioerr)(IFFHandle_t *ctx);
    int32_t (*flush)(IFFHandle_t *ctx);

    /* internal data */
    UBYTE reserved[];
};


IFFHandle_t *initIFFHandle(void);
void freeIFFHandle(IFFHandle_t *ctx);

#endif  /* _IFFHANDLE_H_INCLUDED */
