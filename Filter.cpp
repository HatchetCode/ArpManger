#include "Filter.h"

void *memfind(const void *in_block,   /* ���ݿ� */
	const size_t block_size, /* ���ݿ鳤�� */
	const void *in_pattern,   /* ��Ҫ���ҵ����� */
	const size_t pattern_size,     /* �������ݵĳ��� */
	size_t * shift,   /* ��λ��Ӧ����256*size_t������ */
	bool * init);

void *txtfind(const void *in_block,   /* ���ݿ� */
	const size_t block_size, /* ���ݿ鳤�� */
	const void *in_pattern,   /* ��Ҫ���ҵ����� */
	const size_t pattern_size,     /* �������ݵĳ��� */
	size_t * shift,   /* ��λ��Ӧ����256*size_t������ */
	bool * init);

/* �ַ������Һ���*/
char *bm_strstr(const char *string, const char *pattern)
{
size_t shift[256];
bool init = false;

return (char * ) memfind(string, strlen(string), pattern, strlen(pattern), shift, &init);
}

/* �ַ������ƥ�亯��*/
char *bm_strstr_rp(const char *string, const char *pattern, size_t * shift, bool * init)
{
return (char * ) memfind(string, strlen(string), pattern, strlen(pattern), shift, init);
}

/* �ַ�����Сд�����е�ƥ�亯��*/
char *bm_strcasestr(const char *string, const char *pattern)
{
size_t shift[256];
bool init = false;

return (char * ) txtfind(string, strlen(string), pattern, strlen(pattern), shift, &init);
}

/* �ַ�����δ�Сд������ƥ�亯��*/
char *bm_strcasestr_rp(const char *string, const char *pattern, size_t * shift, bool * init)
{
return (char * ) txtfind(string, strlen(string), pattern, strlen(pattern), shift, init);
}

/* �ڴ�ƥ�亯��memfind
*/
void *memfind(const void *in_block,   /* ���ݿ� */
const size_t block_size, /* ���ݿ鳤�� */
const void *in_pattern,   /* ��Ҫ���ҵ����� */
const size_t pattern_size,     /* �������ݵĳ��� */
size_t * shift,   /* ��λ��Ӧ����256*size_t������ */
bool * init)
{                     /* �Ƿ���Ҫ��ʼ����λ�� */
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

	/* ���ҵĴ���Ӧ��С�� ���ݳ���*/
	if (block_size < pattern_size)
		return (NULL);

	if (pattern_size == 0)     /* �մ�ƥ���һ�� */
		return ((void * ) block);

		/* ���û�г�ʼ����������λ��*/
	if (!init || !*init) 
	{
		for (byte_nbr = 0; byte_nbr < 256; byte_nbr++)
			shift[byte_nbr] = pattern_size + 1;
		for (byte_nbr = 0; byte_nbr < pattern_size; byte_nbr++)
			shift[(unsigned char)pattern[byte_nbr]] = pattern_size - byte_nbr;

		if (init)
			*init = true;
	}

	/*��ʼ�������ݿ飬ÿ��ǰ����λ���е�����*/
	limit = block_size - pattern_size + 1;
	for (byte_nbr = 0; byte_nbr < limit; byte_nbr += shift[block[byte_nbr + pattern_size]])
	{
		if (block[byte_nbr] == *pattern)
		{
			/*
			* �����һ���ֽ�ƥ�䣬��ô����ƥ��ʣ�µ�
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


/* ��Сд�����е�ƥ�亯��txtfind
*/
void *txtfind(const void *in_block,   /* ���ݿ� */
	const size_t block_size, /* ���ݿ鳤�� */
	const void *in_pattern,   /* ��Ҫ���ҵ����� */
	const size_t pattern_size,     /* �������ݵĳ��� */
	size_t * shift,   /* ��λ��Ӧ����256*size_t������ */
	bool * init)
{                     /* �Ƿ���Ҫ��ʼ����λ�� */
	size_t byte_nbr,         /* Distance through block */
		match_size,           /* Size of matched part */
		limit;
	const unsigned char *match_ptr = NULL;
	const unsigned char *block = (unsigned char *)in_block,   /* Concrete pointer to block data */
		*pattern = (unsigned char *)in_pattern;     /* Concrete pointer to search value */

	if (block == NULL || pattern == NULL || shift == NULL)
		return (NULL);

	/* ���ҵĴ���Ӧ��С�� ���ݳ���*/
	if (block_size < pattern_size)
		return (NULL);

	if (pattern_size == 0)     /* �մ�ƥ���һ�� */
		return ((void *)block);

	/* ���û�г�ʼ����������λ��*/
	if (!init || !*init) {
		for (byte_nbr = 0; byte_nbr < 256; byte_nbr++)
			shift[byte_nbr] = pattern_size + 1;
		for (byte_nbr = 0; byte_nbr < pattern_size; byte_nbr++)
			shift[(unsigned char)tolower(pattern[byte_nbr])] = pattern_size - byte_nbr;

		if (init)
			*init = true;
	}

	/*��ʼ�������ݿ飬ÿ��ǰ����λ���е�����*/
	limit = block_size - pattern_size + 1;
	for (byte_nbr = 0; byte_nbr < limit; byte_nbr += shift[tolower(block[byte_nbr + pattern_size])]) {
		if (tolower(block[byte_nbr]) == tolower(*pattern)) {
			/*
			* �����һ���ֽ�ƥ�䣬��ô����ƥ��ʣ�µ�
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

//���ݰ�����
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
	ip_len = (ih->iphVerLen & 0xf) * 4;       //ipͷ���ȣ��汾��+ͷ����
	ipLength = ntohs(ih->ipLength);			  //�����ܳ���

	if (ih->ipProtocol == PROTO_TCP)
	{
		th = (TCPHeader *)((u_char*)ih + ip_len);

		sport = ntohs(th->sourcePort);
		dport = ntohs(th->destinationPort);

		tcpLength = ((th->dataoffset * 0xf) >> 4) * 4;	//tcp�ײ�����
														

		unsigned char *datatcp = (unsigned char *)ih + sizeof(_IPHeader)
			+ sizeof(struct _TCPHeader);

		int lentcp = ntohs(ih->ipLength) - (sizeof(_IPHeader) + sizeof(_TCPHeader));	//��ʵ���ݳ���


		
		//system("pause");
		//�����ݰ�������"\r\n\r\n" �� "HTTP"��"GET"��"POST"���
		if ((bm_strstr((char*)datatcp, delime1)))
		{
			if ((bm_strstr((char*)datatcp, delime2)) || (bm_strstr((char*)datatcp, delime3)))
			{
				for (int j = 0; j < lentcp; j++)
				{
					if ((*(datatcp + j) & 0x000000ff) != 0x07)  // ���˵��ɶ��Beep�ַ�
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



