#include "QMemPools.h"

// �Ƿ�У������ȷ��
#define BLOCK_HEAD_TAG 0xCF22
#ifdef  OPEN_BLOCK_HEAD_CHECK
#define BLOCK_FLAG_SIZE  (sizeof(intptr_t)*2 + sizeof(int))
#else
#define BLOCK_FLAG_SIZE  (sizeof(intptr_t)*2)
#endif //  OPEN_BLOCK_HEAD_CHECK

// �׸��ڴ���б�
QMBlockList* pGlobalList = NULL;
// β���ڴ���б�
QMBlockList* pGlobalTailList = NULL;

/*
* �����ڴ��
*/
QMBlock* CreateBlock(size_t size)
{
	QMBlock* block = (QMBlock*)malloc(sizeof(QMBlock));
	if (NULL == block)
	{	// �����ڴ�ʧ��
		return NULL;
	}
	// �����ڴ� - ����Ĵ�С�����洢������Ϣ
	void* pBuff = malloc(size + BLOCK_FLAG_SIZE);
	if (NULL == pBuff)
	{	// �����ڴ�ʧ��
		free(block);
		return NULL;
	}
	// ��������õ��ڴ�
	block->pData = pBuff;
	block->pNext = NULL;
	block->pPrev = NULL;
	return block;
}
/*
* �����ڴ���б�
*/
QMBlockList* CreateBlockList(size_t size)
{
	QMBlockList* block_list = (QMBlockList*)malloc(sizeof(QMBlockList));
	if (NULL == block_list)
	{	//�����ڴ�ʧ��
		return NULL;
	}
	block_list->mSize = size;
	block_list->pFreeBlock = NULL;
	block_list->pLastFreeBlock = NULL;
	block_list->pUsedBlock = NULL;
	block_list->pLastUsedBlock = NULL;
	block_list->pNext = NULL;
	return block_list;
}
/*
* ��ȡ��Ӧsize��С���ڴ���б�����1��ʾ�´����ģ�0��ʾ�ɵģ�-1��ʾʧ�ܣ�
*/
int GetBlockList(size_t size, QMBlockList** out)
{
	if (NULL == pGlobalList)
	{
		QMBlockList* block_list = CreateBlockList(size);
		if (NULL == block_list)
		{	//����ʧ��
			return -1;
		}
		*out = block_list;
		return 1; // �����´�����
	}
	// ������û�ж�Ӧsize��С���ڴ���б��¼
	QMBlockList* p = pGlobalList;
	while (NULL != p) //���������һ������������
	{
		if (p->mSize != size)
		{
			p = p->pNext;
			continue;
		}
		*out = p;
		return 0; // �ҵ�ƥ���С���ڴ���б�����
	}
	// �ߵ�����ͱ�ʾû��ƥ��
	p = CreateBlockList(size);
	if (NULL == p)
	{	//����ʧ��
		return -1;
	}
	*out = p;
	return 1; //�´�����
}
/*
* ��ȡһ������ʹ�õ��ڴ��
*/
QMBlock* GetFreeBlock(QMBlockList* block_list)
{
	if (NULL == block_list->pFreeBlock) // ��ʹ�õ�ͷ���ǿ�
	{
		QMBlock* block = CreateBlock(block_list->mSize);
		if (NULL == block)
		{	// ����ʧ��
			return NULL;
		}
		return block;
	}
	// ˵���п��õ�
	QMBlock* block = block_list->pFreeBlock;
	block_list->pFreeBlock = block->pNext;
	// �����ֻ��һ���ʱ����Ҫ���������õ�ָ��
	if (block_list->pLastFreeBlock == block)
	{
		block_list->pLastFreeBlock = NULL;
	}
	return block;
}
/*
* �����ڴ�鵽�����б�
*/
void SetUsedBlock(QMBlockList* block_list, QMBlock* block)
{
	// ��������
	block->pNext = NULL; 
	block->pPrev = NULL;
	if (NULL == block_list->pUsedBlock) // ��ʹ���б�ͷ�ǿ�
	{
		block_list->pUsedBlock = block;
		block_list->pLastUsedBlock = block;
		return;
	}
	// ˵����ʹ�ô��� -- �͸���һ�±�
	block_list->pLastUsedBlock->pNext = block;
	block->pPrev = block_list->pLastUsedBlock;
	block_list->pLastUsedBlock = block;
}
/*
* �ӿ��õı����Ƴ���ǰblock
*/
void PopUsedBlock(QMBlockList* block_list, QMBlock* block)
{
	QMBlock* prev = block->pPrev;
	QMBlock* next = block->pNext;
	if (NULL != prev)
	{
		prev->pNext = next;
	}
	if (NULL != next)
	{
		next->pPrev = prev;
	}
	// ���ǵ�һ��
	if (block_list->pUsedBlock == block)
	{
		block_list->pUsedBlock = NULL;
		block_list->pLastUsedBlock = NULL;
	}
}
/*
* �����ڴ�鵽�����б�
*/
void SetFreeBlock(QMBlockList* block_list, QMBlock* block)
{
	block->pNext = NULL;
	block->pPrev = NULL;
	if (NULL == block_list->pFreeBlock) // �����б�ͷ�ǿ�
	{
		block_list->pFreeBlock = block;
		block_list->pLastFreeBlock = block;
		return;
	}
	block_list->pLastFreeBlock->pNext = block;
	block->pPrev = block_list->pLastFreeBlock;
	block_list->pLastFreeBlock = block;
}
/*
* �󶨵�ǰ�ڴ���ڴ���ַ���ڴ����ַ
*/
void* BindBlockData(QMBlockList* block_list, QMBlock* block)
{
#ifdef OPEN_BLOCK_HEAD_CHECK
	int* temp_ptr = (int*)block->pData;
	temp_ptr[0] = BLOCK_HEAD_TAG; //ע������ֻ��ռ��4���ֽڣ�����֤
	intptr_t* ptr = (intptr_t*)(((char*)block->pData)+sizeof(int)); //ǿתΪָ�����ͣ���ƽ̨��
	ptr[0] = (intptr_t)block_list;
	ptr[1] = (intptr_t)block; 
	return &ptr[2];
#else
	intptr_t* ptr = (intptr_t*)block->pData; //ǿתΪָ�����ͣ���ƽ̨��
	ptr[0] = (intptr_t)block_list;
	ptr[1] = (intptr_t)block;
	return &ptr[2];
#endif // OPEN_BLOCK_HEAD_CHECK
}
/*
* �������
*/
int UnbindBlockData(void* p, QMBlockList** block_list, QMBlock** block)
{
	intptr_t* ptr = (intptr_t*)p; //ǿת
	*block_list = (QMBlockList*)(*(ptr - 2)); //��ǰ��
	*block = (QMBlock*)(*(ptr - 1));
#ifdef OPEN_BLOCK_HEAD_CHECK
	char* temp_ptr = ((char*)p) - BLOCK_FLAG_SIZE;
	int check_k = *(int*)(temp_ptr);
	if (check_k != BLOCK_HEAD_TAG)
	{	//У��ʧ����
		return -1;
	}
#endif // OPEN_BLOCK_HEAD_CHECK
	return 0;
}

void* QMalloc(size_t size)
{
	QMBlockList* block_list = NULL;
	int res = GetBlockList(size, &block_list);
	if (-1 == res)  
	{	//��ȡ�ڴ���ʧ����
		return NULL;
	}
	else if (1 == res) // �´�����
	{
		if (NULL == pGlobalList) //��һ���ڴ���
		{
			pGlobalList = block_list;
			pGlobalTailList = block_list;
		}
		else
		{
			pGlobalTailList->pNext = block_list;
			pGlobalTailList = pGlobalTailList->pNext; //ʼ��ָ������Ǹ��ڴ���
		}
	}
	QMBlock* block = GetFreeBlock(block_list); // ��ȡ�����ڴ��
	if (NULL == block)
	{	// ��ȡʧ��
		return NULL;
	}
	// ����Ϊ����
	SetUsedBlock(block_list, block);
	// ��������У����Ϣ�Ŀ����ڴ��ַ
	return BindBlockData(block_list, block);
}

void QRelease(void* p)
{
	QMBlockList* block_list = NULL;
	QMBlock* block = NULL;
	int res = UnbindBlockData(p, &block_list, &block);
	if (-1 == res)
	{
		return;
	}
	PopUsedBlock(block_list, block);
	SetFreeBlock(block_list, block);
}

void DisplayMBlockInfo()
{
	QMBlockList* p = pGlobalList;
	while (NULL != p)
	{
		printf("[MEMList] size:%d \n", p->mSize);
		QMBlock* temp = p->pFreeBlock;
		while (NULL != temp)
		{
			printf("[MEMList] [Block] [Free] address:%p\n", temp);
			temp = temp->pNext;
		}
		temp = p->pUsedBlock;
		while (NULL != temp)
		{
			printf("[MEMList] [Block] [Used] address:%p\n", temp);
			temp = temp->pNext;
		}
		p = p->pNext;
		printf("============================================\n");
	}
}

#ifndef USE_SYSTEM_MEM_ALLOC
void* operator new(size_t size)
{
	return QMalloc(size);
}
void* operator new[](size_t size)
{
	return QMalloc(size);
}
void operator delete(void *p)
{
	QRelease(p);
}
void operator delete[](void *p)
{
	QRelease(p);
}
#endif //USE_SYSTEM_MEM_ALLOC
int main()
{
	//for (int i = 0; i < 10; i++)
	{
		char* a = new char[2];
		char* a2 = new char[2];
		char* a1 = new char[2222];
		DisplayMBlockInfo();
		delete a2;
		DisplayMBlockInfo();
		delete a;
		int* b = new int[3];
		DisplayMBlockInfo();
		delete a1;
		long* c = new long[5];
		delete b;
		DisplayMBlockInfo();
		delete c;
		DisplayMBlockInfo();
	}
	
	return 0;
}