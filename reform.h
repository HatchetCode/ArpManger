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
	u_char *pkt_data;				//TCP����Ϣ	
	u_int seq_num;					// ��ǰ�İ�����
	u_int nxt_seq;					//��һ��������
	u_int ack;
	struct seq *next;				//��һ�����кŽṹ
	struct seq *prev;				//��һ����
	bool ackflag;					//���кŵõ�ȷ��
	u_int count;					//����TCP���ݶγ���֮��
	unsigned int data_len;			//�����ݳ���
	unsigned int sign = 0;			//���ı��
};



/** data to order a tcp stream */
typedef struct _order order;
struct _order {
	bool port_diff;       /* different src and dst port  Դ�˿ں�Ŀ�Ķ˿ڲ�ͬ ��һ��������˫����*/
	unsigned short port;  /* source port  ��Ϊһ��Դ�˿ڣ���Ϊ�ͻ��ˣ�һ��һ������һ��syn��ȷ���� */
	bool ipv6;            /* ipv6 or ipv4 */
	char ip[16];		  /*ȫ�ֱ��IP*/
	char id_s1[16];
	char id_d1[16];
	bool first;			//��һ����

	unsigned long num;	  //����������ݰ�
	unsigned long seq_s;  /* last seq source sent to flow ���ͻ��˷�����һ�������*/
	unsigned long seq_d;  /* last seq destination sent to flow ������˷�����һ�������*/
	bool mono;            /* stream monodirectional �� ���ǵ����*/
	struct seq *src;      /* source packet list ordered ���ͻ��˷����������������Ǵ��ڿն��ȣ����ݰ����뵽����ʱ�Ѿ�����seq���������򣬵�ĳ����������ʱ���������͵�������*/
	struct seq *dst;      /* destination packet list ordered ������˷����������������Ǵ��ڿն��ȣ����ݰ����뵽����ʱ�Ѿ�����seq���������򣬵�ĳ����������ʱ���������͵�������*/
						  /*  ÿ�������ź�������ݰ�������������뷽����ͷ�巨���������Ľڵ���ǰ�� */
	struct seq *ordTcp;	  /*����*/
	unsigned long fin_s;  /* seq fin source �� �ͻ��� fin��ʶ���к�*/
	unsigned long fin_d;  /* seq fin destination������� fin��ʶ���к�*/
	bool rst;             /* reset  ���Ƿ��յ�rst�����յ�rst������������������������ؽṹ*/
};



//����ĳ�ʼ��
void TcpOrderInit(order *ord,const u_char *pkt_data);

//������ͷ�
void TcpOrdFree(order* ord);

//������������
void TcpOrdPrint(order* ord);

//����Ĳ���
void TcpOrder(order* ord, const u_char *pkt_data, unsigned int pkt_len);


//�������ݴ���
void TcpHandle(order* ord);

//�����Ƿ�Ϊhttp���ݰ�
BOOL ReformHttpPacket(const u_char *pkt_data, unsigned int pkt_len);


//�ͻ���������
void ClientTcpHandle(order* ord, struct seq* Client);

//���������������
BOOL ServerTcpInsert(order* ord, struct seq* Server);

//�ͻ����������
BOOL ClientTcpInsert(order* ord, struct seq* Client);


//�ͻ��˺ͷ��������������ʹ��
void ClientOrdPrint(struct seq * data);
#endif