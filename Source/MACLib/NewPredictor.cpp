#include "All.h"
#include "APECompress.h"
#include "NewPredictor.h"
	
/*****************************************************************************************
CPredictorCompressNormal
*****************************************************************************************/
CPredictorCompressNormal::CPredictorCompressNormal(int nCompressionLevel) 
	: IPredictorCompress(nCompressionLevel)
{
	if (nCompressionLevel == COMPRESSION_LEVEL_FAST)
	{
		m_pNNFilter = NULL;
		m_pNNFilter1 = NULL;
	}
	else if (nCompressionLevel == COMPRESSION_LEVEL_NORMAL)
	{
		m_pNNFilter = new CNNFilter(16, 11);
		m_pNNFilter1 = NULL;
	}
	else if (nCompressionLevel == COMPRESSION_LEVEL_HIGH)
	{
		m_pNNFilter = new CNNFilter(64, 11);
		m_pNNFilter1 = NULL;
	}
	else if (nCompressionLevel == COMPRESSION_LEVEL_EXTRA_HIGH)
	{
		m_pNNFilter = new CNNFilter(256, 13);
		m_pNNFilter1 = new CNNFilter(32, 10);
	}
	else
	{
		throw(1);
	}
}

CPredictorCompressNormal::~CPredictorCompressNormal()
{
	SAFE_DELETE(m_pNNFilter)
	SAFE_DELETE(m_pNNFilter1)
}
	
int CPredictorCompressNormal::Flush()
{
	if (m_pNNFilter) m_pNNFilter->Flush();
	if (m_pNNFilter1) m_pNNFilter1->Flush();

	m_rbPredictionA.Flush(); m_rbPredictionB.Flush();
	m_rbAdaptA.Flush(); m_rbAdaptB.Flush();
	m_Stage1FilterA.Flush(); m_Stage1FilterB.Flush();

	ZeroMemory(m_aryMA, sizeof(m_aryMA));
	ZeroMemory(m_aryMB, sizeof(m_aryMB));

	m_aryMA[0] = 360;
	m_aryMA[1] = 317;
	m_aryMA[2] = -109;
	m_aryMA[3] = 98;

	m_nLastValueA = 0;

	m_nCurrentIndex = 0;

	return 0;
}


int CPredictorCompressNormal::CompressValue(int nA, int nB)
{
	if (m_nCurrentIndex == WINDOW_BLOCKS)
	{
		m_rbPredictionA.Roll();	m_rbPredictionB.Roll();
		m_rbAdaptA.Roll(); m_rbAdaptB.Roll();

		m_nCurrentIndex = 0;
	}

	// stage 1: simple, non-adaptive order 1 prediction
	int nCurrentA = m_Stage1FilterA.Compress(nA);
	int nCurrentB = m_Stage1FilterB.Compress(nB);

	// stage 2: adaptive offset filter(s)
	m_rbPredictionA[0] = m_nLastValueA;
	m_rbPredictionA[-1] = m_rbPredictionA[0] - m_rbPredictionA[-1];
	
	m_rbPredictionB[0] = nCurrentB;
	m_rbPredictionB[-1] = m_rbPredictionB[0] - m_rbPredictionB[-1];

	int nPredictionA = (m_rbPredictionA[0] * m_aryMA[0]) + (m_rbPredictionA[-1] * m_aryMA[1]) + (m_rbPredictionA[-2] * m_aryMA[2]) + (m_rbPredictionA[-3] * m_aryMA[3]);
	int nPredictionB = (m_rbPredictionB[0] * m_aryMB[0]) + (m_rbPredictionB[-1] * m_aryMB[1]) + (m_rbPredictionB[-2] * m_aryMB[2]) + (m_rbPredictionB[-3] * m_aryMB[3]) + (m_rbPredictionB[-4] * m_aryMB[4]);

	int nOutput = nCurrentA - ((nPredictionA + (nPredictionB >> 1)) >> 10);

	m_nLastValueA = nCurrentA;

	m_rbAdaptA[0] = (m_rbPredictionA[0]) ? ((m_rbPredictionA[0] >> 30) & 2) - 1 : 0;
	m_rbAdaptA[-1] = (m_rbPredictionA[-1]) ? ((m_rbPredictionA[-1] >> 30) & 2) - 1 : 0;
	
	m_rbAdaptB[0] = (m_rbPredictionB[0]) ? ((m_rbPredictionB[0] >> 30) & 2) - 1 : 0;
	m_rbAdaptB[-1] = (m_rbPredictionB[-1]) ? ((m_rbPredictionB[-1] >> 30) & 2) - 1 : 0;

	if (nOutput > 0) 
	{
		m_aryMA[0] -= m_rbAdaptA[0];
		m_aryMA[1] -= m_rbAdaptA[-1];
		m_aryMA[2] -= m_rbAdaptA[-2];
		m_aryMA[3] -= m_rbAdaptA[-3];

		m_aryMB[0] -= m_rbAdaptB[0];
		m_aryMB[1] -= m_rbAdaptB[-1];
		m_aryMB[2] -= m_rbAdaptB[-2];
		m_aryMB[3] -= m_rbAdaptB[-3];
		m_aryMB[4] -= m_rbAdaptB[-4];
	}
	else if (nOutput < 0) 
	{
		m_aryMA[0] += m_rbAdaptA[0];
		m_aryMA[1] += m_rbAdaptA[-1];
		m_aryMA[2] += m_rbAdaptA[-2];
		m_aryMA[3] += m_rbAdaptA[-3];

		m_aryMB[0] += m_rbAdaptB[0];
		m_aryMB[1] += m_rbAdaptB[-1];
		m_aryMB[2] += m_rbAdaptB[-2];
		m_aryMB[3] += m_rbAdaptB[-3];
		m_aryMB[4] += m_rbAdaptB[-4];
	}

	// stage 3: NNFilters

	if (m_pNNFilter)
		nOutput = m_pNNFilter->Compress(nOutput);
	if (m_pNNFilter1)
		nOutput = m_pNNFilter1->Compress(nOutput);

	m_rbPredictionA.IncrementFast(); m_rbPredictionB.IncrementFast();
	m_rbAdaptA.IncrementFast(); m_rbAdaptB.IncrementFast();

	m_nCurrentIndex++;

	return nOutput;
}

/*****************************************************************************************
CPredictorDecompressNormal3930to3950
*****************************************************************************************/
CPredictorDecompressNormal3930to3950::CPredictorDecompressNormal3930to3950(int nCompressionLevel) 
	: IPredictorDecompress(nCompressionLevel)
{
	m_pBuffer[0] = new int [HISTORY_ELEMENTS + WINDOW_BLOCKS];

	if (nCompressionLevel == COMPRESSION_LEVEL_FAST)
	{
		m_pNNFilter = NULL;
		m_pNNFilter1 = NULL;
	}
	else if (nCompressionLevel == COMPRESSION_LEVEL_NORMAL)
	{
		m_pNNFilter = new CNNFilter(16, 11);
		m_pNNFilter1 = NULL;new CNNFilter(32, 10);
	}
	else if (nCompressionLevel == COMPRESSION_LEVEL_HIGH)
	{
		m_pNNFilter = new CNNFilter(64, 11);
		m_pNNFilter1 = NULL;
	}
	else if (nCompressionLevel == COMPRESSION_LEVEL_EXTRA_HIGH)
	{
		m_pNNFilter = new CNNFilter(256, 13);
		m_pNNFilter1 = new CNNFilter(32, 10);
	}
	else
	{
		throw(1);
	}
}

CPredictorDecompressNormal3930to3950::~CPredictorDecompressNormal3930to3950()
{
	SAFE_DELETE(m_pNNFilter)
	SAFE_DELETE(m_pNNFilter1)
	SAFE_ARRAY_DELETE(m_pBuffer[0])
}
	
int CPredictorDecompressNormal3930to3950::Flush()
{
	if (m_pNNFilter) m_pNNFilter->Flush();
	if (m_pNNFilter1) m_pNNFilter1->Flush();

	ZeroMemory(m_pBuffer[0], (HISTORY_ELEMENTS + 1) * sizeof(int));	
	ZeroMemory(&m_aryM[0], M_COUNT * sizeof(int));

	m_aryM[0] = 360;
	m_aryM[1] = 317;
	m_aryM[2] = -109;
	m_aryM[3] = 98;

	m_pInputBuffer = &m_pBuffer[0][HISTORY_ELEMENTS];
	
	m_nLastValue = 0;
	m_nCurrentIndex = 0;

	return 0;
}

int CPredictorDecompressNormal3930to3950::DecompressValue(int nInput, int)
{
	if (m_nCurrentIndex == WINDOW_BLOCKS)
	{
		// copy forward and adjust pointers
		memcpy(&m_pBuffer[0][0], &m_pBuffer[0][WINDOW_BLOCKS], HISTORY_ELEMENTS * sizeof(int));
		m_pInputBuffer = &m_pBuffer[0][HISTORY_ELEMENTS];

		m_nCurrentIndex = 0;
	}

	// stage 2: NNFilter
	if (m_pNNFilter1)
		nInput = m_pNNFilter1->Decompress(nInput);
	if (m_pNNFilter)
		nInput = m_pNNFilter->Decompress(nInput);

	// stage 1: multiple predictors (order 2 and offset 1)

	int p1 = m_pInputBuffer[-1];
	int p2 = m_pInputBuffer[-1] - m_pInputBuffer[-2];
	int p3 = m_pInputBuffer[-2] - m_pInputBuffer[-3];
	int p4 = m_pInputBuffer[-3] - m_pInputBuffer[-4];
	
	m_pInputBuffer[0] = nInput + (((p1 * m_aryM[0]) + (p2 * m_aryM[1]) + (p3 * m_aryM[2]) + (p4 * m_aryM[3])) >> 9);
	
	if (nInput > 0) 
	{
		m_aryM[0] -= ((p1 >> 30) & 2) - 1;
		m_aryM[1] -= ((p2 >> 30) & 2) - 1;
		m_aryM[2] -= ((p3 >> 30) & 2) - 1;
		m_aryM[3] -= ((p4 >> 30) & 2) - 1;
	}
	else if (nInput < 0) 
	{
		m_aryM[0] += ((p1 >> 30) & 2) - 1;
		m_aryM[1] += ((p2 >> 30) & 2) - 1;
		m_aryM[2] += ((p3 >> 30) & 2) - 1;
		m_aryM[3] += ((p4 >> 30) & 2) - 1;
	}

	int nRetVal = m_pInputBuffer[0] + ((m_nLastValue * 31) >> 5);
	m_nLastValue = nRetVal;

	m_nCurrentIndex++;
	m_pInputBuffer++;

	return nRetVal;
}

/*****************************************************************************************
CPredictorDecompress3950toCurrent
*****************************************************************************************/
CPredictorDecompress3950toCurrent::CPredictorDecompress3950toCurrent(int nCompressionLevel) 
	: IPredictorDecompress(nCompressionLevel)
{
	if (nCompressionLevel == COMPRESSION_LEVEL_FAST)
	{
		m_pNNFilter = NULL;
		m_pNNFilter1 = NULL;
	}
	else if (nCompressionLevel == COMPRESSION_LEVEL_NORMAL)
	{
		m_pNNFilter = new CNNFilter(16, 11);
		m_pNNFilter1 = NULL;new CNNFilter(32, 10);
	}
	else if (nCompressionLevel == COMPRESSION_LEVEL_HIGH)
	{
		m_pNNFilter = new CNNFilter(64, 11);
		m_pNNFilter1 = NULL;
	}
	else if (nCompressionLevel == COMPRESSION_LEVEL_EXTRA_HIGH)
	{
		m_pNNFilter = new CNNFilter(256, 13);
		m_pNNFilter1 = new CNNFilter(32, 10);
	}
	else
	{
		throw(1);
	}
}

CPredictorDecompress3950toCurrent::~CPredictorDecompress3950toCurrent()
{
	SAFE_DELETE(m_pNNFilter)
	SAFE_DELETE(m_pNNFilter1)
}
	
int CPredictorDecompress3950toCurrent::Flush()
{
	if (m_pNNFilter) m_pNNFilter->Flush();
	if (m_pNNFilter1) m_pNNFilter1->Flush();

	ZeroMemory(m_aryMA, sizeof(m_aryMA));
	ZeroMemory(m_aryMB, sizeof(m_aryMB));

	m_rbPredictionA.Flush();
	m_rbPredictionB.Flush();
	m_rbAdaptA.Flush();
	m_rbAdaptB.Flush();

	m_aryMA[0] = 360;
	m_aryMA[1] = 317;
	m_aryMA[2] = -109;
	m_aryMA[3] = 98;

	m_Stage1FilterA.Flush();
	m_Stage1FilterB.Flush();
	
	m_nLastValueA = 0;
	
	m_nCurrentIndex = 0;

	return 0;
}

int CPredictorDecompress3950toCurrent::DecompressValue(int nA, int nB)
{
	if (m_nCurrentIndex == WINDOW_BLOCKS)
	{
		// copy forward and adjust pointers
		m_rbPredictionA.Roll();	m_rbPredictionB.Roll();
		m_rbAdaptA.Roll(); m_rbAdaptB.Roll();

		m_nCurrentIndex = 0;
	}

	// stage 2: NNFilter
	if (m_pNNFilter1)
		nA = m_pNNFilter1->Decompress(nA);
	if (m_pNNFilter)
		nA = m_pNNFilter->Decompress(nA);

	// stage 1: multiple predictors (order 2 and offset 1)
	m_rbPredictionA[0] = m_nLastValueA;
	m_rbPredictionA[-1] = m_rbPredictionA[0] - m_rbPredictionA[-1];
	
	m_rbPredictionB[0] = m_Stage1FilterB.Compress(nB);
	m_rbPredictionB[-1] = m_rbPredictionB[0] - m_rbPredictionB[-1];

	int nPredictionA = (m_rbPredictionA[0] * m_aryMA[0]) + (m_rbPredictionA[-1] * m_aryMA[1]) + (m_rbPredictionA[-2] * m_aryMA[2]) + (m_rbPredictionA[-3] * m_aryMA[3]);
	int nPredictionB = (m_rbPredictionB[0] * m_aryMB[0]) + (m_rbPredictionB[-1] * m_aryMB[1]) + (m_rbPredictionB[-2] * m_aryMB[2]) + (m_rbPredictionB[-3] * m_aryMB[3]) + (m_rbPredictionB[-4] * m_aryMB[4]);

	int nCurrentA = nA + ((nPredictionA + (nPredictionB >> 1)) >> 10);

	m_rbAdaptA[0] = (m_rbPredictionA[0]) ? ((m_rbPredictionA[0] >> 30) & 2) - 1 : 0;
	m_rbAdaptA[-1] = (m_rbPredictionA[-1]) ? ((m_rbPredictionA[-1] >> 30) & 2) - 1 : 0;
	
	m_rbAdaptB[0] = (m_rbPredictionB[0]) ? ((m_rbPredictionB[0] >> 30) & 2) - 1 : 0;
	m_rbAdaptB[-1] = (m_rbPredictionB[-1]) ? ((m_rbPredictionB[-1] >> 30) & 2) - 1 : 0;

	if (nA > 0) 
	{
		m_aryMA[0] -= m_rbAdaptA[0];
		m_aryMA[1] -= m_rbAdaptA[-1];
		m_aryMA[2] -= m_rbAdaptA[-2];
		m_aryMA[3] -= m_rbAdaptA[-3];

		m_aryMB[0] -= m_rbAdaptB[0];
		m_aryMB[1] -= m_rbAdaptB[-1];
		m_aryMB[2] -= m_rbAdaptB[-2];
		m_aryMB[3] -= m_rbAdaptB[-3];
		m_aryMB[4] -= m_rbAdaptB[-4];
	}
	else if (nA < 0) 
	{
		m_aryMA[0] += m_rbAdaptA[0];
		m_aryMA[1] += m_rbAdaptA[-1];
		m_aryMA[2] += m_rbAdaptA[-2];
		m_aryMA[3] += m_rbAdaptA[-3];

		m_aryMB[0] += m_rbAdaptB[0];
		m_aryMB[1] += m_rbAdaptB[-1];
		m_aryMB[2] += m_rbAdaptB[-2];
		m_aryMB[3] += m_rbAdaptB[-3];
		m_aryMB[4] += m_rbAdaptB[-4];
	}

	int nRetVal = m_Stage1FilterA.Decompress(nCurrentA);
	m_nLastValueA = nCurrentA;
	
	m_rbPredictionA.IncrementFast(); m_rbPredictionB.IncrementFast();
	m_rbAdaptA.IncrementFast(); m_rbAdaptB.IncrementFast();

	m_nCurrentIndex++;

	return nRetVal;
}
