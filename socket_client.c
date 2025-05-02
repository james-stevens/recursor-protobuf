/*
    #################################################################
    #    (c) Copyright 2009-now JRCS Ltd  - All Rights Reserved    #
    #################################################################
*/

/*
 * socket.c -- socket library functions
 *
 * For license terms, see the file COPYING in this directory.
 */

#include "log_message.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "socket_client.h"
#include "liball.h"


int SockOpenAddr(in_addr_t inaddr, int clientPort, in_addr_t from_addr)
{
int sock,one=1;
struct sockaddr_in ad_v4,from_v4;

	mksin(from_v4,from_addr,0); // port 0 = assign me a client port
    mksin(ad_v4,inaddr,clientPort);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{ logmsg(MSG_ERROR,"ERROR: socket open - %s",ERRMSG); return -1; }

	if (bind(sock, (struct sockaddr *)&from_v4, sizeof (from_v4)))
		{ logmsg(MSG_ERROR,"ERROR: tcp bind - %s:%d - %s",ipchar(from_addr),clientPort,ERRMSG); return -1; }

    if (connect(sock, (struct sockaddr *) &ad_v4, sizeof(ad_v4)) < 0)
		{ close(sock); return -1; }

	if (setsockopt(sock, SOL_SOCKET,SO_KEEPALIVE,(char *)&one,sizeof(one)) < 0)
		log_message(MSG_NORMAL,"SO_KEEPALIVE in SockOpenAddr failed (%s)\n",ERRMSG);

    return(sock);
}




int SockOpen(char *host, int clientPort, in_addr_t from_addr)
{
    unsigned long inaddr;
    struct sockaddr_in ad;
    struct hostent *hp;

    if ((inaddr = inet_addr(host)) == INADDR_NONE)
		{
		if (strlen(host) >= 255) return -1;
        if ((hp = gethostbyname(host)) == NULL) return -1;
        memmove(&ad.sin_addr, hp->h_addr, hp->h_length);
        inaddr = ad.sin_addr.s_addr;
		}
    return SockOpenAddr(inaddr,clientPort,from_addr);
}


int SockRead(int sock, char *buf, int len)
{
    char *newline, *bp = buf;
    int n;

    if (--len < 1)
    return(-1);
    do {
    /*
     * The reason for these gymnastics is that we want two things:
     * (1) to read \n-terminated lines,
     * (2) to return the true length of data read, even if the
     *     data coming in has embedded NULS.
     */
    if ((n = recv(sock, bp, len, MSG_PEEK)) <= 0)
        return(-1);
    if ((newline = memchr(bp, '\n', n)) != NULL)
        n = newline - bp + 1;
    if ((n = read(sock, bp, n)) == -1)
        return(-1);

    bp += n;
    len -= n;
    } while
        (!newline && len);
    *bp = '\0';
    return bp - buf;
}


int SockWrite(int sock, char *buf, int len)
{
int n, wrlen = 0;

    while (len) {
        if ((n = write(sock, buf, len)) <= 0) return -1;
        len -= n;
		wrlen += n;
		buf += n;
		}
    return wrlen;
}



int SockPrintf(int sock, char* format, ...)
{
    va_list ap;
    char buf[8192];

    va_start(ap, format) ;
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    return SockWrite(sock, buf, strlen(buf));

}


int SockOpenAny(struct net_addr_st *target, struct net_addr_st *in_from)
{
int sock,one=1;
struct net_addr_st my_from,*from = in_from;
struct sockaddr_in target_v4,from_v4;
struct sockaddr_in6 target_v6,from_v6;
void *t_addr=NULL,*f_addr=NULL;
int t_sz=0,f_sz=0,domain=0;

	if (!in_from) {
		memset(&my_from,0,sizeof(struct net_addr_st));
		from = &my_from;
		}

	if ((from->is_type)&&(from->is_type!=target->is_type)) return -9;

	if (target->is_type==1) return UnixSockOpen(target->addr.path);

	if (target->is_type==4) {
		mksin(from_v4,from->addr.v4,from->port);
		mksin(target_v4,target->addr.v4,target->port);
		domain = PF_INET;
		t_addr=&target_v4; t_sz=sizeof(target_v4);
		f_addr=&from_v4; f_sz=sizeof(from_v4);
		}

	if (target->is_type==6) {
		mksin6(&from_v6,&from->addr.v6,from->port);
		mksin6(&target_v6,&target->addr.v6,target->port);
		domain = PF_INET6;
		t_addr=&target_v6; t_sz=sizeof(target_v6);
		f_addr=&from_v6; f_sz=sizeof(from_v6);
		}

	if (!t_addr) return -7;

    if ((sock = socket(domain, SOCK_STREAM, 0)) < 0)
		{ logmsg(MSG_ERROR,"ERROR: socket open ret=%d - %s",sock,ERRMSG); return -1; }

	if (bind(sock, f_addr, f_sz) < 0)
		{ logmsg(MSG_ERROR,"ERROR: tcp bind - %s:%d - %s",IPADDRCHAR(*from),from->port,ERRMSG); return -1; }

    if (connect(sock, t_addr,t_sz) < 0) {
    	logmsg(MSG_ERROR,"ERROR: connect failed - %s",ERRMSG);
    	close(sock); return -1; }

	if (setsockopt(sock, SOL_SOCKET,SO_KEEPALIVE,(char *)&one,sizeof(one)) < 0)
		log_message(MSG_NORMAL,"SO_KEEPALIVE in SockOpenAddr failed (%s)\n",ERRMSG);

    return(sock);
}



int UnixSockOpen(char *name)
{
struct sockaddr_un server;
int sock,len = strlen(name);

    memset((char *) &server, 0, sizeof(server));
    server.sun_family = AF_UNIX;
    STRNCPY(server.sun_path, name, sizeof(server.sun_path));
    if (server.sun_path[0]=='@') server.sun_path[0] = 0;

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)  return -1;

    if (connect(sock, (struct sockaddr *) &server, (len + sizeof(sa_family_t)) ) < 0)
        {
        shutdown(sock,2);
        close(sock);
        return -1;
        }

	int one = 1;
	if (setsockopt(sock, SOL_SOCKET,SO_KEEPALIVE,(char *)&one,sizeof(one)) < 0)
		log_message(MSG_NORMAL,"SO_KEEPALIVE in SockOpenAddr failed (%s)\n",ERRMSG);

    return sock;
}


int UnixDgramOpen(char *name,int blocking)
{
struct sockaddr_un server;
int sock;

    memset((char *) &server, 0, sizeof(server));
    server.sun_family = AF_UNIX;
    STRNCPY(server.sun_path, name, sizeof(server.sun_path));

    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)  return -1;

	set_blocking(sock,blocking);

    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
        {
        shutdown(sock,2);
        close(sock);
        return -1;
        }

    return sock;
}
