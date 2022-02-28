#include <string.h>
#include <ncurses.h>
#include "lmem.h"
#include "lerror.h"
#include "panel.h"
#include "pbot.h"

/* i don't expect to ever be 16 layers deep of menus */
#define ACTIVESIZE 16

WINDOW *wmain;

tPanel_list *activeList[ACTIVESIZE] = { NULL };
int activeListSize = -1;

void printTitle(WINDOW *win, char *text, int width, int x, int y) {
    int pad = (width - (strlen(text) + 4)) / 2;
    mvwprintw(win, y, x+pad, "| %s |", text);
}

void printCenter(WINDOW *win, char *text, int width, int x, int y) {
    int strLength = strlen(text);
    int pad = (width - strLength) / 2;
    mvwprintw(win, y, x, "%*s%s%*s", pad, "", text, (width-pad-strLength), "");
}

void printLine(WINDOW *win, char *text, int width, int x, int y) {
    int strLength = strlen(text);
    int pad = (width - strLength);
    mvwprintw(win, y, x, "%s%*s\n", text, pad, "");
}

void panel_init() {
    if ((wmain = initscr()) == NULL)
        LAIKA_ERROR("Failed to init ncurses!");

    activeListSize = -1;

    /* setup ncurses */
    cbreak();
    noecho();
    timeout(1);
    keypad(stdscr, true);
    curs_set(0); /* hide the default screen cursor. */

    refresh();
}

void panel_cleanUp() {
    /* clean up ncurses */
    delwin(wmain);
    endwin();
}

tPanel_list *panel_newBaseList(size_t sz, LISTTYPE type) {
    tPanel_list *list = laikaM_malloc(sz);

    list->itemHead = NULL;
    list->selectedItem = NULL;
    list->win = NULL;
    list->x = 1;
    list->y = 0;
    list->height = 0;
    list->type = type;
    list->hidden = false;

    return list;
}

void panel_freeBaseList(tPanel_list *list) {
    /* free linked list */
    tPanel_listItem *curr = list->itemHead, *last = NULL;

    while(curr != NULL) {
        last = curr;
        curr = curr->next;
        if (last != NULL)
            panelL_freeListItem(list, last);
    }

    /* remove ncurses window */
    wclear(list->win);
    delwin(list->win);

    /* free list */
    laikaM_free(list);
}

tPanel_list *panel_getActiveList() {
    return activeList[activeListSize];
}

int panel_getChar() {
    /* if we have an activeList panel, grab the input from that otherwise return -1 */
    if (panel_getActiveList() != NULL)
        return wgetch(panel_getActiveList()->win);
    return -1;
}

void panel_pushActiveList(tPanel_list *list) {
    /* set activeList window & draw */
    activeList[++activeListSize] = list;
    panelL_draw(panel_getActiveList());
}

void panel_popActiveList() {
    panelL_freeList(activeList[activeListSize--]);

    if (activeListSize >= 0)
        panelL_init(panel_getActiveList());
}

void panel_draw(void) {
    int i;

    /* draw active panels from bottom most (pushed first) to top most (pushed last)
        this way, new-er windows will be drawn on-top of older ones */
    clear();
    for (i = 0; i <= activeListSize; i++) {
        panelL_draw(activeList[i]);
    }
}

bool panel_tick(int ch) {
    int i;
    /* tick each panel from top most (pushed last) to bottom most (pushed first)
        this way, the top-most windows/lists will be the most interactive */
    
    for (i = activeListSize; i >= 0; i--) {
        if (panelL_tick(activeList[i], ch))
            return true;
    }

    /* no ticks returned any results */
    return false;
}

void panel_setTimeout(int timeout) {
    panelL_setTimeout(panel_getActiveList(), timeout);
}

/* ==================================================[[ List ]]================================================== */

tPanel_list *panelL_newList(void) {
    return (tPanel_list*)panel_newBaseList(sizeof(tPanel_list), LIST_LIST);
}

void panelL_freeList(tPanel_list *list) {
    panel_freeBaseList(list);
}

void panelL_initList(tPanel_list *list) {
    tPanel_listItem *curr = list->itemHead;
    int numLines = 0;

    /* count # of lines */
    while (curr != NULL) {
        numLines++;
        curr = curr->next;
    }

    /* create ncurses window */
    list->x = 0;
    list->y = 2;
    list->width = COLS;
    list->height = numLines;
    list->win = newwin(list->height, list->width, list->y, list->x);
    keypad(list->win, TRUE); /* setup keyboard input */
    wtimeout(list->win, 0);
}

void panelL_drawList(tPanel_list *list) {
    tPanel_listItem *curr = list->itemHead;
    int i = 0;

    /* write each line */
    while (curr != NULL) {
        /* highlight selected option */
        if (curr == list->selectedItem)
            wattron(list->win, A_STANDOUT);

        printLine(list->win, curr->name, list->width, 0, i++);
        wattroff(list->win, A_STANDOUT);
        curr = curr->next;
    }

    /* push to screen */
    wrefresh(list->win);
}

bool panelL_tickList(tPanel_list *list, int ch) {
    switch (ch) {
        case KEY_DOWN: panelL_nextItem(list); break;
        case KEY_UP: panelL_prevItem(list); break;
        default: return false;
    }

    return true;
}

/* ==================================================[[ Tabs ]]================================================== */

tPanel_tabs *panelL_newTabs(char *title) {
    tPanel_tabs *tabs = (tPanel_tabs*)panel_newBaseList(sizeof(tPanel_tabs), LIST_TABS);
    tabs->title = title;
    tabs->list.width = strlen(title) + 4;

    return tabs;
}

void panelL_freeTabs(tPanel_tabs *tabs) {
    panel_freeBaseList((tPanel_list*)tabs);
}

void panelL_initTabs(tPanel_list *tabs) {
    tPanel_listItem *curr = tabs->itemHead;
    int numTabs = 0;

    if (tabs->win)
        delwin(tabs->win);

    /* get tab count */
    while (curr != NULL) {
        curr = curr->next;
        numTabs++;
    }

    tabs->x = 0;
    tabs->y = 0;
    if (numTabs <= 1)
        tabs->width = COLS;
    else
        tabs->width = COLS/numTabs;
    tabs->height = 2;
    tabs->win = newwin(tabs->height, COLS, tabs->y, tabs->x);

    keypad(tabs->win, true);
    wtimeout(tabs->win, 0);
}

void panelL_drawTabs(tPanel_tabs *tabs) {
    tPanel_listItem *curr = tabs->list.itemHead;
    int i = 0;

    /* write title */
    printCenter(tabs->list.win, tabs->title, COLS, 0, 0);

    /* write each tab */
    while (curr != NULL) {
        /* highlight selected option */
        if (curr == tabs->list.selectedItem)
            wattron(tabs->list.win, A_STANDOUT);

        printCenter(tabs->list.win, curr->name, tabs->list.width, (i++)*tabs->list.width, 1);
        curr = curr->next;
        wattroff(tabs->list.win, A_STANDOUT);
    }

    /* draw tab */
    refresh();
    if (tabs->list.selectedItem && tabs->list.selectedItem->child)
        panelL_draw(tabs->list.selectedItem->child);

    wrefresh(tabs->list.win);
}

bool panelL_tickTabs(tPanel_tabs *tabs, int ch) {
    switch (ch) {
        case KEY_RIGHT: panelL_nextItem(&tabs->list); break;
        case KEY_LEFT: panelL_prevItem(&tabs->list); break;
        /* tick child */
        default: return (tabs->list.selectedItem && tabs->list.selectedItem->child) ? panelL_tick(tabs->list.selectedItem->child, ch) : false;
    }

    return true;
}

/* ==================================================[[ Menu ]]================================================== */

tPanel_menu *panelL_newMenu(char *title) {
    tPanel_menu *menu = (tPanel_menu*)panel_newBaseList(sizeof(tPanel_menu), LIST_MENU);
    menu->title = title;
    menu->list.width = strlen(title) + 4;

    return menu;
}

void panelL_freeMenu(tPanel_menu *menu) {
    panel_freeBaseList((tPanel_list*)menu);
}

void panelL_initMenu(tPanel_list *menu) {
    tPanel_listItem *curr = menu->itemHead;

    if (menu->win)
        delwin(menu->win);

    /* get max line length & menu height */
    menu->height = 2;
    while (curr != NULL) {
        if (curr->width+6 > menu->width)
            menu->width = curr->width+6;

        curr = curr->next;
        menu->height++;
    }

    menu->y = (LINES-menu->height)/2;
    menu->x = (COLS-menu->width)/2;

    menu->win = newwin(menu->height, menu->width, menu->y, menu->x);
    keypad(menu->win, true);
    wtimeout(menu->win, 0);
}

void panelL_drawMenu(tPanel_menu *menu) {
    tPanel_listItem *curr = menu->list.itemHead;
    int i = 1;

    /* write each line */
    while (curr != NULL) {
        /* highlight selected option */
        if (curr == menu->list.selectedItem)
            wattron(menu->list.win, A_STANDOUT);

        printLine(menu->list.win, curr->name, menu->list.width-4, 2, i++);
        wattroff(menu->list.win, A_STANDOUT);
        curr = curr->next;
    }

    /* write the title & border */
    box(menu->list.win, 0, 0);
    printTitle(menu->list.win, menu->title, menu->list.width, 0, 0);

    /* push to screen */
    wrefresh(menu->list.win);
}

bool panelL_tickMenu(tPanel_menu *menu, int ch) {
    switch (ch) {
        case KEY_UP: panelL_prevItem(&menu->list); break;
        case KEY_DOWN: panelL_nextItem(&menu->list); break;
        case '\n': case KEY_ENTER: panelL_selectItem(&menu->list); break;
        default: return false;
    }

    panelL_drawMenu(menu);
    return true;
}

/* ==================================================[[ Text ]]================================================== */

tPanel_text *panelL_newText(char *title, char *text) {
    tPanel_text *textP = (tPanel_text*)panel_newBaseList(sizeof(tPanel_text), LIST_TEXT);
    textP->title = title;
    textP->list.width = strlen(title) + 4;

    return textP;
}

void panelL_freeText(tPanel_text *textP) {
    panel_freeBaseList((tPanel_list*)textP);
}

/* ==============================================[[ Panel Direct ]]============================================== */

tPanel_listItem *panelL_newListItem(tPanel_list *list, tPanel_list *child, char *name, panelCallback callback, void *uData) {
    tPanel_listItem *item = laikaM_malloc(sizeof(tPanel_listItem));

    item->child = child;
    item->callback = callback;
    item->uData = uData;
    item->name = name;
    item->last = NULL;
    item->next = NULL;
    item->width = strlen(name);
    item->height = 1;
    item->x = 0;
    item->y = 0;

    /* add to list */
    if (list->itemHead)
        list->itemHead->last = item;

    item->next = list->itemHead;
    list->itemHead = item;
    list->selectedItem = item;

    return item;
}

void panelL_freeListItem(tPanel_list *list, tPanel_listItem *item) {
    if (list->itemHead == item) {
        if (item->next)
            item->next->last = NULL;
        list->itemHead = item->next;
    } else {
        /* unlink from list */
        if (item->last)
            item->last->next = item->next;
        
        if (item->next)
            item->next->last = item->last;
    }

     /* free child */
    if (item->child)
        panelL_freeList(item->child);

    /* update selected item */
    if (list->selectedItem == item)
        list->selectedItem = list->itemHead;

    laikaM_free(item);
}

void panelL_nextItem(tPanel_list *list) {
    if (!list->selectedItem || !list->selectedItem->next) /* sanity check */
        return;

    list->selectedItem = list->selectedItem->next;
}

void panelL_prevItem(tPanel_list *list) {
    if (!list->selectedItem || !list->selectedItem->last) /* sanity check */
        return;

    list->selectedItem = list->selectedItem->last;
}

void panelL_selectItem(tPanel_list *list) {
    if (!list->selectedItem || !list->selectedItem->callback) /* sanity check */
        return;

    list->selectedItem->callback(list->selectedItem->uData);
}

void panelL_init(tPanel_list *list) {
    switch (list->type) {
        case LIST_LIST: panelL_initList(list); break;
        case LIST_TABS: panelL_initTabs(list); break;
        case LIST_MENU: panelL_initMenu(list); break;
        default: LAIKA_ERROR("Malformed tPanel_list!\n"); break;
    }
}

void panelL_free(tPanel_list *list) {
    switch (list->type) {
        case LIST_LIST: panelL_freeList(list); break;
        case LIST_TABS: panelL_freeTabs((tPanel_tabs*)list); break;
        case LIST_MENU: panelL_freeMenu((tPanel_menu*)list); break;
        default: LAIKA_ERROR("Malformed tPanel_list!\n"); break;
    }
}

void panelL_draw(tPanel_list *list) {
    if (list->hidden) /* don't draw a hidden list */
        return;

    switch (list->type) {
        case LIST_LIST: panelL_drawList(list); break;
        case LIST_TABS: panelL_drawTabs((tPanel_tabs*)list); break;
        case LIST_MENU: panelL_drawMenu((tPanel_menu*)list); break;
        default: LAIKA_ERROR("Malformed tPanel_list!\n"); break;
    }
}

bool panelL_tick(tPanel_list *list, int ch) {
    if (list->hidden) /* don't tick a hidden list */
        return false;

    switch (list->type) {
        case LIST_LIST: return panelL_tickList(list, ch);
        case LIST_TABS: return panelL_tickTabs((tPanel_tabs*)list, ch);
        case LIST_MENU: return panelL_tickMenu((tPanel_menu*)list, ch);
        default: return false;
    }
}

void panelL_setHidden(tPanel_list *list, bool hidden) {
    list->hidden = hidden;
}

void panelL_setTimeout(tPanel_list *list, int timeout) {
    wtimeout(list->win, timeout);
}