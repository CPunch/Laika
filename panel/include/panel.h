#ifndef PANELMENU_H
#define PANELMENU_H

#include <ncurses.h>
#include <stdbool.h>

#define PANEL_CURSES_TIMEOUT 100
#define COMMONPANELLIST tPanel_list list;

typedef void (*panelCallback)(void *uData);

typedef enum {
    LIST_LIST,
    LIST_TABS,
    LIST_MENU,
    LIST_TEXT,
    LIST_NONE
} LISTTYPE;

struct sPanel_list;
typedef struct sPanel_listItem {
    panelCallback callback;
    struct sPanel_list *child;
    void *uData;
    char *name;
    struct sPanel_listItem *next;
    struct sPanel_listItem *last;
    int x, y, width, height;
} tPanel_listItem;

typedef struct sPanel_list {
    tPanel_listItem *itemHead, *selectedItem;
    WINDOW *win;
    int x, y, width, height;
    LISTTYPE type;
    bool hidden;
} tPanel_list;

typedef struct sPanel_tabs {
    COMMONPANELLIST;
    char *title;
} tPanel_tabs;

typedef struct sPanel_menu {
    COMMONPANELLIST;
    char *title;
} tPanel_menu;

typedef struct sPanel_text {
    COMMONPANELLIST;
    char *title;
    char *text;
} tPanel_text;

extern WINDOW *wmain;
extern tPanel_list *panel_botList;
extern tPanel_tabs *panel_tabList;

void panel_init(void);
void panel_cleanUp(void);

tPanel_list *panel_getActiveList(void);
int panel_getChar(void);
void panel_pushActiveList(tPanel_list *list);
void panel_popActiveList(void); /* also free's the item */
void panel_draw(void);
bool panel_tick(int input); /* ticks activeList list, returns true if input was accepted/valid */
void panel_setTimeout(int timeout);

tPanel_listItem *panelL_newListItem(tPanel_list *list, tPanel_list *child, char *name, panelCallback callback, void *uData);
void panelL_freeListItem(tPanel_list *list, tPanel_listItem *item);

/* call *after* adding listItems with panelL_newListItem */
void panelL_init(tPanel_list *list);
void panelL_draw(tPanel_list *list);
void panelL_free(tPanel_list *list);
bool panelL_tick(tPanel_list *list, int ch);
void panelL_setHidden(tPanel_list *list, bool hidden);
void panelL_setTimeout(tPanel_list *list, int timeout);

/* handles selection */
void panelL_nextItem(tPanel_list *list);
void panelL_prevItem(tPanel_list *list);
void panelL_selectItem(tPanel_list *list); /* calls select item's callback */

/* center list */
tPanel_list *panelL_newList(void);
void panelL_freeList(tPanel_list *list);

/* top tabs */
tPanel_tabs *panelL_newTabs(char *title);
void panelL_freeTabs(tPanel_tabs *tabs);

/* menu popup */
tPanel_menu *panelL_newMenu(char *title);
void panelL_freeMenu(tPanel_menu *menu);

/* textbot popup */
tPanel_text *panelL_newText(char *title, char *text);
void panelL_freeText(tPanel_text *text);

#undef COMMONPANELLIST

#endif