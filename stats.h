/*******************************************************************
*	(c) Copyright 2009-now JRCS Ltd  - See LICENSE for details   *
********************************************************************/
#ifndef _INCLUDE_STATS_H_
#define _INCLUDE_STATS_H_

#include <unistd.h>

struct stats_st {
	uint64_t in_bytes,out_bytes,in_pkts,out_kpts,lost_pkts;
	pid_t pid;
	};

#endif // _INCLUDE_STATS_H_
