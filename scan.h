#pragma once
#include <Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

// ������������Ϣ˫������
typedef struct _LAN_HOST_INFO
{
	char IpAddr[4 * 4]; /* ����IP��ַ */
	char HostName[25]; /* ������ */
	unsigned char ucMacAddr[4]; /* ����������ַ */
	BOOL bIsOnline; /* �Ƿ����� */
	struct _LAN_HOST_INFO *prev; /* ��һ��������ָ�� */
	struct _LAN_HOST_INFO *next; /* ��һ��������ָ�� */
}LAN_HOST_INFO, *PLAN_HOST_INFO;

PLAN_HOST_INFO EnumLanHost(char IpAddr[], char SubMask[]);//ö�پ������ڵ�����

void scan_lan(LPVOID lparam)//ɨ��
{  	//WSADATA wsaData;
	//	WSAStartup(MAKEWORD(2,1), &wsaData);
	u_char ucMacAddr[6];
	char IpAddr[16];
	PLAN_HOST_INFO pInfo;
	memset(ucMacAddr, 0xff, sizeof(ucMacAddr));

	// ��������
	unsigned long nRemoteAddr;
	struct hostent *pHostent;
	memset(IpAddr, 0, sizeof(IpAddr));
	pInfo = (PLAN_HOST_INFO)lparam;
	memcpy(IpAddr, pInfo->IpAddr, sizeof(IpAddr));

	if (GetMac(IpAddr, ucMacAddr))
	{
		// ���������Ϣ����������
		pInfo->bIsOnline = TRUE;
		memcpy(pInfo->ucMacAddr, ucMacAddr, sizeof(ucMacAddr));

		// �õ�������
		nRemoteAddr = inet_addr(IpAddr);
		pHostent = (struct hostent*) malloc(sizeof(struct hostent));
		memset(pHostent, 0, sizeof(struct hostent));
		pHostent = gethostbyaddr((char*)&nRemoteAddr, 4, AF_INET);
		if (pHostent)
		{
			printf("HostName��:   %s\n", pHostent->h_name);
			memcpy(pInfo->HostName, pHostent->h_name, sizeof(pInfo->HostName));
		}
		else
			printf("gethostbyaddr   Error:%d\n", GetLastError());

	}


	else
		pInfo->bIsOnline = FALSE;
}

//
// �õ�������������������Ϣ
// ��ڲ���: ����IP��ַ����������
//

PLAN_HOST_INFO EnumLanHost(char IpAddr[], char SubMask[])
{

	unsigned int uHostByte; // ����λ
	int i, uHostNum;

	ULONG uMacLength = 6;

	// ������ʱ����
	char TempIpAddr[4 * 4] = {0};

	PLAN_HOST_INFO pLanHostInfo, pNextHostInfo; // ��Զָ�������β��

	HANDLE *hThread; // �߳�����ָ��
	DWORD dwThreadID; // �߳�ID
	WSADATA wsaData;

	WSAStartup(MAKEWORD(2, 1), &wsaData);
	printf("[+] Start scan lan ......\n");

	// ��IP��ַ�õ�����λ
	uHostByte = htonl(inet_addr(IpAddr)) & 0xffffff00;

	// ����������õ������ڵ���������
	// ������������ = ~ MASK - 1
	uHostNum = ~htonl(inet_addr(SubMask)) - 1;//~ȡ��

											  // ��ʼ���߳̾������
	hThread = (HANDLE *)malloc(sizeof(HANDLE) * uHostNum);

	// ��ʼ������
	pLanHostInfo = (PLAN_HOST_INFO)malloc(sizeof(LAN_HOST_INFO));
	memset(pLanHostInfo, 0, sizeof(LAN_HOST_INFO));
	pLanHostInfo->prev = NULL;


	printf("[+] Scan for addresses from %d.%d.%d.1-%d\n\n",
		(uHostByte & 0xff000000) >> 0x18,//�߼�����
		(uHostByte & 0x00ff0000) >> 0x10,
		(uHostByte & 0x0000ff00) >> 0x08, uHostNum);

	// ��ʼ���ж��߳�ARPɨ��,����uHostNum���߳�ɨ��
	// Scan Range: 1 ~ uHostNum
	for (i = 0, uHostByte++; i < uHostNum; i++, uHostByte++)
	{
		// ����IP��ַ
		memset(TempIpAddr, 0, sizeof(TempIpAddr));
		sprintf(TempIpAddr, "%d.%d.%d.%d",
			(uHostByte & 0xff000000) >> 0x18,
			(uHostByte & 0x00ff0000) >> 0x10,
			(uHostByte & 0x0000ff00) >> 0x08,
			(uHostByte & 0x000000ff));



		// ��������
		pNextHostInfo = (PLAN_HOST_INFO)malloc(sizeof(LAN_HOST_INFO));
		memset(pNextHostInfo, 0, sizeof(LAN_HOST_INFO));
		memcpy(pLanHostInfo->IpAddr, TempIpAddr, sizeof(TempIpAddr));
		pLanHostInfo->next = pNextHostInfo;
		pNextHostInfo->prev = pLanHostInfo;
		pNextHostInfo->next = NULL;
		//	scan_lan(pLanHostInfo);
		//scan_lanɨ��
		if ((hThread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)scan_lan,
			pLanHostInfo, 0, &dwThreadID)) == NULL)
		{
			printf("[!] Create thread error! IP is %s\n", TempIpAddr);
		}
		pLanHostInfo = pLanHostInfo->next;

		Sleep(500); // �ȴ�����������ϣ������¸�ֵ
	}

	// �ȴ��̷߳���,�˳�����
	WaitForMultipleObjects(uHostNum, hThread, TRUE, -1);

	// ��ʾ���������Ϣ
	printf("IP address        MAC address        NetBIOS Name\n");
	printf("------------------------------------------------------------------------------\n");

	for (i = 0; pLanHostInfo->prev != NULL; )
	{
		pLanHostInfo = pLanHostInfo->prev;
		if (pLanHostInfo->bIsOnline)
		{
			printf("%-16s  %.2X-%.2X-%.2X-%.2X-%.2X-%.2X %s  %s\n",
				pLanHostInfo->IpAddr,
				pLanHostInfo->ucMacAddr[0], pLanHostInfo->ucMacAddr[1],
				pLanHostInfo->ucMacAddr[2], pLanHostInfo->ucMacAddr[3],
				pLanHostInfo->ucMacAddr[4], pLanHostInfo->ucMacAddr[5],
				pLanHostInfo->HostName,
				strlen(pLanHostInfo->HostName) > 0 ? pLanHostInfo->HostName : "N/A");
			i++;
		}
	}
	printf("------------------------------------------------------------------------------\n");
	printf("\n[*] Tatol %d host alive, scan finished\n", i);
	WSACleanup();

	return pLanHostInfo;
}
