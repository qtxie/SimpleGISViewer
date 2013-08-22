// ********************************************************************************************************
// Description: 
//     A collection of features.
//
// Created 2013/1/13
//
// ********************************************************************************************************

#include "stdafx.h"
#include "TXDFeatureSet.h"

const char* TXDFeatureSet::RF_FIELD[RF_FIELD_SIZE] = {
			("TYPE : "), ("ID     : "), ("PTs  : ..."), ("FC   : "), ("DF   : "),
			("RT   : "), ("RST : "), ("SP   : "),    ("LC   : "), ("ON   : "), 
			("HN   : "), ("AA   : "), ("BD   : "),    ("ATTR : "), ("SV   : "),
			("CS   : "), ("MS   : "), ("TI   : "),    ("BP   : "), ("TZ   : "),
			("LN   : "), ("22   : "), ("23   : "),    ("24   : "), ("25   : "),
			("26   : "), ("27   : "), ("28   : "),    ("29   : "), ("30   : ")
};

TXDFeatureSet::TXDFeatureSet()
{}

TXDFeatureSet::~TXDFeatureSet()
{}

void* TXDFeatureSet::GetRawData(size_t Idx)
{
	UINT i = m_vFeatSelected[Idx];
	Feature feat = m_vFeatures[i];
	return m_pDataReader->GetData(feat.nAttributeOffset);
}

void* TXDFeatureSet::GetData(size_t Idx)
{
	UINT i = m_vFeatSelected[Idx];
	Feature feat = m_vFeatures[i];

	char* pData = (char*)m_pDataReader->GetData(feat.nAttributeOffset);
	if (pData == NULL)
		return NULL;
	std::string strData(pData);
	strtk::std_string::token_list_type token_list;
	strtk::single_delimiter_predicate<std::string::value_type> predicate(';');
	strtk::split(predicate,strData,std::back_inserter(token_list));
	strtk::std_string::token_list_type::iterator itr = token_list.begin();
	strtk::std_string::token_list_type::iterator itPoint = itr;
	std::ostringstream ss;
	if (m_eFeatureType != POLYGON_TYPE)
	    ss << strData << "\r\n\r\n";
	i = 0;
	std::ostringstream ssType;
	ssType << (*itr);
	if (ssType.str() == "RF")
	{
		while (token_list.end() != itr)
		{
			if (i == 2)
			{
				itPoint = itr++;
				ss << RF_FIELD[i++] << "\r\n";
				continue;
			}
			ss << RF_FIELD[i++] << (*itr) << "\r\n";
			++itr;
		}
	}
	else
	{
		while (token_list.end() != itr)
		{
			if (i == 2)
			{
				itPoint = itr++;
				ss << i++ << ":  " << "\r\n";
				continue;
			}
			ss << i++ << ":  " << (*itr) << "\r\n";
			++itr;
		}
	}

	ss << "\r\nPoints:\r\n";
	strtk::std_string::token_list_type points_list;
	strtk::std_string::token_list_type polygon_list;
	strtk::single_delimiter_predicate<std::string::value_type> pred(',');
	strtk::single_delimiter_predicate<std::string::value_type> subAttr('|');
	strtk::split(subAttr,*itPoint,std::back_inserter(polygon_list));
	strtk::std_string::token_list_type::iterator iter = polygon_list.begin();
	ss.precision(5);
	ss.setf(std::ios::fixed);
	int nHole = 1;
	bool bInteger = m_pDataReader->m_bInteger;
	while (polygon_list.end() != iter)
	{
		points_list.clear();
		strtk::split(pred, *iter, std::back_inserter(points_list));
		strtk::std_string::token_list_type::iterator iterP = points_list.begin();
		while (points_list.end() != iterP)
		{
			if (bInteger)
			{
				int p1, p2;
				strtk::string_to_type_converter((*iterP), p1); 
				++iterP;
				strtk::string_to_type_converter((*iterP), p2);
				++iterP;
				ss << p1 / 100000.0 << ", " << p2 / 100000.0 << "\r\n";
			}
			else
			{
				double p1, p2;
				strtk::string_to_type_converter((*iterP), p1); 
				++iterP;
				strtk::string_to_type_converter((*iterP), p2);
				++iterP;
				ss << p1 << ", " << p2 << "\r\n";
			}
		}
		++iter;
		if (polygon_list.end() != iter)
			ss << "Hole " << nHole++ << "-----------------------------\r\n";
	}
	m_strData = ss.str();
	return (void*)m_strData.c_str();
}

void* TXDFeatureSet::GetFeatureID(size_t Idx)
{
	strtk::type_to_string(m_vID[Idx], m_strID);
	return (void*)m_strID.c_str();
}	


