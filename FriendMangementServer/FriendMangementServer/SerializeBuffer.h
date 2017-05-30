#pragma once

#include <Windows.h>

/*---------------------------------------------------------------

Aya Library - Packet.

��Ʈ��ũ ��Ŷ�� Ŭ����.
�����ϰ� ��Ŷ�� ������� ����Ÿ�� In, Out �Ѵ�.

- ����.

CSerializeBuffer cPacket;

�ֱ�.
clPacket << 40030;	or	clPacket << iValue;	(int �ֱ�)
clPacket << 3;		or	clPacket << byValue;	(BYTE �ֱ�)
clPacket << 1.4;	or	clPacket << fValue;	(float �ֱ�)

����.
clPacket >> iValue;	(int ����)
clPacket >> byValue;	(BYTE ����)
clPacket >> fValue;	(float ����)

!.	���ԵǴ� ����Ÿ FIFO ������ �����ȴ�.
ť�� �ƴϹǷ�, �ֱ�(<<).����(>>) �� ȥ���ؼ� ����ϸ� �ȵȴ�.

----------------------------------------------------------------*/


class CSerializeBuffer
{
public:

	/*---------------------------------------------------------------
	AyaPacket Enum.

	----------------------------------------------------------------*/
	enum enAYA_PACKET
	{
		eBUFFER_DEFAULT = 20000		// ��Ŷ�� �⺻ ���� ������.
	};

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CSerializeBuffer();
	CSerializeBuffer(int iBufferSize);

	virtual	~CSerializeBuffer();


	//////////////////////////////////////////////////////////////////////////
	// ��Ŷ  �ı�.
	//
	// Parameters: ����.
	// Return: ����.
	//////////////////////////////////////////////////////////////////////////
	void	Release(void);


	//////////////////////////////////////////////////////////////////////////
	// ��Ŷ û��.
	//
	// Parameters: ����.
	// Return: ����.
	//////////////////////////////////////////////////////////////////////////
	void	Clear(void);


	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)��Ŷ ���� ������ ���.
	//////////////////////////////////////////////////////////////////////////
	int		GetBufferSize(void) { return m_iBufferSize; }
	//////////////////////////////////////////////////////////////////////////
	// ���� ������� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)������� ����Ÿ ������.
	//////////////////////////////////////////////////////////////////////////
	int		GetDataSize(void) { return m_iDataSize; }



	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (char *)���� ������.
	//////////////////////////////////////////////////////////////////////////
	char	*GetBufferPtr(void) { return m_chpBuffer; }


	char	*GetReadBufferPtr(void) { return m_chpReadPos;  }

	char	*GetWriteBufferPtr(void) { return m_chpWritePos; }
	//////////////////////////////////////////////////////////////////////////
	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);






	/* ============================================================================= */
	// ������ ���۷�����.
	/* ============================================================================= */
	CSerializeBuffer	&operator = (CSerializeBuffer &clSrCSerializeBuffer);

	//////////////////////////////////////////////////////////////////////////
	// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	CSerializeBuffer	&operator << (BYTE byValue);
	CSerializeBuffer	&operator << (char chValue);

	CSerializeBuffer	&operator << (short shValue);
	CSerializeBuffer	&operator << (WORD wValue);

	CSerializeBuffer	&operator << (int iValue);
	CSerializeBuffer	&operator << (DWORD dwValue);
	CSerializeBuffer	&operator << (float fValue);

	CSerializeBuffer	&operator << (__int64 iValue);
	CSerializeBuffer	&operator << (double dValue);

	//////////////////////////////////////////////////////////////////////////
	// ����.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	CSerializeBuffer	&operator >> (BYTE &byValue);
	CSerializeBuffer	&operator >> (char &chValue);

	CSerializeBuffer	&operator >> (short &shValue);
	CSerializeBuffer	&operator >> (WORD &wValue);

	CSerializeBuffer	&operator >> (int &iValue);
	CSerializeBuffer	&operator >> (DWORD &dwValue);
	CSerializeBuffer	&operator >> (float &fValue);

	CSerializeBuffer	&operator >> (__int64 &iValue);
	CSerializeBuffer	&operator >> (UINT64 &iValue);
	CSerializeBuffer	&operator >> (double &dValue);




	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ���.
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int		GetData(char *chpDest, int iSize);

	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ����.
	//
	// Parameters: (char *)Src ������. (int)SrcSize.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int		PutData(char *chpSrc, int iSrcSize);




protected:

	//------------------------------------------------------------
	// ��Ŷ���� / ���� ������.
	//------------------------------------------------------------
	char*	m_chpBuffer;
	int		m_iBufferSize;

	//------------------------------------------------------------
	// ������ ���� ��ġ, ���� ��ġ.
	//------------------------------------------------------------
	char	*m_chpReadPos;
	char	*m_chpWritePos;


	//------------------------------------------------------------
	// ���� ���ۿ� ������� ������.
	//------------------------------------------------------------
	int		m_iDataSize;

	DWORD errCode;


	union floatdata
	{
		char value[4];
		float a;
	};

	union doubledata
	{
		char value[8];
		double a;
	};

};


