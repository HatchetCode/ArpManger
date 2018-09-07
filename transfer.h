#pragma once
#include "curl.h"

extern CHAR* g_node_url;

BOOL TransferData(const u_char *pkt_data, unsigned int pkt_len);