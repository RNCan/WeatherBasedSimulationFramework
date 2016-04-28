#include "StdAfx.h"
#include "CountrySelection.h"
#include "Basic/UtilStd.h"


//#include "../Resource.h"

using namespace std;


namespace WBSF
{


	const CCountrySelection::CInfo CCountrySelection::DEFAULT_LIST[NB_COUNTRIES] =
	{
		//contry state from wikipedia
		{ "UN", " Unknown" },
		{ "AA", "Aruba" },
		{ "AC", "Antigua and Barbuda" },
		{ "AE", "United Arab Emirates" },
		{ "AF", "Afghanistan" },
		{ "AG", "Algeria" },
		{ "AJ", "Azerbaijan" },
		{ "AL", "Albania" },
		{ "AM", "Armenia" },
		{ "AN", "Andorra" },
		{ "AO", "Angola" },
		{ "AQ", "American Samoa" },
		{ "AR", "Argentina" },
		{ "AS", "Australia" },
		{ "AT", "Ashmore and Cartier Islands" },
		{ "AU", "Austria" },
		{ "AV", "Anguilla" },
		{ "AX", "Akrotiri" },
		{ "AY", "Antarctica" },
		{ "BA", "Bahrain" },
		{ "BB", "Barbados" },
		{ "BC", "Botswana" },
		{ "BD", "Bermuda" },
		{ "BE", "Belgium" },
		{ "BF", "Bahamas" },
		{ "BG", "Bangladesh" },
		{ "BH", "Belize" },
		{ "BK", "Bosnia and Herzegovina" },
		{ "BL", "Bolivia" },
		{ "BM", "Burma (Myanmar)" },
		{ "BN", "Benin" },
		{ "BO", "Belarus" },
		{ "BP", "Solomon Islands" },
		{ "BQ", "Navassa Island" },
		{ "BR", "Brazil" },
		{ "BT", "Bhutan" },
		{ "BU", "Bulgaria" },
		{ "BV", "Bouvet Island" },
		{ "BX", "Brunei" },
		{ "BY", "Burundi" },
		{ "CA", "Canada" },
		{ "CB", "Cambodia" },
		{ "CD", "Chad" },
		{ "CE", "Sri Lanka" },
		{ "CF", "Congo (Republic)" },
		{ "CG", "Congo (Democratic Republic)" },
		{ "CH", "China" },
		{ "CI", "Chile" },
		{ "CJ", "Cayman Islands" },
		{ "CK", "Cocos (Keeling) Islands" },
		{ "CM", "Cameroon" },
		{ "CN", "Comoros" },
		{ "CO", "Colombia" },
		{ "CQ", "Northern Mariana Islands" },
		{ "CR", "Coral Sea Islands" },
		{ "CS", "Costa Rica" },
		{ "CT", "Central African Republic" },
		{ "CU", "Cuba" },
		{ "CV", "Cape Verde" },
		{ "CW", "Cook Islands" },
		{ "CY", "Cyprus" },
		{ "DA", "Denmark" },
		{ "DJ", "Djibouti" },
		{ "DO", "Dominica" },
		{ "DQ", "Jarvis Island" },
		{ "DR", "Dominican Republic" },
		{ "DX", "Dhekelia" },
		{ "EC", "Ecuador" },
		{ "EG", "Egypt" },
		{ "EI", "Ireland" },
		{ "EK", "Equatorial Guinea" },
		{ "EN", "Estonia" },
		{ "ER", "Eritrea" },
		{ "ES", "El Salvador" },
		{ "ET", "Ethiopia" },
		{ "EZ", "Czech Republic" },
		{ "FG", "French Guiana" },
		{ "FI", "Finland" },
		{ "FJ", "Fiji" },
		{ "FK", "Falkland Islands (Islas Malvinas)" },
		{ "FM", "Federated States of Micronesia" },
		{ "FO", "Faroe Islands" },
		{ "FP", "French Polynesia" },
		{ "FQ", "Baker Island" },
		{ "FR", "France" },
		{ "FS", "French Southern and Antarctic Lands" },
		{ "GA", "Gambia" },
		{ "GB", "Gabon" },
		{ "GG", "Georgia" },
		{ "GH", "Ghana" },
		{ "GI", "Gibraltar" },
		{ "GJ", "Grenada" },
		{ "GK", "Guernsey" },
		{ "GL", "Greenland" },
		{ "GM", "Germany" },
		{ "GP", "Guadeloupe" },
		{ "GQ", "Guam" },
		{ "GR", "Greece" },
		{ "GT", "Guatemala" },
		{ "GV", "Guinea" },
		{ "GY", "Guyana" },
		{ "GZ", "Gaza Strip" },
		{ "HA", "Haiti" },
		{ "HK", "Hong Kong" },
		{ "HM", "Heard Island and McDonald Islands" },
		{ "HO", "Honduras" },
		{ "HQ", "Howland Island" },
		{ "HR", "Croatia" },
		{ "HU", "Hungary" },
		{ "IC", "Iceland" },
		{ "ID", "Indonesia" },
		{ "IM", "Isle of Man" },
		{ "IN", "India" },
		{ "IO", "British Indian Ocean Territory" },
		{ "IP", "Clipperton Island" },
		{ "IR", "Iran" },
		{ "IS", "Israel" },
		{ "IT", "Italy" },
		{ "IV", "Côte d'Ivoire" },
		{ "IZ", "Iraq" },
		{ "JA", "Japan" },
		{ "JE", "Jersey" },
		{ "JM", "Jamaica" },
		{ "JN", "Jan Mayen" },
		{ "JO", "Jordan" },
		{ "JQ", "Johnston Atoll" },
		{ "KE", "Kenya" },
		{ "KG", "Kyrgyzstan" },
		{ "KN", "North Korea" },
		{ "KQ", "Kingman Reef" },
		{ "KR", "Kiribati" },
		{ "KS", "South Korea" },
		{ "KT", "Christmas Island" },
		{ "KU", "Kuwait" },
		{ "KV", "Kosovo" },
		{ "KZ", "Kazakhstan" },
		{ "LA", "Laos" },
		{ "LE", "Lebanon" },
		{ "LG", "Latvia" },
		{ "LH", "Lithuania" },
		{ "LI", "Liberia" },
		{ "LO", "Slovakia" },
		{ "LQ", "Palmyra Atoll" },
		{ "LS", "Liechtenstein" },
		{ "LT", "Lesotho" },
		{ "LU", "Luxembourg" },
		{ "LY", "Libya" },
		{ "MA", "Madagascar" },
		{ "MB", "Martinique" },
		{ "MC", "Macau" },
		{ "MD", "Moldova" },
		{ "MF", "Mayotte" },
		{ "MG", "Mongolia" },
		{ "MH", "Montserrat" },
		{ "MI", "Malawi" },
		{ "MJ", "Montenegro" },
		{ "MK", "Macedonia" },
		{ "ML", "Mali" },
		{ "MN", "Monaco" },
		{ "MO", "Morocco" },
		{ "MP", "Mauritius" },
		{ "MQ", "Midway Islands" },
		{ "MR", "Mauritania" },
		{ "MT", "Malta" },
		{ "MU", "Oman" },
		{ "MV", "Maldives" },
		{ "MX", "Mexico" },
		{ "MY", "Malaysia" },
		{ "MZ", "Mozambique" },
		{ "NC", "New Caledonia" },
		{ "NE", "Niue" },
		{ "NF", "Norfolk Island" },
		{ "NG", "Niger" },
		{ "NH", "Vanuatu" },
		{ "NI", "Nigeria" },
		{ "NL", "Netherlands" },
		{ "NO", "Norway" },
		{ "NP", "Nepal" },
		{ "NR", "Nauru" },
		{ "NS", "Suriname" },
		{ "NT", "Netherlands Antilles" },
		{ "NU", "Nicaragua" },
		{ "NZ", "New Zealand" },
		{ "PA", "Paraguay" },
		{ "PC", "Pitcairn Islands" },
		{ "PE", "Peru" },
		{ "PF", "Paracel Islands" },
		{ "PG", "Spratly Islands" },
		{ "PK", "Pakistan" },
		{ "PL", "Poland" },
		{ "PM", "Panama" },
		{ "PO", "Portugal" },
		{ "PP", "Papua New Guinea" },
		{ "PS", "Palau" },
		{ "PU", "Guinea-Bissau" },
		{ "QA", "Qatar" },
		{ "RE", "Réunion" },
		{ "RI", "Serbia" },
		{ "RM", "Marshall Islands" },
		{ "RN", "Saint Martin" },
		{ "RO", "Romania" },
		{ "RP", "Philippines" },
		{ "RQ", "Puerto Rico" },
		{ "RS", "Russia" },
		{ "RW", "Rwanda" },
		{ "SA", "Saudi Arabia" },
		{ "SB", "Saint Pierre and Miquelon" },
		{ "SC", "Saint Kitts and Nevis" },
		{ "SE", "Seychelles" },
		{ "SF", "South Africa" },
		{ "SG", "Senegal" },
		{ "SH", "Saint Helena" },
		{ "SI", "Slovenia" },
		{ "SL", "Sierra Leone" },
		{ "SM", "San Marino" },
		{ "SN", "Singapore" },
		{ "SO", "Somalia" },
		{ "SP", "Spain" },
		{ "ST", "Saint Lucia" },
		{ "SU", "Sudan" },
		{ "SV", "Svalbard" },
		{ "SW", "Sweden" },
		{ "SX", "South Georgia and the South Sandwich Islands" },
		{ "SY", "Syria" },
		{ "SZ", "Switzerland" },
		{ "TB", "Saint Barthelemy" },
		{ "TD", "Trinidad and Tobago" },
		{ "TH", "Thailand" },
		{ "TI", "Tajikistan" },
		{ "TK", "Turks and Caicos Islands" },
		{ "TL", "Tokelau" },
		{ "TN", "Tonga" },
		{ "TO", "Togo" },
		{ "TP", "Sao Tome and Principe" },
		{ "TS", "Tunisia" },
		{ "TT", "Timor-Leste" },
		{ "TU", "Turkey" },
		{ "TV", "Tuvalu" },
		{ "TW", "Taiwan" },
		{ "TX", "Turkmenistan" },
		{ "TZ", "Tanzania" },
		{ "UG", "Uganda" },
		{ "UK", "United Kingdom" },
		{ "UP", "Ukraine" },
		{ "US", "United States" },
		{ "UV", "Burkina Faso" },
		{ "UY", "Uruguay" },
		{ "UZ", "Uzbekistan" },
		{ "VC", "Saint Vincent and the Grenadines" },
		{ "VE", "Venezuela" },
		{ "VI", "Virgin Islands (British)" },
		{ "VM", "Vietnam" },
		{ "VQ", "Virgin Islands(United States)" },
		{ "VT", "Vatican City (Holy See)" },
		{ "WA", "Namibia" },
		{ "WE", "West Bank" },
		{ "WF", "Wallis and Futuna" },
		{ "WI", "Western Sahara" },
		{ "WQ", "Wake Island" },
		{ "WS", "Samoa" },
		{ "WZ", "Swaziland" },
		{ "YM", "Yemen" },
		{ "ZA", "Zambia" },
		{ "ZI", "Zimbabwe" },
		{ "AJ", "Azerbaijan" },
		{ "AZ", "Azores" },
		{ "BZ", "Belgium And Luxembourg" },
		{ "CL", "Caroline Islands" },
		{ "CP", "Canary Islands" },
		{ "CZ", "Canton Island" },
		{ "DY", "Democratic Yemen" },
		{ "EU", "Europa Island (EU)" },
		{ "GC", "Cayman Islands (GC)" },
		{ "JU", "Juan De Nova Island (JU)" },
		{ "LC", "St. Lucia And St. Vincent" },
		{ "PN", "North Pacific Islands" },
		{ "PT", "Portugal (PT)" },
		{ "SD", "Saudi Arabia (SD)" },
		{ "TC", "United Arab Emirates" },
		{ "TE", "Tromelin Island (TE)" },
		{ "UA", "Former USSR (Asia)" },
		{ "UE", "Former USSR (Europe)" },
		{ "YU", "Yugoslavia" },
		{ "YY", "St. Marteen, St. Eustatius, Saba" },
		//{ "ALL", "ALL" }
	};


	std::string CCountrySelection::GetAllPossibleValue(bool bAbvr, bool bName)
	{
		string str;
		for (size_t i = 0; i < NB_COUNTRIES; i++)
		{
			str += i != 0 ? "|" : "";
			if (bAbvr)
				str += DEFAULT_LIST[i].m_abrv;
			if (bAbvr && bName)
				str += "=";
			if (bName)
				str += DEFAULT_LIST[i].m_name;
		}

		return str;
	}

	CCountrySelection::CCountrySelection(const std::string& in)
	{
		FromString(in);
	}

	string CCountrySelection::GetName(size_t i, size_t t)
	{
		ASSERT(i < NB_COUNTRIES);
		ASSERT(t < 2);
		return DEFAULT_LIST[i].m_name;
	}

	string CCountrySelection::ToString()const
	{
		string str;
		if (any())
		{
			for (size_t i = 0; i < NB_COUNTRIES; i++)
			{
				if (at(i))
				{
					str += DEFAULT_LIST[i].m_abrv;
					str += '|';
				}
			}
		}
		return str;
		return str;
	}

	ERMsg CCountrySelection::FromString(const string& in)
	{
		ERMsg msg;

		reset();

		StringVector tmp(in, "|;");
		for (size_t i = 0; i < tmp.size(); i++)
			msg += set(tmp[i]);

		return msg;
	}

	size_t CCountrySelection::GetCountry(const string& in, size_t t)//by abr
	{
		size_t country = -1;
		string tmp(in);
		if (tmp.length() != 2)
			Trim(tmp);

		MakeUpper(tmp);
		for (size_t i = 0; i < NB_COUNTRIES; i++)
		{
			if (tmp == DEFAULT_LIST[i].m_abrv)
			{
				country = i;
				break;
			}
		}

		return country;
	}



	ERMsg CCountrySelection::set(const std::string& country)
	{
		ERMsg message;
		size_t p = GetCountry(country);
		if (p < size())
		{
			set(p);
		}
		else
		{
			message.ajoute("This country is invalid : %s" + country);
		}

		return message;
	}

	//****************************************************************************************************************************************
	const CCountrySelectionWU::CInfo CCountrySelectionWU::DEFAULT_LIST[NB_COUNTRIES_WU] =
	{
		{ "AF", "Afghanistan" },
		{"AL", "Albania"},
		{"AG", "Algeria"},
		{"AO", "Angola"},
		{"AV", "Anguilla"},
		{"AR", "Argentina"},
		{"AM", "Armenia"},
		{"AS", "Australia"},
		{"AU", "Austria"},
		{"", "Azerbaijan"},
		{"", "Bahamas"},
		{"", "Bahrain"},
		{"", "Bangladesh"},
		{"", "Barbados"},
		{"", "Beijing"},
		{"", "Belarus"},
		{"", "Belgium"},
		{"", "Belize"},
		{"", "Benin"},
		{"", "Bermuda"},
		{"", "Bolivia"},
		{"", "Bosnia"},
		{"", "Botswana"},
		{"", "Bouvet Island"},
		{"", "Brazil"},
		{"", "British Columbia"},
		{"", "British Indian Ocean"},
		{"", "British Virgin Islands"},
		{"", "Brunei"},
		{"", "Bulgaria"},
		{"", "Burkina Faso"},
		{"", "Burma / Myanmar"},
		{"", "Burundi"},
		{"", "Cambodia"},
		{"", "Cameroon"},
		{"CA", "Canada"},
		{"", "Canary Islands"},
		{"", "Canton Island"},
		{"", "Cape Verde"},
		{"", "Capital Territory"},
		{"", "Caroline Islands"},
		{"", "Cayman Island"},
		{"", "Central African Republic"},
		{"", "Chad"},
		{"", "Cheng - Du"},
		{"", "Chile"},
		{"", "China"},
		{"", "Colombia"},
		{"", "Comoros"},
		{"", "Congo"},
		{"", "Cook Islands"},
		{"", "Costa Rica"},
		{"", "Croatia"},
		{"", "Cuba"},
		{"", "Cyprus"},
		{"", "Czech Republic"},
		{"", "Democratic Yemen"},
		{"", "Denmark"},
		{"", "Djibouti"},
		{"", "Dominica"},
		{"", "Dominican Republic"},
		{"", "East Timor"},
		{"", "Ecuador"},
		{"", "Egypt"},
		{"", "El Salvador"},
		{"", "Equatorial Guinea"},
		{"", "Eritrea"},
		{"", "Estonia"},
		{"", "Ethiopia"},
		{"", "Falkland Islands"},
		{"", "Fiji"},
		{"", "Finland"},
		{"", "France"},
		{"", "French Guiana"},
		{"", "French Polynesia"},
		{"", "Gabon"},
		{"", "Gambia"},
		{"", "Germany"},
		{"", "Ghana"},
		{"", "Gibraltar"},
		{"", "Greece"},
		{"", "Greenland"},
		{"", "Grenada"},
		{"", "Guam"},
		{"", "Guang - Zhou"},
		{"", "Guatemala"},
		{"", "Guinea"},
		{"", "Guinea - Bissau"},
		{"", "Guyana"},
		{"", "Haiti"},
		{"", "Han - Kou"},
		{"", "Hawaii"},
		{"", "Honduras"},
		{"", "Hong Kong"},
		{"", "Hungary"},
		{"", "Iceland"},
		{"", "India"},
		{"", "Indonesia"},
		{"", "Iran"},
		{"", "Iraq"},
		{"", "Ireland"},
		{"", "Israel"},
		{"", "Italy"},
		{"", "Ivory Coast"},
		{"", "Jamaica"},
		{"", "Japan"},
		{"", "Jordan"},
		{"", "Kazakhstan"},
		{"", "Kenya"},
		{"", "Kiribati"},
		{"", "Kuwait"},
		{"", "Kyrgyzstan"},
		{"", "Lan - Zhou"},
		{"", "Lao Peoples Republic"},
		{"", "Latvia"},
		{"", "Lebanon"},
		{"", "Lesotho"},
		{"", "Liberia"},
		{"", "Libya"},
		{"", "Luxembourg"},
		{"", "Macao"},
		{"", "Macedonia"},
		{"", "Madagascar"},
		{"", "Madeira Islands"},
		{"", "Malawi"},
		{"", "Malaysia"},
		{"", "Maldives"},
		{"", "Mali"},
		{"", "Malta"},
		{"", "Manitoba"},
		{"", "Mariana Islands"},
		{"", "Marshall Islands"},
		{"", "Martinique"},
		{"", "Maryland"},
		{"", "Mauritania"},
		{"", "Mauritius"},
		{"", "Mexico"},
		{"", "Mongolia"},
		{"", "Montana"},
		{"", "Morocco"},
		{"", "Mozambique"},
		{"", "Namibia"},
		{"", "Nauru"},
		{"", "Nepal"},
		{"", "Netherlands"},
		{"", "New Brunswick"},
		{"", "New Caledonia"},
		{"", "New South Wales"},
		{"", "New Zealand"},
		{"", "Newfoundland"},
		{"", "Nicaragua"},
		{"", "Niger"},
		{"", "Nigeria"},
		{"", "North Korea"},
		{"", "North Pacific Islands"},
		{"", "Norway"},
		{"", "Nova Scotia"},
		{"", "Oman"},
		{"", "Pakistan"},
		{"", "Panama"},
		{"", "Papua New Guinea"},
		{"", "Paraguay"},
		{"", "Peru"},
		{"", "Philippines"},
		{"", "Poland"},
		{"", "Portugal"},
		{"", "Puerto Rico"},
		{"", "Qatar"},
		{"", "Republic of Moldova"},
		{"", "Reunion Island"},
		{"", "Romania"},
		{"", "Russia"},
		{"", "Rwanda"},
		{"", "Saudi Arabia"},
		{"", "Senegal"},
		{"", "Seychelles"},
		{"", "Shang - Hai"},
		{"", "Shen - Yang"},
		{"", "Sierra Leone"},
		{"", "Singapore"},
		{"", "Slovakia"},
		{"", "Slovenia"},
		{"", "Solomon Islands"},
		{"", "Somalia"},
		{"", "South Africa"},
		{"", "South Korea"},
		{"", "Southern Line Islands"},
		{"", "Spain"},
		{"", "Sri Lanka"},
		{"", "Sudan"},
		{"", "Suriname"},
		{"", "Swaziland"},
		{"", "Sweden"},
		{"", "Switzerland"},
		{"", "Syria"},
		{"", "Taiwan"},
		{"", "Tajikistan"},
		{"", "Tanzania"},
		{"", "Tasmania"},
		{"", "Thailand"},
		{"", "Togo"},
		{"", "Tokelau Island"},
		{"", "Tonga"},
		{"", "Trinidad And Tobago"},
		{"", "Tunisia"},
		{"", "Turkey"},
		{"", "Turks Islands"},
		{"", "Tuvalu"},
		{"US", "United States"},
		{"", "Uganda"},
		{"", "Ukraine"},
		{"", "United Arab Emirates"},
		{"UK", "United Kingdom"},
		{"", "Uruguay"},
		{"", "Urum - Qui"},
		{"", "Uzbekistan"},
		{"VA", "Vanuata"},
		{"VE", "Venezuela"},
		{"", "Victoria"},
		{"", "Viet Nam"},
		{"", "Virgin Islands"},
		{"", "Virginia"},
		{"", "Wake Island"},
		{"", "Wallis And Futuna Island"},
		{"", "Western Sahara"},
		{"", "Western Samoa"},
		{"YE", "Yemen"},
		{"YU", "Yugoslavia"},
		{"ZA", "Zambia"},
		{"ZI", "Zimbabwe"},
	};


























































	std::string CCountrySelectionWU::GetAllPossibleValue(bool bAbvr, bool bName)
	{
		string str;
		for (size_t i = 0; i < NB_COUNTRIES; i++)
		{
			str += i != 0 ? "|" : "";
			if (bAbvr)
				str += DEFAULT_LIST[i].m_abrv;
			if (bAbvr && bName)
				str += "=";
			if (bName)
				str += DEFAULT_LIST[i].m_name;
		}

		return str;
	}

	CCountrySelectionWU::CCountrySelectionWU(const std::string& in)
	{
		FromString(in);
	}

	string CCountrySelectionWU::GetName(size_t i, size_t t)
	{
		ASSERT(i < NB_COUNTRIES);
		ASSERT(t < 2);
		return DEFAULT_LIST[i].m_name;
	}

	string CCountrySelectionWU::ToString()const
	{
		string str;
		if (any())
		{
			for (size_t i = 0; i < NB_COUNTRIES; i++)
			{
				if (at(i))
				{
					str += DEFAULT_LIST[i].m_abrv;
					str += '|';
				}
			}
		}
		return str;
		return str;
	}

	ERMsg CCountrySelectionWU::FromString(const string& in)
	{
		ERMsg msg;

		reset();

		StringVector tmp(in, "|;");
		for (size_t i = 0; i < tmp.size(); i++)
			msg += set(tmp[i]);

		return msg;
	}

	size_t CCountrySelectionWU::GetCountry(const string& in, size_t t)//by abr
	{
		size_t country = -1;
		string tmp(in);
		if (tmp.length() != 2)
			Trim(tmp);

		MakeUpper(tmp);
		for (size_t i = 0; i < NB_COUNTRIES; i++)
		{
			if (tmp == DEFAULT_LIST[i].m_abrv)
			{
				country = i;
				break;
			}
		}

		return country;
	}



	ERMsg CCountrySelectionWU::set(const std::string& country)
	{
		ERMsg message;
		size_t p = GetCountry(country);
		if (p < size())
		{
			set(p);
		}
		else
		{
			message.ajoute("This country is invalid : %s" + country);
		}

		return message;
	}

}