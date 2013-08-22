#include "stdafx.h"
#include "PostGISFeatureSet.h"

PostGISFeatureSet::PostGISFeatureSet() : m_bHasID(false)
{}

PostGISFeatureSet::~PostGISFeatureSet()
{}

void* PostGISFeatureSet::GetData(size_t Idx)
{
	UINT i = m_vFeatSelected[Idx];
	Feature feat = m_vFeatures[i];
	return m_pDataReader->GetData(feat.nAttributeOffset);
}

void* PostGISFeatureSet::GetRawData(size_t Idx)
{
	UINT i = m_vFeatSelected[Idx];
	Feature feat = m_vFeatures[i];
	return m_pDataReader->GetData(feat.nAttributeOffset);
}
void* PostGISFeatureSet::GetFeatureID(size_t Idx)
{
	if (m_bHasID)
		strtk::type_to_string(m_vID[Idx], m_strID);
	else
		m_strID = "Not set id";
	return (void*)m_strID.c_str();
}

bool PostGISFeatureSet::SearchID(UINT64 nID)
{
	if (m_bHasID)
		return FeatureSet::SearchID(nID);
	return true;
}

size_t PostGISFeatureSet::GetFeatureSize()
{
	return m_nFeatureSize;
}
