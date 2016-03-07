//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 15-11-2013	Rémi Saint-Amant	Completly new version STL and x64
// 05-02-2010	Rémi Saint-Amant	Add MergeStation
// 15-09-2008	Rémi Saint-Amant	Created from old file
//****************************************************************************
#include "stdafx.h"

#include "Basic/NormalsStation.h"
#include "Basic/WeatherCorrection.h"
#include "Basic/Location.h"
#include "Basic/Statistic.h"

#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::NORMALS_DATA;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;
using namespace WBSF::GRADIENT;


namespace WBSF
{
	CNormalsStation::CNormalsStation()
	{}

	CNormalsStation::CNormalsStation(const CNormalsStation& station)
	{
		operator=(station);
	}

	CNormalsStation::~CNormalsStation()
	{}

	void CNormalsStation::Reset()
	{
		CLocation::Reset();
		CNormalsData::Reset();
	}


	CNormalsStation& CNormalsStation::operator=(const CNormalsStation& in)
	{
		if (&in != this)
		{
			CLocation::operator=(in);
			CNormalsData::operator=(in);
		}

		return *this;
	}

	bool CNormalsStation::operator==(const CNormalsStation& in)const
	{
		bool bEqual = true;

		if (CLocation::operator!=(in)) bEqual = false;
		if (CNormalsData::operator!=(in)) bEqual = false;

		return bEqual;
	}

	ERMsg CNormalsStation::LoadHeaderV2(std::istream& stream)
	{
		ERMsg msg;

		getline(stream, m_name);
		Trim(m_name);

		if (stream)
		{
			std::string line[3];
			bool bRep = true;
			for (int i = 0; i < 3; i++)
			{
				getline(stream, line[i]);
				bRep = bRep && (line[i].length() == LINE_LENGTH1);
			}

			if (bRep)
			{
				char bUseIt = '0';
				sscanf(line[0].c_str(), "%lf %lf %lf %c", &m_lat, &m_lon, &m_elev, &bUseIt);
				UseIt(bUseIt != '0');

				m_ID = TrimConst(line[2]);

			}
			else
			{
				msg.ajoute(FormatMsg(IDS_SIM_BAD_NORMAL_RECORD, ToString(LINE_LENGTH1), ToString(line[0].length())));
			}
		}

		//loadData


		return msg;
	}

	ERMsg CNormalsStation::LoadV2(std::istream& stream)
	{
		ERMsg msg;
		msg = LoadHeaderV2(stream);
		if (msg)
		{
			string str;
			msg = CNormalsData::LoadV2(stream);
			msg = IsValid();

			if (msg)
			{
				CWVariables v = GetVariables();
				SetDefaultSSI(CLocation::VARIABLES, v.to_string());
				ASSERT(!m_name.empty());
			}
		}


		return msg;
	}

	/*
	//binary serialisation
	void CNormalsStation::Serialize( CArchive& ar )
	{
	CLocation::Serialize( ar );

	if( ar.IsStoring() )
	{
	ar << m_data;
	}
	else
	{
	ar >> m_data;
	}

	}

	ERMsg CNormalsStation::Read(CStdioFile& file)
	{
	ERMsg msg;
	msg = CLocation::Read(file);
	if( msg )
	{
	msg = m_data.Read(file);
	if( msg  )
	{
	for(int i=0; i<NB_FIELDS; i++)
	{
	ASSERT( m_bHave[i] == m_data.HaveVar(i));
	m_bHave[i] = m_data.HaveVar(i);
	}
	//ASSERT( m_bHaveTemperature == m_data.HaveTemperature());
	//ASSERT( m_bHavePrecipitation == m_data.HavePrecipitation());

	//m_bHaveTemperature = m_data.HaveTemperature();
	//m_bHavePrecipitation = m_data.HavePrecipitation();

	ASSERT( !GetName().empty() );
	ASSERT( IsValid() );
	//msg = IsValid();
	}
	}


	return msg;
	}


	void CNormalsStation::Write(CStdioFile& file)const
	{
	ASSERT( IsValid() );
	ASSERT( !GetName().empty() );
	//	ASSERT( m_bHaveTemperature == m_data.HaveTemperature());
	//	ASSERT( m_bHavePrecipitation == m_data.HavePrecipitation());


	CLocation::Write(file);
	m_data.Write(file);

	}
	*/
	ERMsg CNormalsStation::IsValid() const
	{
		ERMsg msg;

		msg = CLocation::IsValid();
		if (msg)
		{
			msg = CNormalsData::IsValid();
			//for(int v=0; v<NB_FIELDS; v++)
			//{
			//	if( m_bHave[v] != m_data.HaveVar(v))
			//	{
			//		msg.asgType(ERMsg::ERREUR);

			//		CStdString error;
			//		error.LoadString( IDS_SIM_T_INCONSISTENT);
			//		msg.ajoute(error);
			//	}
			//}
			/*if( m_bHavePrecipitation != m_data.HavePrecipitation())
			{
			msg.asgType(ERMsg::ERREUR);

			CStdString error;
			error.LoadString( IDS_SIM_T_INCONSISTENT);
			msg.ajoute(error);
			}
			*/
		}
		return msg;
	}

	float CNormalsStation::GetDelta(size_t type)const
	{
		ASSERT(type >= 0 && type < NB_FIELDS);
		CStatistic deltaStat;
		for (int m = 0; m < 12; m++)
			deltaStat += at(m)[type];

		return float(deltaStat[HIGHEST] - deltaStat[LOWEST]);
	}

	void CNormalsStation::ApplyCorrections(const CWeatherCorrections& correction)
	{
		CWVariables vars = GetVariables();
		CNormalsStation& me = *this;

		for (size_t m = 0; m < 12; m++)
		{
			CTRef TRef(YEAR_NOT_INIT, m);

			if (vars[H_TAIR] && correction.m_variables[H_TAIR])
			{
				//it's temperature correction
				me[m][TMIN_MN] += (float)correction.GetCorrection(me, TRef, H_TAIR);
				me[m][TMAX_MN] += (float)correction.GetCorrection(me, TRef, H_TRNG);
			}

			if (vars[H_PRCP] && correction.m_variables[H_PRCP])
			{
				me[m][PRCP_TT] *= (float)correction.GetCorrection(me, TRef, H_PRCP);
				//responsability of the caller to verify that precipitation is positive
				//prcp must be positive: ça doit créer des biais!!!
				//if (me[m][PRCP_TT] < 0)
				//me[m][PRCP_TT] = 0;
			}
		}
	}



	//******************************************************************************
	void CNormalsStationVector::ApplyCorrections(const CWeatherCorrections& correction)
	{

		for (CNormalsStationVector::iterator it = begin(); it != end(); it++)
		{
			//CWeatherCorrection correction = gradients.GetCorrection(target, *it);
			it->ApplyCorrections(correction);
		}
	}

	void CNormalsStationVector::GetWeight(const CLocation& target, CWVariables variables, CNormalWeight& weight, bool bTakeElevation)const
	{
		for (size_t v = 0; v < variables.size(); v++)
		{
			if (variables[v])
			{
				//short cat = category.GetCat(c);
				weight[v].resize(size());

				double distSum = 0;
				for (size_t i = 0; i < size(); i++)
				{
					double xtemp = at(i).GetXTemp(target, bTakeElevation);
					weight[v][i] = xtemp;
					distSum += xtemp;
				}

				for (int i = 0; i < (int)weight[v].size(); i++)
				{
					weight[v][i] /= distSum;
				}
			}
		}
	}

	void CNormalsStationVector::GetNormalVector(const CLocation& target, CWVariables variables, CNormalDataVector& normalVector)const
	{
		normalVector.resize(size());

		for (size_t i = 0; i < size(); i++)
			normalVector[i].Copy(at(i), variables);

	}

	void CNormalsStationVector::GetInverseDistanceMean(const CLocation& target, CWVariables variables, CNormalsStation& normalsStation, bool bTakeElevation)const
	{
		if (variables[H_TDEW])
			variables.set(H_RELH);

		((CLocation&)normalsStation) = target;

		CNormalWeight weight;
		GetWeight(target, variables, weight, bTakeElevation);

		CNormalDataVector normalVector;
		GetNormalVector(target, variables, normalVector);

		normalsStation.SetInverseDistanceMean(variables, normalVector, weight);

	}
}//namespace WBSF

