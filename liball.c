/*******************************************************************
*    (c) Copyright 2009-2025 JRCS Ltd  - See LICENSE for details   *
********************************************************************/
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "ipall.h"
#include "log_message.h"

void * eolncpy(const char * file,const int line, char * dst, char * src,int size_of)
{

    // logmsg(MSG_DEBUG_HIGH,"%s[%d]: Copying '%s', %d bytes -> %08X\n",file,line,src,size_of,dst);

    if (!src) { dst[0]=0; return dst; }
    int len = strlen(src);
    if (!len) { dst[0]=0; return dst; }

    if (len >= size_of) {
        logmsg(MSG_ERROR,"ERROR: %s[%d] - str/size_of exceeded (%ld >= %d)\n",file,line,len,size_of);
        len = size_of;
        }
    MEMCPY(dst,src,len);
    dst[len] = 0;
    return dst+len;
}



void mksin6(struct sockaddr_in6 *sin,struct in6_addr *addr, uint16_t port)
{
    memset(sin,0,sizeof(struct sockaddr_in6));
    sin->sin6_family = AF_INET6;
    sin->sin6_port = htons(port);
    MEMCPY(&sin->sin6_addr,addr,sizeof(struct in6_addr));
}


char * ipchar(in_addr_t addr)
{
struct in_addr ad;

    memset(&ad,0,sizeof(ad));
    ad.s_addr = addr;
    return inet_ntoa(ad);
}



char * ip6char(struct in6_addr * addr)
{
static char ip6[INET6_ADDRSTRLEN+10];

    inet_ntop(AF_INET6,addr,ip6,INET6_ADDRSTRLEN+5);
    return ip6;
}


int set_blocking(int fd,int blocking)
{
int flags = fcntl(fd, F_GETFL);

    if (blocking) flags &= ~O_NONBLOCK; else flags |= O_NONBLOCK;
    return fcntl(fd,F_SETFL,flags);
}

