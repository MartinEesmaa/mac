#include "MACDll.h"
#include "WinFileIO.h"

#include "resource.h"
#include "APEInfoDialog.h"
#include "WAVInfoDialog.h"

#include "APEDecompress.h"

#include "APECompressCreate.h"
#include "APECompressCore.h"

#include "APECompress.h"

#include "APEInfo.h"
#include "APETag.h"

int __stdcall GetVersionNumber()
{
	return MAC_VERSION_NUMBER;
}

int __stdcall GetInterfaceCompatibility(int nVersion, BOOL bDisplayWarningsOnFailure, HWND hwndParent)
{
	int nRetVal = 0;
	if (nVersion > MAC_VERSION_NUMBER)
	{
		nRetVal = -1;
		if (bDisplayWarningsOnFailure)
		{
			char cMessage[1024];
			sprintf(cMessage, "You system does not have a new enough version of Monkey's Audio installed.\n"
				"Please visit www.monkeysaudio.com for the latest version.\n\n(version %.2f or later required)",
				float(nVersion) / float(1000));
			MessageBox(hwndParent, cMessage, "Please Update Monkey's Audio", MB_OK | MB_ICONINFORMATION);
		}
	}
	else if (nVersion < 3940)
	{
		nRetVal = -1;
		if (bDisplayWarningsOnFailure)
		{
			char cMessage[1024];
			sprintf(cMessage, "This program is trying to use an old version of Monkey's Audio.\n"
				"Please contact the author about updating their support for Monkey's Audio.\n\n"
				"Monkey's Audio currently installed: %.2f\nProgram is searching for: %.2f",
				float(MAC_VERSION_NUMBER) / float(1000), float(nVersion) / float(1000));
			MessageBox(hwndParent, cMessage, "Program Requires Updating", MB_OK | MB_ICONINFORMATION);
		}
	}

	return nRetVal;
}

int __stdcall ShowFileInfoDialog(const char * pFilename, HWND HWndWindow)
{
	//make sure the file exists....
	WIN32_FIND_DATA FD;
	HANDLE h = INVALID_HANDLE_VALUE;
	h = FindFirstFile(pFilename, &FD);
	if (h == INVALID_HANDLE_VALUE) {
		MessageBox(HWndWindow, "File not found...","File Info",MB_OK);
		return 0;
	}
	else 
	{
		FindClose(h);
	}
	
	//see what type the file is
	if ((_stricmp(&pFilename[strlen(pFilename) - 4],".ape") == 0) ||
		(_stricmp(&pFilename[strlen(pFilename) - 4],".apl") == 0)) 
	{
		CAPEInfoDialog APEInfoDialog;
		APEInfoDialog.ShowAPEInfoDialog(pFilename, GetModuleHandle("MACDll.dll"), (LPCSTR) IDD_APE_INFO, HWndWindow);
		return 0;
	}
	else if (_stricmp(&pFilename[strlen(pFilename) - 4],".wav") == 0) 
	{
		CWAVInfoDialog WAVInfoDialog;
		WAVInfoDialog.ShowWAVInfoDialog(pFilename, GetModuleHandle("MACDll.dll"), (LPCSTR) IDD_WAV_INFO, HWndWindow);
		return 0;
	}
	else 
	{
		MessageBox(HWndWindow, "File type not supported... (only .ape, .apl, and .wav files currently supported)", "File Info: Unsupported File Type", MB_OK);
		return 0;
	};
}

int __stdcall TagFileSimple(const char * pFilename, const char * pArtist, const char * pAlbum, const char * pTitle, const char * pComment, const char * pGenre, const char * pYear, const char * pTrack, BOOL bClearFirst, BOOL bUseOldID3)
{
	IO_CLASS_NAME FileIO;
	if (FileIO.Open(pFilename) != 0)
		return -1;
	
	CAPETag APETag(&FileIO, TRUE);

	if (bClearFirst)
		APETag.ClearFields();	
	
	APETag.SetField("Artist", pArtist);
	APETag.SetField("Album", pAlbum);
	APETag.SetField("Title", pTitle);
	APETag.SetField("Genre", pGenre);
	APETag.SetField("Year", pYear);
	APETag.SetField("Comment", pComment);
	APETag.SetField("Track", pTrack);
	
	if (APETag.Save(bUseOldID3) != 0)
	{
		return -1;
	}
	
	return 0;
}

int __stdcall GetID3Tag(const char * pFilename, ID3_TAG * pID3Tag)
{
	IO_CLASS_NAME FileIO;
	if (FileIO.Open(pFilename) != 0)
		return -1;
	
	CAPETag APETag(&FileIO, TRUE);
	
	return APETag.CreateID3Tag(pID3Tag);
}

int __stdcall RemoveTag(char * pFilename)
{
	int nErrorCode = ERROR_SUCCESS;
	CSmartPtr<IAPEDecompress> spAPEDecompress(CreateIAPEDecompress(pFilename, &nErrorCode));
	if (spAPEDecompress == NULL) return -1;
	GET_TAG(spAPEDecompress)->Remove(FALSE);
	return 0;
}

/*****************************************************************************************
CAPEDecompress wrapper(s)
*****************************************************************************************/
APE_DECOMPRESS_HANDLE __stdcall c_APEDecompress_Create(const char * pFilename, int * pErrorCode)
{
	return (APE_DECOMPRESS_HANDLE) CreateIAPEDecompress(pFilename, pErrorCode);
}

void __stdcall c_APEDecompress_Destroy(APE_DECOMPRESS_HANDLE hAPEDecompress)
{
	IAPEDecompress * pAPEDecompress = (IAPEDecompress *) hAPEDecompress;
	if (pAPEDecompress)
		delete pAPEDecompress;
}

int __stdcall c_APEDecompress_GetData(APE_DECOMPRESS_HANDLE hAPEDecompress, char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
	return ((IAPEDecompress *) hAPEDecompress)->GetData(pBuffer, nBlocks, pBlocksRetrieved);
}

int __stdcall c_APEDecompress_Seek(APE_DECOMPRESS_HANDLE hAPEDecompress, int nBlockOffset)
{
	return ((IAPEDecompress *) hAPEDecompress)->Seek(nBlockOffset);
}

int __stdcall c_APEDecompress_GetInfo(APE_DECOMPRESS_HANDLE hAPEDecompress, APE_DECOMPRESS_FIELDS Field, int nParam1, int nParam2)
{
	return ((IAPEDecompress *) hAPEDecompress)->GetInfo(Field, nParam1, nParam2);
}

/*****************************************************************************************
CAPECompress wrapper(s)
*****************************************************************************************/
APE_COMPRESS_HANDLE __stdcall c_APECompress_Create(int * pErrorCode)
{
	return (APE_COMPRESS_HANDLE) CreateIAPECompress(pErrorCode);
}

void __stdcall c_APECompress_Destroy(APE_COMPRESS_HANDLE hAPECompress)
{
	IAPECompress * pAPECompress = (IAPECompress *) hAPECompress;
	if (pAPECompress)
		delete pAPECompress;
}

int __stdcall c_APECompress_Start(APE_COMPRESS_HANDLE hAPECompress, const char * pOutputFilename, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes, int nCompressionLevel, const unsigned char * pHeaderData, int nHeaderBytes)
{
	return ((IAPECompress *) hAPECompress)->Start(pOutputFilename, pwfeInput, nMaxAudioBytes, nCompressionLevel, pHeaderData, nHeaderBytes);
}


int __stdcall c_APECompress_AddData(APE_COMPRESS_HANDLE hAPECompress, unsigned char * pData, int nBytes)
{
	return ((IAPECompress *) hAPECompress)->AddData(pData, nBytes);
}

int __stdcall c_APECompress_GetBufferBytesAvailable(APE_COMPRESS_HANDLE hAPECompress)
{
	return ((IAPECompress *) hAPECompress)->GetBufferBytesAvailable();
}

unsigned char * __stdcall c_APECompress_LockBuffer(APE_COMPRESS_HANDLE hAPECompress, int * pBytesAvailable)
{
	return ((IAPECompress *) hAPECompress)->LockBuffer(pBytesAvailable);
}

int __stdcall c_APECompress_UnlockBuffer(APE_COMPRESS_HANDLE hAPECompress, int nBytesAdded, BOOL bProcess)
{
	return ((IAPECompress *) hAPECompress)->UnlockBuffer(nBytesAdded, bProcess);
}

int __stdcall c_APECompress_Finish(APE_COMPRESS_HANDLE hAPECompress, unsigned char * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes)
{
	return ((IAPECompress *) hAPECompress)->Finish(pTerminatingData, nTerminatingBytes, nWAVTerminatingBytes);
}

int __stdcall c_APECompress_Kill(APE_COMPRESS_HANDLE hAPECompress)
{
	return ((IAPECompress *) hAPECompress)->Kill();
}
