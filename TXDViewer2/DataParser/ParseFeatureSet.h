#ifndef _PARSE_FEATURE_SET_H
#define _PARSE_FEATURE_SET_H

const char SPLIT_ATRRIBUTE  = ',';
const char SPLIT_POINT      = ',';
const char SPLIT_FIELD      = ';';
const char SPLIT_SUB_ATRR   = '|';

enum TXD_TYPE
{
#define TYPECODE(code, str) code,
#include "TXDTypes.h"
#undef TYPECODE
};

enum DB_PARAMS
{
    DB_NAME,
    DB_HOST,
    DB_PORT,
    DB_DATABASE,
    DB_SCHEMA,
    DB_USERNAME,
    DB_PWD,
    DB_PARAMS_SIZE
};

struct ConnectDBParams
{
    std::string host;
    std::string port;
    std::string database;
    std::string schema;
    std::string table;
    std::string geomcolumn;
    std::string username;
    std::string pwd;
    std::string idcolumn;
    std::string geotype;
    std::string sql;
    bool        bComplexSQL;
};

bool IsAlphaPresent( CString& myString );
bool IsAlphaPresent(const std::string& myString );

std::string Unicode2char(CString strUnicode);
void StrGeoType2Enum(const char* strType, OGRwkbGeometryType& OGRType, GEO_TYPE& viewerType);

int ParseTXDFeatureSet(LPCTSTR szFilename, MapManager& mapManager, HWND hWnd);

int ParseTXDFeatureSetInt(LPCTSTR szFilename, MapManager& mapManager, HWND hWnd);
int ParseTXDFeatureSetDouble(LPCTSTR szFilename, MapManager& mapManager, HWND hWnd);

// Parse SHP Feature Set
int ParseSHPFeatureSet(LPCTSTR szFilename, MapManager& mapManager, HWND hWnd);

// Parse GDF Feature Set
// TODO

// Parse PostGIS Feature Set
int ParsePostGISFeatureSet(const ConnectDBParams& db_params, MapManager& mapManager, HWND hWnd);
int ParsePostGISFeatureSetSQL(const ConnectDBParams& db_params, MapManager& mapManager, HWND hWnd);
int ParsePostGISFeatureSetTable(const ConnectDBParams& db_params, MapManager& mapManager, HWND hWnd);

// More data formats ...
// TODO

#endif