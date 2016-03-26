#include "StdAfx.h"
#include "ProvinceSelection.h"


//#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std;

namespace WBSF
{
	CProvinceSelection::CInfo CProvinceSelection::DEFAULT_LIST[NB_PROVINCES] =
	{
		{ "AB", "Alberta", "Alberta" },
		{ "BC", "British-Columbia", "British-Columbia" },
		{ "MB", "Manitoba", "Manitoba" },
		{ "NB", "New-Brunswick", "New-Brunswick" },
		{ "NL", "Newfoundland", "Newfoundland" },
		{ "NT", "Northwest Territories", "Northwest Territories" },
		{ "NS", "Nova Scotia", "Nova Scotia" },
		{ "NU", "Nunavut", "Nunavut" },
		{ "ON", "Ontario", "Ontario" },
		{ "PE", "Prince Edward Island", "Prince Edward Island" },
		{ "QC", "Quebec", "Quebec" },
		{ "SK", "Saskatchewan", "Saskatchewan" },
		{ "YT", "Yukon", "Yukon" },
	};
	
	void CProvinceSelection::UpdateString()
	{
		const StringVector NAME(IDS_STR_PROVINCES, "|;");
		ASSERT(NAME.size() == NB_PROVINCES);

		for (size_t i = 0; i < NB_PROVINCES; i++)
			DEFAULT_LIST[i].m_title = NAME[i];
	}

	size_t CProvinceSelection::GetProvince(const std::string& in, size_t t)
	{
		size_t prov = -1;
		std::string tmp(in);
		Trim(tmp);
		MakeUpper(tmp);
		for (size_t i = 0; i < NB_PROVINCES; i++)
		{
			if (tmp == DEFAULT_LIST[i][t])
			{
				prov = i;
				break;
			}
		}

		return prov;
	}


	std::string CProvinceSelection::GetAllPossibleValue(bool bAbvr, bool bName)
	{
		string str;
		for (size_t i = 0; i < NB_PROVINCES; i++)
		{
			str += i != 0 ? "|" : "";
			if (bAbvr)
				str += DEFAULT_LIST[i].m_abrv;
			if (bAbvr && bName)
				str += "=";
			if (bName)
				str += DEFAULT_LIST[i].m_title;
		}

		return str;
	}
	CProvinceSelection::CProvinceSelection(const std::string& sel)
	{
		/*static bool bInit = false;
		if (!bInit)
		{
			StringVector header(IDS_PROVINCES, "|;");
			ASSERT(header.size() == NB_PROVINCES);
			bInit = true;
			for (size_t i = 0; i < header.size(); i++)
			{
				CInfo& inf = const_cast<CInfo&>(DEFAULT_LIST[i]);
				inf.m_name = header[i];
			}
		}*/


		//Reset();
		FromString(sel);
	}

	string CProvinceSelection::GetName(size_t prov, size_t t)
	{
		ASSERT(prov >= 0 && prov < NB_PROVINCES);
		ASSERT(t >= 0 && t < NB_INFO);
		return DEFAULT_LIST[prov][t];
	}
/*
	string CProvinceSelection::GetNameBySelectionIndex(size_t index, size_t t)
	{
		ASSERT(index >= 0 && index < GetNbSelection());

		size_t prov = index2prov(index);

		return INFO[prov][t];
	}*/

	string CProvinceSelection::ToString()const
	{
		string str;
		if (!none() && !all())
		{
			for (size_t i = 0; i < NB_PROVINCES; i++)
			{
				if (at(i))
				{
					str += DEFAULT_LIST[i].m_abrv;
					str += '|';
				}
			}
		}
		return str;
	}

	ERMsg CProvinceSelection::FromString(const string& in)
	{
		ERMsg msg;

		reset();

		StringVector tmp(in, "|;");
		for (size_t i = 0; i < tmp.size(); i++)
			msg += set(tmp[i]);

		return msg;
	}

	ERMsg CProvinceSelection::set(const std::string& country)
	{
		ERMsg message;
		size_t p = GetProvince(country);
		if (p < size())
		{
			set(p);
		}
		else
		{
			message.ajoute("This province is invalid: %s" + country);
		}

		return message;
	}
	/*
	void CProvinceSelection::GetXML(CMarkup& xml)
	{
	xml.AddElem("PROVINCE_SELECTION");
	xml.AddAttrib("version", "1");
	VERIFY(xml.IntoElem());

	if( IsUsedAll() )
	{
	xml.AddElem("ITEM", INFO[ALL][0] );
	}
	else
	{
	for(size_t i=0; i<NB_PROVINCE; i++)
	{
	if( IsUsed(i) )
	{
	xml.AddElem("ITEM", INFO[i][0] );
	}
	}
	}
	VERIFY( xml.OutOfElem() );

	ASSERT( xml.IsWellFormed() );
	}

	ERMsg CProvinceSelection::SetXML(CMarkup& xml)
	{
	ERMsg msg;

	Reset();
	if( xml.FindElem("PROVINCE_SELECTION") )
	{
	VERIFY(xml.IntoElem());

	while( xml.FindElem("ITEM") )
	{
	msg+=SetUsed(xml.GetData());
	}
	}

	return msg;
	}
	*/

	//*********************************************************************
}