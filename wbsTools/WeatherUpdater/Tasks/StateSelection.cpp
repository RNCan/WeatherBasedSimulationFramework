#include "StdAfx.h"
#include "StateSelection.h"
#include "Basic/UtilStd.h"
//#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std;

namespace WBSF
{

	const CStateSelection::CInfo CStateSelection::DEFAULT_LIST[NB_USA_STATES] =
	{
		{ "AL",  1, "Alabama(AL01)",  "Alabama" },
		{ "AZ",  2, "Arizona(AZ02)", "Arizona" },
		{ "AR",  3, "Arkansas(AR03)", "Arkansas" },
		{ "CA",  4, "California(CA04)", "California" },
		{ "CO",  5, "Colorado(CO05)", "Colorado" },
		{ "CT",  6, "Connecticut(CT06)", "Connecticut" },
		{ "DE",  7, "Delaware(DE07)", "Delaware" },
		{ "FL",  8, "Florida(FL08)", "Florida" },
		{ "GA",  9, "Georgia(GA09)", "Georgia" },
		{ "ID", 10, "Idaho(ID10)", "Idaho" },
		{ "IL", 11, "Illinois(IL11)", "Illinois" },
		{ "IN", 12, "Indiana(IN12)", "Indiana" },
		{ "IA", 13, "Iowa(IA13)", "Iowa" },
		{ "KS", 14, "Kansas(KS14)", "Kansas" },
		{ "KY", 15, "Kentucky(KY15)", "Kentucky" },
		{ "LA", 16, "Louisiana(LA16)", "Louisiana" },
		{ "ME", 17, "Maine(ME17)", "Maine" },
		{ "MD", 18, "Maryland(MD18)", "Maryland" },
		{ "MA", 19, "Massachusetts(MA19)", "Massachusetts" },
		{ "MI", 20, "Michigan(MI20)", "Michigan" },
		{ "MN", 21, "Minnesota(MN21)", "Minnesota" },
		{ "MS", 22, "Mississippi(MS22)", "Mississippi"},
		{ "MO", 23, "Missouri(MO23)", "Missouri" },
		{ "MT", 24, "Montana(MT24)", "Montana" },
		{ "NE", 25, "Nebraska(NE25)", "Nebraska" },
		{ "NV", 26, "Nevada(NV26)", "Nevada"},
		{ "NH", 27, "New Hampshire(NH27)", "New Hampshire" },
		{ "NJ", 28, "New Jersey(NJ28)", "New Jersey" },
		{ "NM", 29, "New Mexico(NM29)", "New Mexico" },
		{ "NY", 30, "New York(NY30)", "New York" },
		{ "NC", 31, "North Carolina(NC31)", "North Carolina" },
		{ "ND", 32, "North Dakota(ND32)", "North Dakota" },
		{ "OH", 33, "Ohio(OH33)", "Ohio" },
		{ "OK", 34, "Oklahoma(OK34)", "Oklahoma" },
		{ "OR", 35, "Oregon(OR35)", "Oregon" },
		{ "PA", 36, "Pennsylvania(PA36)", "Pennsylvania" },
		{ "RI", 37, "Rhode Island(RI37)", "Rhode Island" },
		{ "SC", 38, "South Carolina(SC38)", "South Carolina" },
		{ "SD", 39, "South Dakota(SD39)", "South Dakota" },
		{ "TN", 40, "Tennessee(TN40)", "Tennessee" },
		{ "TX", 41, "Texas(TX41)", "Texas" },
		{ "UT", 42, "Utah(UT42)", "Utah" },
		{ "VT", 43, "Vermont(VT43)", "Vermont" },
		{ "VA", 44, "Virginia(VA44)", "Virginia" },
		{ "WA", 45, "Washington(WA45)", "Washington" },
		{ "WV", 46, "West Virginia(WV46)", "West Virginia" },
		{ "WI", 47, "Wisconsin(WI47)", "Wisconsin" },
		{ "WY", 48, "Wyoming(WY48)", "Wyoming" },
		{ "AK", 50, "Alaska(AK50)", "Alaska" },
		{ "HI", 51, "Hawaii(HI51)", "Hawaii" },
		{ "VI", 67, "Virgin Islands(VI67)", "Virgin Islands" },
		{ "PR", 66, "Puerto Rico(PR66)", "Puerto Rico" },
		{ "PI", 91, "Pacific Islands(PI91)", "Pacific Islands" },
	};

	void CStateSelection::UpdateString()
	{
		const StringVector NAME(IDS_STR_STATES, "|;");
		ASSERT(NAME.size() == NB_USA_STATES);

		for (size_t i = 0; i < NB_USA_STATES; i++)
			const_cast<CInfo&>(DEFAULT_LIST[i]).m_title = NAME[i];
	}

	std::string CStateSelection::GetAllPossibleValue(bool bAbvr, bool bName)
	{
		string str;
		for (size_t i = 0; i < NB_USA_STATES; i++)
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
	

	CStateSelection::CStateSelection(const string& in)
	{
		//reset();
		FromString(in);
	}

	string CStateSelection::GetName(size_t state, size_t t)
	{
		ASSERT(state >= 0 && state < NB_USA_STATES);
		return DEFAULT_LIST[state][t];
	}

	string CStateSelection::ToString()const
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
			for (size_t i = 0; i < NB_USA_STATES; i++)
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

	ERMsg CStateSelection::FromString(const string& in)
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

	size_t CStateSelection::GetState(const std::string& stateID, size_t t)
	{
		size_t state = size_t(-1);

		std::string tmp(stateID);
		Trim(tmp);
		MakeUpper(tmp);

		for (size_t i = 0; i < NB_USA_STATES; i++)
		{
			if (tmp == DEFAULT_LIST[i][t])
			{
				state = i;
				break;
			}
		}

		return state;
	}

	size_t CStateSelection::GetState(long in)//by number
	{
		int no = ToInt(std::to_string(in).substr(0, 2));

		size_t state = size_t(-1);
		for (size_t i = 0; i < NB_USA_STATES; i++)
		{
			if (no == DEFAULT_LIST[i].m_no)
			{
				state = i;
				break;
			}
		}

		return state;
	}


	ERMsg CStateSelection::set(const std::string& country)
	{
		ERMsg message;
		size_t p = GetState(country);
		if (p < size())
		{
			set(p);
		}
		else
		{
			message.ajoute("This state is invalid: %s" + country);
		}

		return message;
	}
}

