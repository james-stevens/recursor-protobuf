/*******************************************************************
*    (c) Copyright 2009-2025 JRCS Ltd  - See LICENSE for details   *
********************************************************************/
#include <time.h>
#include <limits.h>
#include <netinet/in.h>
#include "ipall.h"

#define MEMCPY memmove

extern int set_blocking(int fd,int blocking);
extern void *eolncpy(const char * file,const int line, char * dst, char * src,int size_of);
#define STRECAT(D,E,S)  eolncpy(__FILE__,__LINE__,(char *)D+strlen((char *)D),(char *)S,((char *)E-(char *)D))
#define STRECPY(D,E,S)  eolncpy(__FILE__,__LINE__,(char *)D,(char *)S,((char *)E-(char *)D))
#define STRNCPY(D,S,N)  eolncpy(__FILE__,__LINE__,D,S,N)
#define STRCPY(D,S)     eolncpy(__FILE__,__LINE__,D,S,sizeof(D))
#define STRCAT(D,S)     eolncpy(__FILE__,__LINE__,(char *)D+strlen((char *)D),S,sizeof(D)-strlen(D))
extern void mksin6(struct sockaddr_in6 *sin,struct in6_addr *addr, unsigned short port);
#define IPADDRCHAR(A) (((A).is_type==4)?ipchar((A).addr.v4):ip6char(&((A).addr.v6)))
extern char * ip6char(struct in6_addr * addr);
extern char * ipchar(in_addr_t addr);
