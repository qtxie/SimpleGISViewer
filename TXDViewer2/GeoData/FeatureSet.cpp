#include "stdafx.h"
#include "FeatureSet.h"

inline bool operator < (const POINT& a, const POINT& b)  
{  
	if (a.x == b.x)
		return a.y < b.y;
	else
		return a.x < b.x;
}

bool FeatureSet::SearchID(UINT64 nID)
{
	if (m_vID.empty())
	{
		m_pDataReader->GetAllFeaturesID(m_vID);
	}

	if (!m_vID.empty())
	{
		std::vector<UINT64>::iterator it = std::find(m_vID.begin(), m_vID.end(), nID);
		if (it == m_vID.end())
			return false;

		size_t idx = std::distance(m_vID.begin(), it);
		m_vFeatSelected.clear();
		m_vFeatSelected.push_back(idx);
		g_MapManager.SetWorldExtent(GetFeatureExtent(idx));
	}
	return true;
}

bool FeatureSet::GetSelectedPoints(std::vector<int>& vPoints, std::vector<UINT>& vPtNum, bool& bDrawPoint)
{
	if (m_vFeatSelected.empty())
		return false;
	return GetFeaturesPoints(m_vFeatSelected, vPoints, vPtNum, bDrawPoint);
}

bool FeatureSet::SelectFeatures(EXTENT& extent, int xScreen, int yScreen, bool bHit, bool bCtrl)
{
	const int xPos = extent.xmax - 3500;
	const int yPos = extent.ymax - 3500;

	int worldX = m_worldExtent.xmin;
	int worldY = m_worldExtent.ymax;
	int worldWidth = m_worldExtent.Width();
	int worldHeight = m_worldExtent.Height();

	double x_scale = (double) m_viewRect.Width() / worldWidth;
	double y_scale = (double) m_viewRect.Height() / worldHeight;

	if (!bCtrl)
		m_vFeatSelected.clear();
	int nScaleExtent = 10000;
	if (m_eFeatureType == POLYGON_TYPE)
	{
		nScaleExtent = 100000;
	}
	bool bFind = false;
	size_t nSaveFeat = 0;
	UINT distance = std::numeric_limits<UINT>::max();
	for (UINT i = 0; i < m_vFeatInViews.size(); ++i)
	{
		mapExtent::iterator it = m_vFeatInViews[i];
		if (!extent.Intersect(EXTENT(it->first, nScaleExtent)))
			continue;
		std::vector<DWORD32>& vFeatureNum = it->second;
		DWORD32 nSize = vFeatureNum.size();
		for (DWORD32 i = 0; i < nSize; ++i)
		{
			DWORD32 nCoordSize = 0;
			DWORD32 nFNum = vFeatureNum[i];
			Feature feature = m_vFeatures[nFNum];

			nCoordSize = m_vFeatures[nFNum+1].nCoordOffset - feature.nCoordOffset;

			UINT iStartPos = feature.nCoordOffset;	

			UINT nRead = 0;
			int nActualCoords = 0;
			bool bEven = true;
			int curX = 0;
			int curY = 0;
			int preX = 0;
			int preY = 0;
			int polySides = nCoordSize / 2;
			int* polyY = NULL;
			int* polyX = NULL;
			UINT polygonSize = 0;
			if (m_eFeatureType == POLYGON_TYPE)
			{
				polyY = (int*)malloc(polySides * sizeof(int));
				polyX = (int*)malloc(polySides * sizeof(int));
			}
			while (nRead < nCoordSize)
			{
				if (bEven)
				{
					bEven = !bEven;
					curX = m_pPoints[iStartPos++];
				}
				else
				{
					bEven = !bEven;
					curY = m_pPoints[iStartPos++];
				}
				++nRead;
				if (nRead % 2 == 0)
				{
					if (nRead == 2)
					{
						preX = curX;
						preY = curY;
					}

					if (m_eFeatureType == POINT_TYPE)
					{
						if (!bHit && extent.Include(CPoint(curX, curY)))
						{
							m_vFeatSelected.push_back(nFNum);
							break;
						}
						else if (bHit)
						{
							int scurX = (int)((curX - worldX) * x_scale);
							int scurY = (int)((worldY - curY) * y_scale);
							UINT nDist = (scurX - xScreen) * (scurX - xScreen) + (scurY - yScreen) * (scurY - yScreen);
							if (nDist > 3000)
								continue;
							if (nDist < distance)
							{
								distance = nDist;
								nSaveFeat = nFNum;
								bFind = true;
							}
						}
					}
					else if (nRead > 2 && m_eFeatureType == POLYLINE_TYPE)
					{
						int minX;
						int minY;
						int maxY, maxX;
						if (curX > preX)
						{
							minX = preX;
							maxX = curX;
						}
						else
						{
							minX = curX;
							maxX = preX;
						}

						if (curY > preY)
						{
							minY = preY;
							maxY = curY;
						}
						else
						{
							minY = curY;
							maxY = preY;
						}

						int nOffsetY = 160;
						int nOffsetX = 160;
						if (maxX - minX < 30)
						{
							nOffsetX = 600;
						}
						if (maxY - minY < 30)
						{
							nOffsetY = 600;
						}
						EXTENT tempExtent(maxX, minX, maxY, minY);
						if (!bHit && extent.Intersect(tempExtent))
						{
							m_vFeatSelected.push_back(nFNum);
							break;
						}
						else if (bHit && xPos <= (maxX+nOffsetX) && xPos >= (minX-nOffsetX) && yPos <= (maxY+nOffsetY) && yPos >= (minY-nOffsetY))
						{
							double l = DistancePtToPt(curX, curY, preX, preY);
							UINT nDist = 0;
							if (fabs(l - 0.0) < 0.01)
							{
								nDist = DistancePtToPt(curX, curY, xPos, yPos);
							}
							else
							{
								double r = ((curY - yPos)*(curY - preY) - (curX - xPos)*(preX - curX))/(l*l);
								if(r > 1 || r < 0) /* perpendicular projection of P is on the forward extention of AB */
								{
									nDist = min(DistancePtToPt(xPos, yPos, preX, preY), DistancePtToPt(xPos, yPos, curX, curY));
								}
								else
								{
								   double s = ((curY - yPos)*(preX - curX) - (curX - xPos)*(preY - curY))/(l*l);
								   nDist = fabs(s*l);
								}
							}
							if (nDist < distance)
							{
								distance = nDist;
								nSaveFeat = nFNum;
								bFind = true;
							}
						}
						preX = curX;
						preY = curY;
					}
					else if (m_eFeatureType == POLYGON_TYPE)
					{
						if (bHit)
						{
							polyY[polygonSize] = curY;
							polyX[polygonSize++] = curX;
						}
						else
						{
							int minX;
							int minY;
							int maxY, maxX;
							if (curX > preX)
							{
								minX = preX;
								maxX = curX;
							}
							else
							{
								minX = curX;
								maxX = preX;
							}

							if (curY > preY)
							{
								minY = preY;
								maxY = curY;
							}
							else
							{
								minY = curY;
								maxY = preY;
							}
							EXTENT tempExtent(maxX, minX, maxY, minY);
							if (extent.Intersect(tempExtent))
							{
								m_vFeatSelected.push_back(nFNum);
								break;
							}
							preX = curX;
							preY = curY;
						}
					}
				}
			}

			if (bHit && m_eFeatureType == POLYGON_TYPE)
			{
				int   i, j= polySides- 1;
				bool  oddNodes=false;

				for (i=0; i<polySides; i++) {
				if ((polyY[i]< yPos && polyY[j]>= yPos
				||   polyY[j]< yPos && polyY[i]>=yPos)
				&&  (polyX[i]<=xPos || polyX[j]<=xPos)) {
					oddNodes^=(polyX[i]+(yPos-polyY[i])/(polyY[j]-polyY[i])*(polyX[j]-polyX[i])<xPos); }
				j=i; }
				free(polyX);
				free(polyY);
				if (oddNodes)
				{
					m_vFeatSelected.push_back(nFNum);
					return true;
				}
			}
		}
	}
	if (m_eFeatureType != POLYGON_TYPE && bHit && bFind)
	{
		if (m_eFeatureType == POLYLINE_TYPE && distance > 1000)
		{
			return true;
		}
		//if (m_eFeatureType == POLYLINE_TYPE && distance > 3000)
		//{
		//    return true;
		//}
		std::vector<UINT>::iterator it = std::find(m_vFeatSelected.begin(), m_vFeatSelected.end(), nSaveFeat);
		if (it != m_vFeatSelected.end())
		{
			m_vFeatSelected.erase(it);
		}
		else
		{
			m_vFeatSelected.push_back(nSaveFeat);
		}
	}
	std::sort(m_vFeatSelected.begin(), m_vFeatSelected.end());
	m_vFeatSelected.erase(std::unique(m_vFeatSelected.begin(), m_vFeatSelected.end()), m_vFeatSelected.end());
	return true;
}

void FeatureSet::CollectFeaturesInView()
{
	m_vFeatInViews.clear();
	mapExtent::iterator itEnd = m_mapExtent.end();
	mapExtent::iterator it = m_mapExtent.begin();
	DWORD nCount = 0;
	UINT nPoint = 0;
	int nScaleExtent = 10000;
	if (m_eFeatureType == POLYGON_TYPE)
	{
		nScaleExtent = 100000;
	}
	for (; it != itEnd; ++it)
	{
		if (!m_worldExtent.Intersect(EXTENT(it->first, nScaleExtent)))
			continue;
		m_vFeatInViews.push_back(it);
	}
}
bool FeatureSet::GetFeaturesPoints(std::vector<UINT>& vFeatureIdx, 
									std::vector<int>& vPoints,
									std::vector<UINT>& vPtNum,
									bool& bDrawPoint
								 )
{
	int worldX = m_worldExtent.xmin;
	int worldY = m_worldExtent.ymax;
	int worldWidth = m_worldExtent.Width();
	int worldHeight = m_worldExtent.Height();
	std::vector<UINT> m_vSaveFeat;
	std::vector<UINT> m_vPolygonFeat;

	double x_scale = (double) m_viewRect.Width() / worldWidth;
	double y_scale = (double) m_viewRect.Height() / worldHeight;

	UINT nOutNum = vFeatureIdx.size();
	bool bHasFirst = false;

	// For polygon's outer ring and interior rings, we not distinguish them in feature struct
	// but they have same feature.nAttributeOffset, so we can find all of them using one feature in it.
	m_vSaveFeat = vFeatureIdx;
	for (size_t i = 0; i < nOutNum; ++i)
	{
		size_t nFeatIdx = vFeatureIdx[i];
		Feature feat = m_vFeatures[nFeatIdx];
		m_vPolygonFeat.push_back(nFeatIdx);
		while (true)
		{
			Feature featNext = m_vFeatures[++nFeatIdx];
			if (featNext.nAttributeOffset == feat.nAttributeOffset)
				m_vPolygonFeat.push_back(nFeatIdx);
			else
				break;
		}
		nFeatIdx = vFeatureIdx[i];
		while (true)
		{
			if (nFeatIdx == 0)
				break;
			Feature featNext = m_vFeatures[--nFeatIdx];
			if (featNext.nAttributeOffset == feat.nAttributeOffset)
			{
				m_vPolygonFeat.push_back(nFeatIdx);
				bHasFirst = true;
			}
			else
			{
				break;
			}
		}
	}
	if (nOutNum == 1 && bHasFirst)
	{
		vFeatureIdx.assign(m_vPolygonFeat.rbegin(), m_vPolygonFeat.rend());
	}
	else
	{
		vFeatureIdx = m_vPolygonFeat;
	}

	nOutNum = vFeatureIdx.size();
	DWORD32 nCoordSize = 0;
	size_t nFeatIdx = vFeatureIdx[0];
	Feature feature = m_vFeatures[nFeatIdx];

	int nPoint = 0;
	double scale = ((double)m_worldExtent.Width()) / m_oriExtent.Width();
	for (size_t i = 0; i < nOutNum; ++i)
	{
		size_t nFeatIdx = vFeatureIdx[i];
		Feature feature = m_vFeatures[nFeatIdx];

		UINT nCoordSize = m_vFeatures[nFeatIdx + 1].nCoordOffset - feature.nCoordOffset;
		if (nCoordSize > 3000)
			bDrawPoint = false;

		if (scale > 0.015 && nCoordSize > 1000)
		{
			bDrawPoint = false;
		}

		UINT iStartPos = feature.nCoordOffset;
		// First point
		int curX = (int)((m_pPoints[iStartPos++] - worldX) * x_scale);
		int curY = (int)((worldY - m_pPoints[iStartPos++]) * y_scale);
		vPoints.push_back(curX);
		vPoints.push_back(curY);
		nCoordSize -= 2;

		int nActualPoints = 1;
		while (nCoordSize)
		{
			int preX = curX;
			int preY = curY;
			curX = (int)((m_pPoints[iStartPos++] - worldX) * x_scale);
			curY = (int)((worldY - m_pPoints[iStartPos++]) * y_scale);
			nCoordSize -= 2;

			if (curX != preX || curY != preY)
			{
				++nActualPoints;
				vPoints.push_back(curX);
				vPoints.push_back(curY);
			}
		}
		if (nActualPoints == 1 && m_eFeatureType != POINT_TYPE)  // we don't throw the feature in seleted mode
		{
			++nActualPoints;
			vPoints.push_back(curX);
			vPoints.push_back(curY);
		}

		vPtNum.push_back(nActualPoints);
	}


	vFeatureIdx = m_vSaveFeat;

	return true;
}

bool FeatureSet::NextFeaturesPoints(size_t nFeatureNum,  
									std::vector<int>& vPoints,
									std::vector<UINT>& vPtNum
									)
{
	if (m_iterCurExent == m_mapExtent.end())
	{
		m_iterCurExent = m_mapExtent.begin();
		return false;
	}
	
	double scale = ((double)m_worldExtent.Width()) / m_oriExtent.Width();

	int worldX = m_worldExtent.xmin;
	int worldY = m_worldExtent.ymax;
	int worldWidth = m_worldExtent.Width();
	int worldHeight = m_worldExtent.Height();

	double x_scale = (double) m_viewRect.Width() / worldWidth;
	double y_scale = (double) m_viewRect.Height() / worldHeight;

	DWORD32 nAllFeature = 0;
	mapExtent::iterator itEnd = m_mapExtent.end(); 
	mapExtent::iterator it = m_iterCurExent;
	UINT nPoint = 0;
	int nScaleExtent = 10000;
	if (m_eFeatureType == POLYGON_TYPE)
	{
		nScaleExtent = 100000;
	}

	for (; it != itEnd; ++it)
	{
		if (!m_worldExtent.Intersect(EXTENT(it->first, nScaleExtent)))
			continue;

		int nSkipFeat = 0;
		if (scale > 0.4 && it->second.size() > 50 && m_nAllCoordsNum > 5000000)
		{
			nSkipFeat = 1;
		}
		if (scale > 0.4 && it->second.size() > 200 && m_nAllCoordsNum > 5000000)
		{
			nSkipFeat = 3;
		}
		if (scale > 0.3 && it->second.size() > 50 && m_nAllCoordsNum > 7500000)
		{
			nSkipFeat = 8;
		}
		if (scale > 0.3 && it->second.size() > 500 && m_nAllCoordsNum > 7500000)
		{
			nSkipFeat = 20;
		}
		if (scale > 0.3 && it->second.size() > 1000 && m_nAllCoordsNum > 7500000)
		{
			nSkipFeat = 40;
		}
		if (scale > 0.3 && it->second.size() > 5000 && m_nAllCoordsNum > 7500000)
		{
			nSkipFeat = 80;
		}
		std::vector<DWORD32>& vFeatureNum = it->second;
		DWORD32 nSize = vFeatureNum.size();
		for (DWORD32 i = m_nStartPos; i < nSize; ++i)
		{
			DWORD32 nFNum = vFeatureNum[i];
			Feature feature = m_vFeatures[nFNum];
			UINT nCoordSize = m_vFeatures[nFNum+1].nCoordOffset - feature.nCoordOffset;

			UINT iStartPos = feature.nCoordOffset;
			// First point
			int curX = (int)((m_pPoints[iStartPos++] - worldX) * x_scale);
			int curY = (int)((worldY - m_pPoints[iStartPos++]) * y_scale);
			vPoints.push_back(curX);
			vPoints.push_back(curY);
			nCoordSize -= 2;

			int nActualPoints = 1;
			while (nCoordSize)
			{
				int preX = curX;
				int preY = curY;
				curX = (int)((m_pPoints[iStartPos++] - worldX) * x_scale);
				curY = (int)((worldY - m_pPoints[iStartPos++]) * y_scale);
				nCoordSize -= 2;

				if (curX != preX || curY != preY)
				{
					++nActualPoints;
					vPoints.push_back(curX);
					vPoints.push_back(curY);
				}
			}

			if (nActualPoints == 1 && m_eFeatureType != POINT_TYPE)
			{
				vPoints.pop_back();
				vPoints.pop_back();
			}
			else
			{
				vPtNum.push_back(nActualPoints);
			}

			if (++nAllFeature == nFeatureNum)
			{
				m_nStartPos = ++i;
				m_iterCurExent = it;
				break;
			}
			i += nSkipFeat;
			m_nStartPos = 0;
		}

		if (nAllFeature == nFeatureNum)
		{
			if (m_nStartPos == nSize)
			{
				m_nStartPos = 0;
			}
			break;
		}
	}

	if (it == itEnd)
	{
		m_iterCurExent = it;
	}

	if (vPtNum.size() == 0)
		return false;

	if (m_eFeatureType == POINT_TYPE)
	{
		std::set<POINT> setPoint;
		size_t nCoords = vPoints.size() / 2;
		POINT* pPoints = (POINT*)(void*)&vPoints.front();
		for (UINT i = 0; i < nCoords; ++i)
		{
			setPoint.insert(pPoints[i]);
		}
		vPoints.clear();
		int nPreX = setPoint.begin()->x;
		int nPreY = setPoint.begin()->y;
		vPoints.push_back(nPreX);
		vPoints.push_back(nPreY);
		for (std::set<POINT>::const_iterator it = setPoint.begin(); it != setPoint.end(); ++it)
		{
			int nX = it->x;
			int nY = it->y;
			if (abs(nX - nPreX) > 5 || abs(nY - nPreY) > 5)
			{
				vPoints.push_back(nX);
				vPoints.push_back(nY);
			}
			nPreX = nX;
			nPreY = nY;
		}
		vPtNum.assign(vPoints.size() / 2, 1);
	}
	return true;
}

EXTENT FeatureSet::GetFeatureExtent(size_t idx)
{
	Feature feat = m_vFeatures[idx];
	if (m_eFeatureType == POINT_TYPE)
	{
		int nX = m_pPoints[feat.nCoordOffset];
		int nY = m_pPoints[feat.nCoordOffset + 1];
		EXTENT oldExtent = g_MapManager.GetWorldExtent();
		double scale = (double)oldExtent.Height() / oldExtent.Width();
		return EXTENT(nX + 1200, nX - 1200, nY + 1200 * scale, nY - 1200 * scale);
	}
	else
	{
		std::vector<size_t> vFeatureIdx;
		vFeatureIdx.push_back(idx);
		size_t nFeatIdx = vFeatureIdx[0];
		Feature feat = m_vFeatures[nFeatIdx];
		while (true)
		{
			Feature featNext = m_vFeatures[++nFeatIdx];
			if (featNext.nAttributeOffset == feat.nAttributeOffset)
				vFeatureIdx.push_back(nFeatIdx);
			else
				break;
		}
		nFeatIdx = vFeatureIdx[0];
		while (true)
		{
			if (nFeatIdx == 0)
				break;
			Feature featNext = m_vFeatures[--nFeatIdx];
			if (featNext.nAttributeOffset == feat.nAttributeOffset)
				vFeatureIdx.push_back(nFeatIdx);
			else
				break;
		}

		EXTENT extent;
		for (UINT i = 0; i < vFeatureIdx.size(); ++i)
		{
			DWORD32 nCoordSize = 0;
			size_t nFeatIdx = vFeatureIdx[i];
			Feature feature = m_vFeatures[nFeatIdx];
			nCoordSize = m_vFeatures[nFeatIdx + 1].nCoordOffset - feature.nCoordOffset;
			UINT iStartPos = feature.nCoordOffset;
			UINT nRead = 0;
			while (nRead < nCoordSize)
			{
				int nX = m_pPoints[iStartPos++];
				if (nX < extent.xmin) extent.xmin = nX;
				if (nX > extent.xmax) extent.xmax = nX;
				
				int nY = m_pPoints[iStartPos++];
				if (nY < extent.ymin) extent.ymin = nY;
				if (nY > extent.ymax) extent.ymax = nY;
				nRead += 2;
			}
		}
		return extent * 3;
	}

	return m_worldExtent;
}