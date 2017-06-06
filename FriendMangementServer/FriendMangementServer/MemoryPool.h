#pragma once

#include <Windows.h>
#include <new>


#ifdef PROFILE_CHECK
#define PRO_BEGIN(x)  ProfileBegin(x)
#define PRO_END(x)  ProfileEnd(x)
#else
#define PRO_BEGIN(x)
#define PRO_END(x)
#endif

#define VALIDCODE (DWORD)(0x77777777)

#define dfERR_NOTFREE 2080

template <class DATA>
class CMemoryPool
{
private:
	struct st_BLOCK_NODE
	{
		DWORD ValidCode;
		DATA Data;
		
		st_BLOCK_NODE *pNextBlock;
	};
public:
	//////////////////////////////////
	// ������
	// int - �� ����
	// bool - ��� ������ ȣ�⿩��(�⺻�� = FALSE)
	//////////////////////////////////
	CMemoryPool(int blockSize, bool bConst = false);
	virtual ~CMemoryPool();


	//////////////////////////////////
	// ��� �ϳ��� �Ҵ����ִ� �Լ� -> new ��������� �Ѵٸ� �Ѵ�.
	// ���� : Ư�� ����� ���� ������ ����
	//////////////////////////////////
	DATA* Alloc(void);

	//////////////////////////////////
	// ������� ����� ��ȯ�ϴ� �Լ�
	// �Ķ���� : ������� ������ �ּҰ�-> �Ҹ��� ȣ���ؾ� �ȴٸ� �ϰ� ���Ѵٸ� �׳� ��ȯ
	// ���� : ��������
	//////////////////////////////////
	bool Free(DATA *pData); // �׷��ٸ� �ܺο��� �� �Լ��� ���� ��ȯ�ϰ�, ���߿� �� �ּҰ��� ����Ϸ��� �Ѵٸ�? -> ����

	
	//////////////////////////////////
	// �� Ȯ���� ����� ���� ����
	//////////////////////////////////
	int GetBlockCount(void);



	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// �Ķ����: ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int GetAllocCount(void);


private:
	int m_iBlockSize;
	bool m_bUseConstruct;

	int m_iAllocCount;

	st_BLOCK_NODE *pTop;
	st_BLOCK_NODE *pDataArray;

};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class DATA>
CMemoryPool<DATA>::CMemoryPool(int blockSize, bool bConst)
	:m_iBlockSize(blockSize), m_bUseConstruct(bConst)
{
	m_iAllocCount = 0;
	pTop = nullptr;

	pDataArray = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE) * blockSize);
	
	for (int i = 0; i < blockSize - 1; i++)
	{
		pDataArray[i].pNextBlock = &pDataArray[i + 1];
		pDataArray[i].ValidCode = VALIDCODE;
	}
	pDataArray[blockSize - 1].ValidCode = VALIDCODE;
	pDataArray[blockSize - 1].pNextBlock = nullptr;
		
}

template <class DATA>
CMemoryPool<DATA>::~CMemoryPool()
{
	if (m_iAllocCount == 0)
	{
		// �����߻�
	}

	if (m_bUseConstruct)
		for (int i = 0; i < m_iBlockSize; i++)
			pDataArray[i].Data.~DATA();
	else
		for (int i = 0; i < m_iBlockSize; i++)
			free(&pDataArray[i]);
}

template <class DATA>
DATA* CMemoryPool<DATA>::Alloc(void)
{
	if (m_iAllocCount >= m_iBlockSize)
		return nullptr;
	DATA* ret;
	if (pTop == nullptr)
		// 0�� �ε����� �����ϰ� �Ѵ�
		pTop = &pDataArray[0]; // ���� �����ǹ� ����� ���� �̰� �ϳ��ۿ� ���°�?
	


	if (m_bUseConstruct)
		// New Placement ����
		new (&pTop->Data) DATA();

	ret = &pTop->Data;
	if (m_iAllocCount != m_iBlockSize - 1)
		pTop = pTop->pNextBlock;
	
	m_iAllocCount++;

	return ret;
}
// DATA pData�� st_BLOCK_NODE pTop���� ó���ؾߵȴ�.
// 1. pData�� not null�϶�
// 2. pTop�� next�� pData��
// 3. pData�� next�� pTop�� next��
// 4. ġȯ
template <class DATA>
bool CMemoryPool<DATA>::Free(DATA *pData)
{
	st_BLOCK_NODE *pDel = (st_BLOCK_NODE *)(pData - 1); // DWORD�� 4����Ʈ�̹Ƿ� ������ ���� 4����Ʈ ���� �ø��� ����ü�� ����ų���� �ִ�.

	if (pDel->ValidCode != VALIDCODE)
		return false;

	if (pTop == nullptr)
		return false;
	

	if (pTop != pDel) // ���ٸ� ��������
	{
		st_BLOCK_NODE *T = pTop->pNextBlock;
		pTop->pNextBlock = pDel;
		pDel->pNextBlock = T;
		pTop = pDel;
	}


	m_iAllocCount--;

	return true;
}

template <class DATA>
int CMemoryPool<DATA>::GetBlockCount(void)
{
	return m_iBlockSize;
}

template <class DATA>
int CMemoryPool<DATA>::GetAllocCount(void)
{
	return m_iAllocCount;
}