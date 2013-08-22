// ********************************************************************************************************
// Description: 
//     A collection of features.
//	   Core class for manipulate feature
//
//
// ********************************************************************************************************

#ifndef _FEATURE_SET_H
#define _FEATURE_SET_H

#include <memory>
#include "Feature.h"
#include "IDataReader.h"

typedef std::map<CompactEXTENT, std::vector<DWORD32> > mapExtent;
typedef std::map<UINT64, UINT> mapID;

class FeatureSet
{
public:
	FeatureSet() : m_nAllCoordsNum(0), m_curFeatureIdx(0), m_nStartPos(0)
	{}

	FeatureSet(CString strName, GEO_TYPE eType) : 
		m_nAllCoordsNum(0), m_curFeatureIdx(0), m_nStartPos(0), 
		m_strName(strName), m_eFeatureType(eType)
	{
	}

	virtual ~FeatureSet()
	{
		for (int i = 0; i < m_vAttrSelected.size(); ++i)
		{
			delete[] m_vAttrSelected[i];
		}
	}

public:
	/*
	 * Highlight one feature of all selected feature.
	 */
	virtual bool GetFeaturesPoints(std::vector<UINT>& vFeatureIdx, 
									std::vector<int>& vPoints,
									std::vector<UINT>& vPtNum,
									bool& bDrawPoint);
	/*
	 * All selected feature's points
	 */
	virtual bool GetSelectedPoints(std::vector<int>& vPoints, std::vector<UINT>& vPtNum, bool& bDrawPoint);

	/*
	 * Iterate All Feature's points.
	 */
	virtual bool NextFeaturesPoints(size_t nFeatureNum,	  // the number of features
									std::vector<int>& vPoints,
									std::vector<UINT>& vPtNum);

	virtual size_t GetFeatureSize()
	{
		// Remove the same id, because we split multipolygon to polygon
		size_t nSize = 0;
		for (std::vector<UINT64>::iterator it = m_vID.begin(); it != m_vID.end(); ++it)
		{
			if ((it+1) != m_vID.end() && *it == *(it+1))
			{
				continue;
			}
			++nSize;
		}
		return nSize;
	}

	virtual void* GetData(size_t Idx) = 0;
	virtual void* GetRawData(size_t Idx) = 0;
	virtual void* GetFeatureID(size_t Idx) = 0;
	virtual bool SearchID(UINT64 nID);

	virtual bool SelectFeatures(EXTENT& extent, int xScreen, int yScreen, bool bHit, bool bCtrl);
	virtual void CollectFeaturesInView();
	EXTENT GetFeatureExtent(size_t idx);

	virtual EXTENT GetExtent()
	{
		return m_sExtent;
	}

	virtual void SetExtent(const EXTENT& extent)
	{
		m_sExtent = extent;
	}

	virtual void SetWorldExtent(const EXTENT& extent)
	{
		m_worldExtent = extent;
	}
	
	virtual void SetViewRect(const CRect& viewRect)
	{
		m_viewRect = viewRect;
	}

	virtual bool InView()
	{
		return m_worldExtent.Intersect(m_sExtent);
	}
public:
	CString					    m_strName;
	GEO_TYPE					m_eFeatureType;
	CRect                       m_viewRect;
	EXTENT                      m_worldExtent;
	EXTENT						m_sExtent;
	EXTENT						m_oriExtent;
	int*						m_pPoints;			// All points in memory
	std::vector<Feature>		m_vFeatures;
	std::vector<mapExtent::iterator>		m_vFeatInViews;
	std::vector<UINT>		    m_vFeatSelected;
	mapExtent                   m_mapExtent;
	std::vector<UINT64>         m_vID;
	size_t						m_nAllCoordsNum;    // all the numbers of coordinate.
	size_t						m_curFeatureIdx;
	size_t                      m_nStartPos;
	mapExtent::iterator         m_iterCurExent;
	IDataReader*                m_pDataReader;
	std::vector<LPWSTR>         m_vAttrSelected;
	size_t                      m_nFileSize;
};

#endif