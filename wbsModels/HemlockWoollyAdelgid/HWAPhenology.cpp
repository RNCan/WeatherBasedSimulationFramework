//**************************************************************************************************************
// Hemlock Woolly Adelgid winter mortalty model 
//
// for more information see :
//		Tobin(2018).Phenology of Hemlock Woolly Adelgid (Hemiptera:Adelgidae) in the Central Appalachian Mountains, USA
//
//
// 13/03/2019	1.0.0	Rémi Saint-Amant	Creation 
//**************************************************************************************************************
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "ModelBase/ContinuingRatio.h"
#include "HWAPhenology.h"
#include <queue>

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//links this class with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CHWAPhenologyModel::CreateObject);


	//Defining a simple continuing ratio model
	//
	enum TDaily { O_CDD, O_P_EGG, O_P_HATCH, O_S_HATCH, O_S_L1, NB_OUTPUTS_D };

	extern const char HEADER_D[] = "DD,ProgrediensEGG,ProgrediensHacth,SistensHatchSistensL1";

	CHWAPhenologyModel::CHWAPhenologyModel()
	{
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2019)";

	}

	CHWAPhenologyModel::~CHWAPhenologyModel()
	{}

	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CHWAPhenologyModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		/*	if (parameters.size() == 2)
			{
				m_Z = parameters[0].GetFloat();
				m_equation = parameters[1].GetSizeT();
			}
			else if (parameters.size() == 8)
			{
				m_nbDays = parameters[0].GetSizeT();
				m_Tlow = parameters[1].GetFloat();
				for (size_t i = 0; i < 6; i++)
					m_p[i] = parameters[2+i].GetFloat();
			}

				*/

		return msg;
	}

	ERMsg CHWAPhenologyModel::OnExecuteDaily()
	{
		ERMsg msg;

		m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::DAILY)), NB_OUTPUTS_D, -999, HEADER_D);

		//Execute model on a daily basis
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y], m_output);
		}

		return msg;
	}

	double CHWAPhenologyModel::Eq1(size_t e, double CDD)
	{
		enum { ʀ, β };
		static const double P[4][2] =
		{
			{0.0167, 4.6047},
			{0.0120, 5.2911},
			{0.0035, 4.0459},
			{0.0027, 3.7759},
		};

		
		double M = 1/( 1 + exp( -P[e][ʀ] * CDD + P[e][β]))*100;
		if (M < 0.1)
			M = 0;
		if (M > 99.9)
			M = 100;

		return M;

	}

	void CHWAPhenologyModel::ExecuteDaily(CWeatherYear& weather, CModelStatVector& output)
	{
		int year = weather.GetTRef().GetYear();
		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));
		
		CDegreeDays DD(CDegreeDays::MODIFIED_ALLEN_WAVE, 4.0);
		double CDD = 0;
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			CDD += DD.GetDD(m_weather.GetDay(TRef));

			output[TRef][O_CDD] = CDD;
			for (size_t e = 0; e < 4; e++)
				output[TRef][O_P_EGG + e] = Eq1(e, CDD);
		}

	}



}