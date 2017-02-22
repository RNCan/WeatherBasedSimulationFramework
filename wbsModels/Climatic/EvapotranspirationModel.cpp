//**********************************************************************
// 20/09/2016	1.3.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 21/01/2016	1.2.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 25/05/2013			Rémi Saint-Amant	Creation from old climatic model
//**********************************************************************
#include "ModelBase/EntryPoint.h"
#include "EvapotranspirationModel.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CEvapotranspirationModel::CreateObject);

	//Constructor
	CEvapotranspirationModel::CEvapotranspirationModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 5;
		VERSION = "1.3.1 (2017)";

		//Initialization of input parameters(optional)
		m_ETModelName = "Priestley-Taylor";

	}

	CEvapotranspirationModel::~CEvapotranspirationModel()
	{}

	//This method is call to load your parameter in your variable
	ERMsg CEvapotranspirationModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		m_ETModelName = parameters[c++].GetString();
		if (!CETFactory::IsRegistered(m_ETModelName))
			msg.ajoute(m_ETModelName + " is an unknown evapotranspiration model name");

		for (size_t i = 0; i < 2; i++)
		{
			std::string name = parameters[c++].GetString();
			std::string value = parameters[c++].GetString();
			if (!name.empty() && !value.empty())
				m_options[name] = value;
		}

		return msg;
	}



	ERMsg CEvapotranspirationModel::OnExecuteHourly()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		CETPtr pModel = CETFactory::CreateET(m_ETModelName);
		msg = pModel->SetOptions(m_options);
		if (msg)
			pModel->Execute(m_weather, m_output);

		return msg;
	}


	ERMsg CEvapotranspirationModel::OnExecuteDaily()
	{
		ERMsg msg;

		CETPtr pModel = CETFactory::CreateET(m_ETModelName);
		msg = pModel->SetOptions(m_options);
		if (msg)
		{
			pModel->Execute(m_weather, m_output);
			m_output.Transform(CTM(CTM::DAILY), SUM);
		}

		return msg;
	}


	ERMsg CEvapotranspirationModel::OnExecuteMonthly()
	{
		ERMsg msg;

		CETPtr pModel = CETFactory::CreateET(m_ETModelName);
		msg = pModel->SetOptions(m_options);
		if (msg)
		{
			//CModelStatVector stat;
			pModel->Execute(m_weather, m_output);

			//CTTransformation TT(stat.GetTPeriod(), CTM(CTM::MONTHLY));
			//pModel->Transform(TT, stat, m_output);
			m_output.Transform(CTM(CTM::MONTHLY), SUM);
		}

		return msg;
	}


	ERMsg CEvapotranspirationModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CETPtr pModel = CETFactory::CreateET(m_ETModelName);
		msg = pModel->SetOptions(m_options);
		if (msg)
		{
			//CModelStatVector stat;
			pModel->Execute(m_weather, m_output);
			m_output.Transform(CTM(CTM::ANNUAL), SUM);

			//CTTransformation TT(stat.GetTPeriod(), CTM(CTM::ANNUAL));
			//pModel->Transform(TT, stat, m_output);
		}

		return msg;
	}

	



	//simulated Annealing 
	//void CEvapotranspirationModel::AddSAResult(const CStdStringVector& header, const CStdStringVector& data)
	//{
	//
	//	
	//
	//	if( header.size()==12)
	//	{
	//		std::vector<double> obs(4);
	//	
	//		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1,data[4].ToShort()-1,data[5].ToShort());
	//		for(int i=0; i<4; i++)
	//			obs[i] = data[i+6].ToDouble();
	//		
	//
	//		ASSERT( obs.size() == 4 );
	//		m_SAResult.push_back( CSAResult(TRef, obs ) );
	//	} 
	//
	//	/*if( header.size()==26)
	//	{
	//		std::vector<double> obs(24); 
	//	
	//		for(int h=0; h<24; h++)
	//			obs[h] = data[h+2].ToDouble();
	//	
	//
	//		ASSERT( obs.size() == 24 );
	//		m_SAResult.push_back( CSAResult(CTRef(), obs ) );
	//	}
	//	else if( header.size()==13)
	//	{
	//		std::vector<double> obs(7);
	//	
	//		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1,data[4].ToShort()-1,data[5].ToShort());
	//		for(int c=0; c<7; c++)
	//			obs[c] = data[c+6].ToDouble();
	//	
	//
	//		ASSERT( obs.size() == 7 );
	//		m_SAResult.push_back( CSAResult(TRef, obs ) );
	//	}
	//	else if( header.size()==12)
	//	{
	//		std::vector<double> obs(7);
	//	
	//		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1,data[4].ToShort()-1);
	//		for(int c=0; c<7; c++)
	//			obs[c] = data[c+5].ToDouble();
	//	
	//
	//		ASSERT( obs.size() == 7 );
	//		m_SAResult.push_back( CSAResult(TRef, obs ) );
	//	}
	//	else if( header.size()==11)
	//	{
	//		std::vector<double> obs(7);
	//	
	//		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1);
	//		for(int c=0; c<7; c++)
	//			obs[c] = data[c+4].ToDouble();
	//	
	//
	//		ASSERT( obs.size() == 7 );
	//		m_SAResult.push_back( CSAResult(TRef, obs ) );
	//	}*/
	//}
	//
	//void CEvapotranspirationModel::GetFValueHourly(CStatisticXY& stat)
	//{
	//	if( m_SAResult.size() > 0)
	//	{
	//		CHourlyStatVector data;
	//		GetHourlyStat(data); 
	//
	//		for(size_t d=0; d<m_SAResult.size(); d++) 
	//		{ 
	//			if( m_SAResult[d].m_obs[m_varType] >-999 && data.IsInside( m_SAResult[d].m_ref))
	//			{
	//				static const int HOURLY_TYPE[6] = {HOURLY_T,HOURLY_TDEW,HOURLY_REL_HUM,HOURLY_WIND_SPEED,HOURLY_VPD,HOURLY_VPD};
	//				double obs = m_SAResult[d].m_obs[m_varType];
	//				double sim = data[m_SAResult[d].m_ref][HOURLY_TYPE[m_varType]];
	//
	//
	//				//double test = data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM];
	//				//CFL::RH2Td(data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM], data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM]);
	//
	//				if( !_isnan(sim)  && !_isnan(obs) &&
	//					 _finite(sim) &&  _finite(obs) )
	//					stat.Add(sim,obs);
	//			}
	//
	//			HxGridTestConnection();
	//				
	//		}
	//		
	///*
	//		if( m_SAResult[0].m_obs.size() == 24 )
	//		{
	//			//CTRef TRef = data.GetFirstTRef();
	//			//CStatistic statH[24];
	//			//for(int i=0; i<data.size(); i++, TRef++)
	//			//{
	//			//	double v = data[i][m_varType];
	//			//	statH[TRef.GetHour()]+=v;
	//			//	HxGridTestConnection();
	//			//}
	//
	//			//for(int y=0; y<m_weather.GetNbYear(); y++)
	//			//{
	//			//	double DD=0;
	//			//	for(int m=0; m<12; m++)
	//			//	{
	//			//		for(int d=0; d<m_weather[y][m].GetNbDay(); d++)
	//			//		{
	//			//			const CWeatherDay& wDay = m_weather[y][m][d];
	//			//			for(int h=0; h<24; h++)
	//			//			{
	//			//				
	//			//				//switch(m_varType)
	//			//				//{
	//			//				////case T_MN:
	//			//				//case TDEW: v= Min( wDay.GetT(h), GetVarH(wDay, h, var));break;
	//			//				//case RELH: v= Max(0, Min(100, GetVarH(wDay, h, var)));break;
	//			//				//case WNDS: v = Max(0, GetVarH(wDay, h, var));break;
	//			//				//}
	//
	//			//				statH[h]+=v;
	//			//				HxGridTestConnection();
	//			//			}
	//			//		}
	//			//	}
	//			//}
	//	
	//
	//			//ASSERT( m_SAResult.size() == 1 );
	//			//ASSERT( m_SAResult[0].m_obs.size() == 24 );
	//			//for(int h=0; h<24; h++)
	//			//{
	//			//	stat.Add(statH[h][MEAN], m_SAResult[0].m_obs[h]);
	//			//}
	//		}
	//		else if( m_SAResult[0].m_obs.size() == 7 )
	//		{
	//			
	//
	//			for(size_t i=0; i<m_SAResult.size(); i++)
	//			{
	//				
	//				if( m_SAResult[i].m_obs[m_varType] >-999 && data.IsInside( m_SAResult[i].m_ref))
	//				{
	//					double obs =  m_SAResult[i].m_obs[m_varType];
	//					double sim = data[m_SAResult[i].m_ref][m_varType];
	//					stat.Add(sim,obs);
	//				}
	//
	//				HxGridTestConnection();
	//				
	//			}
	//		}
	//		*/
	//	}
	//}
	//
	//void CEvapotranspirationModel::GetFValueDaily(CStatisticXY& stat)
	//{
	//  
	//	if( m_SAResult.size() > 0) 
	//	{
	//		OnExecuteDaily();
	//		const CDailyStatVector& data = (const CDailyStatVector&) GetOutput();
	//
	//		for(size_t i=0; i<m_SAResult.size(); i++)
	//		{
	//				
	//			if( m_SAResult[i].m_obs[m_varType] >-999 && data.IsInside( m_SAResult[i].m_ref))
	//			{
	//					
	//				static const int DAILY_TYPE[6] = {DAILY_TMIN, DAILY_TMAX, DAILY_MEAN_TDEW, DAILY_MEAN_REL_HUM, DAILY_MEAN_WNDS, DAILY_MEAN_VPD};
	//				double obs =  m_SAResult[i].m_obs[m_varType];
	//				double sim = data[m_SAResult[i].m_ref][DAILY_TYPE[m_varType]];
	//				stat.Add(sim,obs);
	//			}
	//
	//			HxGridTestConnection();
	//				
	//		}
	//	}
	//}
	//
	//
	//void CEvapotranspirationModel::GetFValueMonthly(CStatisticXY& stat)
	//{
	//
	//	if( m_SAResult.size() > 0)
	//	{
	//		
	//		OnExecuteMonthly();
	//		const CMonthlyStatVector& data = (const CMonthlyStatVector&) GetOutput();
	//
	//		for(size_t i=0; i<m_SAResult.size(); i++)
	//		{
	//				
	//			if( m_SAResult[i].m_obs[m_varType] >-999 && data.IsInside( m_SAResult[i].m_ref))
	//			{
	//
	//					
	//				static const int MONTHLY_TYPE[6] = {MONTHLY_MEAN_TMIN,MONTHLY_MEAN_TMAX, MONTHLY_MEAN_TDEW, MONTHLY_MEAN_REL_HUM, MONTHLY_MEAN_WNDS, MONTHLY_MEAN_VPD};
	//				double obs =  m_SAResult[i].m_obs[m_varType];
	//				double sim = data[m_SAResult[i].m_ref][MONTHLY_TYPE[m_varType]];
	//
	//				
	//
	//
	//
	//				stat.Add(sim,obs);
	//			}
	//
	//			HxGridTestConnection();
	//				
	//		}
	//	}
	//}
	//
}