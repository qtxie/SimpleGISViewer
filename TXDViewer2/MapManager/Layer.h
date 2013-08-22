// ********************************************************************************************************
// Description: 
//		Each layer has a featureset(geodata) and attributes (e.g color, visibility) of it.
//
// Created 2013/1/13
//
// ********************************************************************************************************

#ifndef _LAYER_H
#define	_LAYER_H

#include "FeatureSet.h"

class Layer
{
public:
	Layer();
	Layer(FeatureSet* pfeatureSet);
	~Layer();

public:
	FeatureSet* GetFeatureSet();

	bool isVisable()
	{
		return bVisable;
	}

	void SetFeatureSet(FeatureSet* pfeatureSet);

	COLORREF GetColor()
	{
		return m_color;
	}

	void SetColor(COLORREF color)
	{
		m_color = color;
	}

public:
	bool			bVisable;

private:
	FeatureSet*		m_pFeatureSet;
	COLORREF     	m_color; 
};

#endif