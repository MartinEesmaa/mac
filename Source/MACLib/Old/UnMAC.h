/*****************************************************************************************
UnMAC.h
Copyright (C) 2000-2001 by Matthew T. Ashland   All Rights Reserved.

Methods for decompressing or verifying APE files

Notes:
	-none
*****************************************************************************************/

#pragma once

#include "../BitArray.h"
#include "../UnBitArrayBase.h"

class CAntiPredictor;
class CPrepare;
class CAPEDecompressCore;
class CPredictorBase;
class IPredictorDecompress;
class IAPEDecompress;

/*****************************************************************************************
CUnMAC class... a class that allows decoding on a frame-by-frame basis
*****************************************************************************************/
class CUnMAC 
{
public:
	//construction/destruction
	CUnMAC();
	~CUnMAC();

	//functions
	int Initialize(IAPEDecompress *pAPEDecompress);
	int Uninitialize();
	int DecompressFrame(unsigned char *pOutputData, __int32 FrameIndex, int CPULoadBalancingFactor = 0);

	int SeekToFrame(int FrameIndex);
	
private:

	//data members
	BOOL m_bInitialized;
	int m_LastDecodedFrameIndex;
	IAPEDecompress *m_pAPEDecompress;
	CPrepare *m_pPrepare;

	CAPEDecompressCore *m_pAPEDecompressCore;

	//functions
	void GenerateDecodedArrays(int nBlocks, int nSpecialCodes, int nFrameIndex, int nCPULoadBalancingFactor);
	void GenerateDecodedArray(int *Input_Array, unsigned __int32 Number_of_Elements, int Frame_Index, CAntiPredictor *pAntiPredictor, int CPULoadBalancingFactor = 0);

	int CreateAntiPredictors(int nCompressionLevel, int nVersion);

	int DecompressFrameOld(unsigned char *pOutputData, __int32 FrameIndex, int CPULoadBalancingFactor);
	unsigned __int32 CalculateOldChecksum(int *pDataX, int *pDataY, int nChannels, int nBlocks);

public:
	
	int m_nBlocksProcessed;
	unsigned int m_nCRC;
	unsigned int m_nStoredCRC;
	WAVEFORMATEX m_wfeInput;
};

class CAPEDecompressCore
{
public:
	CAPEDecompressCore(CIO *pIO, IAPEDecompress *pAPEDecompress);
	~CAPEDecompressCore();
	
	void GenerateDecodedArrays(int nBlocks, int nSpecialCodes, int nFrameIndex, int nCPULoadBalancingFactor);
	void GenerateDecodedArray(int *Input_Array, unsigned __int32 Number_of_Elements, int Frame_Index, CAntiPredictor *pAntiPredictor, int CPULoadBalancingFactor = 0);
	
	int * GetDataX() { return m_pDataX; }
	int * GetDataY() { return m_pDataY; }
	
	CUnBitArrayBase * GetUnBitArrray() { return m_pUnBitArray; }
	
// private:
public:
	
	
	int				*m_pTempData;
	int				*m_pDataX;
	int				*m_pDataY;
	
	CAntiPredictor		*m_pAntiPredictorX;
	CAntiPredictor		*m_pAntiPredictorY;
	
	CUnBitArrayBase		*m_pUnBitArray;
	BIT_ARRAY_STATE		m_BitArrayStateX;
	BIT_ARRAY_STATE		m_BitArrayStateY;
	
	IAPEDecompress		*m_pAPEDecompress;
	
	BOOL				m_bMMXAvailable;

	int				m_nBlocksProcessed;
};
