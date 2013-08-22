// ********************************************************************************************************
// Description: 
//     We use a lightweight feature structure, only contain offset of the data.
//     All actual geodata (points) store in a continuous memory.
//
// ********************************************************************************************************

#ifndef _FEATURE_H
#define _FEATURE_H

enum GEO_TYPE 
{
	POINT_TYPE,
	POLYLINE_TYPE,
	POLYGON_TYPE,
	GEO_UNKNOWN_TYPE
};

typedef struct tagCompactEXTENT
{
	short  xmax;
	short  xmin;
	short  ymax;
	short  ymin;
} CompactEXTENT, *PCompactEXTENT;

class EXTENT
{
public:
	EXTENT() : xmax(-18000000), xmin(18000000), ymax(-9000000), ymin(9000000)
	{
	}
	
	EXTENT(int xMax, int xMin, int yMax, int yMin) :
		xmax(xMax), xmin(xMin), ymax(yMax), ymin(yMin)
	{
	}

	EXTENT(CompactEXTENT cExtent, int nScale)
	{
		xmax = cExtent.xmax * nScale;
		xmin = cExtent.xmin * nScale;
		ymax = cExtent.ymax * nScale;
		ymin = cExtent.ymin * nScale;
	}

	void Reset()
	{
		xmax = -18000000;
		xmin = 18000000;
		ymax = -9000000;
		ymin = 9000000;
	}
	bool operator== (const EXTENT other) const
	{
		return other.xmax == xmax && other.xmin == xmin && other.ymax == ymax && other.ymin == ymin;
	}

	EXTENT operator* (double scale)
	{
		int halfWidth = (Width() / 2) == 0 ? 1 : Width() / 2;
		int halfHeight = (Height() / 2) == 0 ? 1 : Height() / 2;
		int centerX = xmin + halfWidth;
		int centerY = ymin + halfHeight;
		return EXTENT(centerX + halfWidth*scale, centerX - halfWidth*scale, 
					  centerY+halfHeight*scale, centerY-halfHeight*scale);
	}

	void Reduce(int nOffset)
	{
		xmax -= nOffset;
		xmin += nOffset;
		ymax -= nOffset;
		ymin += nOffset;
	}

	bool Include(CPoint point) const
	{
		return point.x >= xmin && point.x <= xmax && point.y >= ymin && point.y <= ymax;
	}

	bool Intersect(const EXTENT& extent) const
	{
		return !( extent.xmin > xmax || extent.xmax < xmin || extent.ymax < ymin || extent.ymin > ymax);
	}

	int Width()
	{
		return xmax - xmin;
	}

	int Height()
	{
		return ymax - ymin;
	}

public:
	int  xmax;
	int  xmin;
	int  ymax;
	int  ymin;
};

struct Feature
{
	unsigned int		 nCoordOffset;
	size_t				 nAttributeOffset;
}; 

inline double DistancePtToPt(int aX, int aY, int bX, int bY)
{
	int dx, dy;
	dx = aX - bX;
	dy = aY - bY;
	return sqrt((double)(dx*dx + dy*dy));
}

#endif