#include "Filter.h"

void *memfind(const void *in_block,   /* 数据块 */
	const size_t block_size, /* 数据块长度 */
	const void *in_pattern,   /* 需要查找的数据 */
	const size_t pattern_size,     /* 查找数据的长度 */
	size_t * shift,   /* 移位表，应该是256*size_t的数组 */
	bool * init);

void *txtfind(const void *in_block,   /* 数据块 */
	const size_t block_size, /* 数据块长度 */
	const void *in_pattern,   /* 需要查找的数据 */
	const size_t pattern_size,     /* 查找数据的长度 */
	size_t * shift,   /* 移位表，应该是256*size_t的数组 */
	bool * init);

/* 字符串查找函数*/
char *bm_strstr(const char *string, const char *pattern)
{
size_t shift[256];
bool init = false;

return (char * ) memfind(string, strlen(string), pattern, strlen(pattern), shift, &init);
}

/* 字符串多次匹配函数*/
char *bm_strstr_rp(const char *string, const char *pattern, size_t * shift, bool * init)
{
return (char * ) memfind(string, strlen(string), pattern, strlen(pattern), shift, init);
}

/* 字符串大小写不敏感的匹配函数*/
char *bm_strcasestr(const char *string, const char *pattern)
{
size_t shift[256];
bool init = false;

return (char * ) txtfind(string, strlen(string), pattern, strlen(pattern), shift, &init);
}

/* 字符串多次大小写不敏感匹配函数*/
char *bm_strcasestr_rp(const char *string, const char *pattern, size_t * shift, bool * init)
{
return (char * ) txtfind(string, strlen(string), pattern, strlen(pattern), shift, init);
}

/* 内存匹配函数memfind
*/
void *memfind(const void *in_block,   /* 数据块 */
const size_t block_size, /* 数据块长度 */
const void *in_pattern,   /* 需要查找的数据 */
const size_t pattern_size,     /* 查找数据的长度 */
size_t * shift,   /* 移位表，应该是256*size_t的数组 */
bool * init)
{                     /* 是否需要初始化移位表 */
	size_t byte_nbr,         /* Distance through block */
	match_size,           /* Size of matched part */
	limit;
	const unsigned char *match_ptr = NULL;
	const unsigned char *block = (unsigned char *) in_block,   /* Concrete pointer to block data */
	*pattern = (unsigned char *) in_pattern;     /* Concrete pointer to search value */

	if (block == NULL || pattern == NULL || shift == NULL)
	{
		return (NULL);
	}

	/* 查找的串长应该小于 数据长度*/
	if (block_size < pattern_size)
		return (NULL);

	if (pattern_size == 0)     /* 空串匹配第一个 */
		return ((void * ) block);

		/* 如果没有初始化，构造移位表*/
	if (!init || !*init) 
	{
		for (byte_nbr = 0; byte_nbr < 256; byte_nbr++)
			shift[byte_nbr] = pattern_size + 1;
		for (byte_nbr = 0; byte_nbr < pattern_size; byte_nbr++)
			shift[(unsigned char)pattern[byte_nbr]] = pattern_size - byte_nbr;

		if (init)
			*init = true;
	}

	/*开始搜索数据块，每次前进移位表中的数量*/
	limit = block_size - pattern_size + 1;
	for (byte_nbr = 0; byte_nbr < limit; byte_nbr += shift[block[byte_nbr + pattern_size]])
	{
		if (block[byte_nbr] == *pattern)
		{
			/*
			* 如果第一个字节匹配，那么继续匹配剩下的
			*/
			match_ptr = block + byte_nbr + 1;
			match_size = 1;

			do
			{
				if (match_size == pattern_size)
					return (void *)(block + byte_nbr);
			} while (*match_ptr++ == pattern[match_size++]);
		}
	}		
	return NULL;
}


/* 大小写不敏感的匹配函数txtfind
*/
void *txtfind(const void *in_block,   /* 数据块 */
	const size_t block_size, /* 数据块长度 */
	const void *in_pattern,   /* 需要查找的数据 */
	const size_t pattern_size,     /* 查找数据的长度 */
	size_t * shift,   /* 移位表，应该是256*size_t的数组 */
	bool * init)
{                     /* 是否需要初始化移位表 */
	size_t byte_nbr,         /* Distance through block */
		match_size,           /* Size of matched part */
		limit;
	const unsigned char *match_ptr = NULL;
	const unsigned char *block = (unsigned char *)in_block,   /* Concrete pointer to block data */
		*pattern = (unsigned char *)in_pattern;     /* Concrete pointer to search value */

	if (block == NULL || pattern == NULL || shift == NULL)
		return (NULL);

	/* 查找的串长应该小于 数据长度*/
	if (block_size < pattern_size)
		return (NULL);

	if (pattern_size == 0)     /* 空串匹配第一个 */
		return ((void *)block);

	/* 如果没有初始化，构造移位表*/
	if (!init || !*init) {
		for (byte_nbr = 0; byte_nbr < 256; byte_nbr++)
			shift[byte_nbr] = pattern_size + 1;
		for (byte_nbr = 0; byte_nbr < pattern_size; byte_nbr++)
			shift[(unsigned char)tolower(pattern[byte_nbr])] = pattern_size - byte_nbr;

		if (init)
			*init = true;
	}

	/*开始搜索数据块，每次前进移位表中的数量*/
	limit = block_size - pattern_size + 1;
	for (byte_nbr = 0; byte_nbr < limit; byte_nbr += shift[tolower(block[byte_nbr + pattern_size])]) {
		if (tolower(block[byte_nbr]) == tolower(*pattern)) {
			/*
			* 如果第一个字节匹配，那么继续匹配剩下的
			*/
			match_ptr = block + byte_nbr + 1;
			match_size = 1;

			do {
				if (match_size == pattern_size)
					return (void *)(block + byte_nbr);
			} while (tolower(*match_ptr++) == tolower(pattern[match_size++]));
		}
	}
	return NULL;
}

//数据包过滤
BOOL FilterHttpPacket(const u_char *pkt_data, unsigned int pkt_len)
{
	ETHeader *eh;
	IPHeader *ih;
	TCPHeader *th;
	UDPHeader *uh;
	u_int ip_len, ipLength, tcpLength;
	char szSource[16], szDest[16];
	u_short sport, dport;
	char delime[] = "\r\n\r\n";
	char delime1[] = "HTTP";
	char delime2[] = "GET";
	char delime3[] = "POST";
	char* result = NULL;

	eh = (ETHeader *)pkt_data;
	ih = (IPHeader *)(pkt_data + 14);
	ip_len = (ih->iphVerLen & 0xf) * 4;       //ip头长度，版本号+头长度
	ipLength = ntohs(ih->ipLength);			  //报文总长度

	if (ih->ipProtocol == PROTO_TCP)
	{
		th = (TCPHeader *)((u_char*)ih + ip_len);

		sport = ntohs(th->sourcePort);
		dport = ntohs(th->destinationPort);

		tcpLength = ((th->dataoffset * 0xf) >> 4) * 4;	//tcp首部长度
														

		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);

		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));	//真实数据长度


		
		//system("pause");
		//在数据包中搜索"\r\n\r\n" 和 "HTTP"，"GET"，"POST"标记
		if ((bm_strstr((char*)datatcp, delime1)))
		{
			if ((bm_strstr((char*)datatcp, delime2)) || (bm_strstr((char*)datatcp, delime3)))
			{
				for (int j = 0; j < lentcp; j++)
				{
					if ((*(datatcp + j) & 0x000000ff) != 0x07)  // 过滤掉可恶的Beep字符
						printf("%c", *(datatcp + j));
				}
				return TRUE;
			}
			//{
			//system("pause");
		}
	}
	return FALSE;
}



