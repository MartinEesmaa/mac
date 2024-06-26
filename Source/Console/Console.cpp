/***************************************************************************************
MAC Console Frontend (MAC.exe)

Pretty simple and straightforward console front end.  If somebody ever wants to add 
more functionality like tagging, auto-verify, etc., that'd be excellent.

Copyrighted (c) 2000 - 2017 Matthew T. Ashland.  All Rights Reserved.
***************************************************************************************/
#include "All.h"
#include <stdio.h>
#include "GlobalFunctions.h"
#include "MACLib.h"
#include "CharacterHelper.h"
using namespace APE;

// defines
#define COMPRESS_MODE		0
#define DECOMPRESS_MODE		1
#define VERIFY_MODE			2
#define CONVERT_MODE		3
#define UNDEFINED_MODE		-1

// global variables
TICK_COUNT_TYPE g_nInitialTickCount = 0;

/***************************************************************************************
Displays the proper usage for MAC.exe
***************************************************************************************/
void DisplayProperUsage(FILE * pFile) 
{
	_ftprintf(pFile, _T("Proper Usage: [EXE] [Input File] [Output File] [Mode]\n\n"));

	_ftprintf(pFile, _T("Modes: \n"));
	_ftprintf(pFile, _T("    Compress (fast): '-c1000'\n"));
	_ftprintf(pFile, _T("    Compress (normal): '-c2000'\n"));
	_ftprintf(pFile, _T("    Compress (high): '-c3000'\n"));
	_ftprintf(pFile, _T("    Compress (extra high): '-c4000'\n"));
	_ftprintf(pFile, _T("    Compress (insane): '-c5000'\n"));
	_ftprintf(pFile, _T("    Decompress: '-d'\n"));
	_ftprintf(pFile, _T("    Verify: '-v'\n"));
	_ftprintf(pFile, _T("    Convert: '-nXXXX'\n\n"));

	_ftprintf(pFile, _T("Examples:\n"));
	_ftprintf(pFile, _T("    Compress: mac.exe \"Metallica - One.wav\" \"Metallica - One.ape\" -c2000\n"));
	_ftprintf(pFile, _T("    Decompress: mac.exe \"Metallica - One.ape\" \"Metallica - One.wav\" -d\n"));
	_ftprintf(pFile, _T("    Verify: mac.exe \"Metallica - One.ape\" -v\n"));
	_ftprintf(pFile, _T("    (note: int filenames must be put inside of quotations)\n"));
}

/***************************************************************************************
Progress callback
***************************************************************************************/
void CALLBACK ProgressCallback(int nPercentageDone)
{
    // get the current tick count
	TICK_COUNT_TYPE  nTickCount;
	TICK_COUNT_READ(nTickCount);

	// calculate the progress
	double dProgress = nPercentageDone / 1.e5;											// [0...1]
	double dElapsed = (double) (nTickCount - g_nInitialTickCount) / TICK_COUNT_FREQ;	// seconds
	double dRemaining = dElapsed * ((1.0 / dProgress) - 1.0);							// seconds

	// output the progress
	_ftprintf(stderr, _T("Progress: %.1f%% (%.1f seconds remaining, %.1f seconds total)          \r"), 
		dProgress * 100, dRemaining, dElapsed);
}

/***************************************************************************************
Main (the main function)
***************************************************************************************/
int _tmain(int argc, TCHAR * argv[])
{
	// variable declares
	CSmartPtr<wchar_t> spInputFilename; CSmartPtr<wchar_t> spOutputFilename;
	int nRetVal = ERROR_UNDEFINED;
	int nMode = UNDEFINED_MODE;
	int nCompressionLevel;
	int nPercentageDone;
		
	// output the header
	_ftprintf(stderr, CONSOLE_NAME);
	
	// make sure there are at least four arguments (could be more for EAC compatibility)
	if (argc < 3) 
	{
		DisplayProperUsage(stderr);
		exit(-1);
	}

	// store the filenames
	#ifdef _UNICODE
		spInputFilename.Assign(argv[1], TRUE, FALSE);
		spOutputFilename.Assign(argv[2], TRUE, FALSE);
	#else
		spInputFilename.Assign(CAPECharacterHelper::GetUTF16FromANSI(argv[1]), TRUE);
		spOutputFilename.Assign(CAPECharacterHelper::GetUTF16FromANSI(argv[2]), TRUE);
	#endif

	// verify that the input file exists
	if (!FileExists(spInputFilename))
	{
		_ftprintf(stderr, _T("Input File Not Found...\n\n"));
		exit(-1);
	}

	// if the output file equals '-v', then use this as the next argument
	TCHAR cMode[256];
	_tcsncpy(cMode, argv[2], 255);

	if (_tcsnicmp(cMode, _T("-v"), 2) != 0)
	{
		// verify is the only mode that doesn't use at least the third argument
		if (argc < 4) 
		{
			DisplayProperUsage(stderr);
			exit(-1);
		}

		// check for and skip if necessary the -b XXXXXX arguments (3,4)
		_tcsncpy(cMode, argv[3], 255);
	}

	// get the mode
	nMode = UNDEFINED_MODE;
	if (_tcsnicmp(cMode, _T("-c"), 2) == 0)
		nMode = COMPRESS_MODE;
	else if (_tcsnicmp(cMode, _T("-d"), 2) == 0)
		nMode = DECOMPRESS_MODE;
	else if (_tcsnicmp(cMode, _T("-v"), 2) == 0)
		nMode = VERIFY_MODE;
	else if (_tcsnicmp(cMode, _T("-n"), 2) == 0)
		nMode = CONVERT_MODE;

	// error check the mode
	if (nMode == UNDEFINED_MODE) 
	{
		DisplayProperUsage(stderr);
		exit(-1);
	}

	// get and error check the compression level
	if (nMode == COMPRESS_MODE || nMode == CONVERT_MODE) 
	{
		nCompressionLevel = _ttoi(&cMode[2]);
		if (nCompressionLevel != 1000 && nCompressionLevel != 2000 && 
			nCompressionLevel != 3000 && nCompressionLevel != 4000 &&
			nCompressionLevel != 5000) 
		{
			DisplayProperUsage(stderr);
			return -1;
		}
	}

	// set the initial tick count
	TICK_COUNT_READ(g_nInitialTickCount);
	
	// process
	int nKillFlag = 0;
	if (nMode == COMPRESS_MODE) 
	{
		TCHAR cCompressionLevel[16];
		if (nCompressionLevel == 1000) { _tcscpy(cCompressionLevel, _T("fast")); }
		if (nCompressionLevel == 2000) { _tcscpy(cCompressionLevel, _T("normal")); }
		if (nCompressionLevel == 3000) { _tcscpy(cCompressionLevel, _T("high")); }
		if (nCompressionLevel == 4000) { _tcscpy(cCompressionLevel, _T("extra high")); }
		if (nCompressionLevel == 5000) { _tcscpy(cCompressionLevel, _T("insane")); }

		_ftprintf(stderr, _T("Compressing (%s)...\n"), cCompressionLevel);
		nRetVal = CompressFileW(spInputFilename, spOutputFilename, nCompressionLevel, &nPercentageDone, ProgressCallback, &nKillFlag);
	}
	else if (nMode == DECOMPRESS_MODE) 
	{
		_ftprintf(stderr, _T("Decompressing...\n"));
		nRetVal = DecompressFileW(spInputFilename, spOutputFilename, &nPercentageDone, ProgressCallback, &nKillFlag);
	}	
	else if (nMode == VERIFY_MODE) 
	{
		_ftprintf(stderr, _T("Verifying...\n"));
		nRetVal = VerifyFileW(spInputFilename, &nPercentageDone, ProgressCallback, &nKillFlag);
	}	
	else if (nMode == CONVERT_MODE) 
	{
		_ftprintf(stderr, _T("Converting...\n"));
		nRetVal = ConvertFileW(spInputFilename, spOutputFilename, nCompressionLevel, &nPercentageDone, ProgressCallback, &nKillFlag);
	}

	if (nRetVal == ERROR_SUCCESS) 
		_ftprintf(stderr, _T("\nSuccess...\n"));
	else 
		_ftprintf(stderr, _T("\nError: %i\n"), nRetVal);

	return nRetVal;
}
