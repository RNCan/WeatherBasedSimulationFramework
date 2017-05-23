//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 10/12/2014	Rémi Saint-Amant	remove snowmeld and MTCLim. All these stuf stranfer to BioSIM
// 09/04/2014	Rémi Saint-Amant	Update to UNICODE and VS2013
// 02/06/2013	Rémi Saint-Amant	Add Direct file transfer
// 19/02/2010	Rémi Saint-Amant	Change in the GetModelVersion
// 13/02/2009	Rémi Saint-Amant	Add new SnowMelt models
// 09/02/2009	Rémi Saint-Amant	Move CSimulatedAnnealingVector in SimulatedAnnealingVector.cpp
// 03/02/2009	Rémi Saint-Amant	Move Error and Entry point in EntryPoint.cpp
// 11/12/2008	Rémi Saint-Amant	Add Simulated Annealing functionality
// 24/10/2008   Rémi Saint-Amant	New number of reference
// 11/12/2007   Rémi Saint-Amant	New BioSIM model base. Intermediate version
// 22/03/2007 	Rémi Saint-Amant	Replace CCFLString By std::string
// 01/02/2004	Rémi Saint-Amant	Addition of MTClim43 to compute radiation and VPD (Vapor Pressure Deficit)
// 13/01/2004	Rémi Saint-Amant	Implement new file transfer format
// 27/10/2003   Rémi Saint-Amant    Addition of Overheat and GetHourlyTemperature
// 01/01/2002   Rémi Saint-Amant    Creation from existing code.
//*********************************************************************

#include "stdafx.h"
#include <stdio.h>
#include <math.h>
#include <fstream>

#include "Basic/WeatherDefine.h"
#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/EntryPoint.h"
#include "hxGrid/Interface/IAgent.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{



	const double CBioSIMModelBase::VMISS = WEATHER::MISSING;

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////
	//Simulated Annealing part



	CBioSIMModelBase::CBioSIMModelBase() :
		m_randomGenerator(1)
	{
		m_pAgent = NULL;
		m_hxGridSessionID = 0;
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0";

		Reset();

	}


	CBioSIMModelBase::~CBioSIMModelBase()
	{}

	bool VerifyData(const CWeatherStation& weather)
	{
		bool bRep = true;

		CWVariables variables1 = weather.GetVariables();

		CWVariables variables2;
		CTPeriod p = weather.GetEntireTPeriod();
		for (CTRef TRef = p.Begin(); TRef <= p.End() && bRep; TRef++)
			variables2 |= weather[TRef].GetVariables();

		bRep = variables1 == variables2;
		ASSERT(bRep);

		return bRep;
	}

	ERMsg CBioSIMModelBase::Execute()
	{
		ERMsg msg;

		try
		{
			ASSERT(VerifyData(m_weather));

			//set default timestep
			if (msg)
				m_timeStep = m_weather.IsHourly() ? 1 : 4;

			//automatically generate random number		
			InitRandomGenerator(m_info.m_seed);
			Randomize((unsigned int)m_info.m_seed);//init old random number just in case


			switch (m_info.m_TM.Type())
			{
			case CTRef::HOURLY: msg = OnExecuteHourly(); break;
			case CTRef::DAILY: msg = OnExecuteDaily(); break;
			case CTRef::MONTHLY: msg = OnExecuteMonthly(); break;
			case CTRef::ANNUAL: msg = OnExecuteAnnual(); break;
			case CTRef::ATEMPORAL: msg = OnExecuteAtemporal(); break;
			default: msg = GetErrorMessage(ERROR_REF_NOT_SUPPORT);
			}
		}
		catch (ERMsg e)
		{
			msg = e;
		}
		catch (char* e)
		{
			msg.ajoute(e);
		}
		catch (...)
		{
			msg = GetErrorMessage(0);
		}

		return msg;
	}


	ERMsg CBioSIMModelBase::OnExecuteHourly(){ return GetErrorMessage(ERROR_REF_NOT_SUPPORT); }
	ERMsg CBioSIMModelBase::OnExecuteDaily(){ return GetErrorMessage(ERROR_REF_NOT_SUPPORT); }
	ERMsg CBioSIMModelBase::OnExecuteMonthly(){ return GetErrorMessage(ERROR_REF_NOT_SUPPORT); }
	ERMsg CBioSIMModelBase::OnExecuteAnnual(){ return GetErrorMessage(ERROR_REF_NOT_SUPPORT); }
	ERMsg CBioSIMModelBase::OnExecuteAtemporal(){ return GetErrorMessage(ERROR_REF_NOT_SUPPORT); }

	//override this method to close the file in a different way
	//void CBioSIMModelBase::CloseOutputFile()
	//{
	//	m_outputFile.close();
	//}

	void CBioSIMModelBase::Reset()
	{
		m_info.Reset();
		m_weather.Reset();
	}

	ERMsg CBioSIMModelBase::Init(const std::string& filePath)
	{
		ERMsg msg;

		Reset();

		//Read parameter file
		msg = OpenInputInfoFile(filePath);

		//Finalize
		if (msg)
			msg = FinaliseInit();

		return msg;
	}

	ERMsg CBioSIMModelBase::FinaliseInit()
	{
		ERMsg msg;

		SetLanguage(m_info.m_language);//Set global language variable

		if ((NB_INPUT_PARAMETER == -1 || m_info.m_inputParameters.size() == NB_INPUT_PARAMETER))
		{
			//process parameters
			msg += ProcessParameters(m_info.m_inputParameters);
		}
		else
		{
			//the number of input parameters in the model is incorrect
			msg = GetErrorMessage(ERROR_BAD_NUMBER_PARAMETER);
		}

		return msg;
	}

	ERMsg CBioSIMModelBase::Init(istream& stream)
	{
		ERMsg msg;

		Reset();

		msg = CCommunicationStream::ReadInputStream(stream, m_info, m_weather);
		m_weather.m_pAgent = m_pAgent;
		m_weather.m_hxGridSessionID = m_hxGridSessionID;

		//Finalize
		if (msg)
			msg = FinaliseInit();

		return msg;
	}

	CTransferInfoOut CBioSIMModelBase::FromTransferInfoIn()const
	{
		CTransferInfoOut infoOut;
		infoOut.m_locCounter = m_info.m_locCounter;
		infoOut.m_paramCounter = m_info.m_paramCounter;
		infoOut.m_repCounter = m_info.m_repCounter;
		infoOut.m_TM = m_info.m_TM;

		return infoOut;
	}
	void CBioSIMModelBase::GetOutStream(ERMsg msg, std::ostream& outStream)const
	{
		CTransferInfoOut info = FromTransferInfoIn();
		info.m_msg = msg;
		CCommunicationStream::WriteOutputStream(info, m_output, outStream);
	}

	ERMsg CBioSIMModelBase::OpenInputInfoFile(const std::string& filePath)
	{
		ERMsg msg;

		msg = m_info.Load(filePath);
		if (msg)
			msg = m_weather.LoadData(m_info.m_inputWeatherFilePath);

		return msg;
	}

	ERMsg CBioSIMModelBase::ReadWeather()
	{
		return m_weather.LoadData(m_info.m_inputWeatherFilePath);
	}

	ERMsg CBioSIMModelBase::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;
		ASSERT(parameters.empty());//humm add a 's' to parameter
		return msg;
	}

	ERMsg CBioSIMModelBase::GetErrorMessage(int errorID)
	{
		return ::GetErrorMessage(errorID, m_info.m_language);
	}

	void CBioSIMModelBase::AddResult(const StringVector& header, const StringVector& data)
	{
		switch (m_info.m_TM.Type())
		{
		case CTRef::HOURLY: AddHourlyResult(header, data); break;
		case CTRef::DAILY: AddDailyResult(header, data); break;
		case CTRef::MONTHLY: AddMonthlyResult(header, data); break;
		case CTRef::ANNUAL: AddAnnualResult(header, data); break;
		case CTRef::ATEMPORAL: AddAtemporalResult(header, data); break;
		default: _ASSERTE(false);
		}
	}

	void CBioSIMModelBase::GetFValue(const std::vector<double>& paramArray, CStatisticXY& stat)
	{
		Randomize((unsigned int)m_info.m_seed);
		InitRandomGenerator(m_info.m_seed);

		for (size_t i = 0, j = 0; i < m_info.m_inputParameters.size() && j < paramArray.size(); i++)
		{
			if (m_info.m_inputParameters[i].IsVariable())
			{
				std::string value;
				value = ToString(paramArray[j]);
				m_info.m_inputParameters[i].SetString(value);
				j++;
			}
		}

		ProcessParameters(m_info.m_inputParameters);

		InitializeStat(stat);

		for (size_t i = 0; i < m_info.m_repCounter.GetTotal(); i++)
		{
			CStatisticXY statTmp;

			switch (m_info.m_TM.Type())
			{
			case CTM::HOURLY: GetFValueHourly(statTmp); break;
			case CTM::DAILY: GetFValueDaily(statTmp); break;
			case CTM::MONTHLY: GetFValueMonthly(statTmp); break;
			case CTM::ANNUAL: GetFValueAnnual(statTmp); break;
			case CTM::ATEMPORAL: GetFValueAtemporal(statTmp); break;
			default: _ASSERTE(false);
			}

			stat += statTmp;
		}


		FinalizeStat(stat);
	}
	//
	//void CBioSIMModelBase::SaveTemporalReference(CTRef d)
	//{
	//	if (m_info.m_TM.Mode() == CTM::FOR_EACH_YEAR)
	//	{
	//		if (m_info.m_TM.Type() != CTM::ATEMPORAL)
	//			m_outputFile << d.GetYear();
	//		else
	//			m_outputFile << d.m_ref;
	//	}
	//
	//	switch (m_info.m_TM.Type())
	//	{
	//	case CTM::HOURLY: m_outputFile << ',' << d.GetMonth() + 1 << ',' << d.GetDay() + 1 << ',' << d.GetHour(); break;
	//	case CTM::DAILY: m_outputFile << ',' << d.GetMonth() + 1 << ',' << d.GetDay() + 1; break;
	//	case CTM::MONTHLY: m_outputFile << ',' << d.GetMonth() + 1; break;
	//	case CTM::ANNUAL: break; 
	//	case CTM::ATEMPORAL: break; //m_outputFile << d.m_ref; break; 
	//	default: _ASSERTE(false);
	//	}
	//}

	ERMsg CBioSIMModelBase::SaveOutputFile()
	{
		assert(m_output.GetFirstTRef().GetTM() == m_info.m_TM);
		return m_output.Save(m_info.m_outputFilePath);
	}

	ERMsg CBioSIMModelBase::GetFileText(const std::string& filePath, std::string& text)
	{
		ERMsg msg;

		ifStream file;
		msg = file.open(filePath);
		if (msg)
		{
			text = file.GetText();
			file.close();
		}

		return msg;
	}




	const char* CBioSIMModelBase::GetVersion()
	{
		static char version[250] = { 0 };
		//We have to create an object to update the version of the DLL
		CBioSIMModelBase* pModel = CModelFactory::CreateObject();
		_ASSERTE(pModel);
		strcpy(version, pModel->VERSION.c_str());
		delete pModel; pModel = NULL;

		return version;
	}

	const std::string& CBioSIMModelBase::GetFileData(const std::string& speciesFilePath)const
	{
		return GetStaticDataStream().GetFileData(speciesFilePath);
	}

	const std::string& CBioSIMModelBase::GetFileData(int index)const
	{
		return GetStaticDataStream().GetFileData(index);
	}

	void CBioSIMModelBase::HxGridTestConnection()
	{
		if (m_pAgent && m_pAgent->TestConnection(m_hxGridSessionID) != S_OK)
			throw("hxGrid agent cancel");
	}

}