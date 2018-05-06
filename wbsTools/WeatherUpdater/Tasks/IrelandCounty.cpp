#include "StdAfx.h"
#include "IrelandCounty.h"


//#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std;

namespace WBSF
{
	const char* CIrelandCounty::COUNTIES_NAME[NB_COUNTIES] =
	{  
		"Cork","Kerry","Waterford","Tipperary","Limerick","Clare","Wexford","Kilkenny","Wicklow","Carlow",
		"Laois","Offaly","Kildare","Dublin","Meath","Westmeath","Louth","Longford","Galway","Roscommon",
		"Mayo","Sligo","Leitrim","Cavan","Monaghan","Donegal"	
	};
	

	size_t CIrelandCounty::GetCounty(std::string in, bool bNo)
	{
		size_t county = NOT_INIT;

		Trim(in);
		if (bNo)
		{
			county = WBSF::as<size_t>(in)-1;
			if (county >= NB_COUNTIES)
				county = NOT_INIT;
		}
		else
		{
			for (size_t i = 0; i < NB_COUNTIES&&county==NOT_INIT; i++)
			{
				if (WBSF::IsEqualNoCase(in,COUNTIES_NAME[i]))
					county = i;
			}
		}


		return county;
	}


	std::string CIrelandCounty::GetAllPossibleValue(bool bNo, bool bName)
	{
		string str;
		for (size_t i = 0; i < NB_COUNTIES; i++)
		{
			str += i != 0 ? "|" : "";
			if (bNo)
				str += ToString(i);
			if (bNo && bName)
				str += "=";
			if (bName)
				str += COUNTIES_NAME[i];
		}

		return str;
	}
	CIrelandCounty::CIrelandCounty(const std::string& sel)
	{
		FromString(sel);
	}

	string CIrelandCounty::GetName(size_t county, bool bNo)
	{
		ASSERT(county < NB_COUNTIES);
		
		return bNo? ToString(county):COUNTIES_NAME[county];
	}

	string CIrelandCounty::ToString(size_t county)
	{
		return FormatA("%02ld", county + 1);
	}
	string CIrelandCounty::ToString()const
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
			for (size_t i = 0; i < NB_COUNTIES; i++)
			{
				if (test(i))
				{
					str += ToString(i) + "|";
				}
			}
		}
		return str;
	}

	ERMsg CIrelandCounty::FromString(const string& in)
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

	ERMsg CIrelandCounty::set(const std::string& in)
	{
		ERMsg message;
		size_t p = GetCounty(in);
		if (p < size())
		{
			set(p);
		}
		else
		{
			message.ajoute("This Ireland county is invalid: %s" + in);
		}

		return message;
	}
	

	//*********************************************************************
}