#pragma once
#ifndef __Filter_H__
#define __Filter_H__

#include "stdafx.h"
#include <Windows.h>
#include "protoinfo.h"
#include <stdio.h>

//判断是否为http数据包
BOOL FilterHttpPacket(const u_char *pkt_data, unsigned int pkt_len);

#endif