#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define LOCHAR 32
#define HICHAR 127
#define FONT_BWIDTH 1
#define FONT_HEIGHT 8

extern const uint8_t CGA8[];
extern const uint8_t CGA16[];

typedef enum
{
    SIZE8=0,
    SIZE16
} font_size_t;


#endif /* FONT_H */