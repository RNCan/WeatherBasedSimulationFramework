#pragma once
#include <bitset>
#include "Basic/ERMsg.h"
#include "Basic/UtilStd.h"

namespace WBSF
{

	//*********************************************************************
	enum { NB_IRELAND_COUNTIES=26 };
	class CIrelandCounty : public std::bitset < NB_IRELAND_COUNTIES >
	{
	public:

		enum TCounty{ CORK, KERRY, WATERFORD, TIPPERARY, LIMERICK, CLARE, WEXFORD, KILKENNY, WICKLOW, CARLOW, LAOIS, OFFALY, KILDARE, DUBLIN, MEATH, WESTMEATH, LOUTH, LONGFORD, GALWAY, ROSCOMMON, MAYO, SLIGO, LEITRIM, CAVAN, MONAGHAN, DONEGAL, NB_COUNTIES};

		static const char* COUNTIES_NAME[NB_COUNTIES];
		static std::string GetAllPossibleValue(bool bNo = true, bool bName = true);
		static size_t GetCounty(std::string in, bool bNo = true);
		static std::string GetName(size_t prov, bool bNo = true);
		

		CIrelandCounty(const std::string& sel ="");

		std::string ToString()const;
		ERMsg FromString(const std::string&);

		bool at(const std::string& in)const
		{
			if (none())
				return false;

			if (all())
				return true;


			size_t p = GetCounty(in);
			return p < size() ? test(p) : false;
		}

		using std::bitset<NB_COUNTIES>::set;
		ERMsg set(const std::string& in);

		static std::string ToString(size_t county);

	};
}

