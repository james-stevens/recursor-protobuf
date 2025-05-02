/*******************************************************************
*	(c) Copyright 2009-now JRCS Ltd  - See LICENSE for details   *
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>
#include <fcntl.h>

#include "liball.h"
#include "ipall.h"
#include "log_message.h"


int init_tcp_server(in_addr_t addr,int port,int blocking,int quelen)
{
struct sockaddr_in ad;
struct in_addr bind_address;
int one = 1,sock;
struct linger ling;

	memset(&ad, 0, sizeof(ad));
	memset(&bind_address, 0, sizeof(bind_address));
	memset(&ling,0,sizeof(struct linger));
	ling.l_onoff = 1;
	ling.l_linger = 3;
	bind_address.s_addr = addr;
	ad.sin_family = AF_INET;
	ad.sin_addr = bind_address;
	ad.sin_port = htons(port);

	log_message(MSG_DEBUG,"Opening listening sock on port %s:%d\n",ipchar(addr),port);
	error_exit( ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) , "socket");

	error_exit( (setsockopt(sock, SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one)) < 0) , "sock SO_REUSEADDR");
	error_exit( (setsockopt(sock, SOL_SOCKET,SO_KEEPALIVE,(char *)&one,sizeof(one)) < 0) , "sock SO_KEEPALIVE");
	error_exit( setsockopt(sock, SOL_SOCKET,SO_LINGER,(char *)&ling,sizeof(struct linger))  , "sock SO_LINGER");

	error_exit( bind(sock,(struct sockaddr *)&ad,sizeof(ad)) , "TCP bind");
	error_exit( listen(sock,quelen) , "listen");

	set_blocking(sock,blocking);

	return sock;
}



int init_unix_server(char * sock_file,int blocking,int quelen)
{
int sock;
unsigned int len = strlen(sock_file);
struct sockaddr_un server;

	if (len > sizeof(server.sun_path)) return -1;

	memset((char *) &server, 0, sizeof(server));
	server.sun_family = AF_UNIX;

	STRCPY(server.sun_path, sock_file);
	if (server.sun_path[0]=='@') server.sun_path[0] = 0;

	error_exit( ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) , "socket");
	error_exit( bind(sock,(struct sockaddr *) &server, (len + sizeof(sa_family_t))) , "bind");
	error_exit( listen(sock,quelen) , "listen");

	set_blocking(sock,blocking);

	return sock;
}





int init_dgram_server(char * sock_file,int blocking)
{
int sock;
struct sockaddr_un server;

	memset((char *) &server, 0, sizeof(server));
	server.sun_family = AF_UNIX;
	STRNCPY(server.sun_path, sock_file, sizeof(server.sun_path)-1);

	error_exit( ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) , "socket");
	error_exit( bind(sock,(struct sockaddr *) &server, sizeof(struct sockaddr_un)) , "bind");

	set_blocking(sock,blocking);

	return sock;
}



int init_tcp6_server(struct in6_addr *addr,int port,int blocking,int quelen)
{
struct sockaddr_in6 ad;
int one = 1,sock;
struct linger ling;

	mksin6(&ad,addr,port);

	memset(&ling,0,sizeof(struct linger));
	ling.l_onoff = 0;

	log_message(MSG_HIGH,"Opening listening v6 sock on port %s:%d\n",ip6char(addr),port);
	error_exit( ((sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0) , "socket v6");
	error_exit( (setsockopt(sock, SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one)) < 0) , "sock v6 SO_REUSEADDR");
	error_exit( (setsockopt(sock, SOL_SOCKET,SO_KEEPALIVE,(char *)&one,sizeof(one)) < 0) , "sock v6 SO_KEEPALIVE");
	error_exit( setsockopt(sock, SOL_SOCKET,SO_LINGER,(char *)&ling,sizeof(struct linger))  , "sock v6 SO_LINGER");
	error_exit( bind(sock,(struct sockaddr *)&ad,sizeof(ad)) , "TCP v6 bind");
	error_exit( listen(sock,quelen) , "listen v6");

	set_blocking(sock,blocking);

	return sock;
}



int tcp_server_any(struct net_addr_st *l,int blocking)
{
	switch(l->is_type) {
		case 1: return init_unix_server(l->addr.path,blocking,50); break;
		case 4: return init_tcp_server(l->addr.v4,l->port,blocking,50); break;
		case 6: return init_tcp6_server(&l->addr.v6,l->port,blocking,50); break;
		}
	return -1;
}
