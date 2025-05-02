/*
    #################################################################
    #    (c) Copyright 2009-now JRCS Ltd  - All Rights Reserved    #
    #################################################################
*/

#ifndef _SOCKET_CLIENT
#define _SOCKET_CLIENT

#include <sys/socket.h>
#include <netinet/in.h>
#include "liball.h"

extern int SockOpen(char *host, int clientPort,in_addr_t from_addr);
extern int SockOpenAddr(in_addr_t addr, int clientPort,in_addr_t from_addr);
extern int SockOpenAny(struct net_addr_st *target, struct net_addr_st *from);
extern int SockRead(int sock, char *buf, int len);
extern int SockWrite(int sock, char *buf, int len);
extern int SockPrintf(int sock, char* format, ...);
extern int UnixSockOpen(char *name);
extern int UnixDgramOpen(char *name,int blocking);

#endif // _SOCKET_CLIENT
