#pragma once

#include "RollBuffer.h"
#define NN_WINDOW_ELEMENTS	512
//#define NN_TEST_MMX

class CNNFilter
{
public:

	CNNFilter(int nOrder, int nShift);
	~CNNFilter();

	int Compress(int nInput);
	int Decompress(int nInput);
	void Flush();

private:

	int m_nOrder;
	int m_nShift;
	BOOL m_bMMXAvailable;

	CRollBuffer<short> m_rbInput;
	CRollBuffer<short> m_rbDeltaM;

	short * m_paryM;

	__inline void Adapt(short * pM, short * pAdapt, int nDirection, int nOrder);
	__inline int CalculateDotProduct(short * pA, short * pB, int nOrder);
	__inline short GetSaturatedShortFromInt(int nValue);
};
