//*********************************************************************
// 10/09/2024	1.0		Rémi Saint-Amant	Create from java code
//*********************************************************************
#include "WaterBalance.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/WeatherStation.h"
#include "McCabeWBM.h"


using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace McCabeWBM;


namespace WBSF
{

	std::mutex CWaterBalanceModel::m_mutex;
	GDALDataset* CWaterBalanceModel::m_poDS = NULL;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CWaterBalanceModel::CreateObject);


	enum TInput { I_AWHC_FILE_PATH, I_DEFAULT_AWHC, I_RUNOFF_FACTOR, I_RAIN_TEMP_THRESHOLD, I_SNOW_TEMP_THRESHOLDUS, I_DIRECT_RUNOFF_FACTOR, I_MAX_MELT_RATE, NB_INTPUTS };
	enum TOutput { O_TEMP, O_PRCP, O_PET, O_PRECIPPET, O_SOILMOIST, O_AET, O_PETAET, O_SNOW, O_SURPLUS, O_RUNOFF, O_RODIRECT, O_SNOWMELT, NB_OUTPUTS };



	CWaterBalanceModel::CWaterBalanceModel()
	{
		NB_INPUT_PARAMETER = NB_INTPUTS;
		VERSION = "1.0.0 (2024)";
	}


	ERMsg CWaterBalanceModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;


		string file_path = parameters[I_AWHC_FILE_PATH].GetString();
		if (WBSF::FileExists(file_path))
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_poDS == NULL)
			{
				m_poDS = (GDALDataset*)GDALOpenEx(file_path.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
				if (m_poDS == NULL)
				{
					msg.ajoute("Unable to open: " + file_path);
				}
			}

		}



		return msg;
	}


	ERMsg CWaterBalanceModel::OnExecuteMonthly()
	{
		ERMsg msg;

		//init output
		CTPeriod p = m_weather.GetEntireTPeriod(CTM::MONTHLY);
		m_output.Init(p, NB_OUTPUTS);



		//Create McCabe parameters
		const CParameterVector& parameters = GetInfo().m_inputParameters;

		Parameters params;
		params["soilMoistureStorage"] = GetAWHC(GetInfo().m_loc);//Millimeters
		params["runoffFactor"] = Parameter(parameters[I_RUNOFF_FACTOR].GetReal(), 0, 100);//%
		params["rainTempThreshold"] = Parameter(parameters[I_RAIN_TEMP_THRESHOLD].GetReal(), 0, 5);//Degrees Celsius
		params["snowTempThreshold"] = Parameter(parameters[I_SNOW_TEMP_THRESHOLDUS].GetReal(), -15, 0);//Degrees Celsius
		params["directRunoffFactor"] = Parameter(parameters[I_DIRECT_RUNOFF_FACTOR].GetReal(), 0, 100);//%
		params["maxMeltRate"] = Parameter(parameters[I_MAX_MELT_RATE].GetReal(), 0, 100);//%
		params["latitude"] = Parameter(GetInfo().m_loc.m_lat, -90, 90);//Degrees







		//Create McCabe input file
		Inputs inputs(p.size());
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
				inputs[y * 12 + m] = Input(p.GetFirstYear() + y, m + 1, m_weather[y][m].GetStat(H_TNTX)[MEAN], m_weather[y][m].GetStat(H_PRCP)[SUM]);
			}
		}


		Outputs outputs = McCabeWaterBalanceModel::Execute(params, inputs);
		assert(outputs.size() == m_output.size());

		for (size_t i = 0; i < outputs.size(); i++)
		{
			m_output[i][O_TEMP] = outputs[i].m_temp; //Temperature (Degrees Celsius)
			m_output[i][O_PRCP] = outputs[i].m_prcp;//Precipitation (mm)
			m_output[i][O_PET] = outputs[i].m_pet;//Potential ETn (mm)
			m_output[i][O_PRECIPPET] = outputs[i].m_precipPet;//Precipitation - Pot ET (mm)
			m_output[i][O_SOILMOIST] = outputs[i].m_soilMoist;//Soil Moisture Storage (mm)
			m_output[i][O_AET] = outputs[i].m_aet; //Actual ET (mm)
			m_output[i][O_PETAET] = outputs[i].m_petAet;//Potential ET - Actual ET (mm)
			m_output[i][O_SNOW] = outputs[i].m_snow;//Snow Storage (mm)
			m_output[i][O_SURPLUS] = outputs[i].m_surplus; //Over storage Surplus (mm)
			m_output[i][O_RUNOFF] = outputs[i].m_totalRunoff;//Total Runoff (mm)
			m_output[i][O_RODIRECT] = outputs[i].m_directRunoff;//Direct Runoff (mm)
			m_output[i][O_SNOWMELT] = outputs[i].m_snowMelt;//Snow Melt (mm)
		}

		return msg;
	}


	//Code	Class	Description
	//1	<50mm	Available Water Holding Capacity of < 50mm.
	//2	50 - 99 mm	Available Water Holding Capacity of 50 - 99 mm.
	//3	100 - 149 mm	Available Water Holding Capacity of 100 - 149 mm.
	//4	150 - 199 mm	Available Water Holding Capacity of 150 - 199 mm.
	//5	200 - 250 mm	Available Water Holding Capacity of 200 - 250 mm.
	//6	Solonetzic or saline soils	Not applicable - solonetzic or saline soils.
	//7	High water table	Not applicable - high water table.
	//8	Perennially frozen subsoils	Not applicable - perennially frozen subsoils.
	//-	Water, ice or rock	Not applicable - water, ice or rock.

	double CWaterBalanceModel::GetAWHC(const CLocation& loc)
	{
		const CParameterVector& parameters = GetInfo().m_inputParameters;
		double AWHC = parameters[I_DEFAULT_AWHC].GetReal();


		//OGRRegisterAll();

		std::unique_lock<std::mutex> lock(m_mutex);

		if (m_poDS)
		{
			OGRLayer* poLayer = m_poDS->GetLayerByName("Canada_WHC");
			//    ###########################################################################
			//    # Create query geometry.

			poLayer->SetSpatialFilterRect(loc.m_x, loc.m_y, loc.m_x, loc.m_y);
			//poLayer->SetSpatialFilterRect(-71.0415, 59.0928, -71.0415, 59.0928);
			poLayer->ResetReading();

			//int cnt = poLayer->GetFeatureCount();

			OGRFeature* feat1 = poLayer->GetNextFeature();
			//OGRFeature* feat2 = poLayer->GetNextFeature();

			if (feat1 != NULL)
			{
				int WHCclass = feat1->GetFieldAsInteger(0);

				static const double AWHC_MAX[6] = { 150, 50, 100, 150, 200, 250 };//we used 150 when -!!!
				assert(WHCclass >= 0 && WHCclass < 6);
				AWHC = AWHC_MAX[WHCclass];
			}
		}


		return AWHC;
	}


}