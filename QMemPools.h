#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

//#define OPEN_BLOCK_HEAD_CHECK 1
#define USE_SYSTEM_MEM_ALLOC 1
/*
* 内存分配的块
*/
typedef struct _QMBlock
{
	struct _QMBlock* pNext; //指向下一块
	struct _QMBlock* pPrev; //指向上一块
	void * pData; // 内存数据
}QMBlock; 

/*
* 用于存储分配对应内存大小的块的列表
*/
typedef struct _QMBlockList
{
	QMBlock* pUsedBlock; //已使用的内存块（头）
	QMBlock* pLastUsedBlock; //已使用的内存块（尾）
	QMBlock* pFreeBlock; // 空闲的内存块（头）
	QMBlock* pLastFreeBlock; //空闲的内存块（尾）
	struct _QMBlockList* pNext; //指向下一个内存块列表
	size_t mSize; // 当前内存块列表中块的大小
}QMBlockList;