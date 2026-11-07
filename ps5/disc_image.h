/* rotk-ps5: unpacks an Xbox disc image (XDVDFS) on the console (from reUndr-ps5's disc_image.h, MIT, bystritskiy). */
#ifndef ROTK_DISC_IMAGE_H
#define ROTK_DISC_IMAGE_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ROTK_IMAGE_OK,
    ROTK_IMAGE_NOT_A_DISC,
    ROTK_IMAGE_WRONG_DISC,
    ROTK_IMAGE_DAMAGED,
    ROTK_IMAGE_WRITE /* the folder could not be written: most often, no room */
};

/* Copies the disc into target. accept sees default.xbe before anything else is copied and may refuse it (0);
 * progress gets 0 once the size of the game (total bytes) is known, then each new percentage. */
int rotk_image_extract(const char *image, const char *target, int (*accept)(const char *xbe),
                       void (*progress)(unsigned percent, uint64_t total));

#ifdef __cplusplus
}
#endif
#endif
