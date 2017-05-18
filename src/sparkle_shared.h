#ifndef SPARKLE_SHARED_H
#define SPARKLE_SHARED_H

#include <stdint.h>

#define SPARKLE_FORMAT_BGRA8888     0x00
#define SPARKLE_FORMAT_RGB565       0x01
#define SPARKLE_FORMAT_STATIC_GRAY  0x02

struct sparkle_shared_t
{
    uint32_t pixmapWidth;
    uint32_t pixmapHeight;
    uint32_t pixmapFormat;
    uint32_t surfaceWidth;
    uint32_t surfaceHeight;
    uint32_t damage;
    uint32_t damageX1;
    uint32_t damageY1;
    uint32_t damageX2;
    uint32_t damageY2;
};

#endif //SPARKLE_SHARED_H

