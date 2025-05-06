/*******************************************************************
*    (c) Copyright 2009-now JRCS Ltd  - See LICENSE for details   *
********************************************************************/
#ifndef _INCLUDE_CLIENT_H
#define _INCLUDE_CLIENT_H

#include "stats.h"

int run_client(int client_fd,struct net_addr_st *to_ni,struct stats_st *client_stats);

#endif
