#include "All.h"
#ifdef BACKWARDS_COMPATIBILITY

#include "../APEInfo.h"
#include "UnBitarrayOld.h"
#include "../BitArray.h"

const unsigned __int32 K_SUM_MIN_BOUNDARY_OLD[32] = {0,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0,0,0};
const unsigned __int32 K_SUM_MAX_BOUNDARY_OLD[32] = {128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0,0,0,0};
const unsigned __int32 Powers_of_Two[32] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648};
const unsigned __int32 Powers_of_Two_Reversed[32] = {2147483648,1073741824,536870912,268435456,134217728,67108864,33554432,16777216,8388608,4194304,2097152,1048576,524288,262144,131072,65536,32768,16384,8192,4096,2048,1024,512,256,128,64,32,16,8,4,2,1};
const unsigned __int32 Powers_of_Two_Minus_One[33] = {0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535,131071,262143,524287,1048575,2097151,4194303,8388607,16777215,33554431,67108863,134217727,268435455,536870911,1073741823,2147483647,4294967295};
const unsigned __int32 Powers_of_Two_Minus_One_Reversed[33] = {4294967295,2147483647,1073741823,536870911,268435455,134217727,67108863,33554431,16777215,8388607,4194303,2097151,1048575,524287,262143,131071,65535,32767,16383,8191,4095,2047,1023,511,255,127,63,31,15,7,3,1,0};

const unsigned __int32 K_SUM_MIN_BOUNDARY[32] = {0,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0};
const unsigned __int32 K_SUM_MAX_BOUNDARY[32] = {32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648,0,0,0,0,0};

/***********************************************************************************
Construction
***********************************************************************************/
CUnBitArrayOld::CUnBitArrayOld(IAPEDecompress *pAPEDecompress, int nVersion) 
	: CUnBitArrayBase(pAPEDecompress, nVersion)
{
	int nBitArrayBytes = 262144;

	//calculate the bytes
	if (nVersion <= 3880)
	{
		int nMaxFrameBytes = (pAPEDecompress->GetInfo(APE_INFO_BLOCKS_PER_FRAME) * 50) / 8;
		nBitArrayBytes = 65536;
		while (nBitArrayBytes < nMaxFrameBytes)
		{
			nBitArrayBytes <<= 1;
		}
		
		nBitArrayBytes = max(nBitArrayBytes, 262144);
	}
	else if (nVersion <= 3890)
	{
		nBitArrayBytes = 65536;
	}
	else
	{
		//error
	}
	
	CreateHelper(pAPEDecompress, nBitArrayBytes, nVersion);

	//set the refill threshold
	if (m_nVersion <= 3880)
		m_nRefillBitThreshold = (m_nBits - (16384 * 8));
	else
		m_nRefillBitThreshold = (m_nBits - 512);
}

CUnBitArrayOld::~CUnBitArrayOld()
{
	SAFE_ARRAY_DELETE(m_pBitArray)
}

////////////////////////////////////////////////////////////////////////////////////
//Gets the number of m_nBits of data left in the m_nCurrentBitIndex array
////////////////////////////////////////////////////////////////////////////////////
unsigned __int32 CUnBitArrayOld::GetBitsRemaining()
{
	return (m_nElements * 32 - m_nCurrentBitIndex);
}

////////////////////////////////////////////////////////////////////////////////////
//Gets a rice value from the array
////////////////////////////////////////////////////////////////////////////////////
unsigned __int32 CUnBitArrayOld::DecodeValueRiceUnsigned(unsigned __int32 k) 
{
	//variable declares
	unsigned __int32 v;
	
	//plug through the string of 0's (the overflow)
	unsigned __int32 BitInitial = m_nCurrentBitIndex;
	while (!(m_pBitArray[m_nCurrentBitIndex >> 5] & Powers_of_Two_Reversed[m_nCurrentBitIndex++ & 31])) {}
	
	//if k = 0, your done
	if (k == 0)
		return (m_nCurrentBitIndex - BitInitial - 1);
	
	//put the overflow value into v
	v = (m_nCurrentBitIndex - BitInitial - 1) << k;
    
	return v | DecodeValueXBits(k);
}

////////////////////////////////////////////////////////////////////////////////////
//Get the optimal k for a given value
////////////////////////////////////////////////////////////////////////////////////
__inline unsigned __int32 CUnBitArrayOld::Get_K(unsigned __int32 x) 
{
	if (x == 0)	return 0;

	unsigned __int32 k = 0;
	while (x >= Powers_of_Two[++k]) {}
	return k;	
}

unsigned int CUnBitArrayOld::DecodeValue(DECODE_VALUE_METHOD DecodeMethod, int nParam1, int nParam2)
{
	switch (DecodeMethod)
	{
	case DECODE_VALUE_METHOD_UNSIGNED_INT:
		return DecodeValueXBits(32);
	case DECODE_VALUE_METHOD_UNSIGNED_RICE:
		return DecodeValueRiceUnsigned(nParam1);
	case DECODE_VALUE_METHOD_X_BITS:
		return DecodeValueXBits(nParam1);
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////
//Generates an array from the m_nCurrentBitIndexarray
////////////////////////////////////////////////////////////////////////////////////
void CUnBitArrayOld::GenerateArrayOld(int* Output_Array, unsigned __int32 Number_of_Elements, int Minimum_nCurrentBitIndex_Array_Bytes) {

	//variable declarations
	unsigned __int32 K_Sum;
	unsigned __int32 q;
	unsigned __int32 kmin, kmax;
	unsigned __int32 k;
	unsigned __int32 Max;
	int *p1, *p2;

	//fill bit array if necessary
	//could use seek information to determine what the max was...
	unsigned __int32 Max_Bits_Needed = Number_of_Elements * 50;

	if (Minimum_nCurrentBitIndex_Array_Bytes > 0) {
		//this is actually probably double what is really needed
		//we can only calculate the space needed for both arrays in multichannel
		Max_Bits_Needed = ((Minimum_nCurrentBitIndex_Array_Bytes + 4) * 8);
	}
	
	if (Max_Bits_Needed > GetBitsRemaining())
		FillBitArray();

	//unsigned int OriginalBit = m_nCurrentBitIndex;

	//decode the first 5 elements (all k = 10)
    Max = (Number_of_Elements < 5) ? Number_of_Elements : 5;
	for (q = 0; q < Max; q++) {
		Output_Array[q] = DecodeValueRiceUnsigned(10);
	}
	
	//quit if that was all
	if (Number_of_Elements <= 5) { 
		for (p2 = &Output_Array[0]; p2 < &Output_Array[Number_of_Elements]; p2++)
			*p2 = (*p2 & 1) ? (*p2 >> 1) + 1 : -(*p2 >> 1);
		return; 
	}

	//update k and K_Sum
	K_Sum = Output_Array[0] + Output_Array[1] + Output_Array[2] + Output_Array[3] + Output_Array[4];
	k = Get_K(K_Sum / 10);

	//work through the rest of the elements before the primary loop
	Max = (Number_of_Elements < 64) ? Number_of_Elements : 64;
	for (q = 5; q < Max; q++) {
		Output_Array[q] = DecodeValueRiceUnsigned(k);
		K_Sum += Output_Array[q];
		k = Get_K(K_Sum / (q  + 1) / 2);
	}

	//quit if that was all
	if (Number_of_Elements <= 64) { 
		for (p2 = &Output_Array[0]; p2 < &Output_Array[Number_of_Elements]; p2++)
			*p2 = (*p2 & 1) ? (*p2 >> 1) + 1 : -(*p2 >> 1);
		return; 
	}
		
	//set all of the variables up for the primary loop
	k = Get_K(K_Sum >> 7);
	kmin = K_SUM_MIN_BOUNDARY_OLD[k];
	kmax = K_SUM_MAX_BOUNDARY_OLD[k];

	unsigned __int32 *pBitArray = m_pBitArray;
	//unsigned __int32 *Bit = &m_nCurrentBitIndex;
	unsigned __int32 BitX = m_nCurrentBitIndex;

	//the primary loop
	//for (p1 = &Output_Array[64], p2 = &Output_Array[0]; p1 < &Output_Array[Number_of_Elements]; p1++, p2++) {
	p1 = &Output_Array[64]; p2 = &Output_Array[0];

#ifdef ENABLE_ASSEMBLY
	//the primary loop
	__asm 
	{

		//push the registers
		push eax
		push ebx
		push ecx
		push edx
		push esi
		push edi
		mov esi, DWORD PTR p1
		mov edi, DWORD PTR p2

		//test to see if we should continue
		mov ebx, Number_of_Elements
		mov eax, DWORD PTR Output_Array
		shl ebx, 2
		add eax, ebx
		cmp esi, eax
		jge LBL_DONE_MAIN_LOOP


		//the start of the main loop
		LBL_START_MAIN_LOOP:

			///////////////////////////////////////////////////////////////////////////
			//calculate the overflow
			///////////////////////////////////////////////////////////////////////////

			//load the values
			mov eax, DWORD PTR pBitArray
			mov ecx, BitX
			mov edx, ecx
			shr edx, 5
			lea eax, DWORD PTR [eax+edx*4]
			
			and ecx, 31
			
			//get and mask the current value
			mov ebx, [eax]
			shl ebx, cl
			shr ebx, cl
			
			//search for a "1"
			bsr edx, ebx

			//initialize the overflow
			mov ebx, 32
			
			//quit if a "1" was found
			jne LBL_OVERFLOW_DONE
				
				LBL_OVERFLOW_START:
			
				//increment the bit array pointer
				add eax, 4

				//increment the overflow
				add ebx, 32

				//search for a "1"
				bsr edx, [eax]

				je LBL_OVERFLOW_START
				
			LBL_OVERFLOW_DONE:


			//increment the pointer to the bit array if necessary
			cmp edx, 0
			jne LBL_SKIP_BIT_ARRAY_POINTER_INCREMENT
				add eax, 4
			LBL_SKIP_BIT_ARRAY_POINTER_INCREMENT:
				
			//calculate the overflow
			sub ebx, edx
			sub ebx, ecx

			//update the bit pointer
			add BitX, ebx

			//put the ((overflow - 1) << k) into the overflow variable
			sub ebx, 1
			mov ecx, k
			shl ebx, cl

			//move the shifted overflow into the "Value"
			mov [esi], ebx

			///////////////////////////////////////////////////////////////////////////
			//calculate the remainder
			///////////////////////////////////////////////////////////////////////////
			cmp k, 0
			je LBL_REMAINDER_DONE

				//get the bit index for shifting
				mov ecx, BitX
				
				//get the left and right values
				mov ebx, [eax]
				mov eax, [eax+4]
				
				//shift it left
				shld ebx, eax, cl
				
				//shift it back right
				mov ecx, 32
				sub ecx, k
				
				shr ebx, cl
				
				//mov Remainder, ebx
				add [esi], ebx

				//update the bit pointer
				mov ecx, k
				add BitX, ecx

			LBL_REMAINDER_DONE:

			///////////////////////////////////////////////////////////////////////////
			//update k sum and convert p2 to a signed value
			///////////////////////////////////////////////////////////////////////////

			//update K_Sum
			mov edx, K_Sum
			add edx, [esi]
			sub edx, [edi]
			mov K_Sum, edx

			//convert p2 to signed
			mov	eax, [edi]
			test al, 1
			je LBL_EVEN_SIGN
				sar eax, 1
				inc eax
				jmp LBL_DONE_SIGN
			LBL_EVEN_SIGN:
				sar eax, 1
				neg eax
			LBL_DONE_SIGN:
			mov [edi], eax

			///////////////////////////////////////////////////////////////////////////
			//update k
			///////////////////////////////////////////////////////////////////////////
			mov eax, k

			cmp edx, kmin
			jge LBL_TEST_MAX_K
				dec eax
				lea ecx, K_SUM_MIN_BOUNDARY_OLD[eax*4]
				cmp edx, [ecx]

				jge LBL_UPDATE_K
				LBL_DECREASE_K_START:

					dec eax
					sub ecx, 4
					cmp edx, [ecx]
					jl LBL_DECREASE_K_START

				jmp LBL_UPDATE_K

			LBL_TEST_MAX_K:

			cmp edx, kmax
			jl LBL_TEST_K_DONE
				inc eax
				lea ecx, K_SUM_MAX_BOUNDARY_OLD[eax*4]

				cmp edx, [ecx]
				jl LBL_UPDATE_K
				LBL_INCREASE_K_START:

					inc eax
					add ecx, 4
					cmp edx, [ecx]
					jge LBL_INCREASE_K_START

			LBL_UPDATE_K:

			//update k
			mov k, eax
			
			//update k min and max
			lea ecx, K_SUM_MIN_BOUNDARY_OLD[eax*4]
			mov ebx, [ecx]
			mov kmin, ebx
			lea ecx, K_SUM_MAX_BOUNDARY_OLD[eax*4]
			mov ebx, [ecx]
			mov kmax, ebx

			LBL_TEST_K_DONE:

			///////////////////////////////////////////////////////////////////////////
			//increment the pointers and test to see if we should continue
			///////////////////////////////////////////////////////////////////////////
			add esi, 4
			add edi, 4

			mov ebx, Number_of_Elements
			mov eax, DWORD PTR Output_Array
			shl ebx, 2
			add eax, ebx
			cmp esi, eax
			jl LBL_START_MAIN_LOOP

		LBL_DONE_MAIN_LOOP:

		mov p1, esi
		mov p2, edi

		//pop the registers
		pop edi
		pop esi
		pop edx
		pop ecx
		pop ebx
		pop eax
	}
#else
	#error "Assembly required by UnBitArrayOld.cpp."
#endif
	
	for (; p2 < &Output_Array[Number_of_Elements]; p2++) {
		*p2 = (*p2 & 1) ? (*p2 >> 1) + 1 : -(*p2 >> 1);
	}

	m_nCurrentBitIndex = BitX;
}

void CUnBitArrayOld::GenerateArray(int *pOutputArray, int nElements, int nBytesRequired) 
{
	if (m_nVersion < 3860)
	{
		GenerateArrayOld(pOutputArray, nElements, nBytesRequired);
	}
	else if (m_nVersion <= 3890)
	{
		GenerateArrayRice(pOutputArray, nElements, nBytesRequired);
	}
	else
	{	
		//error
	}
}

void CUnBitArrayOld::GenerateArrayRice(int* Output_Array, unsigned __int32 Number_of_Elements, int Minimum_nCurrentBitIndex_Array_Bytes) 
{
	/////////////////////////////////////////////////////////////////////////////
	//decode the bit array
	/////////////////////////////////////////////////////////////////////////////
	
	k = 10;
	K_Sum = 1024 * 16;

	if (m_nVersion <= 3880)
	{
		//the primary loop
		for (int *p1 = &Output_Array[0]; p1 < &Output_Array[Number_of_Elements]; p1++) 
		{
			*p1 = DecodeValueNew(FALSE);
		}
	}
	else
	{
		//the primary loop
		for (int *p1 = &Output_Array[0]; p1 < &Output_Array[Number_of_Elements]; p1++) 
		{
			*p1 = DecodeValueNew(TRUE);
		}
	}
}

__inline int CUnBitArrayOld::DecodeValueNew(BOOL bCapOverflow)
{
	//make sure there is room for the data
	//this is a little slower than ensuring a huge block to start with, but it's safer
	if (m_nCurrentBitIndex > m_nRefillBitThreshold)
	{
		FillBitArray();
	}
	
	unsigned int v;
	
	//plug through the string of 0's (the overflow)
	unsigned __int32 Bit_Initial = m_nCurrentBitIndex;
	while (!(m_pBitArray[m_nCurrentBitIndex >> 5] & Powers_of_Two_Reversed[m_nCurrentBitIndex++ & 31])) {}
	
	int nOverflow = (m_nCurrentBitIndex - Bit_Initial - 1);
	
	if (bCapOverflow)
	{
		while (nOverflow >= 16)
		{
			k += 4;
			nOverflow -= 16;
		}
	}
	
	//if k = 0, your done
	if (k != 0)
	{
		//put the overflow value into v
		v = nOverflow << k;
		
		//store the bit information and incement the bit pointer by 'k'
		unsigned int Bit_Array_Index = m_nCurrentBitIndex >> 5;
		unsigned int Bit_Index = m_nCurrentBitIndex & 31;
		m_nCurrentBitIndex += k;
		
		//figure the extra bits on the left and the left value
		int Left_Extra_Bits = (32 - k) - Bit_Index;
		unsigned int Left_Value = m_pBitArray[Bit_Array_Index] & Powers_of_Two_Minus_One_Reversed[Bit_Index];
		
		if (Left_Extra_Bits >= 0) 
		{
			v |= (Left_Value >> Left_Extra_Bits);
		}
		else 
		{
			v |= (Left_Value << -Left_Extra_Bits) | (m_pBitArray[Bit_Array_Index + 1] >> (32 + Left_Extra_Bits));
		}
	}	
	else
	{
		v = nOverflow;
	}
	
	//update K_Sum
	K_Sum += v - ((K_Sum + 8) >> 4);
	
	//update k
	if (K_Sum < K_SUM_MIN_BOUNDARY[k]) 
	{
		k--;
	}
	else if (K_Sum >= K_SUM_MAX_BOUNDARY[k]) 
	{
		k++;
	}
	
	//convert to unsigned and save
	return (v & 1) ? (v >> 1) + 1 : -(int(v >> 1));
}

#endif // #ifdef BACKWARDS_COMPATIBILITY
