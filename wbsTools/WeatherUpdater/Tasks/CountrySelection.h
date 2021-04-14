#pragma once

#include <bitset>
#include <boost/dynamic_bitset.hpp>
#include "Basic/ERMsg.h"
//#include "TaskBase.h"

namespace WBSF
{

	enum { NB_COUNTRIES = 284 };
	class CCountrySelection : public std::bitset < NB_COUNTRIES >
	{
	public:

		enum TSearch { BY_ABVR, BY_NAME };
		static std::string GHCN_to_GADM(std::string country);
		static std::string GHCN_to_GADM(std::string country, std::string subDivision);


		class CInfo
		{
		public:

			std::string operator[](size_t i)const
			{
				ASSERT(i >= 0 && i < 2);
				return (i == 0 ? m_abrv : m_name);
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

		//		using std::bitset<NB_COUNTRIES>::at;
		bool at(const std::string& in)const
		{
			if (none())
				return false;

			if (all())
				return true;


			size_t p = GetCountry(in);
			return p < size() ? test(p) : false;
		}

		using std::bitset<NB_COUNTRIES>::set;
		ERMsg set(const std::string& in);
	};



	enum { NB_COUNTRIES_WU = 230 };
	class CCountrySelectionWU : public std::bitset < NB_COUNTRIES_WU >
	{
	public:

		enum TSearch { BY_ABVR, BY_NAME };

		class CInfo
		{
		public:

			std::string operator[](size_t i)const
			{
				ASSERT(i >= 0 && i < 2);
				return (i == 0 ? m_abrv : m_name);
			}

			std::string m_abrv;
			std::string m_name;
		};


		static const CInfo DEFAULT_LIST[NB_COUNTRIES_WU];

		static std::string GetAllPossibleValue(bool bAbvr = true, bool bName = true);
		static size_t GetCountry(const std::string& in, size_t t = BY_ABVR);//by abr

		CCountrySelectionWU(const std::string& in = "");

		std::string ToString()const;
		ERMsg FromString(const std::string& in);

		static std::string GetName(size_t i, size_t t = BY_ABVR);

		//		using std::bitset<NB_COUNTRIES_WU>::at;
		bool at(const std::string& in)const
		{
			if (none())
				return true;

			size_t p = GetCountry(in);
			return p < size() ? test(p) : false;
		}

		using std::bitset<NB_COUNTRIES_WU>::set;
		ERMsg set(const std::string& in);
	};







	//enum { NB_COUNTRIES_GADM = 217 };
	class CCountrySelectionGADM : public boost::dynamic_bitset<size_t>
	{
	public:

		enum TSearch { BY_ABVR, BY_NAME };

		class CInfo
		{
		public:

			std::string operator[](size_t i)const
			{
				ASSERT(i >= 0 && i < 3);
				return (i == 0 ? m_abrv : i == 1 ? m_name: m_ID2);
			}

			std::string m_abrv;
			std::string m_name;
			std::string m_ID2;
		};


		
		//static const CInfo DEFAULT_LIST[CCountrySelectionGADM];

		static std::vector<CInfo> m_default_list;
		static void LoadDefault();
		static ERMsg Load(const std::string& file_path);


		static std::string GetAllPossibleValue(bool bAbvr = true, bool bName = true);
		static size_t GetCountry(const std::string& in, size_t t = BY_ABVR);//by abr

		CCountrySelectionGADM(const std::string& in = "");

		std::string ToString()const;
		ERMsg FromString(const std::string& in);

		static std::string GetName(size_t i, size_t t = BY_ABVR);

		//		using std::bitset<NB_COUNTRIES_WU>::at;
		bool at(const std::string& in)const
		{
			if (none())
				return true;

			size_t p = GetCountry(in);
			return p < size() ? test(p) : false;
		}

		using boost::dynamic_bitset<size_t>::set;
		ERMsg set(const std::string& in);
	};

}