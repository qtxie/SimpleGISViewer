#ifndef _TXD_FEATURE_SET_H
#define _TXD_FEATURE_SET_H

#include "FeatureSet.h"

static const int RF_FIELD_SIZE = 30;

class TXDFeatureSet : public FeatureSet
{
public:
	TXDFeatureSet();

	TXDFeatureSet(CString strName, GEO_TYPE eType) : FeatureSet(strName, eType)
	{
	}
	
	~TXDFeatureSet();

public:
	virtual void* GetData(size_t Idx);
	virtual void* GetRawData(size_t Idx);
	virtual void* GetFeatureID(size_t Idx);

public:
	static const char* RF_FIELD[RF_FIELD_SIZE];
	std::string m_strData;
	std::string m_strID;
	std::vector<int> m_vPoints;
};

#endif