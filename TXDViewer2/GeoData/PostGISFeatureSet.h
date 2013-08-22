#ifndef _POSTGIS_FEATURE_SET_H
#define _POSTGIS_FEATURE_SET_H

#include "FeatureSet.h"

class PostGISFeatureSet : public FeatureSet
{
public:
	PostGISFeatureSet();

	PostGISFeatureSet(CString strName, GEO_TYPE eType) : FeatureSet(strName, eType), m_bHasID(false)
	{}

	~PostGISFeatureSet();

public:
	virtual void* GetData(size_t Idx);
	virtual void* GetRawData(size_t Idx);
	virtual void* GetFeatureID(size_t Idx);
	virtual bool SearchID(UINT64 nID);
	virtual size_t GetFeatureSize();

public:
	bool m_bHasID;
	std::string m_strID;
	size_t m_nFeatureSize;
	std::vector<int> m_vPoints;
};

#endif