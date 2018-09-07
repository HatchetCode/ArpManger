#pragma once

#ifndef __Parse_H__
#define __Parse_H__

#include "stdafx.h"
#include <Windows.h>

#include "protoinfo.h"
#include <stdio.h>





//判断是否为http数据包
BOOL ParseHttpPacket(const u_char *pkt_data, unsigned int pkt_len);

#endif