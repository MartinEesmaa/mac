#include "All.h"

#ifdef IO_USE_STD_LIB_FILE_IO

#include "StdLibFileIO.h"

CStdLibFileIO::CStdLibFileIO()
{
	memset(m_cFileName, 0, MAX_PATH);
	m_bReadOnly = FALSE;
	m_pFile = NULL;
}

CStdLibFileIO::~CStdLibFileIO()
{
	Close();
}

int CStdLibFileIO::Open(const char * pName)
{
	Close();

	m_bReadOnly = FALSE;
	m_pFile = fopen(pName, "r+b");
	if (!m_pFile)
		return -1;

	m_bReadOnly = FALSE;
	strcpy(m_cFileName, pName);

	return 0;
}

int CStdLibFileIO::Close()
{
	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}

	return 0;
}
	
int CStdLibFileIO::Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead)
{
	*pBytesRead = fread(pBuffer, 1, nBytesToRead, m_pFile);
	return ferror(m_pFile) ? ERROR_IO_READ : 0;
}

int CStdLibFileIO::Write(const void * pBuffer, unsigned  int nBytesToWrite, unsigned int * pBytesWritten)
{
	*pBytesWritten = fwrite(pBuffer, 1, nBytesToWrite, m_pFile);
	
	return (ferror(m_pFile) || (*pBytesWritten != nBytesToWrite)) ? ERROR_IO_WRITE : 0;
}
	
int CStdLibFileIO::Seek(int nDistance, unsigned int nMoveMode)
{
	return fseek(m_pFile, nDistance, nMoveMode);
}
	
int CStdLibFileIO::SetEOF()
{
	return -1;
}

int CStdLibFileIO::GetPosition()
{
	fpos_t fPosition; memset(&fPosition, 0, sizeof(fPosition));
	fgetpos(m_pFile, &fPosition);
	return _FPOSOFF(fPosition);
}

int CStdLibFileIO::GetSize()
{
	int nCurrentPosition = GetPosition();
	Seek(0, FILE_END);
	int nLength = GetPosition();
	Seek(nCurrentPosition, FILE_BEGIN);
	return nLength;
}

int CStdLibFileIO::GetName(char * pBuffer)
{
	strcpy(pBuffer, m_cFileName);
	return 0;
}

int CStdLibFileIO::Create(const char * pName)
{
	Close();

	m_pFile = fopen(pName, "w+b");
	if (!m_pFile)
		return -1;

	m_bReadOnly = FALSE;
	strcpy(m_cFileName, pName);

	return 0;
}

int CStdLibFileIO::Delete()
{
//	Close();
//	DeleteFile(m_cFileName);
//	return 0;

	return -1;
}

#endif // #ifdef IO_USE_STD_LIB_FILE_IO

