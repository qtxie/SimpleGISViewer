#include "stdafx.h"
#include "MapManager.h"

MapManager::MapManager()
{
	m_worldExtent.xmax = -18000000;
	m_worldExtent.xmin = 18000000;
	m_worldExtent.ymax = -9000000;
	m_worldExtent.ymin = 9000000;
	m_screenRect.top = 0;
	m_screenRect.left = 0;
	m_screenRect.bottom = 600;
	m_screenRect.right = 800;
	m_colorList[0] = RGB(0, 0, 0);
	m_colorList[1] = RGB(250, 0, 0);
	m_colorList[2] = RGB(0, 250, 0);
	m_colorList[3] = RGB(0, 0, 250);
	m_colorList[4] = RGB(200, 100, 0);
	m_colorList[5] = RGB(0, 200, 100);
	m_colorList[6] = RGB(100, 0, 200);
	m_colorList[7] = RGB(200, 0, 100);
	m_colorList[8] = RGB(100, 200, 0);
	m_colorList[9] = RGB(100, 32, 99);
	m_colorList[10] = RGB(23, 3, 87);
	m_colorList[11] = RGB(11, 138, 0);
	m_colorList[12] = RGB(156, 0, 33);
	m_colorList[13] = RGB(0, 183, 32);
	m_colorList[14] = RGB(0, 1, 23);
	m_colorList[15] = RGB(0, 99, 0);
	m_curColorIdx = 0;
	m_bSelectMode = false;
	m_bExtentSelectMode = false;
	m_bZoomInMode = false;
	m_bZoomOutMode = false;
	m_bLocate = false;
	m_bShowID = false;
}

void MapManager::reset()
{
	m_curColorIdx = 0;
	m_worldExtent.xmax = -18000000;
	m_worldExtent.xmin = 18000000;
	m_worldExtent.ymax = -9000000;
	m_worldExtent.ymin = 9000000;
	m_screenRect.top = 0;
	m_screenRect.left = 0;
	m_screenRect.bottom = 600;
	m_screenRect.right = 800;
}

MapManager::~MapManager()
{
	for(UINT i = 0; i < m_vLayers.size(); ++i)
	{
		delete m_vLayers[i];
	}
	for (UINT i = 0; i < m_vDataReader.size(); ++i)
	{
		delete m_vDataReader[i];
	}
}

std::vector<Layer*>& MapManager::GetAllLayers()
{
	return m_vLayers;
}

void MapManager::AddLayer(Layer* pLayer)
{
	if (m_curColorIdx == 16)
	{
		m_curColorIdx = 0;
	}
	pLayer->SetColor(m_colorList[m_curColorIdx++]);
	m_vLayers.push_back(pLayer);
}

void MapManager::ScreenToWorld(int& xlon, int& ylat)
{
	double x_scale = (double) xlon / m_screenRect.Width();
	double y_scale = (double) ylat / m_screenRect.Height();

	xlon = m_worldExtent.xmin + (int)(m_worldExtent.xmax - m_worldExtent.xmin) * x_scale;
	ylat = m_worldExtent.ymax - (int)(m_worldExtent.ymax - m_worldExtent.ymin) * y_scale;
}

void MapManager::SetScreenRect(const CRect& screenRect)
{
	m_screenRect = screenRect;
}

void MapManager::SetWorldExtent(const EXTENT& worldExtent)
{
	m_worldExtent = worldExtent;
}