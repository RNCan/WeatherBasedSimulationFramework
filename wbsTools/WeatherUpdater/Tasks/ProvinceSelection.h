#pragma once
#include <bitset>
#include "Basic/ERMsg.h"
#include "Basic/UtilStd.h"

namespace WBSF
{

	//*********************************************************************
	enum { NB_PROVINCES=13 };
	class CProvinceSelection : public std::bitset < NB_PROVINCES >
	{
	public:

		enum TProvince{ ALTA, BC, MAN, NB, NFLD, NWT, NS, NU, ONT, PEI, QUE, SASK, YT, NB_PROVINCES };
		enum TInfo{ ABVR, NAME, TITLE, NB_INFO};

		class CInfo
		{
		public:
			
			const char* operator[](size_t i)const
			{
				ASSERT(i >= 0 && i < 2);
				return (i == 0 ? m_abrv : (i == 1) ? m_name : m_title.c_str());
			}

			const char* m_abrv;
			const char* m_name;
			std::string m_title;
		};


		static CInfo DEFAULT_LIST[NB_PROVINCES];
		static std::string GetAllPossibleValue(bool bAbvr = true, bool bName = true);
		static size_t GetProvince(const std::string& in, size_t t = ABVR);
		static std::string GetName(size_t prov, size_t t = ABVR);
		static void UpdateString();

		CProvinceSelection(const std::string& sel ="");

		std::string ToString()const;
		ERMsg FromString(const std::string&);

	

		using std::bitset<NB_PROVINCES>::at;
		bool at(const std::string& in)const
		{
			if (none())
				return true;

			size_t p = GetProvince(in);
			return p < size() ? at(p) : false;
		}

		using std::bitset<NB_PROVINCES>::set;
		ERMsg set(const std::string& in);

	};
}