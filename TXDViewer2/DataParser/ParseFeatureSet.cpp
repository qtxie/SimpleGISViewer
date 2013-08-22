#include "stdafx.h"
#include "ParseFeatureSet.h"
#include "BigFileReader.h"
#include "ShapefileReader.h"
#include "PostGISReader.h"
#include "TXDFeatureSet.h"
#include "SHPFeatureSet.h"
#include "PostGISFeatureSet.h"

/*
 * All supported TXD record types
 */
static LPCTSTR TXD_TYPE_STR[] = 
{
#define TYPECODE(code, str) str,
#include "TXDTypes.h"
#undef TYPECODE
};

bool IsAlphaPresent( CString& myString ) 
{ 
	// Search for alpha in string. 
	LPCTSTR pLast = std::find_if( myString.operator LPCTSTR(),  
							myString.operator LPCTSTR() + myString.GetLength(),  
							isalpha ); 
	
	// If alpha is not found, then pLast will be end of string. 
	return (*pLast != 0); 
}

bool IsAlphaPresent(const std::string& myString ) 
{ 
	// Search for alpha in string. 
	std::string::const_iterator  itLast = std::find_if( myString.begin(),  
							myString.end(),  
							isalpha ); 
	
	// If alpha is not found, then pLast will be end of string. 
	return (itLast != myString.end()); 
}

std::string Unicode2char(CString strUnicode)
{
	wchar_t * t1;
	t1= strUnicode.GetBuffer(0);
	size_t convertedChars = 0;
	size_t sizeInBytes = ((strUnicode.GetLength() + 1) * 2);
	char *ch = (char *)malloc(sizeInBytes);
	wcstombs_s(&convertedChars, ch, sizeInBytes, t1, sizeInBytes);
	std::string strRet(ch);
	free(ch);
	return strRet;
}

void GetBoundingBox(std::string& strBox, EXTENT& extent)
{
	size_t index1 = strBox.find('(');
	size_t index2 = strBox.find(')');
	strBox = strBox.substr(index1+1, index2 - index1 - 1);
	size_t commasIdx = strBox.find(',');
	std::string strMin = strBox.substr(0, commasIdx);
	std::string strMax = strBox.substr(commasIdx+1);
	size_t spaceIdx = strMin.find(' ');
	double x, y;
	strtk::string_to_type_converter(strMin.substr(0, spaceIdx), x);
	strtk::string_to_type_converter(strMin.substr(spaceIdx+1), y);
	extent.xmin = x * 100000;
	extent.ymin = y * 100000;
	spaceIdx = strMax.find(' ');
	strtk::string_to_type_converter(strMax.substr(0, spaceIdx), x);
	strtk::string_to_type_converter(strMax.substr(spaceIdx+1), y);
	extent.xmax = x * 100000;
	extent.ymax = y * 100000;
}

inline bool operator == (const CompactEXTENT& a, const CompactEXTENT& b)  
{  
	return b.xmax == a.xmax && b.xmin == a.xmin && b.ymax == a.ymax && b.ymin == a.ymin;        
}  

inline bool operator < (const CompactEXTENT& a, const CompactEXTENT& b)  
{  
	if (a.xmax == b.xmax)
		if (a.xmin == b.xmin)
			if (a.ymax == b.ymax)
				return a.ymin < b.ymin;
			else
				return a.ymax < b.ymax;
		else
			return a.xmin < b.xmin;
	else
		return a.xmax < b.xmax;
}

/*
 * All supported TXD record types
 */
inline static TXD_TYPE GetRecordType(const char* szType)
{
	if (szType[0] == 'R' && szType[1] == 'F')
		return TXD_TYPE_RF;
	else if (szType[0] == 'A' && szType[1] == 'F')
		return TXD_TYPE_AF;
	else if (szType[0] == 'P' && szType[1] == 'F')
		return TXD_TYPE_PF;
	else if (szType[0] == 'G' && szType[1] == 'S')
		return TXD_TYPE_GS;
	else if (szType[0] == 'A' && szType[1] == 'P')
		return TXD_TYPE_AP;
	else if (szType[0] == 'L' && szType[1] == 'F')
		return TXD_TYPE_LF;
	else if (szType[0] == 'C' && szType[1] == 'T')
		return TXD_TYPE_CT;
	else if (szType[0] == 'T' && szType[1] == 'B')
		return TXD_TYPE_TB;
	else if (szType[0] == 'T' && szType[1] == 'G')
		return TXD_TYPE_TG;
	else if (szType[0] == 'S' && szType[1] == 'R')
		return TXD_TYPE_SR;
	else if (szType[0] == 'S' && szType[1] == 'P')
		return TXD_TYPE_SP;
	else if (szType[0] == 'G' && szType[1] == 'T')
		return TXD_TYPE_GT;
	else if (szType[0] == 'D' && szType[1] == 'J')
		return TXD_TYPE_DJ;
	else if (szType[0] == 'B' && szType[1] == 'F')
		return TXD_TYPE_BF;
	else if (szType[0] == 'L' && szType[1] == 'C')
		return TXD_TYPE_LC;
	else if (szType[0] == 'J' && szType[1] == 'V')
		return TXD_TYPE_JV;
	else if (szType[0] == 'M' && szType[1] == 'T')
		return TXD_TYPE_MT;
	else if (szType[0] == 'V' && szType[1] == 'S')
		return TXD_TYPE_VS;
	else if (szType[0] == 'V' && szType[1] == 'C')
		return TXD_TYPE_VC;
	else if (szType[0] == 'T' && szType[1] == 'S')
		return TXD_TYPE_TS;
	else if (szType[0] == 'L' && szType[1] == 'M')
		return TXD_TYPE_LM;
	else
		return TXD_TYPE_NONE;
}

/*
 * Inorder to load file quicker, we guess how much mem will be used by experience ;-)
 * I do implement a datastruct, use a link list of mem blocks, it is no need to guess memory size,
 * but because the memory not continuous, we need to do more work to read it.
 * so I choose this simplest plan, luckly it works very well :-)
 */
void InitialTXDFeatureSet(TXDFeatureSet** arrFeatSet, LPCTSTR szFilename, BigFileReader* pReader)
{
	CString strFullName(szFilename);
	CString strName;
	int idx = strFullName.ReverseFind(_T('\\'));
	if (idx != -1)
	{
		strName = strFullName.Right(strFullName.GetLength() - idx - 1);
		int dotIdx = strName.ReverseFind(_T('.'));
		if (dotIdx != -1)
		{
			strName = strName.Left(dotIdx);
		}
	}
	bool bFindAD = pReader->FindInBuffer("\nAD;");
	size_t nFileSize = pReader->GetFileSize();
	size_t nPointSize = 1024*1024;
	size_t nFeatureSize = 1024*512;
	size_t nAFPointSize = 1024*1024;
	size_t nAFFeatureSize = 1024*512;
	size_t nAPPointSize = 1024*1024;
	size_t nAPFeatureSize = 1024*512;
	size_t nPFPointSize = 1024*1024;
	size_t nPFFeatureSize = 1024*512;
	if (strName.Find(_T("_rd")) != -1 || strName.Find(_T("_RD")) != -1)
	{
		if (bFindAD || nFileSize > 1024*1024*900)
		{
			nPointSize = nFileSize / 120;
			nFeatureSize = nFileSize / 510;
		}
		else
		{
			nPointSize = nFileSize / 70;
			nFeatureSize = nFileSize / 440;
		}
	}
	else if (strName.Find(_T("_pt")) != -1 || strName.Find(_T("_PF")) != -1)
	{
		if (nFileSize > 1024 * 1024 * 1000)
		{
			nAPPointSize = nFileSize / 144;
			nAPFeatureSize = nAPPointSize / 2;
		}
		else if (nFileSize > 1024 * 1024 * 300)
		{
			nPFPointSize = nFileSize / 144;
			nPFFeatureSize = nPFPointSize / 2;
		}
	}
	else if (strName.Find(_T("_la")) != -1 || strName.Find(_T("_LA")) != -1)
	{
		nPointSize = 16;
		nAFFeatureSize = nFileSize / 820;
		nAFPointSize = nFileSize / 11;
	}
	else
	{
		if (bFindAD || nFileSize > 1024*1024*900)
		{
			nPointSize = nFileSize / 140;
			nFeatureSize = nFileSize / 530;
			nAFPointSize = nFileSize / 70;
		}
		else
		{
			nPointSize = nFileSize / 100;
			nFeatureSize = nFileSize / 470;
			nAFPointSize = nFileSize / 70;
		}
	}
	for (int i = 0; i < TXD_TYPE_END; ++i)
	{
		CString  strFeatureName = strName + _T('_') + TXD_TYPE_STR[i];; 
		GEO_TYPE eGeoType;

		switch ((TXD_TYPE)i)
		{
		case TXD_TYPE_RF:
			eGeoType = POLYLINE_TYPE;
			arrFeatSet[i] = new TXDFeatureSet(strFeatureName, eGeoType);
			arrFeatSet[i]->m_vPoints.reserve(nPointSize);
			arrFeatSet[i]->m_vID.reserve(nFeatureSize);
			arrFeatSet[i]->m_vFeatures.reserve(nFeatureSize);
			break;
		case TXD_TYPE_LF:
			eGeoType = POLYLINE_TYPE;
			arrFeatSet[i] = new TXDFeatureSet(strFeatureName, eGeoType);
			break;
		case TXD_TYPE_AF:
			eGeoType = POLYGON_TYPE;
			arrFeatSet[i] = new TXDFeatureSet(strFeatureName, eGeoType);
			arrFeatSet[i]->m_vPoints.reserve(nAFPointSize);
			arrFeatSet[i]->m_vID.reserve(nAFFeatureSize);
			arrFeatSet[i]->m_vFeatures.reserve(nAFFeatureSize);
			break;
		case TXD_TYPE_PF:
			eGeoType = POINT_TYPE;
			arrFeatSet[i] = new TXDFeatureSet(strFeatureName, eGeoType);
			arrFeatSet[i]->m_vPoints.reserve(nPFPointSize);
			arrFeatSet[i]->m_vID.reserve(nPFFeatureSize);
			arrFeatSet[i]->m_vFeatures.reserve(nPFFeatureSize);
			break;
		case TXD_TYPE_AP:
			eGeoType = POINT_TYPE;
			arrFeatSet[i] = new TXDFeatureSet(strFeatureName, eGeoType);
			arrFeatSet[i]->m_vPoints.reserve(nAPPointSize);
			arrFeatSet[i]->m_vID.reserve(nAPFeatureSize);
			arrFeatSet[i]->m_vFeatures.reserve(nAPFeatureSize);
			break;
		case TXD_TYPE_BF:
		case TXD_TYPE_CT:
		case TXD_TYPE_DJ:
		case TXD_TYPE_GS:
		case TXD_TYPE_GT:
		case TXD_TYPE_JV:
		case TXD_TYPE_LC:
		case TXD_TYPE_SR:
		case TXD_TYPE_SP:
		case TXD_TYPE_LM:
		case TXD_TYPE_TS:
		case TXD_TYPE_TB:
		case TXD_TYPE_TG:
		case TXD_TYPE_VS:
		case TXD_TYPE_VC:
			eGeoType = POINT_TYPE;
			arrFeatSet[i] = new TXDFeatureSet(strFeatureName, eGeoType);
			break;
		case TXD_TYPE_MT:
			eGeoType = GEO_UNKNOWN_TYPE;
			arrFeatSet[i] = new TXDFeatureSet(strFeatureName, eGeoType);
		default:
			break;
		}
	}
}

void InitialPostGISFeatureSet(PostGISFeatureSet** arrFeatSet, std::string& strName, 
							  const std::string& idcolumn, EXTENT& featSetExtent)
{
	arrFeatSet[POINT_TYPE] = new PostGISFeatureSet(strName.c_str(), POINT_TYPE);
	arrFeatSet[POLYLINE_TYPE] = new PostGISFeatureSet(strName.c_str(), POLYLINE_TYPE);
	arrFeatSet[POLYGON_TYPE] = new PostGISFeatureSet(strName.c_str(), POLYGON_TYPE);
	bool bHasID = false;
	if (!idcolumn.empty())
	{
		bHasID = true;
	}
	const int nFeatSize = 1024*10;
	for (int i = 0; i < GEO_UNKNOWN_TYPE; ++i)
	{
		arrFeatSet[i]->m_sExtent = featSetExtent;
		arrFeatSet[i]->m_bHasID = bHasID;
		arrFeatSet[i]->m_vFeatures.reserve(nFeatSize/2);
		arrFeatSet[i]->m_vPoints.reserve(nFeatSize);
		if (bHasID)
			arrFeatSet[i]->m_vID.reserve(nFeatSize/2);
	}
}

int ParseTXDFeatureSet(LPCTSTR szFilename, MapManager& mapManager, HWND hWnd)
{
	BigFileReader TxdReader;
	int nRet = TxdReader.Open(szFilename, FILE_FLAG_SEQUENTIAL_SCAN);
	if (nRet != VIEWER_OK)
		return nRet;
	bool bDouble = TxdReader.FindInBuffer("COORPRECISION=1,1");
	TxdReader.Close();
	if (bDouble)
		nRet = ParseTXDFeatureSetDouble(szFilename, mapManager, hWnd);
	else
		nRet = ParseTXDFeatureSetInt(szFilename, mapManager, hWnd);
	return nRet;
}

int ParseTXDFeatureSetInt(LPCTSTR szFilename, MapManager& mapManager, HWND hWnd)
{
	DWORD dwStart = GetTickCount();
	DWORD dwEnd = dwStart;
	
	BigFileReader TxdReader;
	int nRet = TxdReader.Open(szFilename, FILE_FLAG_SEQUENTIAL_SCAN);
	if (nRet != VIEWER_OK)
		return nRet;

	// Initial all TXD type featureset
	TXDFeatureSet* arrFeatureSet[TXD_TYPE_END];
	InitialTXDFeatureSet(arrFeatureSet, szFilename, &TxdReader);

	char *pLine = NULL;   
	nRet = TxdReader.GetLine(pLine);
	Feature		 feature;
	size_t nAllFeatureNum = 0;

	while (nRet == VIEWER_OK)
	{
		TXD_TYPE eType = GetRecordType(pLine);
		if (eType == TXD_TYPE_MT || eType == TXD_TYPE_NONE)
		{
			nRet = TxdReader.GetLine(pLine);
			continue;
		}

		TXDFeatureSet* pFeatureSet		= arrFeatureSet[eType];
		size_t& nCurFeature				= pFeatureSet->m_curFeatureIdx;
		std::vector<int>& vPoints		= pFeatureSet->m_vPoints;			// All points in memory
		std::vector<Feature>& vFeatures = pFeatureSet->m_vFeatures;
		mapExtent& mapExtentFeat		= pFeatureSet->m_mapExtent;
		std::vector<UINT64>& vID	    = pFeatureSet->m_vID;
		EXTENT& featSetExent			= pFeatureSet->m_sExtent;

		feature.nAttributeOffset = TxdReader.GetOffset();
		feature.nCoordOffset = vPoints.size();

		EXTENT extent;
		pLine += 3;   // skip type string.

		// Starting Process record
		// get feature id
		int nIDSize = 0;
		while (*pLine != SPLIT_FIELD) 
		{
			++nIDSize;
			++pLine;
		};

		UINT64 nID;
		strtk::fast::numeric_convert<UINT64, const char*>(nIDSize, pLine - nIDSize, nID);
		++pLine;

		// starting get points
		while (true)
		{
			bool bEven = false;
			int nTempY = 0;
			int nNumSize = 0;

			while (*pLine != SPLIT_SUB_ATRR && *pLine != SPLIT_FIELD)
			{
				if (*pLine == SPLIT_POINT)
				{
					int nTempNum;
					strtk::fast::signed_numeric_convert<int, const char*>(nNumSize, pLine - nNumSize, nTempNum);

					if (bEven)
					{
						vPoints.push_back(nTempNum);
						vPoints.push_back(nTempY);
						bEven = !bEven;
						if (nTempNum < extent.xmin) extent.xmin = nTempNum;
						if (nTempNum > extent.xmax) extent.xmax = nTempNum;
					}
					else
					{
						nTempY = nTempNum;
						bEven = !bEven;
						if (nTempNum < extent.ymin) extent.ymin = nTempNum;
						if (nTempNum > extent.ymax) extent.ymax = nTempNum;
					}
					++pLine;
					nNumSize = 0;
				}
				++pLine;
				++nNumSize;
			}

			if (nNumSize == 0)  // GS may not have point
			{
				break;
			}

			// Last coordinate
			int nTempNum;
			strtk::fast::signed_numeric_convert<int, const char*>(nNumSize, pLine - nNumSize, nTempNum);
			ATLASSERT(bEven);
			vPoints.push_back(nTempNum);
			vPoints.push_back(nTempY);
			if (nTempNum < extent.xmin) extent.xmin = nTempNum;
			if (nTempNum > extent.xmax) extent.xmax = nTempNum;

			// Got a feature;
			vFeatures.push_back(feature);
			vID.push_back(nID);

			CompactEXTENT cExtent;
			if (eType == TXD_TYPE_AF)
			{
				cExtent.xmax = extent.xmax > 0 ? (extent.xmax / 100000 + 1) : extent.xmax / 100000;
				cExtent.xmin = extent.xmin  > 0 ? extent.xmin  / 100000 : (extent.xmin / 100000 - 1);
				cExtent.ymax = extent.ymax > 0 ? (extent.ymax / 100000 + 1) : extent.ymax / 100000;
				cExtent.ymin = extent.ymin > 0 ? (extent.ymin / 100000) : (extent.ymin / 100000 - 1);
			}
			else
			{
				cExtent.xmax = extent.xmax > 0 ? (extent.xmax / 10000 + 1) : extent.xmax / 10000;
				cExtent.xmin = extent.xmin  > 0 ? extent.xmin  / 10000 : (extent.xmin / 10000 - 1);
				cExtent.ymax = extent.ymax > 0 ? (extent.ymax / 10000 + 1) : extent.ymax / 10000;
				cExtent.ymin = extent.ymin > 0 ? (extent.ymin / 10000) : (extent.ymin / 10000 - 1);
			}

			std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
			if (itFind != mapExtentFeat.end())
			{
				itFind->second.push_back(nCurFeature);
			}
			else
			{
				std::vector<DWORD32> vTemp;
				vTemp.push_back(nCurFeature);
				mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
			}

			if (extent.xmax > featSetExent.xmax) featSetExent.xmax = extent.xmax;
			if (extent.xmin < featSetExent.xmin) featSetExent.xmin = extent.xmin;
			if (extent.ymax > featSetExent.ymax) featSetExent.ymax = extent.ymax;
			if (extent.ymin < featSetExent.ymin) featSetExent.ymin = extent.ymin;

			++nCurFeature;
			++nAllFeatureNum;
			if (nAllFeatureNum % 20000 == 0)
				PostMessage(hWnd, WM_APP + 100, nAllFeatureNum, 0);

			if (*pLine == SPLIT_SUB_ATRR)
			{
				++pLine;
				feature.nCoordOffset = vPoints.size();
			}
			else
			{
				break;
			}
		}  //while true
		nRet = TxdReader.GetLine(pLine);
	}   // End Of File
	
	if (nRet != VIEWER_EOF) // Error
	{
		return nRet;
	}

	BigFileReader* pDataReader = new BigFileReader(szFilename);
	pDataReader->m_bInteger = true;
	for (int i = 0; i < TXD_TYPE_END; ++i)
	{
		TXDFeatureSet* pFeatureSet = arrFeatureSet[i];
		if (pFeatureSet->m_vPoints.empty())
		{
			delete pFeatureSet;
			continue;
		}

		//std::vector<int>(pFeatureSet->m_vPoints).swap(pFeatureSet->m_vPoints);

		Feature feat;
		feat.nAttributeOffset = std::numeric_limits<size_t>::max();
		feat.nCoordOffset = pFeatureSet->m_vPoints.size();
		pFeatureSet->m_vFeatures.push_back(feat);
		pFeatureSet->m_nAllCoordsNum = feat.nCoordOffset;
		pFeatureSet->m_pDataReader = pDataReader;
		pFeatureSet->m_nFileSize = TxdReader.GetFileSize();
		pFeatureSet->m_pPoints = &pFeatureSet->m_vPoints.front();
		pFeatureSet->m_curFeatureIdx = 0;
		pFeatureSet->m_nStartPos = 0;
		pFeatureSet->m_iterCurExent = pFeatureSet->m_mapExtent.begin();
		mapManager.AddLayer(new Layer(pFeatureSet));
	}
	mapManager.m_vFilename.push_back(szFilename); 
	mapManager.m_vDataReader.push_back(pDataReader);
	mapManager.CaculateExtent();

	dwEnd = GetTickCount();
	PostMessage(hWnd, WM_APP + 100, (dwEnd - dwStart), 2);
	return VIEWER_OK;
}

int ParseTXDFeatureSetDouble(LPCTSTR szFilename, MapManager& mapManager, HWND hWnd)
{
	DWORD dwStart = GetTickCount();
	DWORD dwEnd = dwStart;
	
	BigFileReader TxdReader;
	int nRet = TxdReader.Open(szFilename, FILE_FLAG_SEQUENTIAL_SCAN);
	if (nRet != VIEWER_OK)
		return nRet;

	// Initial all TXD type featureset
	TXDFeatureSet* arrFeatureSet[TXD_TYPE_END];
	InitialTXDFeatureSet(arrFeatureSet, szFilename, &TxdReader);

	char *pLine = NULL;   
	nRet = TxdReader.GetLine(pLine);
	Feature		 feature;
	size_t nAllFeatureNum = 0;

	while (nRet == VIEWER_OK)
	{
		TXD_TYPE eType = GetRecordType(pLine);
		if (eType == TXD_TYPE_MT || eType == TXD_TYPE_NONE)
		{
			nRet = TxdReader.GetLine(pLine);
			continue;
		}

		TXDFeatureSet* pFeatureSet		= arrFeatureSet[eType];
		size_t& nCurFeature				= pFeatureSet->m_curFeatureIdx;
		std::vector<int>& vPoints		= pFeatureSet->m_vPoints;			// All points in memory
		std::vector<Feature>& vFeatures = pFeatureSet->m_vFeatures;
		mapExtent& mapExtentFeat		= pFeatureSet->m_mapExtent;
		std::vector<UINT64>& vID	    = pFeatureSet->m_vID;
		EXTENT& featSetExent			= pFeatureSet->m_sExtent;

		feature.nAttributeOffset = TxdReader.GetOffset();
		feature.nCoordOffset = vPoints.size();

		EXTENT extent;
		pLine += 3;   // skip type string.

		// Starting Process record
		// get feature id
		int nIDSize = 0;
		while (*pLine != SPLIT_FIELD) 
		{
			++nIDSize;
			++pLine;
		};

		UINT64 nID;
		strtk::fast::numeric_convert<UINT64, const char*>(nIDSize, pLine - nIDSize, nID);
		++pLine;

		// starting get points
		while (true)
		{
			bool bEven = false;
			int nTempY = 0;
			int nNumSize = 0;

			while (*pLine != SPLIT_SUB_ATRR && *pLine != SPLIT_FIELD)
			{
				if (*pLine == SPLIT_POINT)
				{
					double nTempNum;
					strtk::string_to_type_converter(pLine - nNumSize, pLine, nTempNum);
					nTempNum *= 100000;

					if (bEven)
					{
						vPoints.push_back(nTempNum);
						vPoints.push_back(nTempY);
						bEven = !bEven;
						if (nTempNum < extent.xmin) extent.xmin = nTempNum;
						if (nTempNum > extent.xmax) extent.xmax = nTempNum;
					}
					else
					{
						nTempY = nTempNum;
						bEven = !bEven;
						if (nTempNum < extent.ymin) extent.ymin = nTempNum;
						if (nTempNum > extent.ymax) extent.ymax = nTempNum;
					}
					++pLine;
					nNumSize = 0;
				}
				++pLine;
				++nNumSize;
			}

			if (nNumSize == 0)  // GS may not have point
			{
				break;
			}

			// Last coordinate
			double nTempNum;
			strtk::string_to_type_converter(pLine - nNumSize, pLine, nTempNum);
			nTempNum *= 100000;
			ATLASSERT(bEven);
			vPoints.push_back(nTempNum);
			vPoints.push_back(nTempY);
			if (nTempNum < extent.xmin) extent.xmin = nTempNum;
			if (nTempNum > extent.xmax) extent.xmax = nTempNum;

			// Got a feature;
			vFeatures.push_back(feature);
			vID.push_back(nID);

			CompactEXTENT cExtent;
			if (eType == TXD_TYPE_AF)
			{
				cExtent.xmax = extent.xmax > 0 ? (extent.xmax / 100000 + 1) : extent.xmax / 100000;
				cExtent.xmin = extent.xmin  > 0 ? extent.xmin  / 100000 : (extent.xmin / 100000 - 1);
				cExtent.ymax = extent.ymax > 0 ? (extent.ymax / 100000 + 1) : extent.ymax / 100000;
				cExtent.ymin = extent.ymin > 0 ? (extent.ymin / 100000) : (extent.ymin / 100000 - 1);
			}
			else
			{
				cExtent.xmax = extent.xmax > 0 ? (extent.xmax / 10000 + 1) : extent.xmax / 10000;
				cExtent.xmin = extent.xmin  > 0 ? extent.xmin  / 10000 : (extent.xmin / 10000 - 1);
				cExtent.ymax = extent.ymax > 0 ? (extent.ymax / 10000 + 1) : extent.ymax / 10000;
				cExtent.ymin = extent.ymin > 0 ? (extent.ymin / 10000) : (extent.ymin / 10000 - 1);
			}

			std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
			if (itFind != mapExtentFeat.end())
			{
				itFind->second.push_back(nCurFeature);
			}
			else
			{
				std::vector<DWORD32> vTemp;
				vTemp.push_back(nCurFeature);
				mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
			}

			if (extent.xmax > featSetExent.xmax) featSetExent.xmax = extent.xmax;
			if (extent.xmin < featSetExent.xmin) featSetExent.xmin = extent.xmin;
			if (extent.ymax > featSetExent.ymax) featSetExent.ymax = extent.ymax;
			if (extent.ymin < featSetExent.ymin) featSetExent.ymin = extent.ymin;

			++nCurFeature;
			++nAllFeatureNum;
			if (nAllFeatureNum % 20000 == 0)
				PostMessage(hWnd, WM_APP + 100, nAllFeatureNum, 0);

			if (*pLine == SPLIT_SUB_ATRR)
			{
				++pLine;
				feature.nCoordOffset = vPoints.size();
			}
			else
			{
				break;
			}
		}  //while true
		nRet = TxdReader.GetLine(pLine);
	}   // End Of File
	
	if (nRet != VIEWER_EOF) // Error
	{
		return nRet;
	}

	BigFileReader* pDataReader = new BigFileReader(szFilename);
	pDataReader->m_bInteger = false;
	for (int i = 0; i < TXD_TYPE_END; ++i)
	{
		TXDFeatureSet* pFeatureSet = arrFeatureSet[i];
		if (pFeatureSet->m_vPoints.empty())
		{
			delete pFeatureSet;
			continue;
		}

		Feature feat;
		feat.nAttributeOffset = std::numeric_limits<size_t>::max();
		feat.nCoordOffset = pFeatureSet->m_vPoints.size();
		pFeatureSet->m_vFeatures.push_back(feat);
		pFeatureSet->m_nAllCoordsNum = feat.nCoordOffset;
		pFeatureSet->m_pDataReader = pDataReader;
		pFeatureSet->m_nFileSize = TxdReader.GetFileSize();
		pFeatureSet->m_pPoints = &pFeatureSet->m_vPoints.front();
		pFeatureSet->m_curFeatureIdx = 0;
		pFeatureSet->m_nStartPos = 0;
		pFeatureSet->m_iterCurExent = pFeatureSet->m_mapExtent.begin();
		mapManager.AddLayer(new Layer(pFeatureSet));
	}
	mapManager.m_vFilename.push_back(szFilename); 
	mapManager.m_vDataReader.push_back(pDataReader);
	mapManager.CaculateExtent();

	dwEnd = GetTickCount();
	PostMessage(hWnd, WM_APP + 100, (dwEnd - dwStart), 2);
	return VIEWER_OK;
}

int ParseSHPFeatureSet(LPCTSTR szFilename, MapManager& mapManager, HWND hWnd)
{
	DWORD dwStart = GetTickCount();
	DWORD dwEnd = dwStart;

	ShapefileReader shpReader;
	int nRet = shpReader.Open(szFilename, 0);
	if (nRet != VIEWER_OK)
		return nRet;
	CString strFullName(szFilename);
	CString strName;
	int idx = strFullName.ReverseFind(_T('\\'));
	if (idx != -1)
	{
		strName = strFullName.Right(strFullName.GetLength() - idx - 1);
	}
	int nRecords = shpReader.GetRecordsNumber();
	size_t nAllPoints = shpReader.GetAllPointsNum();
	GEO_TYPE eGeoType = shpReader.GetGeoType();
	SHPFeatureSet* pFeatSet = new SHPFeatureSet(strName, eGeoType);
	pFeatSet->m_pPoints = (int*)malloc(nAllPoints);
	int*& pPoints = pFeatSet->m_pPoints;

	size_t nCurPoints = 0;

	pFeatSet->m_vFeatures.reserve(nRecords);
	shpReader.GetFeatureSetExtent(pFeatSet->m_sExtent);

	mapExtent& mapExtentFeat = pFeatSet->m_mapExtent;
	Feature feature;
	UINT nCurFeature = 0;
	for (int recIdx = 0; recIdx < nRecords; ++recIdx)
	{
		if (recIdx % 20000 == 0)
			PostMessage(hWnd, WM_APP + 100, recIdx, 0);

		shpReader.At(recIdx);
		SHPObject* shpObject = shpReader.GetShpObject();

		feature.nAttributeOffset = recIdx;
		pFeatSet->m_nAllCoordsNum += shpObject->nVertices;
		if (eGeoType == POLYLINE_TYPE ||eGeoType == POLYGON_TYPE)
		{
			for (int i = 0; i < shpObject->nParts; ++i)
			{
				CompactEXTENT extent;
				if (eGeoType == POLYGON_TYPE)
				{
					shpReader.GetFeatureExtentForPolygon(extent);
				}
				else
				{
					shpReader.GetFeatureExtent(extent);
				}
				std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(extent);
				if (itFind != mapExtentFeat.end())
				{
					itFind->second.push_back(nCurFeature++);
				}
				else
				{
					std::vector<DWORD32> vTemp;
					vTemp.push_back(nCurFeature++);
					mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(extent, vTemp));
				}
				
				feature.nCoordOffset = nCurPoints;
				pFeatSet->m_vFeatures.push_back(feature);

				int nStart = shpObject->panPartStart[i];
				int nEnd;
				if (i + 1 < shpObject->nParts)
				{
					nEnd = shpObject->panPartStart[i+1];
				}
				else
				{
					nEnd = shpObject->nVertices;
				}

				for (int j = nStart; j < nEnd; ++j)
				{
					pPoints[nCurPoints++] = (int)(shpObject->padfX[j] * 100000 + 0.5);
					pPoints[nCurPoints++] = (int)(shpObject->padfY[j] * 100000 + 0.5);
				}
			}
		}
		else if (eGeoType == POINT_TYPE)
		{
			CompactEXTENT extent;
			shpReader.GetFeatureExtent(extent);
			std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(extent);
			if (itFind != mapExtentFeat.end())
			{
				itFind->second.push_back(recIdx);
			}
			else
			{
				std::vector<DWORD32> vTemp;
				vTemp.push_back(recIdx);
				mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(extent, vTemp));
			}
			feature.nCoordOffset = nCurPoints;
			pFeatSet->m_vFeatures.push_back(feature);
			pPoints[nCurPoints++] = (int)(shpObject->padfX[0] * 100000 + 0.5);
			pPoints[nCurPoints++] = (int)(shpObject->padfY[0] * 100000 + 0.5);
		}
	} // end For loop, read all records.

	IDataReader* pDataReader = new ShapefileReader(szFilename, nRecords);

	pFeatSet->m_nAllCoordsNum *= 2;
	Feature feat;
	feat.nAttributeOffset = std::numeric_limits<size_t>::max();
	feat.nCoordOffset = pFeatSet->m_nAllCoordsNum;
	pFeatSet->m_vFeatures.push_back(feat);
	pFeatSet->m_pDataReader = pDataReader;
	pFeatSet->m_curFeatureIdx = 0;
	pFeatSet->m_nStartPos = 0;
	pFeatSet->m_iterCurExent = pFeatSet->m_mapExtent.begin();
	mapManager.AddLayer(new Layer(pFeatSet));
	mapManager.m_vFilename.push_back(szFilename);
	mapManager.m_vDataReader.push_back(pDataReader);
	mapManager.CaculateExtent();

	dwEnd = GetTickCount();
	PostMessage(hWnd, WM_APP + 100, (dwEnd - dwStart), 2);
	return VIEWER_OK;
}

/*
** Decode a hex character.
*/
static unsigned char msPostGISHexDecodeChar[256] = {
  /* not Hex characters */
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  /* 0-9 */
  0,1,2,3,4,5,6,7,8,9,
  /* not Hex characters */
  64,64,64,64,64,64,64,
  /* A-F */
  10,11,12,13,14,15,
  /* not Hex characters */
  64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,
  /* a-f */
  10,11,12,13,14,15,
  64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  /* not Hex characters (upper 128 characters) */
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
};

/*
** Decode hex string "src" (null terminated)
** into "dest" (not null terminated).
** Returns length of decoded array or 0 on failure.
*/
int msPostGISHexDecode(unsigned char *dest, const char *src, int srclen)
{

  if (src && *src && (srclen % 2 == 0) ) {

	unsigned char *p = dest;
	int i;

	for ( i=0; i<srclen; i+=2 ) {
	  register unsigned char b1=0, b2=0;
	  register unsigned char c1 = src[i];
	  register unsigned char c2 = src[i + 1];

	  b1 = msPostGISHexDecodeChar[c1];
	  b2 = msPostGISHexDecodeChar[c2];

	  *p++ = (b1 << 4) | b2;

	}
	return(p-dest);
  }
  return 0;
}

void StrGeoType2Enum(const char* strType, OGRwkbGeometryType& OGRType, GEO_TYPE& viewerType)
{
	if (stricmp(strType, "POLYGON") == 0)
	{
		OGRType = wkbPolygon;
		viewerType = POLYGON_TYPE;
	}
	else if (stricmp(strType, "MultiPolygon") == 0)
	{
		OGRType = wkbMultiPolygon;
		viewerType = POLYGON_TYPE;
	}
	else if (stricmp(strType, "LINESTRING") == 0)
	{
		OGRType = wkbLineString;
		viewerType = POLYLINE_TYPE;
	}
	else if (stricmp(strType, "MultiLINESTRING") == 0)
	{
		OGRType = wkbMultiLineString;
		viewerType = POLYLINE_TYPE;
	}
	else if (stricmp(strType, "POINT") == 0)
	{
		OGRType = wkbPoint;
		viewerType = POINT_TYPE;
	}
	else if (stricmp(strType, "MULTIPOINT") == 0)
	{
		OGRType = wkbMultiPoint;
		viewerType = POINT_TYPE;
	}
	else if (stricmp(strType, "GEOMETRYCollection") == 0)
	{
		OGRType = wkbGeometryCollection;
		viewerType = GEO_UNKNOWN_TYPE;
	}
	else
	{
		OGRType = wkbUnknown;
		viewerType = GEO_UNKNOWN_TYPE;
	}
}

int ParsePostGISFeatureSet(const ConnectDBParams& db_params, MapManager& mapManager, HWND hWnd)
{
	if (db_params.sql.empty())
	{
		return ParsePostGISFeatureSetTable(db_params, mapManager, hWnd);
	}
	else
	{
		return ParsePostGISFeatureSetSQL(db_params, mapManager, hWnd);
	}
}

/*
 * TODO: Need to refactor this function, move DB reading part to PostGISReader
 */
int ParsePostGISFeatureSetSQL(const ConnectDBParams& db_params, MapManager& mapManager, HWND hWnd)
{
	DWORD dwStart = GetTickCount();
	DWORD dwEnd = dwStart;
	PGconn     *conn;
	PGresult   *res;
	const char* fmt_getPointsNum = "SELECT SUM(ST_NPoints(\"%s\")) from %s.\"%s\" %s";
	const char* fmt_getRecordNum = "SELECT COUNT(\"%s\") from %s.\"%s\" %s";
	const char* fmt_CursorHasID = "DECLARE myportal SCROLL CURSOR FOR SELECT \"%s\", \
								  encode(ST_AsBinary(ST_Force_2D(\"%s\")), 'hex') from %s.\"%s\" %s";
	const char* fmt_CursorNoID = "DECLARE myportal SCROLL CURSOR FOR SELECT \
								 encode(ST_AsBinary(ST_Force_2D(\"%s\")), 'hex') from %s.\"%s\" %s";
	// complex sql
	const char* fmt_CursorComplex = "DECLARE myportal SCROLL CURSOR FOR %s";

	const char* fmt_conninfo = "%s = '%s' port = '%s' \
							dbname = '%s' user = '%s' \
							password = '%s' connect_timeout = '6'";

	PostMessage(hWnd, WM_APP + 100, 0, 0);

	char conninfo[256] = {0};
	if (IsAlphaPresent(db_params.host))
	{
		sprintf(conninfo, fmt_conninfo, "host", db_params.host.c_str(), db_params.port.c_str(), 
				db_params.database.c_str(), db_params.username.c_str(), db_params.pwd.c_str());
	}
	else
	{
		sprintf(conninfo, fmt_conninfo, "hostaddr", db_params.host.c_str(), db_params.port.c_str(), 
				db_params.database.c_str(), db_params.username.c_str(), db_params.pwd.c_str());
	}

	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) != CONNECTION_OK) 
	{
		PQfinish(conn);
		return VIEWER_OPEN_FILE_FAILED;
	}

	/* Start a transaction block */
	res = PQexec(conn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		PQclear(res);
		PQfinish(conn);
		return VIEWER_OPEN_FILE_FAILED;
	}

	/*
	 * Should PQclear PGresult whenever it is no longer needed to avoid memory
	 * leaks
	 */
	PQclear(res);
	bool bHasID = false;
	if (!db_params.idcolumn.empty())
	{
		bHasID = true;
	}
	size_t nPointsNum = 0;
	size_t nRecords = 0;
	if (!db_params.bComplexSQL)
	{
		// Get the total number of points;
		PostMessage(hWnd, WM_APP + 100, 10, 0);
		sprintf(conninfo, fmt_getPointsNum, db_params.geomcolumn.c_str(), db_params.schema.c_str(),
				db_params.table.c_str(), db_params.sql.c_str());
		res = PQexec(conn, conninfo);
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			PQclear(res);
			PQfinish(conn);
			return VIEWER_OPEN_FILE_FAILED;
		}
		strtk::string_to_type_converter(std::string(PQgetvalue(res, 0, 0)), nPointsNum);
		PQclear(res);

		// Get the total number of records
		PostMessage(hWnd, WM_APP + 100, 100, 0);
		sprintf(conninfo, fmt_getRecordNum, db_params.geomcolumn.c_str(), db_params.schema.c_str(),
				db_params.table.c_str(), db_params.sql.c_str());
		res = PQexec(conn, conninfo);
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			PQclear(res);
			PQfinish(conn);
			return VIEWER_READ_FILE_FAILED;
		}
		strtk::string_to_type_converter(std::string(PQgetvalue(res, 0, 0)), nRecords);
		PQclear(res);

		/*
		 * Declare cursor
		 */
		if (db_params.idcolumn.empty())
			sprintf(conninfo, fmt_CursorNoID, db_params.geomcolumn.c_str(), db_params.schema.c_str(),
					db_params.table.c_str(), db_params.sql.c_str());
		else
			sprintf(conninfo, fmt_CursorHasID, db_params.idcolumn.c_str(), db_params.geomcolumn.c_str(),
					db_params.schema.c_str(), db_params.table.c_str(), db_params.sql.c_str());

		res = PQexec(conn, conninfo);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			PQclear(res);
			PQfinish(conn);
			return VIEWER_OPEN_FILE_FAILED;
		}
		PQclear(res);
	}
	else
	{
		sprintf(conninfo, fmt_CursorComplex, db_params.sql.c_str());
		res = PQexec(conn, conninfo);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			PQclear(res);
			PQfinish(conn);
			return VIEWER_OPEN_FILE_FAILED;
		}
		PQclear(res);
	}

	std::string strName = "PG_";
	if (db_params.bComplexSQL)
		strName += db_params.database + "_" + strtk::type_to_string(mapManager.GetAllLayers().size());
	else
		strName += db_params.database + "_" + db_params.schema + "_" + db_params.table;
	PostGISFeatureSet* pAllFeatSet[4] = {0};
	InitialPostGISFeatureSet(pAllFeatSet, strName, db_params.idcolumn, EXTENT());
	int nMaxPointNum = 4096;
	OGRRawPoint* pRawPt = (OGRRawPoint*)malloc(nMaxPointNum * sizeof(OGRRawPoint));
	unsigned char* wkb = (unsigned char*)malloc(nMaxPointNum * sizeof(OGRRawPoint));

	PostGISFeatureSet* pFeatSet = NULL;

	// Test geom type
	res = PQexec(conn, "FETCH FIRST in myportal");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		PQfinish(conn);
		return VIEWER_OPEN_FILE_FAILED;
	}
	int wkbstrlen = 0;
	int result = 0;
	char *wkbstr;
	if (bHasID)
	{
		wkbstr = PQgetvalue(res, 0, 1);
		wkbstrlen = PQgetlength(res, 0, 1);
	}
	else
	{
		wkbstr = PQgetvalue(res, 0, 0);
		wkbstrlen = PQgetlength(res, 0, 0);
	}

	if (wkbstrlen > nMaxPointNum * 16)
	{
		free(wkb);
		wkb = (unsigned char*)malloc(wkbstrlen);
	}
	result = msPostGISHexDecode(wkb, wkbstr, wkbstrlen);
	PQclear(res);
		
	OGRwkbByteOrder     eByteOrder;
	eByteOrder = DB2_V72_FIX_BYTE_ORDER((OGRwkbByteOrder) *wkb);
	OGRwkbGeometryType eGeometryType;
	
	if( eByteOrder == wkbNDR )
		eGeometryType = (OGRwkbGeometryType) wkb[1];
	else
		eGeometryType = (OGRwkbGeometryType) wkb[4];
	GEO_TYPE eViewerGeoType;
	switch (eGeometryType)
	{
		case wkbUnknown:
			eViewerGeoType = GEO_UNKNOWN_TYPE;
			break;
		case wkbPoint:
			eViewerGeoType = POINT_TYPE;
			break;
		case wkbLineString:
			eViewerGeoType = POLYLINE_TYPE;
			break;
		case wkbPolygon:
			eViewerGeoType = POLYGON_TYPE;
			break;
		case wkbMultiPoint:
			eViewerGeoType = POINT_TYPE;
			break;
		case wkbMultiLineString:
			eViewerGeoType = POLYLINE_TYPE;
			break;
		case wkbMultiPolygon:
			eViewerGeoType = POLYGON_TYPE;
			break;
		case wkbGeometryCollection:
			eViewerGeoType = GEO_UNKNOWN_TYPE;
			break;
		default:
			eViewerGeoType = GEO_UNKNOWN_TYPE;
	}
	res = PQexec(conn, "FETCH PRIOR in myportal");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		PQfinish(conn);
		return VIEWER_OPEN_FILE_FAILED;
	}
	PQclear(res);

	if (eViewerGeoType != GEO_UNKNOWN_TYPE && !db_params.bComplexSQL)
	{
		pAllFeatSet[eViewerGeoType]->m_vPoints.reserve(nPointsNum * 2);
		pAllFeatSet[eViewerGeoType]->m_vFeatures.reserve(nRecords);
		if (bHasID)
			pAllFeatSet[eViewerGeoType]->m_vID.reserve(nRecords);
	}
	else
	{
		pAllFeatSet[eViewerGeoType]->m_vPoints.reserve(1024 * 1024 * 2);
	}

	Feature feature;
	UINT nRecIdx = 0;
	size_t nCurPoints = 0;
	OGREnvelope featEnvelope;

	PostMessage(hWnd, WM_APP + 100, 1001, 0);
	res = PQexec(conn, "FETCH FORWARD 3000 in myportal");
	while (PQresultStatus(res) == PGRES_TUPLES_OK)
	{
		PostMessage(hWnd, WM_APP + 100, nRecIdx, 0);
		int nReadRecords = PQntuples(res);
		if (nReadRecords == 0)
			break;
		int curPos = 0;
		while (nReadRecords)
		{   
			int wkbstrlen = 0;
			int result = 0;
			char *wkbstr;
			if (bHasID)
			{
				wkbstr = PQgetvalue(res, curPos, 1);
				wkbstrlen = PQgetlength(res, curPos, 1);
			}
			else
			{
				wkbstr = PQgetvalue(res, curPos, 0);
				wkbstrlen = PQgetlength(res, curPos, 0);
			}
			if (wkbstrlen > nMaxPointNum * 16)
			{
				free(wkb);
				wkb = (unsigned char*)malloc(wkbstrlen);
			}
			result = msPostGISHexDecode(wkb, wkbstr, wkbstrlen);
			if( !result ) 
			{
				++curPos;
				++nRecIdx;
				--nReadRecords;              
				continue;
			}

			OGRwkbGeometryType eORGType = wkbUnknown;
			OGRwkbByteOrder     eByteOrder;
			eByteOrder = DB2_V72_FIX_BYTE_ORDER((OGRwkbByteOrder) *wkb);  
			if( eByteOrder == wkbNDR )
				eORGType = (OGRwkbGeometryType) wkb[1];
			else
				eORGType = (OGRwkbGeometryType) wkb[4];
			GEO_TYPE eViewerGeoType;
			switch (eORGType)
			{
				case wkbUnknown:
					eViewerGeoType = GEO_UNKNOWN_TYPE;
					break;
				case wkbPoint:
					eViewerGeoType = POINT_TYPE;
					break;
				case wkbLineString:
					eViewerGeoType = POLYLINE_TYPE;
					break;
				case wkbPolygon:
					eViewerGeoType = POLYGON_TYPE;
					break;
				case wkbMultiPoint:
					eViewerGeoType = POINT_TYPE;
					break;
				case wkbMultiLineString:
					eViewerGeoType = POLYLINE_TYPE;
					break;
				case wkbMultiPolygon:
					eViewerGeoType = POLYGON_TYPE;
					break;
				case wkbGeometryCollection:
					eViewerGeoType = GEO_UNKNOWN_TYPE;
					break;
				default:
					eViewerGeoType = GEO_UNKNOWN_TYPE;
			}
			if (eViewerGeoType != GEO_UNKNOWN_TYPE)
			{
				pFeatSet = pAllFeatSet[eViewerGeoType];
			}

			mapExtent& mapExtentFeat = pFeatSet->m_mapExtent;
			std::vector<UINT64>& vID = pFeatSet->m_vID;
			int*& pPoints = pFeatSet->m_pPoints;
			std::vector<int>& vPoints = pFeatSet->m_vPoints;
			size_t& nCurFeature	= pFeatSet->m_curFeatureIdx;

			if (bHasID)
			{
				UINT64 feat_id;
				char* pFeatID = PQgetvalue(res, curPos, 0);
				strtk::string_to_type_converter(std::string(pFeatID), feat_id);
				vID.push_back(feat_id);
			}
			feature.nAttributeOffset = nRecIdx;
			switch (eORGType)
			{
			case wkbMultiPoint:
				{
				OGRMultiPoint multiPoint;
				multiPoint.importFromWkb(wkb);
				int nPoints = multiPoint.getNumGeometries();
				for (int i = 0; i < nPoints; ++i)
				{
					OGRPoint* pPoint = (OGRPoint*)multiPoint.getGeometryRef(i);

					feature.nCoordOffset = vPoints.size();
					pFeatSet->m_vFeatures.push_back(feature);   // got a feature
					CompactEXTENT cExtent;
					OGREnvelope envelope;
					pPoint->getEnvelope(&envelope);
					featEnvelope.Merge(envelope);
					cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX*10 + 1 : envelope.MaxX*10);
					cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX*10 : envelope.MinX*10 - 1);
					cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY*10 + 1 : envelope.MaxY*10);
					cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY*10 : envelope.MinY*10 - 1);
					std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
					if (itFind != mapExtentFeat.end())
					{
						itFind->second.push_back(nCurFeature++);
					}
					else
					{
						std::vector<DWORD32> vTemp;
						vTemp.push_back(nCurFeature++);
						mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
					}

					vPoints.push_back((int)(pPoint->getX() * 100000 + 0.5));
					vPoints.push_back((int)(pPoint->getY() * 100000 + 0.5));
				}

				break;
				}
			case wkbPoint:
				{
				OGRPoint point;
				point.importFromWkb(wkb);

				feature.nCoordOffset = vPoints.size();
				pFeatSet->m_vFeatures.push_back(feature);   // got a feature
				CompactEXTENT cExtent;
				OGREnvelope envelope;
				point.getEnvelope(&envelope);
				featEnvelope.Merge(envelope);
				cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX*10 + 1 : envelope.MaxX*10);
				cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX*10 : envelope.MinX*10 - 1);
				cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY*10 + 1 : envelope.MaxY*10);
				cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY*10 : envelope.MinY*10 - 1);
				std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
				if (itFind != mapExtentFeat.end())
				{
					itFind->second.push_back(nCurFeature++);
				}
				else
				{
					std::vector<DWORD32> vTemp;
					vTemp.push_back(nCurFeature++);
					mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
				}

				vPoints.push_back((int)(point.getX() * 100000 + 0.5));
				vPoints.push_back((int)(point.getY() * 100000 + 0.5));

				break;
				}
			case wkbMultiLineString:
				{
				OGRMultiLineString multiLine;
				multiLine.importFromWkb(wkb);
				int nLines = multiLine.getNumGeometries();
				for (int i = 0; i < nLines; ++i)
				{
					feature.nCoordOffset = vPoints.size();
					pFeatSet->m_vFeatures.push_back(feature);   // got a feature
					OGRLineString* pLine = (OGRLineString*)multiLine.getGeometryRef(i);
					int numNode = pLine->getNumPoints();
					if (numNode > nMaxPointNum)
					{
						nMaxPointNum = numNode;
						free(pRawPt);
						pRawPt = (OGRRawPoint*)malloc(numNode * sizeof(OGRRawPoint));
					}
					pLine->getPoints(pRawPt);
					for (int j = 0; j < numNode; ++j)
					{
						vPoints.push_back((int)(pRawPt[j].x * 100000 + 0.5));
						vPoints.push_back((int)(pRawPt[j].y * 100000 + 0.5));
					}
					CompactEXTENT cExtent;
					OGREnvelope envelope;
					pLine->getEnvelope(&envelope);
					featEnvelope.Merge(envelope);
					cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX*10 + 1 : envelope.MaxX*10);
					cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX*10 : envelope.MinX*10 - 1);
					cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY*10 + 1 : envelope.MaxY*10);
					cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY*10 : envelope.MinY*10 - 1);
					std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
					if (itFind != mapExtentFeat.end())
					{
						itFind->second.push_back(nCurFeature++);
					}
					else
					{
						std::vector<DWORD32> vTemp;
						vTemp.push_back(nCurFeature++);
						mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
					}
				}
				break;
				}
			case wkbLineString:
				{
				OGRLineString line;
				line.importFromWkb(wkb);

				feature.nCoordOffset = vPoints.size();
				pFeatSet->m_vFeatures.push_back(feature);   // got a feature
				int numNode = line.getNumPoints();
				if (numNode > nMaxPointNum)
				{
					nMaxPointNum = numNode;
					free(pRawPt);
					pRawPt = (OGRRawPoint*)malloc(numNode * sizeof(OGRRawPoint));
				}
				line.getPoints(pRawPt);
				for (int j = 0; j < numNode; ++j)
				{
					vPoints.push_back((int)(pRawPt[j].x * 100000 + 0.5));
					vPoints.push_back((int)(pRawPt[j].y * 100000 + 0.5));
				}
				CompactEXTENT cExtent;
				OGREnvelope envelope;
				line.getEnvelope(&envelope);
				featEnvelope.Merge(envelope);
				cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX*10 + 1 : envelope.MaxX*10);
				cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX*10 : envelope.MinX*10 - 1);
				cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY*10 + 1 : envelope.MaxY*10);
				cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY*10 : envelope.MinY*10 - 1);
				std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
				if (itFind != mapExtentFeat.end())
				{
					itFind->second.push_back(nCurFeature++);
				}
				else
				{
					std::vector<DWORD32> vTemp;
					vTemp.push_back(nCurFeature++);
					mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
				}
					
				break;
				}
			case wkbMultiPolygon:
				{		
				OGRMultiPolygon multiPolygon;
				multiPolygon.importFromWkb(wkb);
				int nPolygon = multiPolygon.getNumGeometries();
				for (int i = 0; i < nPolygon; ++i)
				{
					OGRPolygon* pPolygon = (OGRPolygon*)multiPolygon.getGeometryRef(i);
					std::vector<OGRLinearRing*> vRings;
					vRings.push_back(pPolygon->getExteriorRing());
					for (int i = 0; i < pPolygon->getNumInteriorRings(); ++i)
					{
						vRings.push_back(pPolygon->getInteriorRing(i));
					}
					for (std::vector<OGRLinearRing*>::iterator it = vRings.begin(); it != vRings.end(); ++it)
					{
						feature.nCoordOffset = vPoints.size();
						pFeatSet->m_vFeatures.push_back(feature);   // got a feature
						int numNode = (*it)->getNumPoints();
						if (numNode > nMaxPointNum)
						{
							nMaxPointNum = numNode;
							free(pRawPt);
							pRawPt = (OGRRawPoint*)malloc(numNode * sizeof(OGRRawPoint));
						}
						(*it)->getPoints(pRawPt);
						for (int j = 0; j < numNode; ++j)
						{
							vPoints.push_back((int)(pRawPt[j].x * 100000 + 0.5));
							vPoints.push_back((int)(pRawPt[j].y * 100000 + 0.5));
						}
						CompactEXTENT cExtent;
						OGREnvelope envelope;
						pPolygon->getEnvelope(&envelope);
						featEnvelope.Merge(envelope);
						cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX + 1 : envelope.MaxX);
						cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX : envelope.MinX - 1);
						cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY + 1 : envelope.MaxY);
						cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY : envelope.MinY - 1);
						std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
						if (itFind != mapExtentFeat.end())
						{
							itFind->second.push_back(nCurFeature++);
						}
						else
						{
							std::vector<DWORD32> vTemp;
							vTemp.push_back(nCurFeature++);
							mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
						}
					}
				}
				break;
				}
			case wkbPolygon:
				{		
				OGRPolygon polygon;
				polygon.importFromWkb(wkb);
				std::vector<OGRLinearRing*> vRings;
				vRings.push_back(polygon.getExteriorRing());
				for (int i = 0; i < polygon.getNumInteriorRings(); ++i)
				{
					vRings.push_back(polygon.getInteriorRing(i));
				}
				for (std::vector<OGRLinearRing*>::iterator it = vRings.begin(); it != vRings.end(); ++it)
				{
					feature.nCoordOffset = vPoints.size();
					pFeatSet->m_vFeatures.push_back(feature);   // got a feature
					int numNode = (*it)->getNumPoints();
					if (numNode > nMaxPointNum)
					{
						nMaxPointNum = numNode;
						free(pRawPt);
						pRawPt = (OGRRawPoint*)malloc(numNode * sizeof(OGRRawPoint));
					}
					(*it)->getPoints(pRawPt);
					for (int j = 0; j < numNode; ++j)
					{
						vPoints.push_back((int)(pRawPt[j].x * 100000 + 0.5));
						vPoints.push_back((int)(pRawPt[j].y * 100000 + 0.5));
					}
					CompactEXTENT cExtent;
					OGREnvelope envelope;
					polygon.getEnvelope(&envelope);
					featEnvelope.Merge(envelope);
					cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX + 1 : envelope.MaxX);
					cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX : envelope.MinX - 1);
					cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY + 1 : envelope.MaxY);
					cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY : envelope.MinY - 1);
					std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
					if (itFind != mapExtentFeat.end())
					{
						itFind->second.push_back(nCurFeature++);
					}
					else
					{
						std::vector<DWORD32> vTemp;
						vTemp.push_back(nCurFeature++);
						mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
					}
				}
				break;
				}
			default:
				break;
			}
			++curPos;
			++nRecIdx;
			--nReadRecords;
		}
		PQclear(res);
		res = PQexec(conn, "FETCH FORWARD 3000 in myportal");
	}

	free(pRawPt);
	free(wkb);
	PQclear(res);

	/* close the portal ... we don't bother to check for errors ... */
	res = PQexec(conn, "CLOSE myportal");
	PQclear(res);

	/* end the transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	/* close the connection to the database and cleanup */
	PQfinish(conn);

	PostGISReader* pDataReader = new PostGISReader(db_params);
	pDataReader->Open(0);
	for (int i = 0; i < GEO_UNKNOWN_TYPE; ++i)
	{
		PostGISFeatureSet* pFeatureSet = pAllFeatSet[i];
		Feature feat;
		feat.nAttributeOffset = std::numeric_limits<size_t>::max();

		if (pFeatureSet->m_vPoints.empty())
		{
			delete pFeatureSet;
			continue;
		}

		EXTENT& featSetExtent = pFeatureSet->m_sExtent;
		featSetExtent.xmax = featEnvelope.MaxX * 100000;
		featSetExtent.xmin = featEnvelope.MinX * 100000;
		featSetExtent.ymax = featEnvelope.MaxY * 100000;
		featSetExtent.ymin = featEnvelope.MinY * 100000;
		feat.nCoordOffset = pFeatureSet->m_vPoints.size();
		pFeatureSet->m_vFeatures.push_back(feat);
		pFeatureSet->m_nAllCoordsNum = feat.nCoordOffset;
		pFeatureSet->m_pPoints = &pFeatureSet->m_vPoints.front();

		pFeatureSet->m_pDataReader = pDataReader;
		pFeatureSet->m_nFeatureSize = nRecIdx;
		pFeatureSet->m_curFeatureIdx = 0;
		pFeatureSet->m_nStartPos = 0;
		pFeatureSet->m_iterCurExent = pFeatureSet->m_mapExtent.begin();
		mapManager.AddLayer(new Layer(pFeatureSet));
	}

	mapManager.m_vFilename.push_back(strName.c_str());
	mapManager.m_vDataReader.push_back(pDataReader);
	mapManager.CaculateExtent();

	dwEnd = GetTickCount();
	PostMessage(hWnd, WM_APP + 100, (dwEnd - dwStart), 2);
	return VIEWER_OK;
}

/*
 * TODO: Need to refactor this function, move DB reading part to PostGISReader
 */
int ParsePostGISFeatureSetTable(const ConnectDBParams& db_params, MapManager& mapManager, HWND hWnd)
{
	DWORD dwStart = GetTickCount();
	DWORD dwEnd = dwStart;
	PGconn     *conn;
	PGresult   *res;
	const char* fmt_getPointsNum = "SELECT SUM(ST_NPoints(\"%s\")) from %s.%s";
	const char* fmt_getRecordNum = "SELECT COUNT(\"%s\") from %s.%s";
	const char* fmt_getExent = "SELECT ST_extent(\"%s\") from %s.%s";
	const char* fmt_CursorHasID = "DECLARE myportal SCROLL CURSOR FOR SELECT \"%s\", \
								  encode(ST_AsBinary(ST_Force_2D(\"%s\")), 'hex') from %s.%s";
	const char* fmt_CursorNoID = "DECLARE myportal SCROLL CURSOR FOR SELECT \
								 encode(ST_AsBinary(ST_Force_2D(\"%s\")), 'hex') from %s.%s";
	const char* fmt_conninfo = "%s = '%s' port = '%s' \
							dbname = '%s' user = '%s' \
							password = '%s' connect_timeout = '6'";

	PostMessage(hWnd, WM_APP + 100, 0, 0);

	char conninfo[256] = {0};
	if (IsAlphaPresent(db_params.host))
	{
		sprintf(conninfo, fmt_conninfo, "host", db_params.host.c_str(), db_params.port.c_str(), 
				db_params.database.c_str(), db_params.username.c_str(), db_params.pwd.c_str());
	}
	else
	{
		sprintf(conninfo, fmt_conninfo, "hostaddr", db_params.host.c_str(), db_params.port.c_str(), 
				db_params.database.c_str(), db_params.username.c_str(), db_params.pwd.c_str());
	}

	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) != CONNECTION_OK) 
	{
		PQfinish(conn);
		return VIEWER_OPEN_FILE_FAILED;
	}

	/* Start a transaction block */
	res = PQexec(conn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		PQclear(res);
		PQfinish(conn);
		return VIEWER_OPEN_FILE_FAILED;
	}

	/*
	 * Should PQclear PGresult whenever it is no longer needed to avoid memory
	 * leaks
	 */
	PQclear(res);

	// Geo Type
	OGRwkbGeometryType eORGType;
	GEO_TYPE eGeoType;
	StrGeoType2Enum(db_params.geotype.c_str(), eORGType, eGeoType);

	// Get the total number of points;
	PostMessage(hWnd, WM_APP + 100, 10, 0);
	sprintf(conninfo, fmt_getPointsNum, db_params.geomcolumn.c_str(), db_params.schema.c_str(), db_params.table.c_str());
	 res = PQexec(conn, conninfo);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		PQfinish(conn);
		return VIEWER_OPEN_FILE_FAILED;
	}
	size_t nPointsNum = 0;
	strtk::string_to_type_converter(std::string(PQgetvalue(res, 0, 0)), nPointsNum);
	PQclear(res);

	// Get the total number of records
	PostMessage(hWnd, WM_APP + 100, 100, 0);
	sprintf(conninfo, fmt_getRecordNum, db_params.geomcolumn.c_str(), db_params.schema.c_str(), db_params.table.c_str());
	res = PQexec(conn, conninfo);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		PQfinish(conn);
		return VIEWER_READ_FILE_FAILED;
	}
	size_t nRecords = 0;
	strtk::string_to_type_converter(std::string(PQgetvalue(res, 0, 0)), nRecords);
	PQclear(res);

	// Get featureSet Extent
	PostMessage(hWnd, WM_APP + 100, 1000, 0);
	sprintf(conninfo, fmt_getExent, db_params.geomcolumn.c_str(), db_params.schema.c_str(), db_params.table.c_str());
	res = PQexec(conn, conninfo);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		PQfinish(conn);
		return VIEWER_READ_FILE_FAILED;
	}
	EXTENT featSetExtent;
	GetBoundingBox(std::string(PQgetvalue(res, 0, 0)), featSetExtent);
	PQclear(res);

	/*
	 * Fetch rows from pg_database, the system catalog of databases
	 */
	if (db_params.idcolumn.empty())
		sprintf(conninfo, fmt_CursorNoID, db_params.geomcolumn.c_str(), db_params.schema.c_str(), db_params.table.c_str());
	else
		sprintf(conninfo, fmt_CursorHasID, db_params.idcolumn.c_str(),
					db_params.geomcolumn.c_str(), db_params.schema.c_str(), db_params.table.c_str());

	res = PQexec(conn, conninfo);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		PQclear(res);
		PQfinish(conn);
		return VIEWER_OPEN_FILE_FAILED;
	}
	PQclear(res);

	std::string strName = "PG_";
	strName += db_params.database + "_" + db_params.schema + "_" + db_params.table;
	PostGISFeatureSet* pAllFeatSet[4] = {0};
	InitialPostGISFeatureSet(pAllFeatSet, strName, db_params.idcolumn, featSetExtent);
	int nMaxPointNum = 4096;
	OGRRawPoint* pRawPt = (OGRRawPoint*)malloc(nMaxPointNum * sizeof(OGRRawPoint));
	unsigned char* wkb = (unsigned char*)malloc(nMaxPointNum * sizeof(OGRRawPoint));

	PostGISFeatureSet* pFeatSet = NULL;
	bool bHasID = false;
	if (!db_params.idcolumn.empty())
	{
		bHasID = true;
	}
	// Test geom type
	if (db_params.geotype == "GEOMETRY")
	{
		res = PQexec(conn, "FETCH FIRST in myportal");
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			PQclear(res);
			PQfinish(conn);
			return VIEWER_OPEN_FILE_FAILED;
		}
		int wkbstrlen = 0;
		int result = 0;
		char *wkbstr;
		if (!db_params.idcolumn.empty())
		{
			wkbstr = PQgetvalue(res, 0, 1);
			wkbstrlen = PQgetlength(res, 0, 1);
		}
		else
		{
			wkbstr = PQgetvalue(res, 0, 0);
			wkbstrlen = PQgetlength(res, 0, 0);
		}
		if (wkbstrlen > nMaxPointNum * 16)
		{
			free(wkb);
			wkb = (unsigned char*)malloc(wkbstrlen);
		}
		result = msPostGISHexDecode(wkb, wkbstr, wkbstrlen);
		PQclear(res);
		if( !result ) 
		{
			free(wkb);
			PQclear(res);
			PQfinish(conn);
			return VIEWER_READ_FILE_FAILED;
		}
		OGRwkbByteOrder     eByteOrder;
		eByteOrder = DB2_V72_FIX_BYTE_ORDER((OGRwkbByteOrder) *wkb);
		OGRwkbGeometryType eGeometryType;
	
		if( eByteOrder == wkbNDR )
			eGeometryType = (OGRwkbGeometryType) wkb[1];
		else
			eGeometryType = (OGRwkbGeometryType) wkb[4];
		GEO_TYPE eViewerGeoType;
		switch (eGeometryType)
		{
			case wkbUnknown:
				eViewerGeoType = GEO_UNKNOWN_TYPE;
				break;
			case wkbPoint:
				eViewerGeoType = POINT_TYPE;
				break;
			case wkbLineString:
				eViewerGeoType = POLYLINE_TYPE;
				break;
			case wkbPolygon:
				eViewerGeoType = POLYGON_TYPE;
				break;
			case wkbMultiPoint:
				eViewerGeoType = POINT_TYPE;
				break;
			case wkbMultiLineString:
				eViewerGeoType = POLYLINE_TYPE;
				break;
			case wkbMultiPolygon:
				eViewerGeoType = POLYGON_TYPE;
				break;
			case wkbGeometryCollection:
				eViewerGeoType = GEO_UNKNOWN_TYPE;
				break;
			default:
				eViewerGeoType = GEO_UNKNOWN_TYPE;
		}

		res = PQexec(conn, "FETCH PRIOR in myportal");
		PQclear(res);

		if (eViewerGeoType != GEO_UNKNOWN_TYPE)
		{
			pAllFeatSet[eViewerGeoType]->m_vPoints.reserve(nPointsNum * 2);
			pAllFeatSet[eViewerGeoType]->m_vFeatures.reserve(nRecords);
			if (bHasID)
				pAllFeatSet[eViewerGeoType]->m_vID.reserve(nRecords);
		}
	}
	else
	{
		pFeatSet = pAllFeatSet[eGeoType];
		pFeatSet->m_nAllCoordsNum = nPointsNum * 2;
		pFeatSet->m_pPoints = (int*)malloc(nPointsNum*2*sizeof(int));
		if (bHasID)
			pFeatSet->m_vID.reserve(nRecords);
		pFeatSet->m_vFeatures.reserve(nRecords);
	}

	Feature feature;
	UINT nRecIdx = 0;
	size_t nCurPoints = 0;

	bool bNotGEOMETRY = (db_params.geotype == "GEOMETRY" ? false : true);

	PostMessage(hWnd, WM_APP + 100, 1001, 0);
	res = PQexec(conn, "FETCH FORWARD 5000 in myportal");
	while (PQresultStatus(res) == PGRES_TUPLES_OK)
	{
		PostMessage(hWnd, WM_APP + 100, nRecIdx, 0);
		int nReadRecords = PQntuples(res);
		if (nReadRecords == 0)
			break;
		int curPos = 0;
		while (nReadRecords)
		{   
			int wkbstrlen = 0;
			int result = 0;
			char *wkbstr;
			if (bHasID)
			{
				wkbstr = PQgetvalue(res, curPos, 1);
				wkbstrlen = PQgetlength(res, curPos, 1);
			}
			else
			{
				wkbstr = PQgetvalue(res, curPos, 0);
				wkbstrlen = PQgetlength(res, curPos, 0);
			}
			if (wkbstrlen > nMaxPointNum * 16)
			{
				free(wkb);
				wkb = (unsigned char*)malloc(wkbstrlen);
			}
			result = msPostGISHexDecode(wkb, wkbstr, wkbstrlen);
			if( !result ) 
			{
				++curPos;
				++nRecIdx;
				--nReadRecords;              
				continue;
			}

			if (db_params.geotype == "GEOMETRY")
			{
				OGRwkbByteOrder     eByteOrder;
				eByteOrder = DB2_V72_FIX_BYTE_ORDER((OGRwkbByteOrder) *wkb);  
				if( eByteOrder == wkbNDR )
					eORGType = (OGRwkbGeometryType) wkb[1];
				else
					eORGType = (OGRwkbGeometryType) wkb[4];
				GEO_TYPE eViewerGeoType;
				switch (eORGType)
				{
					case wkbUnknown:
						eViewerGeoType = GEO_UNKNOWN_TYPE;
						break;
					case wkbPoint:
						eViewerGeoType = POINT_TYPE;
						break;
					case wkbLineString:
						eViewerGeoType = POLYLINE_TYPE;
						break;
					case wkbPolygon:
						eViewerGeoType = POLYGON_TYPE;
						break;
					case wkbMultiPoint:
						eViewerGeoType = POINT_TYPE;
						break;
					case wkbMultiLineString:
						eViewerGeoType = POLYLINE_TYPE;
						break;
					case wkbMultiPolygon:
						eViewerGeoType = POLYGON_TYPE;
						break;
					case wkbGeometryCollection:
						eViewerGeoType = GEO_UNKNOWN_TYPE;
						break;
					default:
						eViewerGeoType = GEO_UNKNOWN_TYPE;
				}
				if (eViewerGeoType != GEO_UNKNOWN_TYPE)
				{
					pFeatSet = pAllFeatSet[eViewerGeoType];
				}
			}

			mapExtent& mapExtentFeat = pFeatSet->m_mapExtent;
			std::vector<UINT64>& vID = pFeatSet->m_vID;
			int*& pPoints = pFeatSet->m_pPoints;
			std::vector<int>& vPoints = pFeatSet->m_vPoints;
			size_t& nCurFeature	= pFeatSet->m_curFeatureIdx;

			if (bHasID)
			{
				UINT64 feat_id;
				char* pFeatID = PQgetvalue(res, curPos, 0);
				strtk::string_to_type_converter(std::string(pFeatID), feat_id);
				vID.push_back(feat_id);
			}
			feature.nAttributeOffset = nRecIdx;
			switch (eORGType)
			{
			case wkbMultiPoint:
				{
				OGRMultiPoint multiPoint;
				multiPoint.importFromWkb(wkb);
				int nPoints = multiPoint.getNumGeometries();
				for (int i = 0; i < nPoints; ++i)
				{
					OGRPoint* pPoint = (OGRPoint*)multiPoint.getGeometryRef(i);
					if (bNotGEOMETRY)
						feature.nCoordOffset = nCurPoints;
					else
						feature.nCoordOffset = vPoints.size();
					pFeatSet->m_vFeatures.push_back(feature);   // got a feature
					CompactEXTENT cExtent;
					OGREnvelope envelope;
					pPoint->getEnvelope(&envelope);
					cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX*10 + 1 : envelope.MaxX*10);
					cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX*10 : envelope.MinX*10 - 1);
					cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY*10 + 1 : envelope.MaxY*10);
					cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY*10 : envelope.MinY*10 - 1);
					std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
					if (itFind != mapExtentFeat.end())
					{
						itFind->second.push_back(nCurFeature++);
					}
					else
					{
						std::vector<DWORD32> vTemp;
						vTemp.push_back(nCurFeature++);
						mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
					}

					if (bNotGEOMETRY)
					{
						pPoints[nCurPoints++] = (int)(pPoint->getX() * 100000 + 0.5);
						pPoints[nCurPoints++] = (int)(pPoint->getY() * 100000 + 0.5);
					}
					else
					{
						vPoints.push_back((int)(pPoint->getX() * 100000 + 0.5));
						vPoints.push_back((int)(pPoint->getY() * 100000 + 0.5));
					}
				}

				break;
				}
			case wkbPoint:
				{
				OGRPoint point;
				point.importFromWkb(wkb);
				if (bNotGEOMETRY)
					feature.nCoordOffset = nCurPoints;
				else
					feature.nCoordOffset = vPoints.size();
				pFeatSet->m_vFeatures.push_back(feature);   // got a feature
				CompactEXTENT cExtent;
				OGREnvelope envelope;
				point.getEnvelope(&envelope);
				cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX*10 + 1 : envelope.MaxX*10);
				cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX*10 : envelope.MinX*10 - 1);
				cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY*10 + 1 : envelope.MaxY*10);
				cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY*10 : envelope.MinY*10 - 1);
				std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
				if (itFind != mapExtentFeat.end())
				{
					itFind->second.push_back(nCurFeature++);
				}
				else
				{
					std::vector<DWORD32> vTemp;
					vTemp.push_back(nCurFeature++);
					mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
				}

				if (bNotGEOMETRY)
				{
					pPoints[nCurPoints++] = (int)(point.getX() * 100000 + 0.5);
					pPoints[nCurPoints++] = (int)(point.getY() * 100000 + 0.5);
				}
				else
				{
					vPoints.push_back((int)(point.getX() * 100000 + 0.5));
					vPoints.push_back((int)(point.getY() * 100000 + 0.5));
				}
				break;
				}
			case wkbMultiLineString:
				{
				OGRMultiLineString multiLine;
				multiLine.importFromWkb(wkb);
				int nLines = multiLine.getNumGeometries();
				for (int i = 0; i < nLines; ++i)
				{
					if (bNotGEOMETRY)
						feature.nCoordOffset = nCurPoints;
					else
						feature.nCoordOffset = vPoints.size();
					pFeatSet->m_vFeatures.push_back(feature);   // got a feature
					OGRLineString* pLine = (OGRLineString*)multiLine.getGeometryRef(i);
					int numNode = pLine->getNumPoints();
					if (numNode > nMaxPointNum)
					{
						nMaxPointNum = numNode;
						free(pRawPt);
						pRawPt = (OGRRawPoint*)malloc(numNode * sizeof(OGRRawPoint));
					}
					pLine->getPoints(pRawPt);
					for (int j = 0; j < numNode; ++j)
					{
						if (bNotGEOMETRY)
						{
							pPoints[nCurPoints++] = (int)(pRawPt[j].x * 100000 + 0.5);
							pPoints[nCurPoints++] = (int)(pRawPt[j].y * 100000 + 0.5);
						}
						else
						{
							vPoints.push_back((int)(pRawPt[j].x * 100000 + 0.5));
							vPoints.push_back((int)(pRawPt[j].y * 100000 + 0.5));
						}
					}
					CompactEXTENT cExtent;
					OGREnvelope envelope;
					pLine->getEnvelope(&envelope);
					cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX*10 + 1 : envelope.MaxX*10);
					cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX*10 : envelope.MinX*10 - 1);
					cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY*10 + 1 : envelope.MaxY*10);
					cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY*10 : envelope.MinY*10 - 1);
					std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
					if (itFind != mapExtentFeat.end())
					{
						itFind->second.push_back(nCurFeature++);
					}
					else
					{
						std::vector<DWORD32> vTemp;
						vTemp.push_back(nCurFeature++);
						mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
					}
				}
				break;
				}
			case wkbLineString:
				{
				OGRLineString line;
				line.importFromWkb(wkb);
				if (bNotGEOMETRY)
					feature.nCoordOffset = nCurPoints;
				else
					feature.nCoordOffset = vPoints.size();
				pFeatSet->m_vFeatures.push_back(feature);   // got a feature
				int numNode = line.getNumPoints();
				if (numNode > nMaxPointNum)
				{
					nMaxPointNum = numNode;
					free(pRawPt);
					pRawPt = (OGRRawPoint*)malloc(numNode * sizeof(OGRRawPoint));
				}
				line.getPoints(pRawPt);
				for (int j = 0; j < numNode; ++j)
				{
					if (bNotGEOMETRY)
					{
						pPoints[nCurPoints++] = (int)(pRawPt[j].x * 100000 + 0.5);
						pPoints[nCurPoints++] = (int)(pRawPt[j].y * 100000 + 0.5);
					}
					else
					{
						vPoints.push_back((int)(pRawPt[j].x * 100000 + 0.5));
						vPoints.push_back((int)(pRawPt[j].y * 100000 + 0.5));
					}
				}
				CompactEXTENT cExtent;
				OGREnvelope envelope;
				line.getEnvelope(&envelope);
				cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX*10 + 1 : envelope.MaxX*10);
				cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX*10 : envelope.MinX*10 - 1);
				cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY*10 + 1 : envelope.MaxY*10);
				cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY*10 : envelope.MinY*10 - 1);
				std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
				if (itFind != mapExtentFeat.end())
				{
					itFind->second.push_back(nCurFeature++);
				}
				else
				{
					std::vector<DWORD32> vTemp;
					vTemp.push_back(nCurFeature++);
					mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
				}
					
				break;
				}
			case wkbMultiPolygon:
				{		
				OGRMultiPolygon multiPolygon;
				multiPolygon.importFromWkb(wkb);
				int nPolygon = multiPolygon.getNumGeometries();
				for (int i = 0; i < nPolygon; ++i)
				{
					OGRPolygon* pPolygon = (OGRPolygon*)multiPolygon.getGeometryRef(i);
					std::vector<OGRLinearRing*> vRings;
					vRings.push_back(pPolygon->getExteriorRing());
					for (int i = 0; i < pPolygon->getNumInteriorRings(); ++i)
					{
						vRings.push_back(pPolygon->getInteriorRing(i));
					}
					for (std::vector<OGRLinearRing*>::iterator it = vRings.begin(); it != vRings.end(); ++it)
					{
						if (bNotGEOMETRY)
							feature.nCoordOffset = nCurPoints;
						else
							feature.nCoordOffset = vPoints.size();
						pFeatSet->m_vFeatures.push_back(feature);   // got a feature
						int numNode = (*it)->getNumPoints();
						if (numNode > nMaxPointNum)
						{
							nMaxPointNum = numNode;
							free(pRawPt);
							pRawPt = (OGRRawPoint*)malloc(numNode * sizeof(OGRRawPoint));
						}
						(*it)->getPoints(pRawPt);
						for (int j = 0; j < numNode; ++j)
						{
							if (bNotGEOMETRY)
							{
								pPoints[nCurPoints++] = (int)(pRawPt[j].x * 100000 + 0.5);
								pPoints[nCurPoints++] = (int)(pRawPt[j].y * 100000 + 0.5);
							}
							else
							{
								vPoints.push_back((int)(pRawPt[j].x * 100000 + 0.5));
								vPoints.push_back((int)(pRawPt[j].y * 100000 + 0.5));
							}
						}
						CompactEXTENT cExtent;
						OGREnvelope envelope;
						pPolygon->getEnvelope(&envelope);
						cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX + 1 : envelope.MaxX);
						cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX : envelope.MinX - 1);
						cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY + 1 : envelope.MaxY);
						cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY : envelope.MinY - 1);
						std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
						if (itFind != mapExtentFeat.end())
						{
							itFind->second.push_back(nCurFeature++);
						}
						else
						{
							std::vector<DWORD32> vTemp;
							vTemp.push_back(nCurFeature++);
							mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
						}
					}
				}
				break;
				}
			case wkbPolygon:
				{		
				OGRPolygon polygon;
				polygon.importFromWkb(wkb);
				std::vector<OGRLinearRing*> vRings;
				vRings.push_back(polygon.getExteriorRing());
				for (int i = 0; i < polygon.getNumInteriorRings(); ++i)
				{
					vRings.push_back(polygon.getInteriorRing(i));
				}
				for (std::vector<OGRLinearRing*>::iterator it = vRings.begin(); it != vRings.end(); ++it)
				{
					if (bNotGEOMETRY)
						feature.nCoordOffset = nCurPoints;
					else
						feature.nCoordOffset = vPoints.size();
					pFeatSet->m_vFeatures.push_back(feature);   // got a feature
					int numNode = (*it)->getNumPoints();
					if (numNode > nMaxPointNum)
					{
						nMaxPointNum = numNode;
						free(pRawPt);
						pRawPt = (OGRRawPoint*)malloc(numNode * sizeof(OGRRawPoint));
					}
					(*it)->getPoints(pRawPt);
					for (int j = 0; j < numNode; ++j)
					{
						if (bNotGEOMETRY)
						{
							pPoints[nCurPoints++] = (int)(pRawPt[j].x * 100000 + 0.5);
							pPoints[nCurPoints++] = (int)(pRawPt[j].y * 100000 + 0.5);
						}
						else
						{
							vPoints.push_back((int)(pRawPt[j].x * 100000 + 0.5));
							vPoints.push_back((int)(pRawPt[j].y * 100000 + 0.5));
						}
					}
					CompactEXTENT cExtent;
					OGREnvelope envelope;
					polygon.getEnvelope(&envelope);
					cExtent.xmax = (short)(envelope.MaxX > 0 ? envelope.MaxX + 1 : envelope.MaxX);
					cExtent.xmin = (short)(envelope.MinX > 0 ? envelope.MinX : envelope.MinX - 1);
					cExtent.ymax = (short)(envelope.MaxY > 0 ? envelope.MaxY + 1 : envelope.MaxY);
					cExtent.ymin = (short)(envelope.MinY > 0 ? envelope.MinY : envelope.MinY - 1);
					std::map<CompactEXTENT, std::vector<DWORD32> >::iterator itFind = mapExtentFeat.find(cExtent);
					if (itFind != mapExtentFeat.end())
					{
						itFind->second.push_back(nCurFeature++);
					}
					else
					{
						std::vector<DWORD32> vTemp;
						vTemp.push_back(nCurFeature++);
						mapExtentFeat.insert(std::pair<CompactEXTENT, std::vector<DWORD32> >(cExtent, vTemp));
					}
				}
				break;
				}
			default:
				break;
			}
			++curPos;
			++nRecIdx;
			--nReadRecords;
		}
		PQclear(res);
		res = PQexec(conn, "FETCH FORWARD 5000 in myportal");
	}
	free(pRawPt);
	free(wkb);
	PQclear(res);

	/* close the portal ... we don't bother to check for errors ... */
	res = PQexec(conn, "CLOSE myportal");
	PQclear(res);

	/* end the transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	/* close the connection to the database and cleanup */
	PQfinish(conn);

	PostGISReader* pDataReader = new PostGISReader(db_params);
	pDataReader->Open(0);
	for (int i = 0; i < GEO_UNKNOWN_TYPE; ++i)
	{
		PostGISFeatureSet* pFeatureSet = pAllFeatSet[i];
		Feature feat;
		feat.nAttributeOffset = std::numeric_limits<size_t>::max();
		if (bNotGEOMETRY)
		{
			if (pFeatureSet->m_eFeatureType != eGeoType)
			{
				delete pFeatureSet;
				continue;
			}
			feat.nCoordOffset = pFeatureSet->m_nAllCoordsNum;
			pFeatureSet->m_vFeatures.push_back(feat);
		}
		else
		{
			if (pFeatureSet->m_vPoints.empty())
			{
				delete pFeatureSet;
				continue;
			}
			feat.nCoordOffset = pFeatureSet->m_vPoints.size();
			pFeatureSet->m_vFeatures.push_back(feat);
			pFeatureSet->m_nAllCoordsNum = feat.nCoordOffset;
			pFeatureSet->m_pPoints = &pFeatureSet->m_vPoints.front();
		}
		pFeatureSet->m_pDataReader = pDataReader;
		pFeatureSet->m_nFeatureSize = nRecIdx;
		pFeatureSet->m_curFeatureIdx = 0;
		pFeatureSet->m_nStartPos = 0;
		pFeatureSet->m_iterCurExent = pFeatureSet->m_mapExtent.begin();
		mapManager.AddLayer(new Layer(pFeatureSet));
	}

	mapManager.m_vFilename.push_back(strName.c_str());
	mapManager.m_vDataReader.push_back(pDataReader);
	mapManager.CaculateExtent();

	dwEnd = GetTickCount();
	PostMessage(hWnd, WM_APP + 100, (dwEnd - dwStart), 2);
	return VIEWER_OK;
}