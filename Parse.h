#pragma once

#ifndef __Parse_H__
#define __Parse_H__

#include "stdafx.h"
#include <Windows.h>

#include "protoinfo.h"
#include <stdio.h>





//�ж��Ƿ�Ϊhttp���ݰ�
BOOL ParseHttpPacket(const u_char *pkt_data, unsigned int pkt_len);

#endif