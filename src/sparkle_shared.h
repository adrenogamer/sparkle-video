#ifndef SPARKLE_SHARED_H
#define SPARKLE_SHARED_H

#include <stdint.h>

struct sparkle_shared_t
{
    uint32_t pixmapWidth;
    uint32_t pixmapHeight;
    uint32_t surfaceWidth;
    uint32_t surfaceHeight;
    uint32_t damage;
    uint32_t damageX1;
    uint32_t damageY1;
    uint32_t damageX2;
    uint32_t damageY2;
};

#endif //SPARKLE_SHARED_H

