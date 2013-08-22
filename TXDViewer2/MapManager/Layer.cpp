#include "stdafx.h"
#include "Layer.h"

Layer::Layer() : m_pFeatureSet(NULL), bVisable(true)
{
}

Layer::Layer(FeatureSet* pFeatureSet) : m_pFeatureSet(pFeatureSet), bVisable(true)
{
}

Layer::~Layer()
{
	delete m_pFeatureSet;
	m_pFeatureSet = NULL;
}

FeatureSet* Layer::GetFeatureSet()
{
	return m_pFeatureSet;
}

void Layer::SetFeatureSet(FeatureSet* pFeatureSet)
{
	m_pFeatureSet = pFeatureSet;
}
