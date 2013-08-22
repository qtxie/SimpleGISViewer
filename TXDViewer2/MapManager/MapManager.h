// ********************************************************************************************************
// Description: 
//     A collection of layers.
//
// Created 2013/1/13
//
// ********************************************************************************************************

#ifndef _MAP_MANAGER_H
#define _MAP_MANAGER_H

#include "Layer.h"

class MapManager
{
public:
	MapManager();
	~MapManager();

public:
	std::vector<Layer*>& GetAllLayers();
	void AddLayer(Layer* pLayer);
	void ScreenToWorld(int& xlon, int& ylat);
	void reset();
	void SetWorldExtent(const EXTENT& worldExtent);
	void SetScreenRect(const CRect& screenRect);
	void AdjustWorldExtent()
	{
		int newWorldHeight = (double) m_screenRect.Height() / m_screenRect.Width() * m_worldExtent.Width();
		if (newWorldHeight > m_worldExtent.Height())
		{
			int worldMidHeight = m_worldExtent.ymax - m_worldExtent.Height() / 2;
			m_worldExtent.ymax = worldMidHeight + newWorldHeight / 2;
			m_worldExtent.ymin = worldMidHeight - newWorldHeight / 2;
		}
		else
		{
			int newWorldWidth = (double) m_screenRect.Width() / m_screenRect.Height() * m_worldExtent.Height();
			if (newWorldWidth > m_worldExtent.Width())
			{
				int worldMidWidth = m_worldExtent.xmax - m_worldExtent.Width() / 2;
				m_worldExtent.xmax = worldMidWidth + newWorldWidth / 2;
				m_worldExtent.xmin = worldMidWidth - newWorldWidth / 2;
			}
		}
	}
	EXTENT GetWorldExtent()
	{
		return m_worldExtent;
	}
	CRect GetScreenExtent()
	{
		return m_screenRect;
	}
	void SetFullExtent()
	{
		m_worldExtent = m_originalExtent;
	}
	void CaculateExtent()
	{
		m_worldExtent.Reset();
		for (int i = 0; i < m_vLayers.size(); ++i)
		{
			Layer* pLayer = m_vLayers[i];
			FeatureSet* pFeatureSet = pLayer->GetFeatureSet();
			EXTENT extent = pFeatureSet->GetExtent();
			if (extent.xmax > m_worldExtent.xmax)
				m_worldExtent.xmax = extent.xmax + 1;
			if (extent.xmin < m_worldExtent.xmin)
				m_worldExtent.xmin = extent.xmin - 1;
			if (extent.ymax > m_worldExtent.ymax)
				m_worldExtent.ymax = extent.ymax + 1;
			if (extent.ymin < m_worldExtent.ymin)
				m_worldExtent.ymin = extent.ymin - 1;
		}
		m_originalExtent = m_worldExtent;

	}
private:
	std::vector<Layer*>		 m_vLayers;
	EXTENT					 m_worldExtent;
	CRect					 m_screenRect;		
	bool                     m_bFisrtLayer;
	COLORREF                 m_colorList[16];
	int                      m_curColorIdx;
public:
	EXTENT					 m_originalExtent;
	bool					 m_bSelectMode;
	bool                     m_bExtentSelectMode;
	bool                     m_bZoomInMode;
	bool                     m_bZoomOutMode;
	bool                     m_bShowID;
	std::vector<CString>     m_vFilename;
	std::vector<IDataReader*> m_vDataReader;
	POINT                    m_vLocatePoint;
	bool                     m_bLocate;
	UINT					 m_nCurSelected;
	UINT				     m_nCurLayerSelected;
};

#endif