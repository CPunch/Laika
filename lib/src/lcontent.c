#include "lcontent.h"
#include "lmem.h"
#include "lerror.h"

#include "lsocket.h"
#include "lpeer.h"

size_t getSize(FILE *fd) {
    size_t sz;
    fseek(fd, 0L, SEEK_END);
    sz = ftell(fd);
    fseek(fd, 0L, SEEK_SET);
    return sz;
}

bool isValidID(struct sLaika_contentContext *context, uint16_t id) {
    struct sLaika_content *curr = context->headIn;

    while (curr) {
        if (curr->id == id) 
            return true;

        curr = curr->next;
    }

    return false;
}

void freeContent(struct sLaika_content *content) {
    fclose(content->fd);
    laikaM_free(content);
}

void rmvFromOut(struct sLaika_contentContext *context, struct sLaika_content *content) {
    struct sLaika_content *last = NULL, *curr = context->headOut;

    while (curr) {
        /* if found, remove it! */
        if (curr == content) {
            if (last)
                last->next = curr->next;
            else
                context->headOut = curr->next;

            freeContent(curr);
            break;
        }

        last = curr;
        curr = curr->next;
    }
}

void sendContentError(struct sLaika_peer *peer, uint16_t id, CONTENT_ERRCODE err) {
    laikaS_startOutPacket(peer, LAIKAPKT_CONTENT_ERROR);
    laikaS_writeInt(&peer->sock, &id, sizeof(uint16_t));
    laikaS_writeByte(&peer->sock, err);
    laikaS_endOutPacket(peer);
}

void laikaF_initContext(struct sLaika_contentContext *context, struct sLaika_peer *peer) {
    context->peer = peer;
    context->headIn = NULL;
    context->headOut = NULL;
    context->nextID = 0;
}

void laikaF_cleanContext(struct sLaika_contentContext *context) {
    struct sLaika_content *tmp, *curr;

    /* free IN list */
    curr = context->headIn;
    while (curr) {
        tmp = curr->next;
        freeContent(curr);
        curr = tmp;
    }

    /* free OUT list */
    curr = context->headOut;
    while (curr) {
        tmp = curr->next;
        freeContent(curr);
        curr = tmp;
    }
}

int laikaF_newOutContent(struct sLaika_contentContext *context, FILE *fd, CONTENT_TYPE type) {
    struct sLaika_content *content = (struct sLaika_content*)laikaM_malloc(sizeof(struct sLaika_content));
    struct sLaika_peer *peer = context->peer;

    /* init content struct */
    content->fd = fd;
    content->sz = getSize(fd);
    content->processed = 0;
    content->id = context->nextID++;
    content->type = type;

    /* add to list */
    content->next = context->headOut;
    context->headOut = content;

    /* let the peer know we're sending them some content */
    laikaS_startOutPacket(peer, LAIKAPKT_CONTENT_NEW);
    laikaS_writeInt(&peer->sock, &content->id, sizeof(uint16_t));
    laikaS_writeInt(&peer->sock, &content->sz, sizeof(uint32_t));
    laikaS_writeByte(&peer->sock, type);
    laikaS_endOutPacket(peer);
}

void laikaF_newInContent(struct sLaika_contentContext *context, uint16_t id, uint32_t sz, CONTENT_TYPE type) {
    struct sLaika_content *content = (struct sLaika_content*)laikaM_malloc(sizeof(struct sLaika_content));

    if (isValidID(context, id)) {
        sendContentError(context->peer, id, CONTENT_ERR_ID_IN_USE);
        LAIKA_ERROR("ID [%d] is in use!\n", id);
    }

    content->fd = tmpfile();
    content->sz = sz;
    content->processed = 0;
    content->id = id;
    content->type = type;

    /* add to list */
    content->next = context->headIn;
    context->headIn = content;
}

void laikaF_poll(struct sLaika_contentContext *context) {
    uint8_t buff[LAIKA_MAX_PKTSIZE-sizeof(uint16_t)];
    struct sLaika_peer *peer = context->peer;
    struct sLaika_content *tmp, *curr = context->headOut;
    int rd;

    /* traverse our out content, sending each chunk */
    while (curr) {
        /* if we've reached the end of the file stream, remove it! */
        if (rd = fread(buff, sizeof(uint8_t), (curr->sz - curr->processed > (LAIKA_MAX_PKTSIZE-sizeof(uint16_t)) ? LAIKA_MAX_PKTSIZE-sizeof(uint16_t) : curr->sz - curr->processed), curr->fd) == 0) {
            tmp = curr->next;
            rmvFromOut(context, curr);
            curr = tmp;
            continue;
        }

        laikaS_startVarPacket(peer, LAIKAPKT_CONTENT_CHUNK);
        laikaS_writeInt(&peer->sock, &curr->id, sizeof(uint16_t));
        laikaS_write(&peer->sock, buff, rd);
        laikaS_endVarPacket(peer);

        curr = curr->next;
    }
}