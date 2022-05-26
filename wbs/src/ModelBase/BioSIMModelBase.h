//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#define _INTSAFE_H_INCLUDED_
#define NOMINMAX
#include <fstream>
#include <sstream>
#include <array>
#include <vector>
#include <deque>

#include "basic/ERMsg.h"
#include "Basic/WeatherStation.h"
#include "Basic/ModelStat.h"
#include "ModelBase/InputParam.h"
#include "ModelBase/ModelBaseError.h"
#include "ModelBase/CommunicationStream.h"


struct IAgent;

//Weather-Based Simulation Framework
namespace WBSF
{

	class CSAResult
	{
	public:

		CSAResult(CTRef ref = CTRef(), double obs = 0)
		{
			m_ref = ref;
			m_obs.push_back(obs);//by default only one observation
		}
		CSAResult(CTRef ref, const std::vector<double>& obs)
		{
			m_ref = ref;
			m_obs = obs;
		}


		CTRef m_ref;
		std::vector<double> m_obs;
	};

	typedef std::vector<CSAResult> CSAResultVector;

	//****************************************************************
	// CBioSIMModelBase  

	class CBioSIMModelBase
	{
	public:
		static const double VMISS;
		static const char* GetVersion();


		//hxGrid variable
		IAgent* m_pAgent;
		DWORD  m_hxGridSessionID;
		CTransferInfoIn m_info;
		CWeatherStation m_weather;			//holds weather and statistics
		CTimeStep m_timeStep;


		CBioSIMModelBase();
		virtual ~CBioSIMModelBase();


		void Reset();
		virtual ERMsg Init(const std::string& filePath);
		virtual ERMsg Init(std::istream& stream);

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		//virtual ERMsg OpenOutputFile( );
		virtual ERMsg Execute();
		virtual ERMsg OnExecuteHourly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteAtemporal();

		//virtual void CloseOutputFile();
		virtual ERMsg GetErrorMessage(int errorID);

		virtual ERMsg OpenInputInfoFile(const std::string& filePath);
		virtual ERMsg ReadWeather();
		//virtual ERMsg ReadWeatherBinary();


		const std::string& GetOutputFilePath()const{ return m_info.m_outputFilePath; }

		//Simulated Annealing functionality
		virtual void AddResult(const StringVector& header, const StringVector& data);
		virtual void AddHourlyResult(const StringVector& header, const StringVector& data){}
		virtual void AddDailyResult(const StringVector& header, const StringVector& data){}
		virtual void AddMonthlyResult(const StringVector& header, const StringVector& data){}
		virtual void AddAnnualResult(const StringVector& header, const StringVector& data){}
		virtual void AddAtemporalResult(const StringVector& header, const StringVector& data){}

		virtual void InitializeStat(CStatisticXY& stat){}
		virtual bool GetFValue(const std::vector<double>& paramArray, CStatisticXY& stat);
		virtual bool GetFValueHourly(CStatisticXY& stat) { ASSERT(false); return true;  }
		virtual bool GetFValueDaily(CStatisticXY& stat){ ASSERT(false); return true;}
		virtual bool GetFValueMonthly(CStatisticXY& stat){ ASSERT(false); return true;}
		virtual bool GetFValueAnnual(CStatisticXY& stat){ ASSERT(false); return true;}
		virtual bool GetFValueAtemporal(CStatisticXY& stat){ ASSERT(false); return true;}
		virtual bool FinalizeStat(CStatisticXY& stat){ return true; }




		void SetOutput(CModelStatVector& in){ m_output.swap(in); }
		const CModelStatVector& GetOutput()const{ return m_output; }

		bool HaveOutput(){ return m_output.size() > 0; }
		ERMsg SaveOutputFile();
		void GetOutStream(ERMsg msg, std::ostream& outStream)const;

		const CTransferInfoIn& GetInfo()const{ return m_info; }

		//	void ComputeSnow();

		static ERMsg GetFileText(const std::string& filePath, std::string& text);
		const CSAResultVector& GetSAResult()const{ return m_SAResult; }

		void InitRandomGenerator(size_t seed){ m_randomGenerator.Randomize(seed); }
		const CRandomGenerator& RandomGenerator()const{ return m_randomGenerator; }
		const CTimeStep& GetTimeStep()const{ return m_timeStep; }

		void HxGridTestConnection();

		const std::string& GetFileData(const std::string& filePath)const;
		const std::string& GetFileData(int index)const;

		//void GetOldWeather(CWeather& weather)const;

	protected:

		ERMsg VerifyNbInputColumn();
		void SaveTemporalReference(CTRef d);
		CTransferInfoOut FromTransferInfoIn()const;
		ERMsg FinaliseInit();


		short NB_INPUT_PARAMETER;
		std::string VERSION;

		//output
		CModelStatVector m_output;  //output as stream

		//Simulated Annealing result
		CSAResultVector m_SAResult;

		//new type of random generator
		CRandomGenerator m_randomGenerator;

	};



	typedef std::vector<CBioSIMModelBase*> CBioSIMModelBaseVector;

}