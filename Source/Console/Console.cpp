/***************************************************************************************
MAC Console Frontend (MAC.exe)

Pretty simple and straightforward console front end.  If somebody ever wants to add 
more functionality like tagging, auto-verify, etc., that'd be excellent.

Copyrighted (c) 2000 - 2002 Matthew T. Ashland.  All Rights Reserved.
***************************************************************************************/
#include "All.h"
#include <stdio.h>
#include "GlobalFunctions.h"
#include "MACLib.h"

// defines
#define COMPRESS_MODE		0
#define DECOMPRESS_MODE		1
#define VERIFY_MODE			2
#define CONVERT_MODE		3
#define UNDEFINED_MODE		-1

// global variables
unsigned __int32 g_nInitialTickCount = 0;

/***************************************************************************************
Displays the proper usage for MAC.exe
***************************************************************************************/
void DisplayProperUsage() 
{
	printf("Proper Usage: [EXE] [Input File] [Output File] [Mode]\r\n\r\n");

	printf("Modes: \r\n");
	printf("\tCompress (fast): '-c1000'\r\n");
	printf("\tCompress (normal): '-c2000'\r\n");
	printf("\tCompress (high): '-c3000'\r\n");
	printf("\tCompress (extra high): '-c4000'\r\n");
	printf("\tDecompress: '-d'\r\n");
	printf("\tVerify: '-v'\r\n");
	printf("\tConvert: '-nXXXX'\r\n\r\n");

	printf("Examples:\r\n");
	printf("\tCompress: mac.exe \"Metallica - One.wav\" \"Metallica - One.ape\" -c2000\r\n");
	printf("\tDecompress: mac.exe \"Metallica - One.ape\" \"Metallica - One.wav\" -d\r\n");
	printf("\tVerify: mac.exe \"Metallica - One.ape\" -v\r\n");
	printf("\t(note: int filenames must be put inside of quotations)\r\n");
}

/***************************************************************************************
Progress callback
***************************************************************************************/
void CALLBACK ProgressCallback(int nPercentageDone)
{
	double dProgress = double(nPercentageDone) / 1000;
	double dElapsedMS = (TICK_COUNT - g_nInitialTickCount);

	double dSecondsRemaining = (((dElapsedMS * 100) / dProgress) - dElapsedMS) / 1000;
	printf("Progress: %.1f%% (%.1f seconds remaining)          \r", dProgress, dSecondsRemaining);	
}

/***************************************************************************************
Main (the main function)
***************************************************************************************/
int main(int argc, char *argv[])
{
	// variable declares
	char * pInputFilename, * pOutputFilename;
	int nRetVal;
	int nMode = UNDEFINED_MODE;
	int nCompressionLevel;
	int nPercentageDone;
		
	// output the header
	printf(CONSOLE_NAME);
	
	// make sure there are at least four arguments (could be more for EAC compatibility)
	if (argc < 3) 
	{
		DisplayProperUsage();
		exit(-1);
	}

	// store the input file
	pInputFilename = argv[1];
	
	// verify that the input file exists
	if (!FileExists(pInputFilename))
	{
		printf("Input File Not Found...\r\n\r\n");
		exit(-1);
	}

	pOutputFilename = argv[2];

	// if the output file equals '-v', then use this as the next argument
	char cMode[256];
	strcpy(cMode, argv[2]);

	if (_strnicmp(cMode, "-v", 2) != 0)
	{
		// verify is the only mode that doesn't use at least the third argument
		if (argc < 4) 
		{
			DisplayProperUsage();
			exit(-1);
		}

		// check for and skip if necessary the -b XXXXXX arguments (3,4)
		strcpy(cMode, argv[3]);
	}

	// get the mode
	nMode = UNDEFINED_MODE;
	if (_strnicmp(cMode, "-c", 2) == 0)
		nMode = COMPRESS_MODE;
	else if (_strnicmp(cMode, "-d", 2) == 0)
		nMode = DECOMPRESS_MODE;
	else if (_strnicmp(cMode, "-v", 2) == 0)
		nMode = VERIFY_MODE;
	else if (_strnicmp(cMode, "-n", 2) == 0)
		nMode = CONVERT_MODE;

	// error check the mode
	if (nMode == UNDEFINED_MODE) 
	{
		DisplayProperUsage();
		exit(-1);
	}

	// get and error check the compression level
	if (nMode == COMPRESS_MODE || nMode == CONVERT_MODE) 
	{
		nCompressionLevel = atol(&cMode[2]);
		if (nCompressionLevel != 1000 && nCompressionLevel != 2000 && nCompressionLevel != 3000 && nCompressionLevel != 4000) 
		{
			DisplayProperUsage();
			exit(-1);
		}
	}

	// set the initial tick count
	g_nInitialTickCount = TICK_COUNT;
	
	// process
	int nKillFlag = 0;
	if (nMode == COMPRESS_MODE) 
	{
		char cCompressionLevel[16];
		if (nCompressionLevel == 1000) { strcpy(cCompressionLevel, "fast");	}
		if (nCompressionLevel == 2000) { strcpy(cCompressionLevel, "normal");	}
		if (nCompressionLevel == 3000) { strcpy(cCompressionLevel, "high");	}
		if (nCompressionLevel == 4000) { strcpy(cCompressionLevel, "extra high");	}

		printf("Compressing (%s)...\r\n", cCompressionLevel);
		nRetVal = CompressFile(pInputFilename, pOutputFilename, nCompressionLevel, &nPercentageDone, ProgressCallback, &nKillFlag);
	}
	else if (nMode == DECOMPRESS_MODE) 
	{
		printf("Decompressing...\r\n");
		nRetVal = DecompressFile(pInputFilename, pOutputFilename, &nPercentageDone, ProgressCallback, &nKillFlag);
	}	
	else if (nMode == VERIFY_MODE) 
	{
		printf("Verifying...\r\n");
		nRetVal = VerifyFile(pInputFilename, &nPercentageDone, ProgressCallback, &nKillFlag);
	}	
	else if (nMode == CONVERT_MODE) 
	{
		printf("Converting...\r\n");
		nRetVal = ConvertFile(pInputFilename, pOutputFilename, nCompressionLevel, &nPercentageDone, ProgressCallback, &nKillFlag);
	}

	if (nRetVal == ERROR_SUCCESS) 
		printf("\r\nSuccess...\r\n");
	else 
		printf("\r\nError: %i\r\n", nRetVal);

	exit(nRetVal);

	// we don't actually get here to return anything, but main(...) needs a return value with some compilers
	return nRetVal;
}
