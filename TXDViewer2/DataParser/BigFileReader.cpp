#include "stdafx.h"
#include "BigFileReader.h"

int BigFileReader::Open(UINT mode)
{
	if (m_bOpened)
		return VIEWER_OK;

	m_hFile = CreateFile(
		m_strFileName,										// name of the read
		GENERIC_READ,                                       // open for reading
		FILE_SHARE_READ,                                    // share read
		NULL,                                               // default security
		OPEN_EXISTING,                                      // open exist file only
		FILE_ATTRIBUTE_NORMAL | mode,						// normal file
		NULL);                                              // no attr. template

	if (m_hFile == INVALID_HANDLE_VALUE) 
	{
		return VIEWER_OPEN_FILE_FAILED;
	}

	m_bOpened = true;
	m_pBuffer = (char*)malloc(m_nBufSize);

	return VIEWER_OK;
}


int BigFileReader::Open(LPCTSTR lpstrFileName, UINT mode)
{
	BOOL bResult;

	m_hFile = CreateFile(
	lpstrFileName,											// name of the read
		GENERIC_READ,                                       // open for reading
		FILE_SHARE_READ,                                    // share read
		NULL,                                               // default security
		OPEN_EXISTING,                                      // open exist file only
		FILE_ATTRIBUTE_NORMAL | mode,						// normal file, sequential scan
		NULL);                                              // no attr. template

	if (m_hFile == INVALID_HANDLE_VALUE) 
	{
		return VIEWER_OPEN_FILE_FAILED;
	}

	LARGE_INTEGER lFileSize = {0};
	if (GetFileSizeEx(m_hFile, &lFileSize))
	{
		m_nFileSize = lFileSize.QuadPart;
	}
	else
	{
		m_nFileSize = 1024*1024*1024;   // 1GB
	}
	m_pBuffer = (char*)malloc(m_nBufSize);
	m_pStart = m_pBuffer;
	bResult = ReadFile(m_hFile, m_pBuffer, m_nBufSize, &m_dwBytesRead, NULL);
	if (!bResult)
	{
		free(m_pBuffer);
		m_pBuffer = NULL;
		return VIEWER_READ_FILE_FAILED;
	}

	return VIEWER_OK;
}
