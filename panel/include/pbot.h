#ifndef PBOT_H
#define PBOT_H

#include "laika.h"
#include "lpeer.h"
#include "lrsa.h"

#include "panel.h"

typedef struct sPanel_bot {
    uint8_t pub[crypto_kx_PUBLICKEYBYTES];
    PEERTYPE type;
    tPanel_listItem *item;
    char *name; /* heap allocated string */
} tPanel_bot;

tPanel_bot *panelB_newBot(uint8_t *pubKey);
void panelB_freeBot(tPanel_bot *bot);

/* search connected bots by public key */
tPanel_bot *panelB_getBot(uint8_t *pubKey);
void panelB_setItem(tPanel_bot *bot, tPanel_listItem *item);

#endif