#pragma once

#include "Basic/ERMsg.h"
#include <bitset>

namespace WBSF
{

	//*********************************************************************
	enum { NB_USA_STATES = 53 };
	class CStateSelection : public std::bitset < NB_USA_STATES >
	{
	public:

		enum TABRV{
			UNKNOWN = -1, $AL, $AZ, $AR, $CA, $CO, $CT, $DE, $FL, $GA, $ID,
			$IL, $IN, $IA, $KS, $KY, $LA, $ME, $MD, $MA, $MI,
			$MN, $MS, $MO, $MT, $NE, $NV, $NH, $NJ, $NM, $NY,
			$NC, $ND, $OH, $OK, $OR, $PA, $RI, $SC, $SD, $TN,
			$TX, $UT, $VT, $VA, $WA, $WV, $WI, $WY, $AK, $HI,
			$VI, $PR, $PI
		};

		//enum TNAME{ ALL = NB_STATES, NB_TAG };
		enum TInfo{ BY_ABVR, BY_NAME, BY_NO };


		class CInfo
		{
		public:
			
			std::string operator[](size_t i)const
			{
				ASSERT(i >= 0 && i < 3);
				return (i == 0 ? m_abrv : i == 1 ? m_name : std::to_string(m_no));
			}


			const char* m_abrv;
			const size_t m_no;
			const char* m_name;
			std::string m_title;

			
		};

		static void UpdateString();
		static std::string GetAllPossibleValue(bool bAbvr = true, bool bName = true);
		static const CInfo DEFAULT_LIST[NB_USA_STATES];

		static size_t GetState(long stateID);
		static size_t GetState(const std::string& in, size_t t = BY_ABVR);
		static std::string GetName(size_t state, size_t t = BY_ABVR);

		CStateSelection(const std::string& in="");

		std::string ToString()const;
		ERMsg FromString(const std::string& in);

		using std::bitset<NB_USA_STATES>::at;
		bool at(const std::string& in)const
		{
			if (none())
				return true;

			size_t p = GetState(in);
			return p < size() ? at(p) : false;
		}

		using std::bitset<NB_USA_STATES>::set;
		ERMsg set(const std::string& in);

		
		

	};

}