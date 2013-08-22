// ********************************************************************************************************
// Description: 
//     simple wrap win32 api in order to get max reading performance.
//	   
// Created 2013/1/13
//
// ********************************************************************************************************

#ifndef _BIG_FILE_READER_H_
#define _BIG_FILE_READER_H_

#include "ErrorCode.h"
#include "IDataReader.h"

#define BUFFER_SIZE 1024 * 1024 * 4
#define BUFFER_KEEP 1024 * 32

class BigFileReader : public IDataReader
{
public:
	BigFileReader() : m_hFile(NULL), m_nBufSize(BUFFER_SIZE), m_dwStart(0),
		m_dwBytesRead(0), m_nOffset(0), m_nCurPos(0), m_pBuffer(NULL), m_pStart(NULL)
	{}

	explicit BigFileReader(CString strFileName) : m_hFile(NULL), m_nBufSize(BUFFER_SIZE), m_dwStart(0),
		m_dwBytesRead(0), m_nOffset(0), m_nCurPos(0), m_pBuffer(NULL), m_pStart(NULL)
	{
		m_bInteger = true;
		m_bOpened = false;
		m_strFileName = strFileName;
	}

	~BigFileReader()
	{
		if (m_pBuffer != NULL)
			free(m_pBuffer);
		if (m_hFile != NULL)
		{
			CloseHandle(m_hFile);       // we can close a NULL handle safely.
		}
	}
	virtual int Open(UINT mode);
	virtual void Close()
	{
		if (m_pBuffer != NULL)
		{
			free(m_pBuffer);
			m_pBuffer = NULL;
		}
		if (m_hFile != NULL)
		{
			CloseHandle(m_hFile);       // we can close a NULL handle safely.	
			m_hFile = NULL;
		}
		m_bOpened = false;
	}
	int Open(LPCTSTR lpstrFileName, UINT mode);
	
	size_t GetFileSize()
	{
		return m_nFileSize;
	}
	virtual void* GetData(size_t Idx)
	{
		Open(FILE_FLAG_RANDOM_ACCESS);
		LARGE_INTEGER nIdx;
		nIdx.QuadPart = Idx;
		DWORD dwRet = SetFilePointerEx(m_hFile, nIdx, NULL, FILE_BEGIN);
		if (dwRet == 0xFFFFFFFF) // Test for failure.
		{
		   // Obtain the error code.
		   DWORD dwError = GetLastError();
		   return NULL;
		   // Resolve the failure.
		} // End of error handler.

		while (true)
		{
			char* pEnd = m_pBuffer;
			BOOL bResult = ReadFile(m_hFile, m_pBuffer, m_nBufSize, &m_dwBytesRead, NULL);
			if (!bResult)
			{
				free(m_pBuffer);
				m_pBuffer = NULL;
				return NULL;
			}

			pEnd = strchr(pEnd, '\n');
			if (pEnd != NULL)
			{
				*pEnd = '\0';
				break;
			}
			else
			{
				m_nBufSize += 1024*512;
				realloc(m_pBuffer, m_nBufSize);
			}
		}
		return m_pBuffer;
	}

	int GetLine(char*& pLine)
	{
		pLine = m_pBuffer;
		m_nOffset = m_nCurPos;
		while (true)
		{            
			if (m_dwStart == m_dwBytesRead)
			{
				DWORD nReserve = m_pBuffer - pLine;
				char cLast = *(m_pBuffer - 1);
				m_pBuffer = m_pStart;
				memcpy(m_pBuffer, pLine, nReserve);

				if ((m_nBufSize - nReserve) < BUFFER_KEEP)
				{
					m_nBufSize *= 2;
					m_pBuffer = (char*)realloc(m_pBuffer, m_nBufSize);
					m_pStart = m_pBuffer;
				}

				pLine = m_pBuffer;
				m_pBuffer += nReserve;
				BOOL bResult = ReadFile(m_hFile, m_pBuffer, m_nBufSize - nReserve, &m_dwBytesRead, NULL);
				if (!bResult)
				{
					m_pBuffer -= nReserve;
					free(m_pBuffer);
					m_pBuffer = NULL;
					return VIEWER_READ_FILE_FAILED;
				}

				m_dwStart = 0;
				if (!m_dwBytesRead)       // End of file, but the last line no '\n'
				{
					if (cLast == '\n')
					{
						return VIEWER_EOF;
					}

					if (m_dwBytesRead == m_nBufSize)
					{
						pLine = (char*)realloc(pLine, m_nBufSize + 16);
					}
					*m_pBuffer = '\n';
					++m_pBuffer;
					break;
				}
			}

			++m_dwStart;
			++m_nCurPos;

			if (*m_pBuffer == '\n')
			{
				++m_pBuffer;
				break;
			}
			++m_pBuffer;
		}

		return VIEWER_OK;
	}

	size_t GetOffset() const
	{
		return m_nOffset;
	}

	bool FindInBuffer(const char* pStr) const
	{
		return (NULL != strstr(m_pBuffer, pStr));
	}
private:
	HANDLE  m_hFile;
	int     m_nBufSize;
	DWORD   m_dwStart;
	DWORD   m_dwBytesRead;
	size_t  m_nOffset;
	size_t  m_nCurPos;
	char*   m_pBuffer;
	char*   m_pStart;
	size_t  m_nFileSize;
};

#endif