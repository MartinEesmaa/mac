#include "All.h"
#include "GlobalFunctions.h"
#include "NNFilter.h"

CNNFilter::CNNFilter(int nOrder, int nShift)
{
	if ((nOrder <= 0) || ((nOrder % 16) != 0)) throw(1);
	m_nOrder = nOrder;
	m_nShift = nShift;
	
	m_bMMXAvailable = GetMMXAvailable();
	
	m_rbInput.Create(NN_WINDOW_ELEMENTS, m_nOrder);
	m_rbDeltaM.Create(NN_WINDOW_ELEMENTS, m_nOrder);
	m_paryM = new short [m_nOrder];

#ifdef NN_TEST_MMX
	srand(GetTickCount());
#endif
}

CNNFilter::~CNNFilter()
{
	SAFE_ARRAY_DELETE(m_paryM)
}

void CNNFilter::Flush()
{
	memset(&m_paryM[0], 0, m_nOrder * sizeof(short));
	m_rbInput.Flush();
	m_rbDeltaM.Flush();
}

short CNNFilter::GetSaturatedShortFromInt(int nValue)
{
	if (nValue > 32767) nValue = 32767;
	else if (nValue < -32768) nValue = -32768;
	return short(nValue);
}

int CNNFilter::Compress(int nInput)
{
	// convert the input to a short and store it
	m_rbInput[0] = GetSaturatedShortFromInt(nInput);

	// figure a dot product
	int nDotProduct = CalculateDotProduct(&m_rbInput[-m_nOrder],
		&m_paryM[0], m_nOrder);

	// calculate the output
	int nOutput = nInput - ((nDotProduct + (1 << (m_nShift - 1))) >> m_nShift);

	// adapt
	Adapt(&m_paryM[0], &m_rbDeltaM[-m_nOrder], -nOutput, m_nOrder);
	m_rbDeltaM[0] = (nInput == 0) ? 0 : ((nInput >> 28) & 8) - 4;
	m_rbDeltaM[-4] >>= 1;
	m_rbDeltaM[-8] >>= 1;

	// increment and roll if necessary
	m_rbInput.IncrementSafe();
	m_rbDeltaM.IncrementSafe();

	return nOutput;
}


int CNNFilter::Decompress(int nInput)
{
	// figure a dot product
	int nDotProduct = CalculateDotProduct(&m_rbInput[-m_nOrder],
		&m_paryM[0], m_nOrder);

	// adapt
	Adapt(&m_paryM[0], &m_rbDeltaM[-m_nOrder], -nInput, m_nOrder);

	// store the output value
	int nOutput = nInput + ((nDotProduct + (1 << (m_nShift - 1))) >> m_nShift);
	
	// update the input buffer
	m_rbInput[0] = GetSaturatedShortFromInt(nOutput);

	m_rbDeltaM[0] = (nOutput == 0) ? 0 : ((nOutput >> 28) & 8) - 4;
	m_rbDeltaM[-4] >>= 1;
	m_rbDeltaM[-8] >>= 1;
	
	// increment and roll if necessary
	m_rbInput.IncrementSafe();
	m_rbDeltaM.IncrementSafe();
	
	return nOutput;
}

void CNNFilter::Adapt(short * pM, short * pAdapt, int nDirection, int nOrder)
{
#ifdef ENABLE_ASSEMBLY
#ifdef NN_TEST_MMX
	if (m_bMMXAvailable && ((rand() & 255) > 128))
#else
	if (m_bMMXAvailable)
#endif // #ifdef NN_TEST_MMX
	{
		__asm
		{
			mov eax, pM
			mov ecx, pAdapt
			mov edx, nOrder
			shr edx, 4

			#define MMX_BLOCK(OP_CODE) \
				__asm movq mm0, [eax] \
				__asm OP_CODE mm0, [ecx] \
				__asm movq [eax], mm0 \
				__asm add eax, 8 \
				__asm add ecx, 8

			cmp nDirection, 0
			jle LBL_SUBTRACT
				LBL_LOOP_ADD:
					EXPAND_4_TIMES(MMX_BLOCK(paddw))
				dec edx
				jnz LBL_LOOP_ADD
				jmp LBL_DONE
			LBL_SUBTRACT:
			je LBL_DONE
				LBL_LOOP_SUBTRACT:
					EXPAND_4_TIMES(MMX_BLOCK(psubw))
				dec edx
				jnz LBL_LOOP_SUBTRACT
			LBL_DONE:
		
			#undef MMX_BLOCK

			emms
		}
	}
	else
	{
#endif // #ifdef ENABLE_ASSEMBLY
		nOrder >>= 4;

		if (nDirection > 0) 
		{	
			while (nOrder--)
			{
				EXPAND_16_TIMES(*pM++ += *pAdapt++;)
			}
		}
		else if (nDirection < 0)
		{
			while (nOrder--)
			{
				EXPAND_16_TIMES(*pM++ -= *pAdapt++;)
			}
		}
#ifdef ENABLE_ASSEMBLY
	}
#endif // #ifdef ENABLE_ASSEMBLY
}

int CNNFilter::CalculateDotProduct(short * pA, short * pB, int nOrder)
{
	int nDotProduct = 0;

#ifdef ENABLE_ASSEMBLY

#ifdef NN_TEST_MMX
	if (m_bMMXAvailable && ((rand() & 255) > 128))
#else
	if (m_bMMXAvailable)
#endif		
	{
		__asm
		{
			mov eax, pA
			mov ecx, pB
			mov edx, nOrder
			shr edx, 4

			pxor mm7, mm7

			#define MMX_BLOCK \
				__asm movq mm0, [eax] \
				__asm pmaddwd mm0, [ecx] \
				__asm paddd mm7, mm0 \
				__asm add eax, 8 \
				__asm add ecx, 8

			
			LBL_LOOP_START:
				EXPAND_4_TIMES(MMX_BLOCK)
			dec edx
			jnz LBL_LOOP_START

			#undef MMX_BLOCK

			// mm7 has the final dot-product (split into two dwords)
			movq mm6, mm7
			psrlq mm7, 32
			paddd mm6, mm7
			movd nDotProduct, mm6

			// this emms may be unnecessary, but it's probably better to be safe...
			emms
		}

		return nDotProduct;
	}
	else
	{
#endif // #ifdef ENABLE_ASSEMBLY
		
		nOrder >>= 4;

		while (nOrder--)
		{
			EXPAND_16_TIMES(nDotProduct += *pA++ * *pB++;)
		}
		
		return nDotProduct;

#ifdef ENABLE_ASSEMBLY
	}
#endif // #ifdef ENABLE_ASSEMBLY
}
