// ReadTest.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <tchar.h>
#include <vector>
#include <windows.h>
#include <libpq-fe.h>
#include <ogrsf_frmts.h>

#define BUFFER_SIZE 1024 * 1024 * 8
LPCTSTR szFilename = _T("E:\\2012TA\\48401.bdr");
const char cEND = '\0';

/*
** Decode a hex character.
*/
static unsigned char msPostGISHexDecodeChar[256] = {
  /* not Hex characters */
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  /* 0-9 */
  0,1,2,3,4,5,6,7,8,9,
  /* not Hex characters */
  64,64,64,64,64,64,64,
  /* A-F */
  10,11,12,13,14,15,
  /* not Hex characters */
  64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,
  /* a-f */
  10,11,12,13,14,15,
  64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  /* not Hex characters (upper 128 characters) */
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
};

/*
** Decode hex string "src" (null terminated)
** into "dest" (not null terminated).
** Returns length of decoded array or 0 on failure.
*/
int msPostGISHexDecode(unsigned char *dest, const char *src, int srclen)
{

  if (src && *src && (srclen % 2 == 0) ) {

	unsigned char *p = dest;
	int i;

	for ( i=0; i<srclen; i+=2 ) {
	  register unsigned char b1=0, b2=0;
	  register unsigned char c1 = src[i];
	  register unsigned char c2 = src[i + 1];

	  b1 = msPostGISHexDecodeChar[c1];
	  b2 = msPostGISHexDecodeChar[c2];

	  *p++ = (b1 << 4) | b2;

	}
	return(p-dest);
  }
  return 0;
}

// ReadFile
BOOL UseReadFile()
{
	DWORD dwStart = GetTickCount();
	DWORD dwEnd = dwStart;

	DWORD dwBytesRead = 0;
	HANDLE hFile = CreateFile(
	szFilename,											// name of the read
	GENERIC_READ,                                       // open for reading
	FILE_SHARE_READ,                                    // share read
	NULL,                                               // default security
	OPEN_EXISTING,                                      // open exist file only
	FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,  // normal file, sequential scan
	NULL);                                              // no attr. template

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return FALSE;
	}

	/* Need free part, all resource in this part must be clear if ERROR occur */
	char* pReadBuffer = (char*)malloc(BUFFER_SIZE + 1);
	pReadBuffer[BUFFER_SIZE] = cEND;
	/* end need free */
	BOOL bResult = ReadFile(hFile, pReadBuffer, BUFFER_SIZE, &dwBytesRead, NULL);
	if (!bResult)
	{
		free(pReadBuffer);
		CloseHandle(hFile);
		return FALSE;
	}

	while (dwBytesRead)		// not reach EOF
	{
		DWORD i = 0;
		while (pReadBuffer[i] != cEND)
		{
			++i;
		}
		bResult = ReadFile(hFile, pReadBuffer, BUFFER_SIZE, &dwBytesRead, NULL);
		if (!bResult)
		{
			free(pReadBuffer);
			return FALSE;
		}
		else
		{
			//pReadBuffer[dwBytesRead] = cEND;
		}
	}

	CloseHandle(hFile);
	dwEnd = GetTickCount();

	_tprintf(_T("ReadFile %d\n"), dwEnd - dwStart);

	return TRUE;
}

// MapView
BOOL UseMapView()
{
	DWORD dwMapSize = 1024 * 1024 * 16;
	DWORD dwStart = GetTickCount();
	DWORD dwEnd = dwStart;

	DWORD dwBytesRead = 0;
	HANDLE hFile = CreateFile(
	szFilename,											// name of the read
	GENERIC_READ,                                       // open for reading
	FILE_SHARE_READ,                                    // share read
	NULL,                                               // default security
	OPEN_EXISTING,                                      // open exist file only
	FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,  // normal file, sequential scan
	NULL);                                              // no attr. template

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return FALSE;
	}

	HANDLE fileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (fileMap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	DWORD dwFileSize = GetFileSize(hFile, 0L);

	DWORD i = 0;
	int nPart = 0;
	int end = 0;;
	while (i < dwFileSize)
	{
		if ((dwFileSize - i) < dwMapSize)
		{
			dwMapSize = dwFileSize - i;
		}
		char* pFileChar = (char*)MapViewOfFile(fileMap, FILE_MAP_READ, 0, i, dwMapSize);
		++nPart;
		i += dwMapSize;
		if (pFileChar == NULL)
		{
			_tprintf(_T("Error map file\n"));
			return FALSE;
		}

		DWORD nStart = 0;
		while (nStart < dwMapSize)
		{
			if (pFileChar[nStart++] == cEND)
			{
				++end;
			}
			else
			{
				++end;
			}
		}
		UnmapViewOfFile(pFileChar);
	}

	CloseHandle(fileMap);
	CloseHandle(hFile);

	dwEnd = GetTickCount();
	
	_tprintf(_T("MapFile %d, part: %d\n"), dwEnd - dwStart, nPart);
	_tprintf(_T("MapFile size %u: read size %u\n"), dwFileSize, end);

	return TRUE;

}

void UseVector()
{
	DWORD dwStart = GetTickCount();
	DWORD dwEnd = dwStart;

	std::vector<size_t> vTest;
	std::vector<size_t> vTest2;
	vTest.reserve(1024*1024*2);
	//vTest2.reserve(1024*1024*2);
	for (size_t i = 0; i < 1024*1024*10; ++i)
	{
		vTest.push_back(i);
		//vTest2.push_back(++i);
	}
	dwEnd = GetTickCount();

	size_t nSize = vTest.size();
	size_t* pV = &vTest.front();
	for (size_t i = 0; i < nSize; ++i)
	{
		if (pV[i] != vTest[i])
		{
			_tprintf(_T("%d, %d, %d"), i, pV[i], vTest[i]);
			break;
		}
		if (i == 1024*1024*82)
			_tprintf(_T("%d, %d, %d"), i, pV[i], vTest[i]);


	}
	_tprintf(_T("vector %d\n"), dwEnd - dwStart);
	getchar();
}

void UseMemBlock()
{
	DWORD dwStart = GetTickCount();
	DWORD dwEnd = dwStart;
	std::vector<size_t*> vPP;
	size_t* pBlock = NULL;
	vPP.push_back(pBlock);
	pBlock = (size_t*)malloc(1024*1024 * sizeof(size_t));
	int j = 0;
	for (int i = 0; i < 1024*1024*100; ++i)
	{
		if (j == 1024*1024)
		{
			pBlock = (size_t*)malloc(1024*1024 * sizeof(size_t));
			vPP.push_back(pBlock);
			j = 0;
			pBlock[j++] = i;
			continue;
		}
		pBlock[j++] = i;
	}
	dwEnd = GetTickCount();
	_tprintf(_T("vector %d\n"), dwEnd - dwStart);
	getchar();
	for (int i = 0; i < vPP.size(); ++i)
	{
		free(vPP[i]);
	}
}

static void exit_nicely(PGconn *conn)
{
	PQfinish(conn);
	exit(1);
}

void ReadWKB()
{
	OGRSpatialReference osr;
	OGRGeometry *geom = NULL;
	char *wkb = "\001\002\000\000\000\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\360?\000\000\000\000\000\000\000\000\000\000\000\000\000\000\360?\000\000\000\000\000\000\360?";

	// Parse WKB
	OGRErr err = OGRGeometryFactory::createFromWkb((unsigned char*)wkb, &osr, &geom);
	if (err != OGRERR_NONE){
		// process error, like emit signal
	}
	 
	// Analyse geometry by type and process them as you wish
	OGRwkbGeometryType type = wkbFlatten(geom->getGeometryType());
	switch(type) 
	{
		case wkbLineString: 
		{
			OGRLineString *poRing = (OGRLineString*)geom;
	 
			// Access line string nodes for example :
			int numNode = poRing->getNumPoints();
			OGRPoint p;
			for(int i = 0;  i < numNode;  i++) 
			{
				poRing->getPoint(i, &p);
				printf("%f, %f", p.getX(), p.getY());
			}
			break;
		}
		case wkbMultiLineString:
		{
			OGRGeometryCollection  *poCol = (OGRGeometryCollection*) geom;
			int numCol = poCol->getNumGeometries();
			//for(int i=0; i<numCol; i++) {
			//     //Access line length for example :
			//    printf("%f, %f", p.getX(), p.getY());

			//}
			break;
		}
		case wkbMultiPolygon:
		{
			OGRGeometryCollection *poCol = (OGRGeometryCollection*) geom;
			int numCol = poCol->getNumGeometries();
			for(int i=0; i<numCol; i++) 
			{
				OGRPolygon *poPolygon = (OGRPolygon*)poCol->getGeometryRef(i);
				OGRLinearRing *pExterior = poPolygon->getExteriorRing();

				int numNode = pExterior->getNumPoints();
				OGRPoint p;
				for (int j = 0; j < numNode; ++j)
				{
					pExterior->getPoint(i, &p);
					int x = p.getX();
					int y = p.getY();
				}
			}
			break;
		}
		default:
			printf("jkjjl");
			// process error, like emit signal
	}
	 
	// Clean-up
	OGRGeometryFactory::destroyGeometry(geom);
}
int ReadPostGIS()
{
	PGconn     *conn;
	PGresult   *res;
	int        nFields;
	int        i, j;
	const char *conninfo = "host = 'hqd-mapdev24-02' port = '5432' \
						   dbname = 'OSM_ODBL' user = 'postgres' \
						   password = 'postgres' connect_timeout = '10'";

	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) != CONNECTION_OK) 
	{
		fprintf(stderr, "Connection to database failed: %s",
				PQerrorMessage(conn));
		exit_nicely(conn);
	}

	/* Start a transaction block */
	res = PQexec(conn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	/*
	 * Should PQclear PGresult whenever it is no longer needed to avoid memory
	 * leaks
	 */
	PQclear(res);

	// Geo Type
	res = PQexec(conn, "select GeometryType(geom) from fraf11a8 LIMIT 1");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "DECLARE CURSOR failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	char* pType = PQgetvalue(res, 0, 0);
	PQclear(res);

	// Test if has column 'id'
	res = PQexec(conn, "SELECT column_name FROM information_schema.columns WHERE table_name='fraf11a8' and column_name='id'");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "DECLARE CURSOR failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	char* pID = PQgetvalue(res, 0, 0);
	PQclear(res);


	res = PQexec(conn, "select SUM(ST_NPoints(geom)) from fraf11a8");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "DECLARE CURSOR failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	char* pSize = PQgetvalue(res, 0, 0);
	PQclear(res);

	res = PQexec(conn, "select count(id) from fraf11a8");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "DECLARE CURSOR failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	char* Size = PQgetvalue(res, 0, 0);
	PQclear(res);

	/*
	 * Fetch rows from pg_database, the system catalog of databases
	 */
	res = PQexec(conn, "DECLARE myportal CURSOR FOR select id, encode(ST_AsBinary(ST_Force_2D(geom)), 'hex') from fraf11a8");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "DECLARE CURSOR failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

	res = PQexec(conn, "FETCH FORWARD 50 in myportal");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "FETCH ALL failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	/* first, print out the attribute names */
	nFields = PQnfields(res);
	//for (i = 0; i < nFields; i++)
	//    printf("%-15s", PQfname(res, i));
	//printf("\n\n");

	int nGeom = nFields - 1;
	OGRSpatialReference osr;
	OGRGeometry *geom = NULL;
	int wkbstrlen = 0;
	int result = 0;
	unsigned char *wkb = NULL;

	char *wkbstr = PQgetvalue(res, 0, nGeom);
	wkbstrlen = PQgetlength(res, 0, nGeom);
	wkb = (unsigned char*)calloc(wkbstrlen, sizeof(char));
	result = msPostGISHexDecode(wkb, wkbstr, wkbstrlen);
	if( ! result ) 
	{
		free(wkb);
	}

	OGRMultiPolygon polygon;
	polygon.importFromWkb(wkb);
	int polygonIn = polygon.getNumGeometries();
	OGRPolygon* pPP = (OGRPolygon*)polygon.getGeometryRef(0);
	OGRLinearRing *pExterior = pPP->getExteriorRing();

	int numNode = pExterior->getNumPoints();
	OGRPoint p;
	for (int j = 0; j < numNode; ++j)
	{
		pExterior->getPoint(j, &p);
		int x = p.getX();
		int y = p.getY();
	}
	
	// Parse WKB
	OGRErr err = OGRGeometryFactory::createFromWkb(wkb, &osr, &geom);
	if (err != OGRERR_NONE){
		// process error, like emit signal
	}
	 
	// Analyse geometry by type and process them as you wish
	OGRwkbGeometryType type = wkbFlatten(geom->getGeometryType());
	switch(type) 
	{
		case wkbLineString: 
		{
			OGRLineString *poRing = (OGRLineString*)geom;
	 
			// Access line string nodes for example :
			int numNode = poRing->getNumPoints();
			OGRPoint p;
			for(int i = 0;  i < numNode;  i++) 
			{
				poRing->getPoint(i, &p);
				printf("%f, %f", p.getX(), p.getY());
			}
			break;
		}
		case wkbPolygon:
		{
			OGRPolygon *poRing = (OGRPolygon*)geom;
	 
			OGRLinearRing *pExterior = poRing->getExteriorRing();

			int numNode = pExterior->getNumPoints();
			OGRPoint p;
			for (int j = 0; j < numNode; ++j)
			{
				pExterior->getPoint(j, &p);
				int x = p.getX();
				int y = p.getY();
			}
			break;
		}
		case wkbMultiLineString:
		{
			OGRGeometryCollection  *poCol = (OGRGeometryCollection*) geom;
			int numCol = poCol->getNumGeometries();
			//for(int i=0; i<numCol; i++) {
			//     //Access line length for example :
			//    printf("%f, %f", p.getX(), p.getY());

			//}
			break;
		}
		case wkbMultiPolygon:
		{
			OGRGeometryCollection *poCol = (OGRGeometryCollection*) geom;
			int numCol = poCol->getNumGeometries();
			for(int i=0; i<numCol; i++) 
			{
				OGRPolygon *poPolygon = (OGRPolygon*)poCol->getGeometryRef(i);
				OGRLinearRing *pExterior = poPolygon->getExteriorRing();

				int numNode = pExterior->getNumPoints();
				OGRPoint p;
				for (int j = 0; j < numNode; ++j)
				{
					pExterior->getPoint(j, &p);
					int x = p.getX();
					int y = p.getY();
				}
			}
			break;
		}
		default:
			printf("jkjjl");
			// process error, like emit signal
	}
	 
	// Clean-up
	OGRGeometryFactory::destroyGeometry(geom);

	///* next, print out the rows */
	//for (i = 0; i < PQntuples(res); i++)
	//{
	//    for (j = 0; j < nFields; j++)
	//        printf("%-15s", PQgetvalue(res, i, j));
	//    printf("\n");
	//}

	PQclear(res);

	/* close the portal ... we don't bother to check for errors ... */
	res = PQexec(conn, "CLOSE myportal");
	PQclear(res);

	/* end the transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	/* close the connection to the database and cleanup */
	PQfinish(conn);

	return 0;
}
enum Test {
	a = 0,
	b = 1,
};
int _tmain(int argc, _TCHAR* argv[])
{
	//UseMapView();

	//UseReadFile();
	//UseVector();
	//UseMemBlock();
	char* strtest = "192.168.1.108";
	char* str123 = "123";

	char strInfo[256];
	sprintf(strInfo, 
			"hostaddr = '%s' port = '%s' \
			 dbname = 'template_postgis_20' user = 'postgres' \
			 password = 'mappna' connect_timeout = '10'",
			 strtest,
			 str123);
	int ii = strlen(strInfo);
	Test eC = a;
	eC = (Test)(b + 1);
	int c = eC;
	ReadPostGIS();
	//ReadWKB();
	return 0;
}