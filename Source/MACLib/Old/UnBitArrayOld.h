#ifndef	_unbitarrayold_h_
#define _unbitarrayold_h_

#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "../UnBitArrayBase.h"

class IAPEDecompress;

//decodes 0000 up to and including 3890
class CUnBitArrayOld : public CUnBitArrayBase
{
	
public:
	//construction/destruction
	CUnBitArrayOld(IAPEDecompress *pAPEDecompress, int nVersion);
	~CUnBitArrayOld();
	
	//functions
	void GenerateArray(int *pOutputArray, int nElements, int nBytesRequired = -1);
	unsigned int DecodeValue(DECODE_VALUE_METHOD DecodeMethod, int nParam1 = 0, int nParam2 = 0);
	
private:
	
	void GenerateArrayOld(int* pOutputArray, unsigned __int32 NumberOfElements, int MinimumBitArrayBytes);
	void GenerateArrayRice(int* pOutputArray, unsigned __int32 NumberOfElements, int MinimumBitArrayBytes);
	
	unsigned __int32 DecodeValueRiceUnsigned(unsigned __int32 k);
	
	//data 
	unsigned __int32 k;
	unsigned __int32 K_Sum;
	unsigned __int32 m_nRefillBitThreshold;
	
	//functions
	__inline int DecodeValueNew(BOOL bCapOverflow);
	unsigned __int32 GetBitsRemaining();
	__inline unsigned __int32 Get_K(unsigned __int32 x);
};

#endif

#endif	/* _unbitarrayold_h_ */

