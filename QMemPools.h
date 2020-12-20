#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

//#define OPEN_BLOCK_HEAD_CHECK 1
#define USE_SYSTEM_MEM_ALLOC 1
/*
* �ڴ����Ŀ�
*/
typedef struct _QMBlock
{
	struct _QMBlock* pNext; //ָ����һ��
	struct _QMBlock* pPrev; //ָ����һ��
	void * pData; // �ڴ�����
}QMBlock; 

/*
* ���ڴ洢�����Ӧ�ڴ��С�Ŀ���б�
*/
typedef struct _QMBlockList
{
	QMBlock* pUsedBlock; //��ʹ�õ��ڴ�飨ͷ��
	QMBlock* pLastUsedBlock; //��ʹ�õ��ڴ�飨β��
	QMBlock* pFreeBlock; // ���е��ڴ�飨ͷ��
	QMBlock* pLastFreeBlock; //���е��ڴ�飨β��
	struct _QMBlockList* pNext; //ָ����һ���ڴ���б�
	size_t mSize; // ��ǰ�ڴ���б��п�Ĵ�С
}QMBlockList;