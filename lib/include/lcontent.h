#ifndef LAIKA_CONTENT_H
#define LAIKA_CONTENT_H

#include "laika.h"
#include "lpacket.h"

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
typedef uint16_t CONTENT_ID;

struct sLaika_content {
    struct sLaika_content *next;
    FILE *fd;
    uint32_t sz; /* content size */
    uint32_t processed;
    CONTENT_ID id;
    CONTENT_TYPE type;
};

struct sLaika_contentContext {
    struct sLaika_content *headIn; /* receiving from peer */
    struct sLaika_content *headOut; /* sending to peer */
    CONTENT_ID nextID;
};

void laikaF_initContext(struct sLaika_contentContext *context);
void laikaF_cleanContext(struct sLaika_contentContext *context);

int laikaF_newOutContent(struct sLaika_peer *peer, FILE *fd, CONTENT_TYPE type);
void laikaF_newInContent(struct sLaika_peer *peer, CONTENT_ID id, uint32_t sz, CONTENT_TYPE type);

void laikaF_pollContent(struct sLaika_peer *peer);

void laikaF_handleContentChunk(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);

#endif