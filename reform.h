#pragma once
#pragma once

#ifndef __REFORM_H__
#define __REFORM_H__

#include "stdafx.h"
#include <Windows.h>

#include "protoinfo.h"
#include <stdio.h>


struct seq
{
	u_char *pkt_data;				//TCP包信息	
	u_int seq_num;					// 当前的包序列
	u_int nxt_seq;					//下一个包序列
	u_int ack;
	struct seq *next;				//下一个序列号结构
	struct seq *prev;				//上一个包
	bool ackflag;					//序列号得到确认
	u_int count;					//所有TCP数据段长度之和
	unsigned int data_len;			//总数据长度
	unsigned int sign = 0;			//包的标记
};



/** data to order a tcp stream */
typedef struct _order order;
struct _order {
	bool port_diff;       /* different src and dst port  源端口和目的端口不同 是一个完整的双向流*/
	unsigned short port;  /* source port  作为一个源端口，作为客户端，一般一条流第一个syn能确定流 */
	bool ipv6;            /* ipv6 or ipv4 */
	char ip[16];		  /*全局标记IP*/
	char id_s1[16];
	char id_d1[16];
	bool first;			//第一个包

	unsigned long num;	  //待处理的数据包
	unsigned long seq_s;  /* last seq source sent to flow ，客户端方向上一个包序号*/
	unsigned long seq_d;  /* last seq destination sent to flow ，服务端方向上一个包序号*/
	bool mono;            /* stream monodirectional ， 流是单向的*/
	struct seq *src;      /* source packet list ordered ，客户端方向已排序链表，但是存在空洞等，数据包插入到链表时已经根据seq进行了排序，当某种条件满足时，将包发送到流表中*/
	struct seq *dst;      /* destination packet list ordered ，服务端方向已排序链表，但是存在空洞等，数据包插入到链表时已经根据seq进行了排序，当某种条件满足时，将包发送到流表中*/
						  /*  每个方向排好序的数据包链表，且链表插入方法是头插法，后面来的节点在前面 */
	struct seq *ordTcp;	  /*链表*/
	unsigned long fin_s;  /* seq fin source ， 客户端 fin标识序列号*/
	unsigned long fin_d;  /* seq fin destination，服务端 fin标识序列号*/
	bool rst;             /* reset  ，是否收到rst包，收到rst包，代表此条流结束，清空相关结构*/
};



//链表的初始化
void TcpOrderInit(order *ord,const u_char *pkt_data);

//链表的释放
void TcpOrdFree(order* ord);

//链表的数据输出
void TcpOrdPrint(order* ord);

//链表的插入
void TcpOrder(order* ord, const u_char *pkt_data, unsigned int pkt_len);


//链表数据处理
void TcpHandle(order* ord);

//重组是否为http数据包
BOOL ReformHttpPacket(const u_char *pkt_data, unsigned int pkt_len);


//客户端链表建立
void ClientTcpHandle(order* ord, struct seq* Client);

//服务器端链表插入
BOOL ServerTcpInsert(order* ord, struct seq* Server);

//客户端链表插入
BOOL ClientTcpInsert(order* ord, struct seq* Client);


//客户端和服务器输出，测试使用
void ClientOrdPrint(struct seq * data);
#endif