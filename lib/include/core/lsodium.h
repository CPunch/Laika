#ifndef LAIKA_RSA_H
#define LAIKA_RSA_H

#include "lconfig.h"
#include "sodium.h"

#include <stdbool.h>

#define LAIKAENC_SIZE(sz) (sz + crypto_box_SEALBYTES)

bool laikaK_loadKeys(uint8_t *outPub, uint8_t *outPriv, const char *inPub, const char *inPriv);
bool laikaK_genKeys(uint8_t *outPub, uint8_t *outPriv);

bool laikaK_checkAuth(uint8_t *pubKey, uint8_t **authKeys, int keys);

#endif
