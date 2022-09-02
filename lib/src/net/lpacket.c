#include "net/lpacket.h"

#ifdef DEBUG
const char *laikaD_getPacketName(LAIKAPKT_ID id)
{
    const char *PKTNAMES[] = {"LAIKAPKT_VARPKT",
                              "LAIKAPKT_HANDSHAKE_REQ",
                              "LAIKAPKT_HANDSHAKE_RES",
                              "LAIKAPKT_PEER_LOGIN_REQ",
                              "LAIKAPKT_PINGPONG",
                              "LAIKAPKT_SHELL_OPEN",
                              "LAIKAPKT_SHELL_CLOSE",
                              "LAIKAPKT_SHELL_DATA",
                              "LAIKAPKT_AUTHENTICATED_ADD_PEER_RES",
                              "LAIKAPKT_AUTHENTICATED_RMV_PEER_RES",
                              "LAIKAPKT_AUTHENTICATED_SHELL_OPEN_REQ"};

    return id >= LAIKAPKT_MAXNONE ? "LAIKAPKT_UNKNOWN" : PKTNAMES[id];
}
#endif