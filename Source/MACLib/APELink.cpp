#include "All.h"
#include "APELink.h"
#include "CharacterHelper.h"
#include IO_HEADER_FILE

namespace APE
{

#define APE_LINK_HEADER                 "[Monkey's Audio Image Link File]"
#define APE_LINK_IMAGE_FILE_TAG         "Image File="
#define APE_LINK_START_BLOCK_TAG        "Start Block="
#define APE_LINK_FINISH_BLOCK_TAG       "Finish Block="

CAPELink::CAPELink(const str_utfn * pFilename)
{
    // empty
    m_bIsLinkFile = false;
    m_nStartBlock = 0;
    m_nFinishBlock = 0;
    m_cImageFilename[0] = 0;

    // open the file
    IO_CLASS_NAME ioLinkFile;
    if (ioLinkFile.Open(pFilename) == ERROR_SUCCESS)
    {
        // create a buffer
        CSmartPtr<char> spBuffer(new char [1024], true);
        
        // fill the buffer from the file and null terminate it
        unsigned int nBytesRead = 0;
        ioLinkFile.Read(spBuffer.GetPtr(), 1023, &nBytesRead);
        spBuffer[nBytesRead] = 0;

        // call the other constructor (uses a buffer instead of opening the file)
        ParseData(spBuffer, pFilename);
    }
}

CAPELink::CAPELink(const char * pData, const str_utfn * pFilename)
{
    ParseData(pData, pFilename);
}

CAPELink::~CAPELink()
{
}

void CAPELink::ParseData(const char * pData, const str_utfn * pFilename)
{
    // empty
    m_bIsLinkFile = false;
    m_nStartBlock = 0;
    m_nFinishBlock = 0;
    m_cImageFilename[0] = 0;

    if (pData != NULL)
    {
        // parse out the information
        const char * pHeader = (const char *) strstr(pData, APE_LINK_HEADER);
        const char * pImageFile = (const char *) strstr(pData, APE_LINK_IMAGE_FILE_TAG);
        const char * pStartBlock = (const char *) strstr(pData, APE_LINK_START_BLOCK_TAG);
        const char * pFinishBlock = (const char *) strstr(pData, APE_LINK_FINISH_BLOCK_TAG);

        if (pHeader && pImageFile && pStartBlock && pFinishBlock)
        {
            if ((_strnicmp(pHeader, APE_LINK_HEADER, strlen(APE_LINK_HEADER)) == 0) &&
                (_strnicmp(pImageFile, APE_LINK_IMAGE_FILE_TAG, strlen(APE_LINK_IMAGE_FILE_TAG)) == 0) &&
                (_strnicmp(pStartBlock, APE_LINK_START_BLOCK_TAG, strlen(APE_LINK_START_BLOCK_TAG)) == 0) &&
                (_strnicmp(pFinishBlock, APE_LINK_FINISH_BLOCK_TAG, strlen(APE_LINK_FINISH_BLOCK_TAG)) == 0))
            {
                // get the start and finish blocks
                m_nStartBlock = atoi(&pStartBlock[strlen(APE_LINK_START_BLOCK_TAG)]);
                m_nFinishBlock = atoi(&pFinishBlock[strlen(APE_LINK_FINISH_BLOCK_TAG)]);
                
                // get the path
                char cImageFile[MAX_PATH + 1]; int nIndex = 0;
                const char * pImageCharacter = &pImageFile[strlen(APE_LINK_IMAGE_FILE_TAG)];
                while ((*pImageCharacter != 0) && (*pImageCharacter != '\r') && (*pImageCharacter != '\n'))
                    cImageFile[nIndex++] = *pImageCharacter++;
                cImageFile[nIndex] = 0;

                CSmartPtr<str_utfn> spImageFileUTF16(CAPECharacterHelper::GetUTF16FromUTF8((const str_utf8 *) cImageFile), true);

                // process the path
                if ((wcsrchr(spImageFileUTF16, '\\') == NULL) && (wcsrchr(pFilename, '\\') != NULL))
                {
                    str_utfn cImagePath[MAX_PATH + 1];
                    wcscpy(cImagePath, pFilename);
                    wcscpy(wcsrchr(cImagePath, '\\') + 1, spImageFileUTF16);
                    wcscpy(m_cImageFilename, cImagePath);
                }
                else
                {
                    wcscpy(m_cImageFilename, spImageFileUTF16);
                }

                // this is a valid link file
                m_bIsLinkFile = true;
            }
        }
    }
}

int CAPELink::GetStartBlock()
{
    return m_nStartBlock;
}

int CAPELink::GetFinishBlock()
{
    return m_nFinishBlock;
}

const str_utfn * CAPELink::GetImageFilename()
{
    return m_cImageFilename;
}

bool CAPELink::GetIsLinkFile()
{
    return m_bIsLinkFile;
}

}