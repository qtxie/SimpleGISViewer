/////////////////////////////////////////////////////////////////////////////
//
// TXDViewer2View.h : interface of the CTXDViewer2View class
// Because only one place use this file and not too many codes,
// I don't put the implement code to TXDViewer2View.cpp file.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

static const double ZOOM_SCALE = 0.2f;

class CTXDViewer2View : public CWindowImpl<CTXDViewer2View>,
						public CDoubleBufferImpl<CTXDViewer2View>		// Double buffering
{
public:
	DECLARE_WND_CLASS(NULL)

	CIdentifyDlg   m_wndIdentifyDlg;
	CLocateDlg     m_wndLocateDlg;
	HWND	   m_hMainFrm;
	POINT      m_startPoint;        //start point to move map
	POINT	   m_startScreenPoint;
	POINT	   m_curScreenPoint;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	BEGIN_MSG_MAP(CTXDViewer2View)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MSG_WM_SETCURSOR(OnSetCursor)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
		MSG_WM_MOUSEMOVE(OnMouseMove)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
		MESSAGE_HANDLER(WM_APP + 105, OnTreeItemCheck)
		CHAIN_MSG_MAP(CDoubleBufferImpl<CTXDViewer2View>)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		m_wndIdentifyDlg.Create(m_hWnd);
		m_wndIdentifyDlg.m_hwndView = m_hWnd;
		m_wndIdentifyDlg.m_wndTreeView.m_hwndView = m_hWnd;

		((CComboBox)m_wndIdentifyDlg.GetDlgItem(IDC_COMBO_IDENTIFY_LAYERS)).AddString(_T("<Visable layers>"));
		((CComboBox)m_wndIdentifyDlg.GetDlgItem(IDC_COMBO_IDENTIFY_LAYERS)).SetCurSel(0);
		((CEdit)m_wndIdentifyDlg.GetDlgItem(IDC_EDIT1)).SetLimitText(0);

		m_wndLocateDlg.Create(m_hWnd);
		m_wndLocateDlg.m_wndView = m_hWnd;

		return 0;
	}

	BOOL OnSetCursor(CWindow wnd, UINT uHitTest, UINT message )
	{
		if (g_MapManager.m_bZoomInMode)
		{
			HCURSOR hCurs1;     // cursor handles 
			hCurs1 = LoadCursor(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_CUR_ZOOM_IN));
			SetCursor(hCurs1);
		}
		else if (g_MapManager.m_bZoomOutMode)
		{
			HCURSOR hCurs1;     // cursor handles  
			hCurs1 = LoadCursor(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_CUR_ZOOM_OUT));
			SetCursor(hCurs1);
		}
		else
		{
			HCURSOR hCurs1;
			hCurs1 = LoadCursor(NULL, IDC_ARROW);
			SetCursor(hCurs1);
		}
		return TRUE;
	}

	LRESULT OnRButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		int xPos = GET_X_LPARAM(lParam);  
		int yPos = GET_Y_LPARAM(lParam); 
		m_startScreenPoint.x = xPos;
		m_startScreenPoint.y = yPos;
		g_MapManager.ScreenToWorld(xPos, yPos);
		m_startPoint.x = xPos;
		m_startPoint.y = yPos;
		if (g_MapManager.m_bSelectMode)
		{
			// Select feature in view
			std::vector<Layer*> vLayers = g_MapManager.GetAllLayers();
			for (size_t i = 0; i < vLayers.size(); ++i)
			{
				Layer* pLayer = vLayers[i];
				if (!pLayer->isVisable())
				{
					continue;
				}
				FeatureSet* pFeatureSet = pLayer->GetFeatureSet();
				if (!pFeatureSet->InView())
				{
					continue;
				}
				pFeatureSet->CollectFeaturesInView();
			}
		}
		return 0;
	}

	LRESULT OnRButtonUp(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		int xPos = GET_X_LPARAM(lParam);  
		int yPos = GET_Y_LPARAM(lParam); 
		int xScreen = xPos;
		int yScreen = yPos;
		g_MapManager.ScreenToWorld(xPos, yPos);
		g_MapManager.m_bExtentSelectMode = false;

		if (g_MapManager.m_bSelectMode)
		{
			EXTENT hitExent;
			if (m_startPoint.x > xPos)
			{
				hitExent.xmin = xPos;
				hitExent.xmax = m_startPoint.x;
			}
			else
			{
				hitExent.xmin = m_startPoint.x;
				hitExent.xmax = xPos;
			}
			if (m_startPoint.y > yPos)
			{
				hitExent.ymin = yPos;
				hitExent.ymax = m_startPoint.y;
			}
			else
			{
				hitExent.ymin = m_startPoint.y;
				hitExent.ymax = yPos;
			}

			if (wParam & MK_CONTROL)
				m_wndIdentifyDlg.UpdateContent(hitExent, xScreen, yScreen, false, true);           
			else
				m_wndIdentifyDlg.UpdateContent(hitExent, xScreen, yScreen, false, false);

			Invalidate();
			m_wndIdentifyDlg.ShowWindow(SW_SHOWDEFAULT);
		}
		return 0;
	}

	LRESULT OnTreeItemCheck(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		std::vector<Layer*>& vLayers = g_MapManager.GetAllLayers();
		vLayers[(int)wParam]->bVisable = !vLayers[(int)wParam]->bVisable;
		Invalidate();
		return 0;
	}

	LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		SetFocus();
		int xPos = GET_X_LPARAM(lParam);  
		int yPos = GET_Y_LPARAM(lParam); 
		m_startScreenPoint.x = xPos;
		m_startScreenPoint.y = yPos;
		m_curScreenPoint.x = xPos;
		m_curScreenPoint.y = yPos;
		g_MapManager.ScreenToWorld(xPos, yPos);
		m_startPoint.x = xPos;
		m_startPoint.y = yPos;
		if (g_MapManager.m_bSelectMode)
		{
			// Select feature in view
			std::vector<Layer*> vLayers = g_MapManager.GetAllLayers();
			for (size_t i = 0; i < vLayers.size(); ++i)
			{
				Layer* pLayer = vLayers[i];
				if (!pLayer->isVisable())
				{
					continue;
				}
				FeatureSet* pFeatureSet = pLayer->GetFeatureSet();
				if (!pFeatureSet->InView())
				{
					continue;
				}
				pFeatureSet->CollectFeaturesInView();
			}
		}
		
		if (g_MapManager.m_bZoomInMode || g_MapManager.m_bZoomOutMode)
		{
			SetCapture();
		}
		return 0;
	}

	LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		int xPos = GET_X_LPARAM(lParam);  
		int yPos = GET_Y_LPARAM(lParam); 
		int xScreen = xPos;
		int yScreen = yPos;
		g_MapManager.ScreenToWorld(xPos, yPos);
		g_MapManager.m_bExtentSelectMode = false;

		if (g_MapManager.m_bSelectMode && 
			abs(m_curScreenPoint.x - m_startScreenPoint.x) < 5 && 
			abs(m_curScreenPoint.y - m_startScreenPoint.y) < 5)
		{
			EXTENT hitExent;
			hitExent.xmin = xPos - 3500;
			hitExent.xmax = xPos + 3500;
			hitExent.ymin = yPos - 3500;
			hitExent.ymax = yPos + 3500;

			if (wParam & MK_CONTROL)
				m_wndIdentifyDlg.UpdateContent(hitExent, xScreen, yScreen, true, true);           
			else
				m_wndIdentifyDlg.UpdateContent(hitExent, xScreen, yScreen, true, false);

			Invalidate();
			m_wndIdentifyDlg.ShowWindow(SW_SHOWDEFAULT);
		}
		else if (g_MapManager.m_bZoomInMode)
		{
			EXTENT newWorldExtent;
			if ((abs(m_startPoint.x - xPos) < 50    // only click
				&& abs(m_startPoint.y - yPos) < 50))
			{
				newWorldExtent = g_MapManager.GetWorldExtent();
				int nCurWidth = newWorldExtent.xmax - newWorldExtent.xmin;
				int nCurHeight = newWorldExtent.ymax - newWorldExtent.ymin;
				double fRatioX = (double)(xPos - newWorldExtent.xmin) / nCurWidth;
				double fRatioY = (double)(newWorldExtent.ymax - yPos) / nCurHeight;
				const int nScaleWidth = (const int) (nCurWidth * ZOOM_SCALE * 3);
				const int nScaleHeight = (const int) (nCurHeight * ZOOM_SCALE * 3);
				int nNewWidth = 0;
				int nNewHeight = 0;

				nNewWidth = nCurWidth - nScaleWidth;
				nNewHeight = nCurHeight - nScaleHeight;
				newWorldExtent.xmax = (int) (xPos + nNewWidth * (1 - fRatioX));
				newWorldExtent.xmin = (int) (xPos - nNewWidth * fRatioX);
				newWorldExtent.ymax = (int) (yPos + nNewHeight * fRatioY);
				newWorldExtent.ymin = (int) (yPos - nNewHeight * (1 - fRatioY));
			}
			else
			{
				newWorldExtent.xmax = xPos > m_startPoint.x ? xPos : m_startPoint.x;
				newWorldExtent.xmin = xPos > m_startPoint.x ? m_startPoint.x : xPos;
				newWorldExtent.ymax = yPos > m_startPoint.y ? yPos : m_startPoint.y;
				newWorldExtent.ymin = yPos > m_startPoint.y ? m_startPoint.y : yPos;
			}
			g_MapManager.SetWorldExtent(newWorldExtent);
			ReleaseCapture();
			Invalidate();
			return 0;
		}
		else if (g_MapManager.m_bZoomOutMode)
		{
			EXTENT newWorldExtent;
			if ((abs(m_startPoint.x - xPos) < 50    // only click
				&& abs(m_startPoint.y - yPos) < 50))
			{
				newWorldExtent = g_MapManager.GetWorldExtent();
				int nCurWidth = newWorldExtent.xmax - newWorldExtent.xmin;
				int nCurHeight = newWorldExtent.ymax - newWorldExtent.ymin;
				double fRatioX = (double)(xPos - newWorldExtent.xmin) / nCurWidth;
				double fRatioY = (double)(newWorldExtent.ymax - yPos) / nCurHeight;
				const int nScaleWidth = (const int) (nCurWidth * ZOOM_SCALE * 3);
				const int nScaleHeight = (const int) (nCurHeight * ZOOM_SCALE * 3);
				int nNewWidth = 0;
				int nNewHeight = 0;
				nNewWidth = nCurWidth + nScaleWidth;
				nNewHeight = nCurHeight + nScaleHeight;
				newWorldExtent.xmax = (int) (xPos + nNewWidth * (1 - fRatioX));
				newWorldExtent.xmin = (int) (xPos - nNewWidth * fRatioX);
				newWorldExtent.ymax = (int) (yPos + nNewHeight * fRatioY);
				newWorldExtent.ymin = (int) (yPos - nNewHeight * (1 - fRatioY));
				if (((double)newWorldExtent.Width()) / g_MapManager.m_originalExtent.Width() < 1.2)
					g_MapManager.SetWorldExtent(newWorldExtent);
				else
					g_MapManager.SetWorldExtent(g_MapManager.m_originalExtent);
			}
			else
			{
				newWorldExtent = g_MapManager.GetWorldExtent();
				int nCurWidth = newWorldExtent.xmax - newWorldExtent.xmin;
				int nCurHeight = newWorldExtent.ymax - newWorldExtent.ymin;
				double scale = ((double)newWorldExtent.Width()) / abs(xPos - m_startPoint.x) / 3;
				double fRatioX = (double)(m_startPoint.x - newWorldExtent.xmin) / nCurWidth;
				double fRatioY = (double)(newWorldExtent.ymax - m_startPoint.y) / nCurHeight;
				const int nScaleWidth = (const int) (nCurWidth * scale);
				const int nScaleHeight = (const int) (nCurHeight * scale);
				int nNewWidth = 0;
				int nNewHeight = 0;
				nNewWidth = nCurWidth + nScaleWidth;
				nNewHeight = nCurHeight + nScaleHeight;
				newWorldExtent.xmax = (int) (m_startPoint.x + nNewWidth * (1 - fRatioX));
				newWorldExtent.xmin = (int) (m_startPoint.x - nNewWidth * fRatioX);
				newWorldExtent.ymax = (int) (m_startPoint.y + nNewHeight * fRatioY);
				newWorldExtent.ymin = (int) (m_startPoint.y - nNewHeight * (1 - fRatioY));
				if (((double)newWorldExtent.Width()) / g_MapManager.m_originalExtent.Width() < 1.2)
					g_MapManager.SetWorldExtent(newWorldExtent);
				else
					g_MapManager.SetWorldExtent(g_MapManager.m_originalExtent);
			}
			ReleaseCapture();
			Invalidate();
			return 0;
		}

		int nOffsetX = xPos - m_startPoint.x;
		int nOffsetY = yPos - m_startPoint.y;

		EXTENT newWorldExtent = g_MapManager.GetWorldExtent();
		newWorldExtent.xmax -= nOffsetX;
		newWorldExtent.xmin -= nOffsetX;
		newWorldExtent.ymax -= nOffsetY;
		newWorldExtent.ymin -= nOffsetY;

		g_MapManager.SetWorldExtent(newWorldExtent);
		Invalidate();
		return 0;
	}

	LRESULT OnMouseMove(UINT nflags, CPoint point)
	{
		int xPos = point.x;
		int yPos = point.y;
		g_MapManager.ScreenToWorld(xPos, yPos);		

		if (nflags & MK_LBUTTON)
		{
			if (g_MapManager.m_bZoomInMode || g_MapManager.m_bZoomOutMode)
			{
				// Draw a rect without redraw the whole map
				// The same effect as MFC's CRectTracker
				HDC hdc = GetDC();
				CRect rect;
				rect.left = m_startScreenPoint.x;
				rect.top = m_startScreenPoint.y;
				rect.right = m_curScreenPoint.x;
				rect.bottom = m_curScreenPoint.y;

				rect.NormalizeRect();
				if (rect.left != rect.right && rect.top != rect.bottom)
					DrawFocusRect(hdc, rect);

				m_curScreenPoint.x = point.x;
				m_curScreenPoint.y = point.y;
				rect.left = m_startScreenPoint.x;
				rect.top = m_startScreenPoint.y;
				rect.right = m_curScreenPoint.x;
				rect.bottom = m_curScreenPoint.y;
				rect.NormalizeRect();
				DrawFocusRect(hdc, rect);
				return 0;
			}
			int nOffsetX = xPos - m_startPoint.x;
			int nOffsetY = yPos - m_startPoint.y;
			m_curScreenPoint.x = point.x;
			m_curScreenPoint.y = point.y;
			EXTENT newWorldExtent = g_MapManager.GetWorldExtent();
			newWorldExtent.xmax -= nOffsetX;
			newWorldExtent.xmin -= nOffsetX;
			newWorldExtent.ymax -= nOffsetY;
			newWorldExtent.ymin -= nOffsetY;

			g_MapManager.SetWorldExtent(newWorldExtent);
			Invalidate();
		}
		else if (nflags & MK_RBUTTON)
		{
			if (g_MapManager.m_bSelectMode)
			{
				m_curScreenPoint.x = point.x;
				m_curScreenPoint.y = point.y;
				g_MapManager.m_bExtentSelectMode = true;
				Invalidate();
				return 0;
			}
		}
		else
		{
			m_curScreenPoint.x = point.x;
			m_curScreenPoint.y = point.y;
			SendMessage(m_hMainFrm, WM_APP + 101, yPos, xPos);
		}
		return 0;
	}

	LRESULT OnMouseWheel(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(&point);
		int xPos = point.x;
		int yPos = point.y;
		g_MapManager.ScreenToWorld(xPos, yPos);
		EXTENT newWorldExtent = g_MapManager.GetWorldExtent();
		int nCurWidth = newWorldExtent.xmax - newWorldExtent.xmin;
		int nCurHeight = newWorldExtent.ymax - newWorldExtent.ymin;
		double fRatioX = (double)(xPos - newWorldExtent.xmin) / nCurWidth;
		double fRatioY = (double)(newWorldExtent.ymax - yPos) / nCurHeight;
		const int nScaleWidth = (const int) (nCurWidth * ZOOM_SCALE);
		const int nScaleHeight = (const int) (nCurHeight * ZOOM_SCALE);
		int nNewWidth = 0;
		int nNewHeight = 0;

		short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (zDelta > 0) // zoom in
		{
			nNewWidth = nCurWidth - nScaleWidth;
			nNewHeight = nCurHeight - nScaleHeight;
			newWorldExtent.xmax = (int) (xPos + nNewWidth * (1 - fRatioX));
			newWorldExtent.xmin = (int) (xPos - nNewWidth * fRatioX);
			newWorldExtent.ymax = (int) (yPos + nNewHeight * fRatioY);
			newWorldExtent.ymin = (int) (yPos - nNewHeight * (1 - fRatioY));
		}
		else if (zDelta < 0)
		{
			nNewWidth = nCurWidth + nScaleWidth;
			nNewHeight = nCurHeight + nScaleHeight;
			newWorldExtent.xmax = (int) (xPos + nNewWidth * (1 - fRatioX));
			newWorldExtent.xmin = (int) (xPos - nNewWidth * fRatioX);
			newWorldExtent.ymax = (int) (yPos + nNewHeight * fRatioY);
			newWorldExtent.ymin = (int) (yPos - nNewHeight * (1 - fRatioY));
			if (((double)newWorldExtent.Width()) / g_MapManager.m_originalExtent.Width() > 1.2)
			{
				g_MapManager.SetWorldExtent(g_MapManager.m_originalExtent);
				return 0;
			}
		}
		g_MapManager.SetWorldExtent(newWorldExtent);
		Invalidate();
		return 0;
	}

	void PaintID(CDCHandle& dc,
				 FeatureSet* pFeatSet,
				 const std::vector<int>& vPoints,
				 const std::vector<UINT>& vPtNum)
	{
		std::vector<POINT> vMidPoints;
		if (pFeatSet->m_eFeatureType == POINT_TYPE)
		{
			std::vector<int>::const_iterator itPt = vPoints.begin();
			for (std::vector<UINT>::const_iterator it = vPtNum.begin(); it != vPtNum.end(); ++it)
			{
				// GS may be has more than one point
				for (int i = 0; i < *it; ++i)
				{
					POINT point = {0};
					point.x = *itPt++;
					point.y = (*itPt++) - 20;
					vMidPoints.push_back(point);
				}
			}
		}
		else if (pFeatSet->m_eFeatureType == POLYLINE_TYPE)
		{
			std::vector<int>::const_iterator itPt = vPoints.begin();
			for (std::vector<UINT>::const_iterator it = vPtNum.begin(); it != vPtNum.end(); ++it)
			{
				ATLASSERT(*it > 1);
				int nDistanceX = (*(itPt + (*it - 1) * 2) - *itPt);
				int nDistanceY = (*(itPt + (*it - 1) * 2 + 1) - *(itPt + 1));
				int nMidX = *itPt + nDistanceX / 2;
				int nMidY = *(itPt + 1) + nDistanceY / 2;
				if (*it < 5)
				{
					int nLen = 0;
					int nPos = 0;
					// find longest path
					for (int i = 0; i < *it; ++i)
					{
						if (i + 1 == *it)
							break;
						int x1 = *(itPt + i * 2);
						int y1 = *(itPt + i * 2 + 1);
						int x2 = *(itPt + (i+1) * 2);
						int y2 = *(itPt + (i+1) * 2 + 1);
						int tempLen = (x1-x2) * (x1-x2) + (y1-y2) * (y1-y2);
						if (tempLen > nLen)
						{
							nLen = tempLen;
							nPos = i;
						}
					}
					POINT point = {0};
					point.x = *(itPt + (nPos) * 2) + (*(itPt + (nPos+1) * 2) - *(itPt + (nPos) * 2)) / 2;
					point.y = *(itPt + (nPos) * 2 + 1) + (*(itPt + (nPos+1) * 2 + 1) - *(itPt + (nPos) * 2 + 1)) / 2;
					vMidPoints.push_back(point);
					itPt += (*it * 2);
					continue;
				}
				for (int i = 0; i < *it; ++i)
				{
					if (abs(nDistanceX) < 100)
					{
						if (abs(*(itPt + i * 2 + 1) - nMidY) < 50)
						{
							POINT point = {0};
							point.x = *(itPt + i * 2);
							point.y = *(itPt + i * 2 + 1);
							vMidPoints.push_back(point);
							break;
						}
					}
					else if (abs(nDistanceY) < 100)
					{
						if (abs(*(itPt + i * 2) - nMidX) < 50)
						{
							POINT point = {0};
							point.x = *(itPt + i * 2);
							point.y = *(itPt + i * 2 + 1) - 30;
							vMidPoints.push_back(point);
							break;
						}
					}
					else if (abs(*(itPt + i * 2 + 1) - nMidY) < 100 || abs(*(itPt + i * 2) - nMidX) < 100)
					{
						POINT point = {0};
						point.x = *(itPt + i * 2);
						point.y = *(itPt + i * 2 + 1) - 30;
						vMidPoints.push_back(point);
						break;
					}
				}
				itPt += (*it * 2); 
			}
		}
		else if (pFeatSet->m_eFeatureType == POLYGON_TYPE)
		{
			// we select the first point, simplest solution
			if (vPoints.size() < 2)
				return;
			POINT point = {0};
			point.x = vPoints[0];
			point.y = vPoints[1] - 20;
			vMidPoints.push_back(point);
		}

		int oldMode = dc.SetBkMode(TRANSPARENT);
		int i = 0;
		for (std::vector<POINT>::iterator it = vMidPoints.begin(); it != vMidPoints.end(); ++it)
		{
			UINT featIdx = pFeatSet->m_vFeatSelected[i++];
			LPCSTR lpData = (LPCSTR)pFeatSet->GetFeatureID(featIdx);
			ExtTextOutA(dc, it->x, it->y, ETO_CLIPPED, NULL, lpData, strlen(lpData), NULL); 
		}
		dc.SetBkMode(oldMode);
	}

	/*
	 * All Map Drawing process put in this function. This is not good.
	 * NOTE: pen's size larger than 1 will dramaticly decrease the drawing performance.
	 * TODO: Need to refactor
	 */
	void DoPaint(CDCHandle dc)
	{
		CRect rc;
		GetClientRect(&rc);
		g_MapManager.SetScreenRect(rc);
		dc.FillRect(&rc, RGB(255,255,255));
		if (g_MapManager.m_bExtentSelectMode)
		{
			g_MapManager.m_bExtentSelectMode = false;
			if (!(abs(m_startScreenPoint.x - m_curScreenPoint.x) < 4 
				&& abs(m_startScreenPoint.y - m_curScreenPoint.y) < 4))
			{
				dc.Rectangle(m_startScreenPoint.x, m_startScreenPoint.y, m_curScreenPoint.x, m_curScreenPoint.y);
			}
		}

		BOOL bSucess = TRUE;
		g_MapManager.AdjustWorldExtent();
		const EXTENT worldExtent = g_MapManager.GetWorldExtent();
		std::vector<Layer*> vLayers = g_MapManager.GetAllLayers();

		const int FEAT_SIZE = 8000;
		for (size_t nLayer = 0; nLayer < vLayers.size(); ++nLayer)
		{
			Layer* pLayer = vLayers[nLayer];
			if (!pLayer->isVisable())
			{
				continue;
			}
			CPen newPen;
			newPen.CreatePen(PS_SOLID,1,pLayer->GetColor());
			dc.SelectPen(newPen);

			CBrush brush;
			brush.CreateSolidBrush(pLayer->GetColor());
			dc.SelectBrush(brush);

			FeatureSet* pFeatureSet = pLayer->GetFeatureSet();
			pFeatureSet->SetViewRect(rc);
			pFeatureSet->SetWorldExtent(worldExtent);
			if (!pFeatureSet->InView())
			{
				continue;
			}

			std::vector<int> vPoints;
			std::vector<UINT> vPtNum;
			vPoints.reserve(FEAT_SIZE * 16);
			vPtNum.reserve(FEAT_SIZE);

			while (pFeatureSet->NextFeaturesPoints(FEAT_SIZE, vPoints, vPtNum))
			{
				if (vPoints.empty())
				{
					continue;
				}
				if (pFeatureSet->m_eFeatureType == POLYLINE_TYPE)
				{
					bSucess = dc.PolyPolyline((const POINT*)&vPoints.front(), (const DWORD*)&vPtNum.front(), vPtNum.size());
				}
				else if (pFeatureSet->m_eFeatureType == POINT_TYPE)
				{
					POINT* lppoint = (POINT*)&vPoints.front();
					const DWORD* lpdwPt = (const DWORD*)&vPtNum.front();

					size_t cCount = vPtNum.size();
					int nPos = 0;
					for (UINT i = 0; i < cCount; ++i)
					{
						if (lpdwPt[i] == 1)
						{
							int x0 = lppoint[nPos].x;
							int y0 = lppoint[nPos].y;
							bSucess = dc.Rectangle(x0-3,y0-3,x0+3,y0+3);
							++nPos;
						}
						else
						{
							for (UINT j = 0; j < lpdwPt[i]; ++j)
							{
								int x0 = lppoint[nPos].x;
								int y0 = lppoint[nPos].y;
								bSucess = dc.Rectangle(x0-3,y0-3,x0+3,y0+3);
								++nPos;
							}
						}
					}
				}
				else if (pFeatureSet->m_eFeatureType == POLYGON_TYPE)
				{
					bSucess = dc.PolyPolyline((const POINT*)&vPoints.front(), (const DWORD*)&vPtNum.front(), vPtNum.size());
				}

				if (!bSucess)
				{
					MessageBox(_T("Draw shape failed"));
					break;
				}

				vPoints.clear();
				vPtNum.clear();
			}

			vPoints.clear();
			vPtNum.clear();

			// draw selected feature
			if (pFeatureSet->m_eFeatureType == POLYLINE_TYPE || pFeatureSet->m_eFeatureType == POLYGON_TYPE)
			{
				bool bDrawPoint = true;
				if (pFeatureSet->GetSelectedPoints(vPoints, vPtNum, bDrawPoint))
				{
					CPen newPenS;
					int nPenSize = 1;
					if (bDrawPoint)
						nPenSize = 2;
					newPenS.CreatePen(PS_SOLID,nPenSize,RGB(136,255,247));

					//TODO draw holes in different color
					dc.SelectPen(newPenS);
					bSucess = dc.PolyPolyline((const POINT*)&vPoints.front(), (const DWORD*)&vPtNum.front(), vPtNum.size());
					if (g_MapManager.m_bShowID)
						PaintID(dc, pFeatureSet, vPoints, vPtNum);

					if (g_MapManager.m_nCurLayerSelected != nLayer)
					{
						continue;
					}

					CPen newPen1;
					if (vPoints.size() > 3000)
						newPen1.CreatePen(PS_SOLID,nPenSize,RGB(20,216,79));
					else
						newPen1.CreatePen(PS_SOLID,nPenSize+2,RGB(20,216,79));

					HPEN oldPen = dc.SelectPen(newPen1);
					std::vector<UINT> vFeatureIdx;
					vFeatureIdx.push_back(g_MapManager.m_nCurSelected);
					if (vPtNum.size() != 1)
					{                    
						vPoints.clear();
						vPtNum.clear();
						pFeatureSet->GetFeaturesPoints(vFeatureIdx, vPoints, vPtNum, bDrawPoint);
					}

					//TODO draw holes in different color
					bSucess = dc.PolyPolyline((const POINT*)&vPoints.front(), (const DWORD*)&vPtNum.front(), vPtNum.size());
					if (!bDrawPoint)
						continue;

					POINT* lppoint = (POINT*)&vPoints.front();
					const DWORD* lpdwPt = (const DWORD*)&vPtNum.front();
					ATLASSERT(vPtNum.size() == 1);

					for (UINT j = 0; j < lpdwPt[0]; ++j)
					{
						int x0 = lppoint[j].x;
						int y0 = lppoint[j].y;
						if (j == 0)
						{
							dc.SelectPen(oldPen);
							POINT ptTriangle[3] = {0};
							ptTriangle[0].x = x0;
							ptTriangle[0].y = y0 - 6;
							ptTriangle[1].x = x0 - 6;
							ptTriangle[1].y = y0 + 6;
							ptTriangle[2].x = x0 + 6;
							ptTriangle[2].y = y0 + 6;
							dc.Polygon(ptTriangle, 3);
							dc.SelectPen(newPen1);
						}
						else
						{
							if (pFeatureSet->m_eFeatureType == POLYGON_TYPE)
								bSucess = dc.Rectangle(x0-4,y0-4,x0+4,y0+4);
							else
								bSucess = dc.Ellipse(x0-4,y0-4,x0+4,y0+4);
						}
					}
				}
			}
			else if (pFeatureSet->m_eFeatureType == POINT_TYPE)
			{
				CPen newPenS;
				newPenS.CreatePen(PS_SOLID,2,RGB(136,255,247));
				dc.SelectPen(newPenS);
				bool bDraw;
				if (pFeatureSet->GetSelectedPoints(vPoints, vPtNum, bDraw))
				{
					POINT* lppoint = (POINT*)&vPoints.front();
					const DWORD* lpdwPt = (const DWORD*)&vPtNum.front();
					int nPos = 0;
					size_t cCount = vPtNum.size();
					for (UINT i = 0; i < cCount; ++i)
					{
						for (UINT j = 0; j < lpdwPt[i]; ++j)
						{
							int x0 = lppoint[nPos].x;
							int y0 = lppoint[nPos++].y;
							bSucess = dc.Ellipse(x0-4,y0-4,x0+4,y0+4);
						}
					}
					
					if (g_MapManager.m_bShowID)
						PaintID(dc, pFeatureSet, vPoints, vPtNum);
					vPoints.clear();
					vPtNum.clear();
					if (g_MapManager.m_nCurLayerSelected != nLayer)
					{
						continue;
					}
					CPen newPen1;
					newPen1.CreatePen(PS_SOLID,3,RGB(20,216,79));
					dc.SelectPen(newPen1);
					std::vector<UINT> vFeatureIdx;
					vFeatureIdx.push_back(g_MapManager.m_nCurSelected);
					pFeatureSet->GetFeaturesPoints(vFeatureIdx, vPoints, vPtNum, bDraw);
					lppoint = (POINT*)&vPoints.front();
					lpdwPt = (const DWORD*)&vPtNum.front();
					ATLASSERT(vPtNum.size() == 1);
					for (UINT j = 0; j < lpdwPt[0]; ++j)
					{
						int x0 = lppoint[j].x;
						int y0 = lppoint[j].y;
						bSucess = dc.Ellipse(x0-4,y0-4,x0+4,y0+4);
					}
				}
			}
		} // For vLayers

		if (g_MapManager.m_bLocate)
		{
			CPen newPen;
			newPen.CreatePen(PS_SOLID, 2, RGB(16, 115, 0));
			dc.SelectPen(newPen);

			POINT centerPt = g_MapManager.m_vLocatePoint;
			POINT left;
			POINT right;
			POINT up;
			POINT down;

			int r=10;
			left.x = static_cast<LONG>(centerPt.x-r);
			left.y = static_cast<LONG>(centerPt.y);
			right.x = static_cast<LONG>(centerPt.x+r);
			right.y = static_cast<LONG>(centerPt.y);

			up.x = centerPt.x;
			up.y = centerPt.y+r;
			down.x = centerPt.x;
			down.y = centerPt.y-r;

			dc.MoveTo(left);
			dc.LineTo(right);

			dc.MoveTo(down);
			dc.LineTo(up);
		}
	}
};
