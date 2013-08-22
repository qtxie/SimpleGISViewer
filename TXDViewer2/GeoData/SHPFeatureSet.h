#ifndef _SHP_FEATURE_SET_H
#define _SHP_FEATURE_SET_H

#include "FeatureSet.h"

class SHPFeatureSet : public FeatureSet
{
public:
	SHPFeatureSet();

	SHPFeatureSet(CString strName, GEO_TYPE eType) : FeatureSet(strName, eType)
	{}

	~SHPFeatureSet();

public:
	virtual void* GetData(size_t Idx);
	virtual void* GetRawData(size_t Idx);
	virtual void* GetFeatureID(size_t Idx);
	virtual size_t GetFeatureSize();
};

#endif