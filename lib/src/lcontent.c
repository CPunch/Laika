#include "laika.h"
#include "lcontent.h"
#include "lmem.h"
#include "lerror.h"

#include "lsocket.h"
#include "lpeer.h"

#define CONTENTCHUNK_MAX_BODY (LAIKA_MAX_PKTSIZE-sizeof(CONTENT_ID))

/* ===========================================[[ Helper Functions ]]============================================= */

size_t getSize(FILE *fd) {
    size_t sz;
    fseek(fd, 0L, SEEK_END);
    sz = ftell(fd);
    fseek(fd, 0L, SEEK_SET);
    return sz;
}

struct sLaika_content* getContentByID(struct sLaika_contentContext *context, CONTENT_ID id) {
    struct sLaika_content *curr = context->head;

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

void rmvContent(struct sLaika_contentContext *context, struct sLaika_content *content) {
    struct sLaika_content *last = NULL, *curr = context->head;

    while (curr) {
        /* if found, remove it! */
        if (curr == content) {
            if (last)
                last->next = curr->next;
            else
                context->head = curr->next;

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

/* ==============================================[[ Content API ]]=============================================== */

void laikaF_initContext(struct sLaika_contentContext *context) {
    context->head = NULL;
    context->nextID = 0;

    context->onReceived = NULL;
    context->onNew = NULL;
    context->onError = NULL;
}

void laikaF_cleanContext(struct sLaika_contentContext *context) {
    struct sLaika_content *tmp, *curr;

    /* free content list */
    curr = context->head;
    while (curr) {
        tmp = curr->next;
        freeContent(curr);
        curr = tmp;
    }
}

void laikaF_setupEvents(struct sLaika_contentContext *context, contentRecvEvent onRecv, contentNewEvent onNew, contentErrorEvent onError) {
    context->onReceived = onRecv;
    context->onNew = onNew;
    context->onError = onError;
}

struct sLaika_content* laikaF_newContent(struct sLaika_contentContext *context, FILE *fd, size_t sz, CONTENT_ID id, CONTENT_TYPE type, bool direction) {
    struct sLaika_content *content = (struct sLaika_content*)laikaM_malloc(sizeof(struct sLaika_content));

    /* init content struct */
    content->fd = fd;
    content->sz = sz;
    content->processed = 0;
    content->id = id;
    content->type = type;
    content->direction = direction;

    /* add to list */
    content->next = context->head;
    context->head = content;

    return content;
}

int laikaF_nextID(struct sLaika_peer *peer) {
    return peer->context.nextID + 1;
}

int laikaF_sendContent(struct sLaika_peer *peer, FILE *fd, CONTENT_TYPE type) {
    struct sLaika_contentContext *context = &peer->context;
    struct sLaika_content *content = laikaF_newContent(context, fd, getSize(fd), context->nextID++, type, CONTENT_OUT);

    /* let the peer know we're sending them some content */
    laikaS_startOutPacket(peer, LAIKAPKT_CONTENT_NEW);
    laikaS_writeInt(&peer->sock, &content->id, sizeof(CONTENT_ID));
    laikaS_writeInt(&peer->sock, &content->sz, sizeof(uint32_t));
    laikaS_writeByte(&peer->sock, type);
    laikaS_endOutPacket(peer);

    return content->id;
}

/* new content we're recieving from a peer */
struct sLaika_content* laikaF_recvContent(struct sLaika_peer *peer, CONTENT_ID id, uint32_t sz, CONTENT_TYPE type) {
    struct sLaika_contentContext *context = &peer->context;

    if (getContentByID(context, id)) {
        sendContentError(peer, id, CONTENT_ERR_ID_IN_USE);
        LAIKA_ERROR("ID [%d] is in use!\n", id);
    }

    return laikaF_newContent(context, tmpfile(), sz, id, type, CONTENT_IN);
}

void laikaF_pollContent(struct sLaika_peer *peer) {
    uint8_t buff[CONTENTCHUNK_MAX_BODY];
    struct sLaika_contentContext *context = &peer->context;
    struct sLaika_content *tmp, *curr = context->head;
    int rd;

    /* traverse our out content, sending each chunk */
    while (curr) {
        if (curr->direction == CONTENT_OUT) {
            /* if we've reached the end of the file stream, remove it! */
            if (rd = fread(buff, sizeof(uint8_t), MIN(curr->sz - curr->processed, CONTENTCHUNK_MAX_BODY), curr->fd) == 0) {
                tmp = curr->next;
                rmvContent(context, curr);
                curr = tmp;
                continue;
            }

            /* send chunk */
            laikaS_startVarPacket(peer, LAIKAPKT_CONTENT_CHUNK);
            laikaS_writeInt(&peer->sock, &curr->id, sizeof(CONTENT_ID));
            laikaS_write(&peer->sock, buff, rd);
            laikaS_endVarPacket(peer);
            
            curr->processed += rd;
        }

        curr = curr->next;
    }
}

/* ============================================[[ Packet Handlers ]]============================================= */

void laikaF_handleContentNew(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_contentContext *context = &peer->context;
    struct sLaika_content *content;
    uint32_t contentSize;
    CONTENT_ID contentID;
    CONTENT_TYPE contentType;

    laikaS_readInt(&peer->sock, &contentID, sizeof(CONTENT_ID));
    laikaS_readInt(&peer->sock, &contentSize, sizeof(uint32_t));
    contentType = laikaS_readByte(&peer->sock);

    content = laikaF_recvContent(peer, contentID, contentSize, contentType);
    if (context->onNew && !context->onNew(peer, context, content)) {
        sendContentError(peer, contentID, CONTENT_ERR_REJECTED);
        rmvContent(context, content);
    }
}

void laikaF_handleContentError(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_contentContext *context = &peer->context;
    struct sLaika_content *content;
    CONTENT_ID contentID;
    uint8_t errCode;

    laikaS_readInt(&peer->sock, &contentID, sizeof(CONTENT_ID));
    errCode = laikaS_readByte(&peer->sock);

    if ((content = getContentByID(context, contentID)) == NULL)
        LAIKA_ERROR("Received error for non-existant id %d!\n", contentID);

    LAIKA_DEBUG("We received an errcode for id %d, err: %d\n", contentID, errCode);
    if (context->onError) /* check if event exists! */
        context->onError(peer, context, content, errCode);

    rmvContent(context, content);
}

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
    if ((content = getContentByID(context, id)) == NULL || content->direction != CONTENT_IN)
        LAIKA_ERROR("chunk recieved with invalid id! [%d]\n", id);

    /* read data & write to file */
    laikaS_read(&peer->sock, buff, bodySz);
    if (fwrite(buff, sizeof(uint8_t), bodySz, content->fd) != bodySz) {
        rmvContent(context, content);
    } else if ((content->processed += bodySz) == content->sz) {
        if (context->onReceived) /* check if event exists! */
            context->onReceived(peer, context, content);
    }
}
