//*********************************************************************
//21-02-2017	1.0.0	Rémi Saint-Amant	Creation from code of Beguería, Santiago; Vicente Serrano, Sergio M: http://digital.csic.es/handle/10261/10002?
//*********************************************************************

#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"
#include "SPEIModel.h"

#include "spei.h"
#include "Thornthwaite.h"


using namespace std;
using namespace WBSF::HOURLY_DATA; 


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSPEIModel::CreateObject);


	CSPEIModel::CSPEIModel() 
	{
		// initialise your variable here (optionnal)
		NB_INPUT_PARAMETER=1;
		VERSION = "1.0.0 (2017)";

		m_k = 1;
	}

	CSPEIModel::~CSPEIModel()
	{}


	//this method is call to load your parameter in your variable
	ERMsg CSPEIModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		m_k = parameters[0].GetInt();
		return msg;
	}


	//This method is call to compute solution
	ERMsg CSPEIModel::OnExecuteMonthly()
	{
		ERMsg msg;


		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
		size_t i = 0;
		vector<double> Tair(p.size());
		vector<double> Prcp(p.size());
		
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++, i++)//for all years
		{
			const CWeatherMonth& weather = m_weather.GetMonth(TRef);
			Tair[i] = weather[H_TAIR2][MEAN];
			Prcp[i] = weather[H_PRCP][SUM];
		}
		
		vector<double> ETP(Tair.size());
		Thornthwaite(Tair.data(), Prcp.size(), m_info.m_loc.m_lat, ETP.data());

		vector<double> balance(ETP.size());
		for (size_t i = 0; i<ETP.size(); i++)
			balance[i] = Prcp[i] - ETP[i];

		// Compute the cumulative series
		enum TOutput{ O_TAIRE, O_PRCP, O_PET, O_BAL, O_SPEI, NB_OUTPUTS };
		array<vector<double>, NB_OUTPUTS> output; 
		for (size_t i = 0; i < output.size(); i++)
			output[i].resize(balance.size() - m_k + 1);

		for (size_t i = m_k - 1; i<balance.size(); i++)
		{
			for (size_t j = 0; j<m_k; j++) 
			{
				output[O_TAIRE][i - m_k + 1] += Tair[i - j];
				output[O_PRCP][i - m_k + 1] += Prcp[i - j];
				output[O_PET][i - m_k + 1] += ETP[i - j];
				output[O_BAL][i - m_k + 1] += balance[i - j];
			}
		}

		// Compute the SPEI series
		spei(output[O_BAL].data(), output[O_BAL].size(), output[O_SPEI].data());

		
		m_output.Init(output[O_SPEI].size(), p.Begin() + m_k - 1, NB_OUTPUTS);
		for (size_t i = 0; i<output.size(); i++)
		{
			for (size_t j = 0; j<output[i].size(); j++)
				m_output[j][i] = output[i][j];
		}


		return msg;
	}


}