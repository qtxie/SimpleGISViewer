#ifndef _SHAPE_FILE_READER_H_
#define _SHAPE_FILE_READER_H_

#include "Feature.h"
#include "ErrorCode.h"
#include "IDataReader.h"
#include "shapefil.h"

class ShapefileReader : public IDataReader
{
public:
	ShapefileReader() : m_hShp(NULL), m_hDbf(NULL), m_curShpObj(NULL), m_nCurIdx(-1), m_nRecordNum(0)
	{}

	ShapefileReader(CString strFileName, int nRecords) : m_hShp(NULL), m_hDbf(NULL), m_curShpObj(NULL), m_nCurIdx(-1), m_nRecordNum(nRecords)
	{
		m_bOpened = false;
		m_strFileName = strFileName;
	}

	~ShapefileReader()
	{
		SHPClose(m_hShp);
		DBFClose(m_hDbf);
		if (m_curShpObj)
			SHPDestroyObject(m_curShpObj);
	}

	int Open(LPCTSTR lpstrFileName, UINT mode)
	{
		m_strFileName = lpstrFileName;
		LPCTSTR lpData = lpstrFileName;
		DWORD dwNum = WideCharToMultiByte(CP_ACP, 0, lpData, -1, NULL, 0, NULL, NULL);
		LPSTR pData = new CHAR[dwNum+4];
		if (!pData)
		{
			delete []pData;
		}
		WideCharToMultiByte(CP_ACP, 0, lpData, -1, pData, dwNum, NULL, NULL);
		m_hShp = SHPOpen(pData, "rb");
		if (!m_hShp)
			return VIEWER_OPEN_FILE_FAILED;
		double min[4];
		double max[4];
		SHPGetInfo(m_hShp, &m_nRecordNum, &m_nType, min, max);
		m_curShpObj = SHPReadObject(m_hShp, 0);
		delete []pData;
		m_bOpened = true;
		return VIEWER_OK;
	}

	virtual int Open(UINT mode)
	{
		if (m_bOpened)
			return VIEWER_OK;

		LPCTSTR lpData = (LPCTSTR)m_strFileName.GetBuffer(m_strFileName.GetLength());
		DWORD dwNum = WideCharToMultiByte(CP_ACP, 0, lpData, -1, NULL, 0, NULL, NULL);
		LPSTR pData = new CHAR[dwNum+4];
		if (!pData)
		{
			delete []pData;
		}
		WideCharToMultiByte(CP_ACP, 0, lpData, -1, pData, dwNum, NULL, NULL);
		strcpy(pData + dwNum-3, "dbf\0");
		m_hDbf = DBFOpen(pData, "rb");
		if (!m_hDbf)
			return VIEWER_OPEN_FILE_FAILED;
		delete []pData;
		m_bOpened = true;
		m_nRecordNum = DBFGetRecordCount(m_hDbf);

		return VIEWER_OK;
	}

	virtual void Close()
	{
		SHPClose(m_hShp);
		DBFClose(m_hDbf);
		if (m_curShpObj)
			SHPDestroyObject(m_curShpObj);
		m_hDbf = NULL;
		m_hShp = NULL;
		m_curShpObj = NULL;
		m_bOpened = false;

	}

	void GetFeatureExtentForPolygon(CompactEXTENT& cExtent)
	{
		cExtent.xmax = (short)(m_curShpObj->dfXMax > 0 ? m_curShpObj->dfXMax + 1 : m_curShpObj->dfXMax);
		cExtent.xmin = (short)(m_curShpObj->dfXMin > 0 ? m_curShpObj->dfXMin : m_curShpObj->dfXMin - 1);
		cExtent.ymax = (short)(m_curShpObj->dfYMax > 0 ? m_curShpObj->dfYMax + 1 : m_curShpObj->dfYMax);
		cExtent.ymin = (short)(m_curShpObj->dfYMin > 0 ? m_curShpObj->dfYMin : m_curShpObj->dfYMin - 1);
	}

	void GetFeatureExtent(CompactEXTENT& cExtent)
	{
		cExtent.xmax = (short)(m_curShpObj->dfXMax > 0 ? m_curShpObj->dfXMax * 10 + 1 : m_curShpObj->dfXMax * 10);
		cExtent.xmin = (short)(m_curShpObj->dfXMin > 0 ? m_curShpObj->dfXMin * 10 : m_curShpObj->dfXMin * 10 - 1);
		cExtent.ymax = (short)(m_curShpObj->dfYMax > 0 ? m_curShpObj->dfYMax * 10 + 1 : m_curShpObj->dfYMax * 10);
		cExtent.ymin = (short)(m_curShpObj->dfYMin > 0 ? m_curShpObj->dfYMin * 10 : m_curShpObj->dfYMin * 10 - 1);
	}

	void At(int nIdx)
	{
		SHPDestroyObject(m_curShpObj);
		m_curShpObj = SHPReadObject(m_hShp, nIdx);
	}

	void GetFeatureSetExtent(EXTENT& extent)
	{
		double min[4];
		double max[4];
		SHPGetInfo(m_hShp, &m_nRecordNum, &m_nType, min, max);
		extent.xmax = (int) (max[0] * 100000);
		extent.ymax = (int) (max[1] * 100000);
		extent.xmin = (int) (min[0] * 100000);
		extent.ymin = (int) (min[1] * 100000);
	}

	size_t GetAllPointsNum()
	{
		HANDLE hFile = CreateFile(
					m_strFileName,										// name of the read
					GENERIC_READ,                                       // open for reading
					FILE_SHARE_READ,                                    // share read
					NULL,                                               // default security
					OPEN_EXISTING,                                      // open exist file only
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,	// normal file
					NULL);                                              // no attr. template

		if (hFile == INVALID_HANDLE_VALUE) 
		{
			return 0;
		}

		size_t nFileSize = GetFileSize(hFile, NULL);
		size_t nPointSize = nFileSize - m_nRecordNum * 8;

		if ( m_nType == SHPT_POLYGON
				|| m_nType == SHPT_POLYGONM
				|| m_nType == SHPT_POLYGONZ 
				||m_nType == SHPT_ARC
				|| m_nType == SHPT_ARCM
				|| m_nType == SHPT_ARCZ)
		{
			nPointSize -= m_nRecordNum * 48;
		}
		else if (m_nType == SHPT_POINT
			|| m_nType == SHPT_POINTM
			|| m_nType == SHPT_POINTZ)
		{
			nPointSize -= m_nRecordNum * 4;
		}
		CloseHandle(hFile);
		return nPointSize / 2;
	}

	SHPObject* GetShpObject()
	{
		return m_curShpObj;
	}

	int GetRecordsNumber()
	{
		return m_nRecordNum;
	}

	virtual void* GetFeatureID(size_t Idx)
	{
		if (VIEWER_OK != Open(0))
			m_strID = "Can't Open/Find *.dbf";
		else
			m_strID = DBFReadStringAttribute(m_hDbf, Idx, 0);
		return (void*)m_strID.c_str();
	}

	virtual bool GetAllFeaturesID(std::vector<UINT64>& vID)
	{
		if (VIEWER_OK != Open(0))
		{
			return false;
		}
		char x = DBFGetNativeFieldType(m_hDbf, 0);
		if (DBFGetNativeFieldType(m_hDbf, 0) != 'F')
		{
			return false;
		}
		for(int i = 0; i < m_nRecordNum; ++i)
		{
			vID.push_back(DBFReadDoubleAttribute(m_hDbf, i, 0));
		}
		return true;
	}

	GEO_TYPE GetGeoType()
	{
		if ( m_nType == SHPT_POLYGON
				|| m_nType == SHPT_POLYGONM
				|| m_nType == SHPT_POLYGONZ )
		{
			return POLYGON_TYPE;
		}
		else if (m_nType == SHPT_ARC
			|| m_nType == SHPT_ARCM
			|| m_nType == SHPT_ARCZ)
		{
			return POLYLINE_TYPE;
		}
		else if (m_nType == SHPT_POINT
			|| m_nType == SHPT_POINTM
			|| m_nType == SHPT_POINTZ)
		{
			return POINT_TYPE;
		}
		return GEO_UNKNOWN_TYPE;
	}

	virtual void* GetData(size_t Idx)
	{
		if (VIEWER_OK != Open(0))
		{
			return NULL;
		}

		m_strID = DBFReadStringAttribute(m_hDbf, Idx, 0);

		char pszFieldName[12];  //the longest possible field name of 11 characters plus a terminating zero character.
		int nFields = DBFGetFieldCount(m_hDbf);
		m_strData.clear();

		for (int i = 0; i < nFields; ++i)
		{
			DBFGetFieldInfo(m_hDbf, i, pszFieldName, NULL, NULL);
			m_strData.append(pszFieldName);
			m_strData.append(": ");
			if (DBFIsAttributeNULL(m_hDbf, Idx, i))
				m_strData.append("NULL");
			else
				m_strData.append(DBFReadStringAttribute(m_hDbf, Idx, i));
			m_strData.append("\r\n");
		}
		return (void*)m_strData.c_str();
	}

private:
	std::string   m_strData;
	std::string   m_strID;
	SHPHandle m_hShp;
	DBFHandle m_hDbf;
	SHPObject* m_curShpObj;
	int        m_nCurIdx;
	int        m_nRecordNum;
	int        m_nType;
};

#endif