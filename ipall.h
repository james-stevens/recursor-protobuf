/*******************************************************************
*	(c) Copyright 2009-now JRCS Ltd  - See LICENSE for details   *
********************************************************************/
#ifndef _INCLUDE_IPALL_H_
#define _INCLUDE_IPALL_H_

#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>

struct net_addr_st {
	uint16_t port;
	uint8_t is_type;
	union addr_u {
		in_addr_t v4;
		struct in6_addr v6;
		char path[PATH_MAX];
		} addr;
	};

#endif // _INCLUDE_LIBALL_H_
