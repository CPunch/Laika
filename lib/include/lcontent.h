#ifndef LAIKA_CONTENT_H
#define LAIKA_CONTENT_H

#include "laika.h"
#include "lpacket.h"

#include <stdio.h>
#include <inttypes.h>

enum {
    CONTENT_IN = true, /* being recv'd from peer */
    CONTENT_OUT = false /* being sent to peer */
};

enum {
    CONTENT_TYPE_FILE
};

enum {
    CONTENT_ERR_ID_IN_USE,
    CONTENT_ERR_INVALID_ID,
    CONTENT_ERR_REJECTED
};

typedef uint8_t CONTENT_TYPE;
typedef uint8_t CONTENT_ERRCODE;
typedef uint16_t CONTENT_ID;

typedef void (*contentRecvEvent)(struct sLaika_peer *peer, struct sLaika_contentContext *context, struct sLaika_content *content);
typedef bool (*contentNewEvent)(struct sLaika_peer *peer, struct sLaika_contentContext *context, struct sLaika_content *content);
typedef void (*contentErrorEvent)(struct sLaika_peer *peer, struct sLaika_contentContext *context, struct sLaika_content *content, CONTENT_ERRCODE err);

struct sLaika_content {
    struct sLaika_content *next;
    FILE *fd;
    uint32_t sz; /* content size */
    uint32_t processed;
    CONTENT_ID id;
    CONTENT_TYPE type;
    bool direction;
};

struct sLaika_contentContext {
    struct sLaika_content *head;
    CONTENT_ID nextID;
    contentRecvEvent onReceived;
    contentNewEvent onNew;
    contentErrorEvent onError;
};

void laikaF_initContext(struct sLaika_contentContext *context);
void laikaF_cleanContext(struct sLaika_contentContext *context);

void laikaF_setupEvents(struct sLaika_contentContext *context, contentRecvEvent onRecv, contentNewEvent onNew, contentErrorEvent onError);

int laikaF_nextID(struct sLaika_peer *peer); /* returns the id that will be assigned to the next sent content */
int laikaF_sendContent(struct sLaika_peer *peer, FILE *fd, CONTENT_TYPE type);

void laikaF_pollContent(struct sLaika_peer *peer);

void laikaF_handleContentNew(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaF_handleContentError(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaF_handleContentChunk(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);

#endif