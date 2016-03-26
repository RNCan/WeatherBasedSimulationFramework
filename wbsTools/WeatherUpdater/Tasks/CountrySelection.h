#pragma once

#include <bitset>
#include "Basic/ERMsg.h"
//#include "TaskBase.h"

namespace WBSF
{

	enum { NB_COUNTRIES = 284 };
	class CCountrySelection : public std::bitset < NB_COUNTRIES >
	{
	public:

		
		//enum TABRV{ UNKNOW = -1, NB_COUNTRY = 284 };
		enum TSearch{ BY_ABVR, BY_NAME };
		//enum TName{ ALL = NB_COUNTRIES, NB_TAG };


		class CInfo
		{
		public:
			
			std::string operator[](size_t i)const
			{
				ASSERT(i >= 0 && i < 2);
				return (i == 0 ? m_abrv : m_name );
			}

			std::string m_abrv;
			std::string m_name;
		};


		static const CInfo DEFAULT_LIST[NB_COUNTRIES];

		static std::string GetAllPossibleValue(bool bAbvr = true, bool bName = true);
		static size_t GetCountry(const std::string& in, size_t t = BY_ABVR);//by abr

		CCountrySelection(const std::string& in = "");

		std::string ToString()const;
		ERMsg FromString(const std::string& in);

		static std::string GetName(size_t i, size_t t = BY_ABVR);

		using std::bitset<NB_COUNTRIES>::at;
		bool at(const std::string& in)const
		{
			size_t p = GetCountry(in);
			return p < size() ? at(p) : false;
		}

		using std::bitset<NB_COUNTRIES>::set;
		ERMsg set(const std::string& in);
	};
}