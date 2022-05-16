#ifndef LAIKA_CONTENT_H
#define LAIKA_CONTENT_H

#include "lpeer.h"

#include <stdio.h>
#include <inttypes.h>

enum {
    CONTENT_FILE,
    CONTENT_PAYLOAD
};

enum {
    CONTENT_ERR_ID_IN_USE,
    CONTENT_ERR_INVALID_ID
};

typedef uint8_t CONTENT_TYPE;
typedef uint8_t CONTENT_ERRCODE;

struct sLaika_content {
    FILE *fd;
    uint32_t sz; /* content size */
    uint32_t processed;
    uint16_t id;
    CONTENT_TYPE type;
    struct sLaika_content *next;
};

struct sLaika_contentContext {
    struct sLaika_peer *peer;
    struct sLaika_content *headIn; /* receiving from peer */
    struct sLaika_content *headOut; /* sending to peer */
    uint16_t nextID;
};

void laikaF_initContext(struct sLaika_contentContext *context, struct sLaika_peer *peer);
void laikaF_cleanContext(struct sLaika_contentContext *context);

int laikaF_newOutContent(struct sLaika_contentContext *context, FILE *fd, CONTENT_TYPE type);
void laikaF_newInContent(struct sLaika_contentContext *context, uint16_t id, uint32_t sz, CONTENT_TYPE type);

void laikaF_poll(struct sLaika_contentContext *context);

#endif