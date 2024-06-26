/*****************************************************************************************
CAPEInfo:
    -a class to make working with APE files and getting information about them simple
*****************************************************************************************/
#include "All.h"
#include "APEInfo.h"
#include IO_HEADER_FILE
#include "APECompress.h"
#include "APEHeader.h"
#include "GlobalFunctions.h"

namespace APE
{

/*****************************************************************************************
Construction
*****************************************************************************************/
CAPEInfo::CAPEInfo(int * pErrorCode, const wchar_t * pFilename, CAPETag * pTag) 
{
    *pErrorCode = ERROR_SUCCESS;
    CloseFile();
    
    // open the file
    m_spIO.Assign(new IO_CLASS_NAME);
    
    if (m_spIO->Open(pFilename) != 0)
    {
        CloseFile();
        *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }
    
    // get the file information
    if (GetFileInformation(true) != 0)
    {
        CloseFile();
        *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    // get the tag (do this second so that we don't do it on failure)
    if (pTag == NULL)
    {
        // we don't want to analyze right away for non-local files
        // since a single I/O object is shared, we can't tag and read at the same time (i.e. in multiple threads)
        bool bAnalyzeNow = true;
        if (StringIsEqual(pFilename, L"http://", false, 7) || StringIsEqual(pFilename, L"m01p://", false, 7))
            bAnalyzeNow = false;

        m_spAPETag.Assign(new CAPETag(m_spIO, bAnalyzeNow));
    }
    else
    {
        m_spAPETag.Assign(pTag);
    }

    // update
    CheckHeaderInformation();
}

CAPEInfo::CAPEInfo(int * pErrorCode, CIO * pIO, CAPETag * pTag)
{
    *pErrorCode = ERROR_SUCCESS;
    CloseFile();

    m_spIO.Assign(pIO, false, false);

    // get the file information
    if (GetFileInformation(true) != 0)
    {
        CloseFile();
        *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    // get the tag (do this second so that we don't do it on failure)
    if (pTag == NULL)
        m_spAPETag.Assign(new CAPETag(m_spIO, true));
    else
        m_spAPETag.Assign(pTag);

    // update
    CheckHeaderInformation();
}

/*****************************************************************************************
Destruction
*****************************************************************************************/
CAPEInfo::~CAPEInfo() 
{
    CloseFile();
}

/*****************************************************************************************
Close the file
*****************************************************************************************/
int CAPEInfo::CloseFile() 
{
    m_spIO.Delete();
    m_APEFileInfo.spWaveHeaderData.Delete();
    m_APEFileInfo.spSeekBitTable.Delete();
    m_APEFileInfo.spSeekByteTable.Delete();
    m_APEFileInfo.spAPEDescriptor.Delete();
    
    m_spAPETag.Delete();
    
    // re-initialize variables
    m_APEFileInfo.nSeekTableElements = 0;
    m_bHasFileInformationLoaded = false;

    return ERROR_SUCCESS;
}

/*****************************************************************************************
Performs sanity checks on all of the header data.
*****************************************************************************************/
int CAPEInfo::CheckHeaderInformation()
{
    // Fixes a bug with MAC 3.99 where conversion from APE to APE could include the file tag
    // as part of the WAV terminating data.  This sanity check fixes the problem.
    if ((m_APEFileInfo.spAPEDescriptor != NULL) &&
        (m_APEFileInfo.spAPEDescriptor->nTerminatingDataBytes > 0))
    {
        int nFileBytes = m_spIO->GetSize();
        if (nFileBytes > 0)
        {
            nFileBytes -= m_spAPETag->GetTagBytes();
            nFileBytes -= m_APEFileInfo.spAPEDescriptor->nDescriptorBytes;
            nFileBytes -= m_APEFileInfo.spAPEDescriptor->nHeaderBytes;
            nFileBytes -= m_APEFileInfo.spAPEDescriptor->nSeekTableBytes;
            nFileBytes -= m_APEFileInfo.spAPEDescriptor->nHeaderDataBytes;
            nFileBytes -= m_APEFileInfo.spAPEDescriptor->nAPEFrameDataBytes;
            if (nFileBytes < m_APEFileInfo.nWAVTerminatingBytes)
            {
                m_APEFileInfo.nMD5Invalid = true;
                m_APEFileInfo.nWAVTerminatingBytes = nFileBytes;
                m_APEFileInfo.spAPEDescriptor->nTerminatingDataBytes = nFileBytes;
            }
        }
    }

    return ERROR_SUCCESS;
}

/*****************************************************************************************
Get the file information about the file
*****************************************************************************************/
int CAPEInfo::GetFileInformation(bool bGetTagInformation) 
{
    // quit if there is no simple file
    if (m_spIO == NULL) { return -1; }

    // quit if the file information has already been loaded
    if (m_bHasFileInformationLoaded) { return ERROR_SUCCESS; }

    // use a CAPEHeader class to help us analyze the file
    CAPEHeader APEHeader(m_spIO);
    int nResult = APEHeader.Analyze(&m_APEFileInfo);

    // update our internal state
    if (nResult == ERROR_SUCCESS)
        m_bHasFileInformationLoaded = true;

    // return
    return nResult;
}

/*****************************************************************************************
Primary query function
*****************************************************************************************/
intn CAPEInfo::GetInfo(APE_DECOMPRESS_FIELDS Field, intn nParam1, intn nParam2)
{
    intn nResult = -1;

    switch (Field)
    {
    case APE_INFO_FILE_VERSION:
        nResult = m_APEFileInfo.nVersion;
        break;
    case APE_INFO_COMPRESSION_LEVEL:
        nResult = m_APEFileInfo.nCompressionLevel;
        break;
    case APE_INFO_FORMAT_FLAGS:
        nResult = m_APEFileInfo.nFormatFlags;
        break;
    case APE_INFO_SAMPLE_RATE:
        nResult = m_APEFileInfo.nSampleRate;
        break;
    case APE_INFO_BITS_PER_SAMPLE:
        nResult = m_APEFileInfo.nBitsPerSample;
        break;
    case APE_INFO_BYTES_PER_SAMPLE:
        nResult = m_APEFileInfo.nBytesPerSample;
        break;
    case APE_INFO_CHANNELS:
        nResult = m_APEFileInfo.nChannels;
        break;
    case APE_INFO_BLOCK_ALIGN:
        nResult = m_APEFileInfo.nBlockAlign;
        break;
    case APE_INFO_BLOCKS_PER_FRAME:
        nResult = m_APEFileInfo.nBlocksPerFrame;
        break;
    case APE_INFO_FINAL_FRAME_BLOCKS:
        nResult = m_APEFileInfo.nFinalFrameBlocks;
        break;
    case APE_INFO_TOTAL_FRAMES:
        nResult = m_APEFileInfo.nTotalFrames;
        break;
    case APE_INFO_WAV_HEADER_BYTES:
        nResult = m_APEFileInfo.nWAVHeaderBytes;
        break;
    case APE_INFO_WAV_TERMINATING_BYTES:
        nResult = m_APEFileInfo.nWAVTerminatingBytes;
        break;
    case APE_INFO_WAV_DATA_BYTES:
        nResult = m_APEFileInfo.nWAVDataBytes;
        break;
    case APE_INFO_WAV_TOTAL_BYTES:
        nResult = m_APEFileInfo.nWAVTotalBytes;
        break;
    case APE_INFO_APE_TOTAL_BYTES:
        nResult = m_APEFileInfo.nAPETotalBytes;
        break;
    case APE_INFO_TOTAL_BLOCKS:
        nResult = m_APEFileInfo.nTotalBlocks;
        break;
    case APE_INFO_LENGTH_MS:
        nResult = m_APEFileInfo.nLengthMS;
        break;
    case APE_INFO_AVERAGE_BITRATE:
        nResult = m_APEFileInfo.nAverageBitrate;
        break;
    case APE_INFO_FRAME_BITRATE:
    {
        int nFrame = nParam1;
        
        nResult = 0;

        int nFrameBytes = GetInfo(APE_INFO_FRAME_BYTES, nFrame);
        int nFrameBlocks = GetInfo(APE_INFO_FRAME_BLOCKS, nFrame);
        if ((nFrameBytes > 0) && (nFrameBlocks > 0) && m_APEFileInfo.nSampleRate > 0)
        {
            int nFrameMS = (nFrameBlocks * 1000) / m_APEFileInfo.nSampleRate;
            if (nFrameMS != 0)
            {
                nResult = (nFrameBytes * 8) / nFrameMS;
            }
        }
        break;
    }
    case APE_INFO_DECOMPRESSED_BITRATE:
        nResult = m_APEFileInfo.nDecompressedBitrate;
        break;
    case APE_INFO_PEAK_LEVEL:
        nResult = -1; // no longer supported
        break;
    case APE_INFO_SEEK_BIT:
    {
        int nFrame = nParam1;
        if (GET_FRAMES_START_ON_BYTES_BOUNDARIES(this)) 
        {
            nResult = 0;
        }
        else 
        {
            if (nFrame < 0 || nFrame >= m_APEFileInfo.nTotalFrames)
                nResult = 0;
            else
                nResult = m_APEFileInfo.spSeekBitTable[nFrame];
        }
        break;
    }
    case APE_INFO_SEEK_BYTE:
    {
        int nFrame = nParam1;
        if (nFrame < 0 || nFrame >= m_APEFileInfo.nTotalFrames)
            nResult = 0;
        else
            nResult = m_APEFileInfo.spSeekByteTable[nFrame] + m_APEFileInfo.nJunkHeaderBytes;
        break;
    }
    case APE_INFO_WAV_HEADER_DATA:
    {
        char * pBuffer = (char *) nParam1;
        int nMaxBytes = nParam2;
        
        if (m_APEFileInfo.nFormatFlags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER)
        {
            if (sizeof(WAVE_HEADER) > nMaxBytes)
            {
                nResult = -1;
            }
            else
            {
                WAVEFORMATEX wfeFormat; GetInfo(APE_INFO_WAVEFORMATEX, (intn) &wfeFormat, 0);
                WAVE_HEADER WAVHeader; FillWaveHeader(&WAVHeader, m_APEFileInfo.nWAVDataBytes, &wfeFormat,
                    m_APEFileInfo.nWAVTerminatingBytes);
                memcpy(pBuffer, &WAVHeader, sizeof(WAVE_HEADER));
                nResult = 0;
            }
        }
        else
        {
            if (m_APEFileInfo.nWAVHeaderBytes > nMaxBytes)
            {
                nResult = -1;
            }
            else
            {
                memcpy(pBuffer, m_APEFileInfo.spWaveHeaderData, m_APEFileInfo.nWAVHeaderBytes);
                nResult = 0;
            }
        }
        break;
    }
    case APE_INFO_WAV_TERMINATING_DATA:
    {
        char * pBuffer = (char *) nParam1;
        int nMaxBytes = nParam2;

        if (m_APEFileInfo.nWAVTerminatingBytes > nMaxBytes)
        {
            nResult = -1;
        }
        else
        {
            if (m_APEFileInfo.nWAVTerminatingBytes > 0) 
            {
                // variables
                int nOriginalFileLocation = m_spIO->GetPosition();
                unsigned int nBytesRead = 0;

                // check for a tag
                m_spIO->Seek(-(m_spAPETag->GetTagBytes() + m_APEFileInfo.nWAVTerminatingBytes), FILE_END);
                m_spIO->Read(pBuffer, m_APEFileInfo.nWAVTerminatingBytes, &nBytesRead);

                // restore the file pointer
                m_spIO->Seek(nOriginalFileLocation, FILE_BEGIN);
            }
            nResult = 0;
        }
        break;
    }
    case APE_INFO_WAVEFORMATEX:
    {
        WAVEFORMATEX * pWaveFormatEx = (WAVEFORMATEX *) nParam1;
        FillWaveFormatEx(pWaveFormatEx, m_APEFileInfo.nSampleRate, m_APEFileInfo.nBitsPerSample, m_APEFileInfo.nChannels);
        nResult = 0;
        break;
    }
    case APE_INFO_IO_SOURCE:
        nResult = (intn) m_spIO.GetPtr();
        break;
    case APE_INFO_FRAME_BYTES:
    {
        int nFrame = nParam1;
        
        // bound-check the frame index
        if ((nFrame < 0) || (nFrame >= m_APEFileInfo.nTotalFrames)) 
        {
            nResult = -1;
        }
        else
        {
            if (nFrame != (m_APEFileInfo.nTotalFrames - 1)) 
                nResult = GetInfo(APE_INFO_SEEK_BYTE, nFrame + 1) - GetInfo(APE_INFO_SEEK_BYTE, nFrame);
            else 
                nResult = m_spIO->GetSize() - m_spAPETag->GetTagBytes() - m_APEFileInfo.nWAVTerminatingBytes - GetInfo(APE_INFO_SEEK_BYTE, nFrame);
        }
        break;
    }
    case APE_INFO_FRAME_BLOCKS:
    {
        int nFrame = nParam1;

        // bound-check the frame index
        if ((nFrame < 0) || (nFrame >= m_APEFileInfo.nTotalFrames)) 
        {
            nResult = -1;
        }
        else
        {
            if (nFrame != (m_APEFileInfo.nTotalFrames - 1)) 
                nResult = m_APEFileInfo.nBlocksPerFrame;
            else 
                nResult = m_APEFileInfo.nFinalFrameBlocks;
        }
        break;
    }
    case APE_INFO_TAG:
        nResult = (intn) m_spAPETag.GetPtr();
        break;
    case APE_INTERNAL_INFO:
        nResult = (intn) &m_APEFileInfo;
        break;
    }

    return nResult;
}

}