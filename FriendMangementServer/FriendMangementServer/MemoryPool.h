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
	// 생성자
	// int - 블럭 갯수
	// bool - 블록 생성자 호출여부(기본값 = FALSE)
	//////////////////////////////////
	CMemoryPool(int blockSize, bool bConst = false);
	virtual ~CMemoryPool();


	//////////////////////////////////
	// 블록 하나를 할당해주는 함수 -> new 선언해줘야 한다면 한다.
	// 리턴 : 특정 블록의 공간 포인터 리턴
	//////////////////////////////////
	DATA* Alloc(void);

	//////////////////////////////////
	// 사용중인 블록을 반환하는 함수
	// 파라미터 : 사용중인 데이터 주소값-> 소멸자 호출해야 된다면 하고 안한다면 그냥 반환
	// 리턴 : 성공여부
	//////////////////////////////////
	bool Free(DATA *pData); // 그렇다면 외부에서 이 함수를 통해 반환하고, 나중에 이 주소값을 사용하려고 한다면? -> 주의

	
	//////////////////////////////////
	// 총 확보된 블록의 갯수 리턴
	//////////////////////////////////
	int GetBlockCount(void);



	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// 파라미터: 사용중인 블럭 개수.
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
		// 누수발생
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
		// 0번 인덱스를 참조하게 한다
		pTop = &pDataArray[0]; // 여기 아직의문 경우의 수가 이거 하나밖에 없는가?
	


	if (m_bUseConstruct)
		// New Placement 실행
		new (&pTop->Data) DATA();

	ret = &pTop->Data;
	if (m_iAllocCount != m_iBlockSize - 1)
		pTop = pTop->pNextBlock;
	
	m_iAllocCount++;

	return ret;
}
// DATA pData와 st_BLOCK_NODE pTop으로 처리해야된다.
// 1. pData가 not null일때
// 2. pTop의 next을 pData로
// 3. pData의 next를 pTop의 next로
// 4. 치환
template <class DATA>
bool CMemoryPool<DATA>::Free(DATA *pData)
{
	st_BLOCK_NODE *pDel = (st_BLOCK_NODE *)(pData - 1); // DWORD는 4바이트이므로 데이터 보다 4바이트 위로 올리면 구조체를 가르킬수가 있다.

	if (pDel->ValidCode != VALIDCODE)
		return false;

	if (pTop == nullptr)
		return false;
	

	if (pTop != pDel) // 같다면 문제있음
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