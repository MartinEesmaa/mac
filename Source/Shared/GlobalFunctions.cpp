#include "All.h"
#include "GlobalFunctions.h"
#include "IO.h"

BOOL GetMMXAvailable() 
{
#ifdef ENABLE_ASSEMBLY
	unsigned long nRegisterEDX;

	try
	{
		__asm mov eax, 1
		__asm CPUID
		__asm mov nRegisterEDX, edx
   	}
	catch(...)
	{
		return FALSE;
	}

	if (nRegisterEDX & 0x800000) 
		RETURN_ON_EXCEPTION(__asm emms, FALSE)
	else
		return FALSE;

	return TRUE;
#else
	return FALSE;
#endif
}

int ReadSafe(CIO * pIO, void * pBuffer, int nBytes)
{
	unsigned int nBytesRead = 0;
	int nRetVal = pIO->Read(pBuffer, nBytes, &nBytesRead);
	if (nRetVal == ERROR_SUCCESS)
	{
		if (nBytes != int(nBytesRead))
			nRetVal = ERROR_IO_READ;
	}

	return nRetVal;
}

int WriteSafe(CIO * pIO, void * pBuffer, int nBytes)
{
	unsigned int nBytesWritten = 0;
	int nRetVal = pIO->Write(pBuffer, nBytes, &nBytesWritten);
	if (nRetVal == ERROR_SUCCESS)
	{
		if (nBytes != int(nBytesWritten))
			nRetVal = ERROR_IO_WRITE;
	}

	return nRetVal;
}

BOOL FileExists(const char * pFilename)
{
#ifdef _WINDOWS_
	BOOL bFound = FALSE;

	WIN32_FIND_DATA WFD;
	HANDLE hFind = FindFirstFile(pFilename, &WFD);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		bFound = TRUE;
		CloseHandle(hFind);
	}

	return bFound;
#else
	return TRUE;
#endif
}