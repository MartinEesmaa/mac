#ifndef _macdll_h_
#define _macdll_h_

//necessary includes
#include "All.h"
#include "APEInfo.h"
#include "MAC.h"
#include "UnMAC.h"

//class creation functions
__declspec( dllexport ) CAPEInfo * __stdcall CreateCAPEInfo();
__declspec( dllexport ) CUnMAC * __stdcall CreateCUnMAC();
__declspec( dllexport ) CUnMACDecoder * __stdcall CreateCUnMACDecoder();
__declspec( dllexport ) CMAC * __stdcall CreateCMAC();
__declspec( dllexport ) CMACEncoder * __stdcall CreateCMACEncoder();

#endif //_macdll_h_