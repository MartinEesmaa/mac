#pragma once

#include "IO.h"
#include "APEInfo.h"

class CAPELink
{
public:
	CAPELink(const char * pFilename);
	~CAPELink();

public:

	int m_nStartBlock;
	int m_nFinishBlock;
	char m_cImageFile[MAX_PATH];
};
