#include "reform.h"
#include "Filter.h"

order *ord;
BOOL g_direct = FALSE;	//FLASE代表是客户端->服务器端，TRUE代表是服务器端->客户端


//链表的初始化
void TcpOrderInit(const u_char *pkt_data, order *ord)
{
	ETHeader *eh;
	IPHeader *ih;
	TCPHeader *th;
	UDPHeader *uh;
	u_int ip_len, ipLength, tcpLength;
	char szSource[16], szDest[16];
	u_short sport, dport;
	char* result = NULL;
	u_int sequenceNumber;
	u_int acknowledgeNumber;
	eh = (ETHeader *)pkt_data;
	ih = (IPHeader *)(pkt_data + 14);
	ip_len = (ih->iphVerLen & 0xf) * 4;       //ip头长度，版本号+头长度
	ipLength = ntohs(ih->ipLength);			  //报文总长度

	//获取ip地址
	wsprintf(szSource, "%d.%d.%d.%d",
		ih->ipSourceByte.byte1, ih->ipSourceByte.byte2,
		ih->ipSourceByte.byte3, ih->ipSourceByte.byte4);

	wsprintf(szDest, "%d.%d.%d.%d",
		ih->ipDestinationByte.byte1, ih->ipDestinationByte.byte2,
		ih->ipDestinationByte.byte3, ih->ipDestinationByte.byte4);

	memcpy(ord->id_d1, szDest, 16);
	memcpy(ord->id_s1, szSource, 16);
	memcpy(ord->ip, szSource, 16);


	ord->src = NULL;
	ord->dst = NULL;
	ord->num = 0;

	//TcpOrdPrint(ord);

}


//客户端和服务器输出，测试使用
void ClientOrdPrint(struct seq * data)
{
	ETHeader *eh;
	IPHeader *ih;
	TCPHeader *th;
	UDPHeader *uh;
	u_int ip_len, ipLength, tcpLength;
	char szSource[16], szDest[16];
	u_short sport, dport;
	char* result = NULL;
	ULONG sequenceNumber;
	ULONG acknowledgeNumber;



	struct  seq *p;
	printf("\n**********************链表数据部分*******************************\n");

	printf("ord->ip:%s\n", ord->ip);
	printf("ord->id_d1:%s\n", ord->id_d1);
	printf("ord->id_s1:%s\n", ord->id_s1);
	printf("ord->num:%d\n", ord->num);

	p = data;
	while (p->next != NULL)
	{
		p = p->next;
	}
	while (p->prev != NULL)
	{
		eh = (ETHeader *)p->pkt_data;
		ih = (IPHeader *)(p->pkt_data + 14);
		ip_len = (ih->iphVerLen & 0xf) * 4;       //ip头长度，版本号+头长度
		ipLength = ntohs(ih->ipLength);			  //报文总长度
		th = (TCPHeader *)((u_char*)ih + ip_len);
		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);
		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));
		//显示链表元素内容
		printf("seq->seq_num:%x\n", p->seq_num);
		printf("seq->nxt_seq:%x\n", p->nxt_seq);
		printf("seq->data_len:%d\n", p->data_len);

		//  显示数据包的内容
		for (int j = 0; j < lentcp; j++)
		{
			if ((*(datatcp+ j) & 0x000000ff) != 0x07)  // 过滤掉可恶的Beep字符
				printf("%c", *(datatcp + j));
		}
		p = p->prev;
	}
	printf("\n*****************************************************************\n");
}


//客户端链表插入
BOOL ClientTcpInsert(order* ord, struct seq* Client)
{
	printf("客户端链表正式开始插入\n");
	struct seq* p;
	struct seq* q;
	struct seq *TcpInfo = NULL;
	struct seq *Tmp;
	p = q = Client;

	ord->src = (struct seq*)malloc(sizeof(struct seq));
	ord->src->next = NULL;
	ord->src->prev = NULL;
	while (p->prev != NULL)
	{
		q = q->prev;

		TcpInfo = (struct seq*)malloc(sizeof(struct seq));
		memset(TcpInfo, 0, sizeof(TcpInfo));
		TcpInfo->pkt_data = (u_char *)malloc(p->data_len);
		memcpy(TcpInfo->pkt_data, p->pkt_data, p->data_len);

		TcpInfo->seq_num = p->seq_num;
		TcpInfo->data_len = p->data_len;
		TcpInfo->next = NULL;
		TcpInfo->prev = NULL;

			
		//待处理双链表处理
		if (ord->src->next != NULL&&(q->seq_num == p->nxt_seq))
		{
			Tmp = ord->src->next;
			TcpInfo->prev = ord->src;
			TcpInfo->next = Tmp;
			Tmp->prev = TcpInfo;
			ord->src->next = TcpInfo;
			q->sign = 1;
		}
		else
		{
			ord->src->next = TcpInfo;
			TcpInfo->prev = ord->src;
			TcpInfo->next = NULL;
			q->sign = 1;
		}

		p = p->prev;
	}
	printf("客户端链表正式开始插入结束\n");
	return TRUE;
}


//客户端和服务器端链表插入
BOOL ServerTcpInsert(order* ord, struct seq* Client)
{
	printf("服务端链表正式开始插入\n");
	struct seq* p;
	struct seq* q;
	struct seq *TcpInfo = NULL;
	struct seq *Tmp;
	p = q = Client;

	ord->dst = (struct seq*)malloc(sizeof(struct seq));
	ord->dst->next = NULL;
	ord->dst->prev = NULL;
	while (p->prev != NULL)
	{
		q = q->prev;

		TcpInfo = (struct seq*)malloc(sizeof(struct seq));
		memset(TcpInfo, 0, sizeof(TcpInfo));
		TcpInfo->pkt_data = (u_char *)malloc(p->data_len);
		memcpy(TcpInfo->pkt_data, p->pkt_data, p->data_len);

		TcpInfo->seq_num = p->seq_num;
		TcpInfo->data_len = p->data_len;
		TcpInfo->next = NULL;
		TcpInfo->prev = NULL;


		//待处理双链表处理
		if (ord->dst->next != NULL && (q->seq_num == p->nxt_seq))
		{
			Tmp = ord->dst->next;
			TcpInfo->prev = ord->dst;
			TcpInfo->next = Tmp;
			Tmp->prev = TcpInfo;
			ord->dst->next = TcpInfo;
			q->sign = 2;
		}
		else
		{
			ord->dst->next = TcpInfo;
			TcpInfo->prev = ord->dst;
			TcpInfo->next = NULL;
			q->sign = 2;
		}

		p = p->prev;
	}
	printf("服务端链表正式开始插入结束\n");
	return TRUE;
}

//客户端链表建立
void ClientTcpHandle(order* ord, struct seq* Client)
{
	ETHeader *eh;
	IPHeader *ih;
	TCPHeader *th;
	UDPHeader *uh;
	u_int ip_len, ipLength, tcpLength;
	char szSource[16], szDest[16];
	ULONG sequenceNumber;
	ULONG acknowledgeNumber;

	eh = (ETHeader *)Client->pkt_data;
	ih = (IPHeader *)(Client->pkt_data + 14);
	ip_len = (ih->iphVerLen & 0xf) * 4;
	th = (TCPHeader *)((u_char*)ih + ip_len);
	//针对第一个数据包，判断是否有HTTP标志，是否是客户端发起的。
	//客户端链表建立，判断seq->nxt_seq = tcpinfo->seq_num,判断IP是否相同
	while (Client->prev != NULL)
	{
		//判断客户端的数据包是否含有HTTP标志
		if ((Client->sign == 0)&&FilterHttpPacket(Client->pkt_data, Client->data_len))
		{
			printf("\n****************打印SEQ的值*******************\n");
			printf("TCP packet sequenceNumber %ld\n", ntohl(th->sequenceNumber));
			printf("TCP packet acknowledgeNumber %ld\n", ntohl(th->acknowledgeNumber));
			TcpOrderInit(Client->pkt_data, ord);		//初始化ord

			eh = (ETHeader *)Client->pkt_data;
			ih = (IPHeader *)(Client->pkt_data + 14);
			//获取ip地址																				// 得到源IP地址，目标IP地址
			wsprintf(szSource, "%d.%d.%d.%d",
				ih->ipSourceByte.byte1, ih->ipSourceByte.byte2,
				ih->ipSourceByte.byte3, ih->ipSourceByte.byte4);

			wsprintf(szDest, "%d.%d.%d.%d",
				ih->ipDestinationByte.byte1, ih->ipDestinationByte.byte2,
				ih->ipDestinationByte.byte3, ih->ipDestinationByte.byte4);

			printf("判断客户端源IP和目的IP是否相同....\n");
			//判断客户端源IP和目的IP是否相同
			if ((strcmp(szDest, ord->id_d1) == 0)&& (strcmp(szSource, ord->id_s1) == 0))
			{
				printf("客户端链表插入...........\n");
				if (ClientTcpInsert(ord, Client))
				{
					if (Client->next != NULL)
					{
						ClientOrdPrint(ord->src);
					}

					
				}
			}
			printf("判断服务器端源IP和目的IP是否相同\n");
			//判断服务器端源IP和目的IP是否相同
			if ((strcmp(szDest, ord->id_d1) == 0) && (strcmp(szSource, ord->id_s1) == 0))
			{
				//服务端链表插入
				if (ServerTcpInsert(ord, Client))
				{
					ClientOrdPrint(ord->dst);
				}
			}
		}
		Client = Client->prev;
	}
}






//链表待处理数据处理
void TcpHandle(order* ord)
{
	struct seq *Client, *Server;


	//TcpOrdPrint(ord);
	//处理ord->ordTcp链表
	//针对第一个数据包，判断是否有HTTP标志，是否是客户端发起的。
	//客户端链表建立，判断seq->nxt_seq = tcpinfo->seq_num,判断IP是否相同
	//服务器端链表建立，判断seq->nxt_seq = tcpinfo->seq_num,判断IP是否相同。
	Client = ord->ordTcp;
	Server = ord->ordTcp;

	while (Client->next != NULL)
	{
		Client = Client->next;
		Server = Server->next;
	}
	if (Client != NULL)
	{

		ClientTcpHandle(ord, Client);
		//ClientOrdPrint(ord->src);
	}
	if (Server != NULL)
	{
		ClientTcpHandle(ord, Server);
		//ClientOrdPrint(ord->dst);
	}
}

//链表的插入
void TcpOrder(order* ord, const u_char *pkt_data, unsigned int pkt_len)
{

	ETHeader *eh;
	IPHeader *ih;
	TCPHeader *th;
	UDPHeader *uh;
	u_int ip_len, ipLength, tcpLength;
	char* result = NULL;
	ULONG sequenceNumber;
	ULONG acknowledgeNumber;
	char szSource[16], szDest[16];

	eh = (ETHeader *)pkt_data;
	ih = (IPHeader *)(pkt_data + 14);
	
	ip_len = (ih->iphVerLen & 0xf) * 4;       //ip头长度，版本号+头长度
	ipLength = ntohs(ih->ipLength);			  //报文总长度

	th = (TCPHeader *)((u_char*)ih + ip_len);

	struct seq *TcpInfo, *p;
	TcpInfo = (struct seq*)malloc(sizeof(struct seq));
	memset(TcpInfo, 0, sizeof(TcpInfo));

	//获取TCP包的序列号
	sequenceNumber = ntohl(th->sequenceNumber);
	acknowledgeNumber = ntohl(th->acknowledgeNumber);

	tcpLength = ((th->dataoffset * 0xf) >> 4) * 4;	//tcp首部长度
	unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
		+ sizeof(struct _TCPHeader);
	int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));	//真实数据长度

	TcpInfo->pkt_data = (u_char *)malloc(pkt_len);
	memcpy(TcpInfo->pkt_data, pkt_data, pkt_len);
	TcpInfo->seq_num = sequenceNumber;
	TcpInfo->ack = acknowledgeNumber;
	TcpInfo->nxt_seq = sequenceNumber + lentcp;
	TcpInfo->count += lentcp;
	TcpInfo->next = NULL;
	TcpInfo->prev = NULL;
	TcpInfo->data_len = pkt_len;

	ord->num += 1;									//每插入一个元素，待处理包加一



	
	//获取ip地址																				// 得到源IP地址，目标IP地址
	wsprintf(szSource, "%d.%d.%d.%d",
		ih->ipSourceByte.byte1, ih->ipSourceByte.byte2,
		ih->ipSourceByte.byte3, ih->ipSourceByte.byte4);

	wsprintf(szDest, "%d.%d.%d.%d",
		ih->ipDestinationByte.byte1, ih->ipDestinationByte.byte2,
		ih->ipDestinationByte.byte3, ih->ipDestinationByte.byte4);

	//待处理双链表处理
	if (ord->ordTcp->next != NULL)
	{
		p = ord->ordTcp->next;
		TcpInfo->prev = ord->ordTcp;
		TcpInfo->next = p;
		p->prev = TcpInfo;
		ord->ordTcp->next = TcpInfo;
		

	}
	else
	{
		ord->ordTcp->next = TcpInfo;
		TcpInfo->prev = ord->ordTcp;
		TcpInfo->next = NULL;
	}

	//如果待处理链表中有15条，就进行处理
	if (ord->num == 20)
	{
		printf("开始处理待处理链表\n");
		//TcpOrdPrint(ord);
		//system("pause");
		TcpHandle(ord);
	}


}



//链表元素输出
void TcpOrdPrint(order* ord)
{
	ETHeader *eh;
	IPHeader *ih;
	TCPHeader *th;
	UDPHeader *uh;
	u_int ip_len, ipLength, tcpLength;
	char szSource[16], szDest[16];
	u_short sport, dport;
	char* result = NULL;
	ULONG sequenceNumber;
	ULONG acknowledgeNumber;



	struct  seq *p;
	printf("\n**********************链表数据部分开始*******************************\n");

	printf("ord->ip:%s\n", ord->ip);
	printf("ord->id_d1:%s\n", ord->id_d1);
	printf("ord->id_s1:%s\n", ord->id_s1);
	printf("ord->num:%d\n", ord->num);


	p = ord->ordTcp;
	while (p->next != NULL)
	{
		p = p->next;
	}
	while ((p != NULL)&&(p->prev !=NULL))
	{
		eh = (ETHeader *)p->pkt_data;
		ih = (IPHeader *)(p->pkt_data + 14);
		ip_len = (ih->iphVerLen & 0xf) * 4;       //ip头长度，版本号+头长度
		ipLength = ntohs(ih->ipLength);			  //报文总长度
		th = (TCPHeader *)((u_char*)ih + ip_len);
		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);
		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));

		printf("\n************************打印数据部分开始***********************\n");
		//显示链表元素内容
		printf("seq->seq_num:%x\n", p->seq_num);
		printf("seq->nxt_seq:%x\n", p->nxt_seq);
		printf("seq->count:%d\n", p->count);

		//  显示数据包的内容
		for (int j = 0; j < lentcp; j++)
		{
			if ((*(datatcp + j) & 0x000000ff) != 0x07)  // 过滤掉可恶的Beep字符
				printf("%c", *(datatcp + j));
		}
		printf("\n*************************打印数据部分结束************************\n");
		p = p->prev;
	}
	printf("\n***************************链表部分结束******************************\n");
}



//组合HTTP包

BOOL ReformHttpPacket(const u_char *pkt_data, unsigned int pkt_len)
{
	ETHeader *eh;
	IPHeader *ih;
	TCPHeader *th;
	UDPHeader *uh;
	u_int ip_len, ipLength, tcpLength;
	char szSource[16], szDest[16];
	u_short sport, dport;
	char* result = NULL;
	ULONG sequenceNumber;
	ULONG acknowledgeNumber;

	eh = (ETHeader *)pkt_data;
	ih = (IPHeader *)(pkt_data + 14);
	ip_len = (ih->iphVerLen & 0xf) * 4;       //ip头长度，版本号+头长度
	ipLength = ntohs(ih->ipLength);			  //报文总长度



	if (ih->ipProtocol == PROTO_TCP)
	{
		th = (TCPHeader *)((u_char*)ih + ip_len);
		sport = ntohs(th->sourcePort);
		dport = ntohs(th->destinationPort);
		//获取TCP包的序列号
		sequenceNumber = th->sequenceNumber;
		acknowledgeNumber = th->acknowledgeNumber;


		tcpLength = ((th->dataoffset * 0xf) >> 4) * 4;	//tcp首部长度
		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);
		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));	//真实数据长度


		//获取ip地址																				// 得到源IP地址，目标IP地址
		wsprintf(szSource, "%d.%d.%d.%d",
			ih->ipSourceByte.byte1, ih->ipSourceByte.byte2,
			ih->ipSourceByte.byte3, ih->ipSourceByte.byte4);

		wsprintf(szDest, "%d.%d.%d.%d",
			ih->ipDestinationByte.byte1, ih->ipDestinationByte.byte2,
			ih->ipDestinationByte.byte3, ih->ipDestinationByte.byte4);

		ord = (order*)malloc(sizeof(order));		//初始化链表
		memset(ord, 0, sizeof(ord));
		ord->ordTcp = (struct seq*)malloc(sizeof(struct seq));
		memset(ord->ordTcp, 0, sizeof(ord->ordTcp));
		ord->ordTcp->next = NULL;
		ord->ordTcp->prev = NULL;
		TcpOrderInit(pkt_data,ord);
		TcpOrder(ord, pkt_data, pkt_len);
	}
	return TRUE;
}