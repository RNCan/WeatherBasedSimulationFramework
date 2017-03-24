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
		FromString(sel);
	}

	string CProvinceSelection::GetName(size_t prov, size_t t)
	{
		ASSERT(prov >= 0 && prov < NB_PROVINCES);
		ASSERT(t >= 0 && t < NB_INFO);
		return DEFAULT_LIST[prov][t];
	}

	string CProvinceSelection::ToString()const
	{
		string str;

		
		if (all())
		{
		}
		else if (none())
		{
			str = "----";
		}
		else
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

		if (IsEqual(in, "----"))
		{ 
		}
		else if (in.empty())
		{
			set();
		}
		else
		{
			StringVector tmp(in, "|;");
			for (size_t i = 0; i < tmp.size(); i++)
				msg += set(tmp[i]);
		}

		return msg;
	}

	ERMsg CProvinceSelection::set(const std::string& in)
	{
		ERMsg message;
		size_t p = GetProvince(in);
		if (p < size())
		{
			set(p);
		}
		else
		{
			message.ajoute("This province is invalid: %s" + in);
		}

		return message;
	}
	

	//*********************************************************************
}