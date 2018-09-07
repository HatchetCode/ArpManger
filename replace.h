#pragma once


//
// �ڴ��滻�������޸���memfind����
// �ɹ��滻����1,��������0
//
BOOL memreplace(void *in_block,     /* ���ݿ� */
	size_t block_size,  /* ���ݿ鳤�� */
	void *in_pattern,   /* ��Ҫ���ҵ����� */
	size_t pattern_size,        /* �������ݵĳ��� */
	void *in_newpattern, /* Ҫ�滻�������� */
	size_t newpattern_size, /* Ҫ�滻�������ݵĳ��� */
	size_t * shift,   /* ��λ��Ӧ����256*size_t������ */
	bool * init)
{                               /* �Ƿ���Ҫ��ʼ����λ�� */
	size_t byte_nbr,            /* Distance through block */
		match_size,                /* Size of matched part */
		limit;
	char *match_ptr = NULL;
	char *block = (char *)in_block,    /* Concrete pointer to block data */
		*pattern = (char *)in_pattern;        /* Concrete pointer to search value */

	char* firstplace = NULL;
	char* secondplace = NULL;
	if (block == NULL || pattern == NULL || shift == NULL)
		return (NULL);

	/* ���ҵĴ���Ӧ��С�� ���ݳ���*/
	if (block_size < pattern_size)
	{
		return 0;
	}

	if (pattern_size == 0)      /* �մ�ƥ���һ�� */
	{
		memcpy((void *)(block), in_newpattern, newpattern_size); // �ҵ�����ʼ�滻
		return NULL;
	}

	/* ���û�г�ʼ����������λ��*/
	if (!init || !*init) {
		for (byte_nbr = 0; byte_nbr < 256; byte_nbr++)
			shift[byte_nbr] = pattern_size + 1;
		for (byte_nbr = 0; byte_nbr < pattern_size; byte_nbr++)
			shift[(unsigned char)pattern[byte_nbr]] = pattern_size - byte_nbr;

		if (init)
			*init = true;
	}

	/* ��ʼ�������ݿ飬ÿ��ǰ����λ���е����� */
	limit = block_size - pattern_size + 1;

	for (byte_nbr = 0; byte_nbr < limit; byte_nbr += shift[block[byte_nbr + pattern_size]]) {

		if (block[byte_nbr] == *pattern) {
			/*
			* �����һ���ֽ�ƥ�䣬��ô����ƥ��ʣ�µ�
			*/
			match_ptr = block + byte_nbr + 1;
			match_size = 1;
			do {
				if (match_size == pattern_size)
				{
					firstplace = block + byte_nbr;

					/* �ҵ�����ʼ�滻 */
					memcpy((void *)(block + byte_nbr + 7), in_newpattern, newpattern_size);
					return TRUE;
				}
			} while (*match_ptr++ == pattern[match_size++]);
		}
	}
	return 0;
}

//
//  �ڴ��滻��������ҪΪ�˲���memreplace����������
//
BOOL Replace(void *in_block, size_t block_size,
	char *szOld, char *szNew)
{
	size_t shift[256];
	bool init = false;
	//printf("%s\n", szOld);
	//printf("%s\n", szNew);
	return memreplace(in_block, block_size, szOld, strlen(szOld), szNew,
		strlen(szNew), shift, &init);
}