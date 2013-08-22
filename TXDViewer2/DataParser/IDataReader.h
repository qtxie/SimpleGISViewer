#ifndef _INTERFACE_FILE_READER_H_
#define _INTERFACE_FILE_READER_H_

class IDataReader
{
public:
	IDataReader() : m_bOpened(false)
	{
	}

	virtual int Open(UINT mode) = 0;
	virtual void Close() = 0;

	virtual void* GetData(size_t Idx)
	{
		return NULL;
	}

	virtual void* GetData(size_t nOffset, UINT nLen)
	{
		return NULL;
	}

	virtual void* GetFeatureID(size_t Idx)
	{
		return NULL;
	}

	virtual bool GetAllFeaturesID(std::vector<UINT64>& vID)
	{
		return false;
	}

protected:
	CString m_strFileName;
	bool    m_bOpened;
public:
	bool    m_bInteger;
};
#endif