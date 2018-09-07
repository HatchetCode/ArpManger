#include "transfer.h"
#include <string.h>

#define POSTURL    "http://192.168.20.168/data"
#define POSTFIELDS "email=myemail@163.com&password=mypassword&autologin=1&submit=登 录&type="

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

	curl_easy_setopt(curl, CURLOPT_URL, g_node_url); //url地址
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, szSource); //post参数
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, pkt_data); //对返回的数据进行操作的函数地址
	curl_easy_setopt(curl, CURLOPT_POST, 1); //设置问非0表示本次操作为post
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1); //打印调试信息
	curl_easy_setopt(curl, CURLOPT_HEADER, 1); //将响应头信息和相应体一起传给write_data
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //设置为非0,响应头信息location
	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
	{
		switch (res)
		{
		case CURLE_UNSUPPORTED_PROTOCOL:
			printf("不支持的协议，由URL的头部指定\n");
		case CURLE_COULDNT_CONNECT:
			printf("不能连接远程的主机或者代理\n");
		case CURLE_HTTP_RETURNED_ERROR:
			printf("http返回错误\n");
		case CURLE_READ_ERROR:
			printf("读取本地文件失败\n");
		default:
			printf("返回值:%d\n",res);
		}
		return FALSE;
	}
	return TRUE;
}