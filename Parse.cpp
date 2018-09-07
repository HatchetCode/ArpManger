#include "Parse.h"


BOOL ParseHttpPacket(const u_char *pkt_data, unsigned int pkt_len)
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
		printf("TCP packet sequenceNumber %ld\n", sequenceNumber);
		printf("TCP packet acknowledgeNumber %ld\n", acknowledgeNumber);



		tcpLength = ((th->dataoffset * 0xf) >> 4) * 4;	//tcp�ײ�����


		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);

		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));	//��ʵ���ݳ���


		//��ʵ���ݽ����и�
		//result = strtok(datatcp, delime);

		/*
		//  ��ʾ���ݰ�������
		for (int j = 0; j < lentcp; j++)
		{
		if ((*(datatcp + j) & 0x000000ff) != 0x07)  // ���˵��ɶ��Beep�ַ�
		printf("%c", *(datatcp + j));
		}
		*/
	}
	return FALSE;
}