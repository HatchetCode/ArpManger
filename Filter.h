#pragma once
#ifndef __Filter_H__
#define __Filter_H__

#include "stdafx.h"
#include <Windows.h>
#include "protoinfo.h"
#include <stdio.h>

//�ж��Ƿ�Ϊhttp���ݰ�
BOOL FilterHttpPacket(const u_char *pkt_data, unsigned int pkt_len);

#endif