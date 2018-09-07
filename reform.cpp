#include "reform.h"
#include "Filter.h"

order *ord;
BOOL g_direct = FALSE;	//FLASE�����ǿͻ���->�������ˣ�TRUE�����Ƿ�������->�ͻ���


//����ĳ�ʼ��
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
	ip_len = (ih->iphVerLen & 0xf) * 4;       //ipͷ���ȣ��汾��+ͷ����
	ipLength = ntohs(ih->ipLength);			  //�����ܳ���

	//��ȡip��ַ
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


//�ͻ��˺ͷ��������������ʹ��
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
	printf("\n**********************�������ݲ���*******************************\n");

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
		ip_len = (ih->iphVerLen & 0xf) * 4;       //ipͷ���ȣ��汾��+ͷ����
		ipLength = ntohs(ih->ipLength);			  //�����ܳ���
		th = (TCPHeader *)((u_char*)ih + ip_len);
		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);
		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));
		//��ʾ����Ԫ������
		printf("seq->seq_num:%x\n", p->seq_num);
		printf("seq->nxt_seq:%x\n", p->nxt_seq);
		printf("seq->data_len:%d\n", p->data_len);

		//  ��ʾ���ݰ�������
		for (int j = 0; j < lentcp; j++)
		{
			if ((*(datatcp+ j) & 0x000000ff) != 0x07)  // ���˵��ɶ��Beep�ַ�
				printf("%c", *(datatcp + j));
		}
		p = p->prev;
	}
	printf("\n*****************************************************************\n");
}


//�ͻ����������
BOOL ClientTcpInsert(order* ord, struct seq* Client)
{
	printf("�ͻ���������ʽ��ʼ����\n");
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

			
		//������˫������
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
	printf("�ͻ���������ʽ��ʼ�������\n");
	return TRUE;
}


//�ͻ��˺ͷ��������������
BOOL ServerTcpInsert(order* ord, struct seq* Client)
{
	printf("�����������ʽ��ʼ����\n");
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


		//������˫������
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
	printf("�����������ʽ��ʼ�������\n");
	return TRUE;
}

//�ͻ���������
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
	//��Ե�һ�����ݰ����ж��Ƿ���HTTP��־���Ƿ��ǿͻ��˷���ġ�
	//�ͻ������������ж�seq->nxt_seq = tcpinfo->seq_num,�ж�IP�Ƿ���ͬ
	while (Client->prev != NULL)
	{
		//�жϿͻ��˵����ݰ��Ƿ���HTTP��־
		if ((Client->sign == 0)&&FilterHttpPacket(Client->pkt_data, Client->data_len))
		{
			printf("\n****************��ӡSEQ��ֵ*******************\n");
			printf("TCP packet sequenceNumber %ld\n", ntohl(th->sequenceNumber));
			printf("TCP packet acknowledgeNumber %ld\n", ntohl(th->acknowledgeNumber));
			TcpOrderInit(Client->pkt_data, ord);		//��ʼ��ord

			eh = (ETHeader *)Client->pkt_data;
			ih = (IPHeader *)(Client->pkt_data + 14);
			//��ȡip��ַ																				// �õ�ԴIP��ַ��Ŀ��IP��ַ
			wsprintf(szSource, "%d.%d.%d.%d",
				ih->ipSourceByte.byte1, ih->ipSourceByte.byte2,
				ih->ipSourceByte.byte3, ih->ipSourceByte.byte4);

			wsprintf(szDest, "%d.%d.%d.%d",
				ih->ipDestinationByte.byte1, ih->ipDestinationByte.byte2,
				ih->ipDestinationByte.byte3, ih->ipDestinationByte.byte4);

			printf("�жϿͻ���ԴIP��Ŀ��IP�Ƿ���ͬ....\n");
			//�жϿͻ���ԴIP��Ŀ��IP�Ƿ���ͬ
			if ((strcmp(szDest, ord->id_d1) == 0)&& (strcmp(szSource, ord->id_s1) == 0))
			{
				printf("�ͻ����������...........\n");
				if (ClientTcpInsert(ord, Client))
				{
					if (Client->next != NULL)
					{
						ClientOrdPrint(ord->src);
					}

					
				}
			}
			printf("�жϷ�������ԴIP��Ŀ��IP�Ƿ���ͬ\n");
			//�жϷ�������ԴIP��Ŀ��IP�Ƿ���ͬ
			if ((strcmp(szDest, ord->id_d1) == 0) && (strcmp(szSource, ord->id_s1) == 0))
			{
				//������������
				if (ServerTcpInsert(ord, Client))
				{
					ClientOrdPrint(ord->dst);
				}
			}
		}
		Client = Client->prev;
	}
}






//������������ݴ���
void TcpHandle(order* ord)
{
	struct seq *Client, *Server;


	//TcpOrdPrint(ord);
	//����ord->ordTcp����
	//��Ե�һ�����ݰ����ж��Ƿ���HTTP��־���Ƿ��ǿͻ��˷���ġ�
	//�ͻ������������ж�seq->nxt_seq = tcpinfo->seq_num,�ж�IP�Ƿ���ͬ
	//�����������������ж�seq->nxt_seq = tcpinfo->seq_num,�ж�IP�Ƿ���ͬ��
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

//����Ĳ���
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
	
	ip_len = (ih->iphVerLen & 0xf) * 4;       //ipͷ���ȣ��汾��+ͷ����
	ipLength = ntohs(ih->ipLength);			  //�����ܳ���

	th = (TCPHeader *)((u_char*)ih + ip_len);

	struct seq *TcpInfo, *p;
	TcpInfo = (struct seq*)malloc(sizeof(struct seq));
	memset(TcpInfo, 0, sizeof(TcpInfo));

	//��ȡTCP�������к�
	sequenceNumber = ntohl(th->sequenceNumber);
	acknowledgeNumber = ntohl(th->acknowledgeNumber);

	tcpLength = ((th->dataoffset * 0xf) >> 4) * 4;	//tcp�ײ�����
	unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
		+ sizeof(struct _TCPHeader);
	int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));	//��ʵ���ݳ���

	TcpInfo->pkt_data = (u_char *)malloc(pkt_len);
	memcpy(TcpInfo->pkt_data, pkt_data, pkt_len);
	TcpInfo->seq_num = sequenceNumber;
	TcpInfo->ack = acknowledgeNumber;
	TcpInfo->nxt_seq = sequenceNumber + lentcp;
	TcpInfo->count += lentcp;
	TcpInfo->next = NULL;
	TcpInfo->prev = NULL;
	TcpInfo->data_len = pkt_len;

	ord->num += 1;									//ÿ����һ��Ԫ�أ����������һ



	
	//��ȡip��ַ																				// �õ�ԴIP��ַ��Ŀ��IP��ַ
	wsprintf(szSource, "%d.%d.%d.%d",
		ih->ipSourceByte.byte1, ih->ipSourceByte.byte2,
		ih->ipSourceByte.byte3, ih->ipSourceByte.byte4);

	wsprintf(szDest, "%d.%d.%d.%d",
		ih->ipDestinationByte.byte1, ih->ipDestinationByte.byte2,
		ih->ipDestinationByte.byte3, ih->ipDestinationByte.byte4);

	//������˫������
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

	//�����������������15�����ͽ��д���
	if (ord->num == 20)
	{
		printf("��ʼ�������������\n");
		//TcpOrdPrint(ord);
		//system("pause");
		TcpHandle(ord);
	}


}



//����Ԫ�����
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
	printf("\n**********************�������ݲ��ֿ�ʼ*******************************\n");

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
		ip_len = (ih->iphVerLen & 0xf) * 4;       //ipͷ���ȣ��汾��+ͷ����
		ipLength = ntohs(ih->ipLength);			  //�����ܳ���
		th = (TCPHeader *)((u_char*)ih + ip_len);
		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);
		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));

		printf("\n************************��ӡ���ݲ��ֿ�ʼ***********************\n");
		//��ʾ����Ԫ������
		printf("seq->seq_num:%x\n", p->seq_num);
		printf("seq->nxt_seq:%x\n", p->nxt_seq);
		printf("seq->count:%d\n", p->count);

		//  ��ʾ���ݰ�������
		for (int j = 0; j < lentcp; j++)
		{
			if ((*(datatcp + j) & 0x000000ff) != 0x07)  // ���˵��ɶ��Beep�ַ�
				printf("%c", *(datatcp + j));
		}
		printf("\n*************************��ӡ���ݲ��ֽ���************************\n");
		p = p->prev;
	}
	printf("\n***************************�����ֽ���******************************\n");
}



//���HTTP��

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
	ip_len = (ih->iphVerLen & 0xf) * 4;       //ipͷ���ȣ��汾��+ͷ����
	ipLength = ntohs(ih->ipLength);			  //�����ܳ���



	if (ih->ipProtocol == PROTO_TCP)
	{
		th = (TCPHeader *)((u_char*)ih + ip_len);
		sport = ntohs(th->sourcePort);
		dport = ntohs(th->destinationPort);
		//��ȡTCP�������к�
		sequenceNumber = th->sequenceNumber;
		acknowledgeNumber = th->acknowledgeNumber;


		tcpLength = ((th->dataoffset * 0xf) >> 4) * 4;	//tcp�ײ�����
		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);
		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));	//��ʵ���ݳ���


		//��ȡip��ַ																				// �õ�ԴIP��ַ��Ŀ��IP��ַ
		wsprintf(szSource, "%d.%d.%d.%d",
			ih->ipSourceByte.byte1, ih->ipSourceByte.byte2,
			ih->ipSourceByte.byte3, ih->ipSourceByte.byte4);

		wsprintf(szDest, "%d.%d.%d.%d",
			ih->ipDestinationByte.byte1, ih->ipDestinationByte.byte2,
			ih->ipDestinationByte.byte3, ih->ipDestinationByte.byte4);

		ord = (order*)malloc(sizeof(order));		//��ʼ������
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