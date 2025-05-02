/*******************************************************************
*    (c) Copyright 2009-now JRCS Ltd  - See LICENSE for details   *
********************************************************************/
#ifndef _INCLUDE_SOCKET_SERVER_H_
#define _INCLUDE_SOCKET_SERVER_H_

#include <netinet/in.h>
#include "ipall.h"

extern int init_tcp_server(in_addr_t addr,int port,int blocking,int quelen);
extern int init_unix_server(char * sock_file,int blocking,int quelen);
extern int init_tcp6_server(struct in6_addr *addr,int port,int blocking,int quelen);
extern int init_dgram_server(char * sock_file,int blocking);
extern int tcp_server_any(struct net_addr_st *l,int blocking);

#endif // _INCLUDE_SOCKET_SERVER_H_
