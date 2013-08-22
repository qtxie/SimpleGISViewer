// ********************************************************************************************************
// Description: 
//     PostGIS reader.
//     Now it is too simple, need to enhance if have time.
//
// Created 2013/1/13
//
// ********************************************************************************************************

#ifndef _POSTGIS_READER_H_
#define _POSTGIS_READER_H_

#include "Feature.h"
#include "ErrorCode.h"
#include "IDataReader.h"

class PostGISReader : public IDataReader
{
public:
	PostGISReader() :  m_nCurIdx(-1)
	{}

	explicit PostGISReader(ConnectDBParams params) : m_nCurIdx(-1), m_conn(NULL)
	{
		m_bOpened = false;
		m_params = params;
	}

	~PostGISReader()
	{
		if (m_conn != NULL)
		{
			/* close the portal ... we don't bother to check for errors ... */
			PGresult* res = PQexec(m_conn, "CLOSE myportal");
			PQclear(res);

			/* end the transaction */
			res = PQexec(m_conn, "END");
			PQclear(res);
			PQfinish(m_conn);
		}
	}

	int Open(LPCTSTR lpstrFileName, UINT mode)
	{
		return VIEWER_OK;
	}

	virtual int Open(UINT mode)
	{
		if (m_bOpened)
			return VIEWER_OK;
		const char* fmt_Cursor = "DECLARE myportal SCROLL CURSOR FOR SELECT *,ST_ASTEXT(\"%s\") AS geom from %s.\"%s\"";
		const char* fmt_CursorSQL = "DECLARE myportal SCROLL CURSOR FOR SELECT *,ST_ASTEXT(\"%s\") AS geom from %s.\"%s\" %s";
		const char* fmt_CursorComplexSQL = "DECLARE myportal SCROLL CURSOR FOR %s";
		const char* fmt_conninfo = "%s = '%s' port = '%s' \
								dbname = '%s' user = '%s' \
								password = '%s' connect_timeout = '6'";
		char conninfo[256] = {0};
		if (IsAlphaPresent(m_params.host))
		{
			sprintf(conninfo, fmt_conninfo, "host", m_params.host.c_str(), m_params.port.c_str(), 
					m_params.database.c_str(), m_params.username.c_str(), m_params.pwd.c_str());
		}
		else
		{
			sprintf(conninfo, fmt_conninfo, "hostaddr", m_params.host.c_str(), m_params.port.c_str(), 
					m_params.database.c_str(), m_params.username.c_str(), m_params.pwd.c_str());
		}
		m_conn = PQconnectdb(conninfo);
		if (PQstatus(m_conn) != CONNECTION_OK) 
		{
			PQfinish(m_conn);
			m_conn = NULL;
			return VIEWER_OPEN_FILE_FAILED;
		}

		/* Start a transaction block */
		PGresult* res = PQexec(m_conn, "BEGIN");
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			PQclear(res);
			PQfinish(m_conn);
			m_conn = NULL;
			return VIEWER_OPEN_FILE_FAILED;
		}
		PQclear(res);

		if (m_params.sql.empty())
		{
			sprintf(conninfo, fmt_Cursor, m_params.geomcolumn.c_str(), m_params.schema.c_str(), m_params.table.c_str());
		}
		else if (m_params.bComplexSQL)
		{
			// query all columns
			std::string temp = m_params.sql;
			std::transform(temp.begin(), temp.end(), temp.begin(), std::tolower);
			std::string::size_type idxSelect = temp.find("select");
			std::string::size_type idxFrom = temp.find("from");
			if (idxSelect == std::string::npos || idxFrom == std::string::npos)
			{
				sprintf(conninfo, fmt_CursorComplexSQL, m_params.sql.c_str());
			}
			else
			{
				idxSelect += strlen("select") + 1;
				std::string newSQL = m_params.sql;
				newSQL.replace(idxSelect, idxFrom-idxSelect-1, "*");
				sprintf(conninfo, fmt_CursorComplexSQL, newSQL.c_str());
			}
		}
		else
		{
			sprintf(conninfo, fmt_CursorSQL, m_params.geomcolumn.c_str(), m_params.schema.c_str(), 
					m_params.table.c_str(), m_params.sql.c_str());
		}
		res = PQexec(m_conn, conninfo);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			PQclear(res);
			PQfinish(m_conn);
			m_conn = NULL;
			return VIEWER_OPEN_FILE_FAILED;
		}
		PQclear(res);

		m_bOpened = true;
		return VIEWER_OK;
	}

	virtual void Close()
	{
		if (m_conn != NULL)
		{
			/* close the portal ... we don't bother to check for errors ... */
			PGresult* res = PQexec(m_conn, "CLOSE myportal");
			PQclear(res);

			/* end the transaction */
			res = PQexec(m_conn, "END");
			PQclear(res);
			PQfinish(m_conn);
			m_conn = NULL;
		}
		m_bOpened = false;
	}

	virtual void* GetFeatureID(size_t Idx)
	{
		return NULL;
	}

	virtual bool GetAllFeaturesID(std::vector<UINT64>& vID)
	{
		return false;
	}

	virtual void* GetData(size_t Idx)
	{
		if (VIEWER_OK != Open(0))
		{
			return NULL;
		}

		const char* fmt_getRecord = "FETCH ABSOLUTE %Iu in myportal";
		char conninfo[64] = {0};
		sprintf(conninfo, fmt_getRecord, (Idx+1));
		PGresult* res = PQexec(m_conn, conninfo);
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			PQclear(res);
			return NULL;
		}

		int nFields = PQnfields(res);
		m_strData.clear();

		m_strData += PQfname(res, nFields - 1);
		m_strData += ": ";
		m_strData += PQgetvalue(res, 0, nFields - 1);
		m_strData += "\r\n";
		for (int i = 0; i < nFields - 1; ++i)
		{
			std::string strFieldName = PQfname(res, i);
			if (strFieldName == m_params.geomcolumn)
				continue;
			m_strData.append(strFieldName);
			m_strData.append(": ");
			char *pValue = PQgetvalue(res, 0, i);
			if (pValue == NULL)
				m_strData.append("NULL");
			else
				m_strData.append(pValue);
			m_strData.append("\r\n");
		}
		PQclear(res);
		return (void*)m_strData.c_str();  
	}

private:
	std::string   m_strData;
	std::string   m_strID;
	PGconn*       m_conn;
	int        m_nCurIdx;
	ConnectDBParams m_params;
};

#endif