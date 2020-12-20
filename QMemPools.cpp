#include "QMemPools.h"

// 是否校验块的正确性
#define BLOCK_HEAD_TAG 0xCF22
#ifdef  OPEN_BLOCK_HEAD_CHECK
#define BLOCK_FLAG_SIZE  (sizeof(intptr_t)*2 + sizeof(int))
#else
#define BLOCK_FLAG_SIZE  (sizeof(intptr_t)*2)
#endif //  OPEN_BLOCK_HEAD_CHECK

// 首个内存块列表
QMBlockList* pGlobalList = NULL;
// 尾部内存块列表
QMBlockList* pGlobalTailList = NULL;

/*
* 创建内存块
*/
QMBlock* CreateBlock(size_t size)
{
	QMBlock* block = (QMBlock*)malloc(sizeof(QMBlock));
	if (NULL == block)
	{	// 申请内存失败
		return NULL;
	}
	// 申请内存 - 额外的大小用来存储管理信息
	void* pBuff = malloc(size + BLOCK_FLAG_SIZE);
	if (NULL == pBuff)
	{	// 申请内存失败
		free(block);
		return NULL;
	}
	// 返回申请好的内存
	block->pData = pBuff;
	block->pNext = NULL;
	block->pPrev = NULL;
	return block;
}
/*
* 创建内存块列表
*/
QMBlockList* CreateBlockList(size_t size)
{
	QMBlockList* block_list = (QMBlockList*)malloc(sizeof(QMBlockList));
	if (NULL == block_list)
	{	//申请内存失败
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
* 获取相应size大小的内存块列表（返回1表示新创建的，0表示旧的，-1表示失败）
*/
int GetBlockList(size_t size, QMBlockList** out)
{
	if (NULL == pGlobalList)
	{
		QMBlockList* block_list = CreateBlockList(size);
		if (NULL == block_list)
		{	//创建失败
			return -1;
		}
		*out = block_list;
		return 1; // 这是新创建的
	}
	// 查找有没有对应size大小的内存块列表记录
	QMBlockList* p = pGlobalList;
	while (NULL != p) //这里产生了一个遍历的消耗
	{
		if (p->mSize != size)
		{
			p = p->pNext;
			continue;
		}
		*out = p;
		return 0; // 找到匹配大小的内存块列表，返回
	}
	// 走到这里就表示没有匹配
	p = CreateBlockList(size);
	if (NULL == p)
	{	//创建失败
		return -1;
	}
	*out = p;
	return 1; //新创建的
}
/*
* 获取一个可以使用的内存块
*/
QMBlock* GetFreeBlock(QMBlockList* block_list)
{
	if (NULL == block_list->pFreeBlock) // 可使用的头都是空
	{
		QMBlock* block = CreateBlock(block_list->mSize);
		if (NULL == block)
		{	// 创建失败
			return NULL;
		}
		return block;
	}
	// 说明有可用的
	QMBlock* block = block_list->pFreeBlock;
	block_list->pFreeBlock = block->pNext;
	// 如果就只有一块的时候，需要更新最后可用的指向
	if (block_list->pLastFreeBlock == block)
	{
		block_list->pLastFreeBlock = NULL;
	}
	return block;
}
/*
* 设置内存块到已用列表
*/
void SetUsedBlock(QMBlockList* block_list, QMBlock* block)
{
	// 清理数据
	block->pNext = NULL; 
	block->pPrev = NULL;
	if (NULL == block_list->pUsedBlock) // 已使用列表头是空
	{
		block_list->pUsedBlock = block;
		block_list->pLastUsedBlock = block;
		return;
	}
	// 说明已使用存在 -- 就更新一下表
	block_list->pLastUsedBlock->pNext = block;
	block->pPrev = block_list->pLastUsedBlock;
	block_list->pLastUsedBlock = block;
}
/*
* 从可用的表中移除当前block
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
	// 就是第一个
	if (block_list->pUsedBlock == block)
	{
		block_list->pUsedBlock = NULL;
		block_list->pLastUsedBlock = NULL;
	}
}
/*
* 设置内存块到可用列表
*/
void SetFreeBlock(QMBlockList* block_list, QMBlock* block)
{
	block->pNext = NULL;
	block->pPrev = NULL;
	if (NULL == block_list->pFreeBlock) // 可用列表头是空
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
* 绑定当前内存的内存块地址和内存块表地址
*/
void* BindBlockData(QMBlockList* block_list, QMBlock* block)
{
#ifdef OPEN_BLOCK_HEAD_CHECK
	int* temp_ptr = (int*)block->pData;
	temp_ptr[0] = BLOCK_HEAD_TAG; //注意这里只能占用4个字节，待验证
	intptr_t* ptr = (intptr_t*)(((char*)block->pData)+sizeof(int)); //强转为指针类型（跨平台）
	ptr[0] = (intptr_t)block_list;
	ptr[1] = (intptr_t)block; 
	return &ptr[2];
#else
	intptr_t* ptr = (intptr_t*)block->pData; //强转为指针类型（跨平台）
	ptr[0] = (intptr_t)block_list;
	ptr[1] = (intptr_t)block;
	return &ptr[2];
#endif // OPEN_BLOCK_HEAD_CHECK
}
/*
* 解绑数据
*/
int UnbindBlockData(void* p, QMBlockList** block_list, QMBlock** block)
{
	intptr_t* ptr = (intptr_t*)p; //强转
	*block_list = (QMBlockList*)(*(ptr - 2)); //往前减
	*block = (QMBlock*)(*(ptr - 1));
#ifdef OPEN_BLOCK_HEAD_CHECK
	char* temp_ptr = ((char*)p) - BLOCK_FLAG_SIZE;
	int check_k = *(int*)(temp_ptr);
	if (check_k != BLOCK_HEAD_TAG)
	{	//校验失败了
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
	{	//获取内存块表失败了
		return NULL;
	}
	else if (1 == res) // 新创建的
	{
		if (NULL == pGlobalList) //第一块内存块表
		{
			pGlobalList = block_list;
			pGlobalTailList = block_list;
		}
		else
		{
			pGlobalTailList->pNext = block_list;
			pGlobalTailList = pGlobalTailList->pNext; //始终指向最后那个内存块表
		}
	}
	QMBlock* block = GetFreeBlock(block_list); // 获取可用内存块
	if (NULL == block)
	{	// 获取失败
		return NULL;
	}
	// 设置为已用
	SetUsedBlock(block_list, block);
	// 返回增加校验信息的可用内存地址
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