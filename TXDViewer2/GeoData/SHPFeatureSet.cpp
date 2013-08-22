#include "stdafx.h"
#include "ShapefileReader.h"
#include "SHPFeatureSet.h"

SHPFeatureSet::SHPFeatureSet()
{}

SHPFeatureSet::~SHPFeatureSet()
{}

void* SHPFeatureSet::GetData(size_t Idx)
{
	UINT i = m_vFeatSelected[Idx];
	Feature feat = m_vFeatures[i];
	return m_pDataReader->GetData(feat.nAttributeOffset);
}

void* SHPFeatureSet::GetRawData(size_t Idx)
{
	UINT i = m_vFeatSelected[Idx];
	Feature feat = m_vFeatures[i];
	char* pData = (char*)m_pDataReader->GetData(feat.nAttributeOffset);
	pData[strlen(pData)-1] = '\0';
	return pData;
}

void* SHPFeatureSet::GetFeatureID(size_t Idx)
{
	Feature feat = m_vFeatures[Idx];
	return m_pDataReader->GetFeatureID(feat.nAttributeOffset);
}

size_t SHPFeatureSet::GetFeatureSize()
{
	return ((ShapefileReader*)m_pDataReader)->GetRecordsNumber();
}