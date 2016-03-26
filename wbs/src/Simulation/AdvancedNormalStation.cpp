//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"

#include "Basic/UtilTime.h"
#include "Basic/Mtrx.h"
#include "Simulation/AdvancedNormalStation.h"
#include "Newmat/Regression.h"


using namespace std;
using namespace WBSF::WEATHER;
using namespace WBSF::NORMALS_DATA;
using namespace WBSF::HOURLY_DATA;
using namespace NEWMAT;


namespace WBSF
{
	int CAdvancedNormalStation::NB_DAY_PER_MONTH_MIN = 20;
	int CAdvancedNormalStation::KIND_OF_VALIDATION = ONE_VALID;



	//********************

	size_t CANStatistic::GetNbDays()const
	{
		return GetNbDaysPerYear(m_year);
	}

	size_t CANStatistic::GetNbDaysPerMonthMin(size_t m)const
	{
		size_t nbDayPerMonthMin = CAdvancedNormalStation::GetNbDayPerMonthMin();
		return 	(nbDayPerMonthMin == CAdvancedNormalStation::FULL_MONTH) ?
			GetNbDayPerMonth(m_year, m) : nbDayPerMonthMin;
	}

	//*********************
	void CANStatisticVector::ComputeMonthStatistic(const CWeatherStation& dailyStation)
	{
		clear();

		resize(dailyStation.size());

		for (size_t y = 0; y < dailyStation.size(); y++)
		{
			if (dailyStation[y].HaveData())
			{
				const CWeatherYear& data = dailyStation[y];
				at(y).SetYear(data.GetTRef().GetYear());

				for (size_t m = 0; m < data.size(); m++)
				{
					for (size_t d = 0; d < data[m].size(); d++)
					{
						for (TVarH v = H_TAIR; v < NB_VAR_H; v++)
						{
							if (data[m][d][v].IsInit())
							{
								switch (v)
								{
								case H_TAIR:
									/*{
										double Tmin = data[m][d][H_TMIN][MEAN];
										double Tmax = data[m][d][H_TMAX][MEAN];
										at(y)[H_TAIR][m] += (Tmin + Tmax) / 2;
										at(y)[H_TRNG][m] += Tmax - Tmin;
										break;
										}*/
								case H_TRNG:	//break;
								case H_TDEW:
								case H_RELH:
								case H_WNDD:
								case H_SNDH:
								case H_PRES:
								case H_SWE:
								case H_ES:
								case H_EA:
								case H_VPD:
								case H_SRAD:	at(y)[v][m] += data[m][d].GetData(v)[MEAN]; break;
								case H_PRCP:
								case H_SNOW:	at(y)[v][m] += data[m][d].GetData(v)[SUM]; break;
								case H_WNDS:
								case H_WND2:
								{
									_ASSERTE(data[m][d].GetData(v)[MEAN] >= 0);
									at(y)[v][m] += log(max(0.00000000001, data[m][d][v][MEAN]));//take the log of the wind speed
									break;
								}
								case H_ADD1:
								case H_ADD2: break;
								default: ASSERT(false);
								}
							}
						}
					}
				}
			}
		}
	}
	//*********************************

	CAdvancedNormalStation::CAdvancedNormalStation()
	{
		Reset();
	}

	void CAdvancedNormalStation::Reset()
	{
		CLocation::Reset();

		//init statistic
		m_monthStatArray.clear();
		m_monthStat.Reset();
		m_dailyStat.Reset();


		for (size_t m = 0; m < 12; m++)
		{
			m_Tmin[m] = 0;
			m_Tmax[m] = 0;
			m_minMaxRelation[m] = 0;
			m_sigmaDelta[m] = 0;
			m_sigmaEpsilon[m] = 0;
			m_TCorrelation[m][0][0] = 0;
			m_TCorrelation[m][0][1] = 0;
			m_TCorrelation[m][1][0] = 0;
			m_TCorrelation[m][1][1] = 0;
		}


	}

	ERMsg CAdvancedNormalStation::GetNormalValidity(const CWeatherStation& station, int nbYearMin, int nbDayPerMonthMin, bool bValid[NB_VAR_H], int kindOfValidation)
	{
		ERMsg msg;

		//const CWeatherStation& me = station;


		if (!station.empty())
		{
			size_t nbYears[12][HOURLY_DATA::NB_VAR_H] = { 0 };

			for (size_t y = 0; y < station.size(); y++)
			{
				if (station[y].HaveData())
				{
					const CWeatherYear& data = station[y];

					for (size_t m = 0; m < 12; m++)
					{
						size_t nbDayMin = (nbDayPerMonthMin == FULL_MONTH) ? GetNbDayPerMonth(data.GetTRef().GetYear(), m) : nbDayPerMonthMin;

						CWVariablesCounter variables = data[m].GetVariablesCount();
						for (size_t v = 0; v < HOURLY_DATA::NB_VAR_H; v++)
						{
							//int c = CWeatherDefine::V2C(v);
							if (variables[v].first >= nbDayMin)
							{
								nbYears[m][v]++;
							}
						}
					}
				}
			}


			for (size_t v = 0; v < HOURLY_DATA::NB_VAR_H; v++)
				bValid[v] = true;

			for (size_t m = 0; m < 12; m++)
			{
				for (size_t v = 0; v < HOURLY_DATA::NB_VAR_H; v++)
				{
					if (nbYears[m][v] < nbYearMin)
					{
						bValid[v] = false;
					}
				}
			}

			switch (kindOfValidation)
			{
			case ONE_VALID:
			{
				bool bOneValid = false;
				for (size_t v = 0; v < H_WNDD/*HOURLY_DATA::NB_VAR_H*/; v++)
					bOneValid = bOneValid || bValid[v];

				if (!bOneValid)
				{
					msg.ajoute("not enought data for: " + station.m_name + " [" + station.m_ID + "]");
				}
			}
			break;
			case ALL_VALID:
			{
				bool bAllValid = true;
				for (size_t v = 0; v < H_WNDD/*HOURLY_DATA::NB_VAR_H*/; v++)
					bAllValid = bAllValid&&bValid[v];

				if (!bAllValid)
				{
					msg.ajoute("At least one variable is missing for: " + station.m_name + " [" + station.m_ID + "]");
				}
			}
			break;
			default: ASSERT(false);
			}
		}
		else
		{
			msg.ajoute("No data available for: " + station.m_name + " [" + station.m_ID + "]");
		}

		return msg;
	}

	ERMsg CAdvancedNormalStation::FromDaily(const CWeatherStation& dailyStation, int nbYearMinimum)
	{
		ERMsg msg;

		Reset();

		bool bValid[HOURLY_DATA::NB_VAR_H] = { 0 };


		msg = GetNormalValidity(dailyStation, nbYearMinimum, NB_DAY_PER_MONTH_MIN, bValid, KIND_OF_VALIDATION);
		//msg = GetNormalValidity(dailyStation, nbYearMinimum, bValid);

		if (!msg)
			return msg;

		m_monthStatArray.ComputeMonthStatistic(dailyStation);
		ASSERT(m_monthStatArray.size() >= nbYearMinimum);


		for (size_t y = 0; y < m_monthStatArray.size(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
				size_t nbDayMin = m_monthStatArray[y].GetNbDaysPerMonthMin(m);
				//int nbDayMin = NB_DAY_PER_MONTH_MIN==FULL_MONTH?UtilWin::GetNbDayPerMonth(m+1, data.GetYear() ):NB_DAY_PER_MONTH_MIN;

				for (size_t v = 0; v < NB_VAR_H; v++)
				{
					if (m_monthStatArray[y][v][m][NB_VALUE] >= nbDayMin)
					{
						m_dailyStat[v][m] += m_monthStatArray[y][v][m];
						size_t  correction = (v == H_PRCP) ? GetNbDayPerMonth(m_monthStatArray[y].GetYear(), m) : 1;

						m_monthStat[v][m] += m_monthStatArray[y][v][m][MEAN] * correction;

						ASSERT(v != H_PRCP || (NB_DAY_PER_MONTH_MIN != FULL_MONTH) || correction == m_monthStatArray[y][v][m][NB_VALUE]);
					}
				}
			}
		}

		if (bValid[H_TAIR])
		{
			//compute the 9 variables of temperature
			ComputeTemperature(dailyStation);
		}

		ASSERT(!dailyStation.GetYears().empty());
		CNormalsData data;
		//	data.m_TReferences[0] = *dailyStation.GetYears().begin();
		//	data.m_TReferences[1] = *dailyStation.GetYears().rbegin();


		ASSERT(NB_FIELDS == 16);
		for (size_t m = 0; m < 12; m++)
		{
			if (bValid[H_TAIR])
			{
				//data[m][TMIN_MN] = float(m_dailyStat[TMIN][m][MEAN]);
				//data[m][TMAX_MN] = float(m_dailyStat[TMAX][m][MEAN]);

				data[m][TMIN_MN] = float(m_Tmin[m]);
				data[m][TMAX_MN] = float(m_Tmax[m]);
				data[m][TMNMX_R] = float(m_minMaxRelation[m]);
				data[m][DEL_STD] = float(m_sigmaDelta[m]);
				data[m][EPS_STD] = float(m_sigmaEpsilon[m]);
				data[m][TACF_A1] = float(m_TCorrelation[m][0][0]);
				data[m][TACF_A2] = float(m_TCorrelation[m][0][1]);
				data[m][TACF_B1] = float(m_TCorrelation[m][1][0]);
				data[m][TACF_B2] = float(m_TCorrelation[m][1][1]);
			}

			if (bValid[H_PRCP])
			{
				ASSERT(m_monthStat[H_PRCP][m][NB_VALUE] >= nbYearMinimum);

				data[m][PRCP_TT] = float(m_monthStat[H_PRCP][m][MEAN]);
				data[m][PRCP_SD] = float(m_monthStat[H_PRCP][m][COEF_VAR]);
			}

			if (bValid[H_TDEW])
			{
				data[m][TDEW_MN] = float(m_dailyStat[H_TDEW][m][MEAN]);
			}

			if (bValid[H_RELH])
			{
				//data[m][TDEW_MN] = float(m_dailyStat[H_TDEW][m][MEAN]);
				data[m][RELH_MN] = float(m_dailyStat[H_RELH][m][MEAN]);
				data[m][RELH_SD] = float(m_dailyStat[H_RELH][m][STD_DEV]);
			}

			if (bValid[H_WNDS])
			{
				data[m][WNDS_MN] = float(m_dailyStat[H_WNDS][m][MEAN]);
				data[m][WNDS_SD] = float(m_dailyStat[H_WNDS][m][STD_DEV]);
			}
		}

		//Omn assigne la station
		((CLocation&)(*this)) = dailyStation;
		((CNormalsData&)(*this)) = data;


		return msg;
	}
	//
	//int GetMatrixPosition(const std::set<int>& years, CTRef& date)
	//{
	//	ASSERT( years.size() > 0);
	//
	//	size_t  m = date.GetMonth();
	//
	//	bool bFound=false;
	//	int pos = 0;
	//
	//	//autocorrelation is made by month
	//
	////	if( date.GetYear() >= years[0] && date.GetYear() <= years[years.GetUpperBound()] )
	////	{
	//	//for(size_t i=0; i<years.size(); i++)
	//	//we find
	//	for (std::set<int>::const_iterator it = years.begin(); it!=years.end(); it++)
	//	{
	//		if( *it == date.GetYear() )
	//		{
	//			pos += date.GetDay();
	//			bFound=true;
	//			break;
	//		}
	//		else
	//		{
	//			//pos += GetNbDays(*it);
	//			pos += GetNbDayPerMonth(*it, m);
	//		}
	//	}
	////	}
	//
	//	return bFound?pos:-1;
	//}


	int GetNbValidDay(const CMatrix < double> &matrix)
	{
		int nbDay = 0;

		for (int i = 0; i<matrix.size_y(); i++)
		{
			if (matrix[i][0] >-999 &&
				matrix[i][1] > -999 &&
				matrix[i][2] > -999)
			{
				nbDay++;
			}
		}

		return nbDay;
	}

	ERMsg ComputeMatrix(const CMatrix < double>& matrix, double& v1, double& v2)
	{
		ERMsg msg;

		int nbDay = GetNbValidDay(matrix);

		NEWMAT::Matrix M(nbDay, 2); //2 = t-1, t-2
		NEWMAT::ColumnVector t(nbDay);

		int p = 0;
		for (int i = 0; i<matrix.size_y(); i++)
		{
			if (matrix[i][0] >-999 &&
				matrix[i][1] > -999 &&
				matrix[i][2] > -999)
			{
				ASSERT(p < nbDay);
				t[p] = matrix[i][0];
				M[p][0] = matrix[i][1];
				M[p][1] = matrix[i][2];
				//std::string line;
				//line.Format( "%1d\t%6d\t%.1lf\t%.1lf\t%.1lf\n", v, p, t[p], M[p][0], M[p][1]);
				//file.WriteString(line);
				p++;
			}
		}

		Try
		{
			NEWMAT::ColumnVector fitted;
			NEWMAT::ColumnVector result;

			double R2 = DoRegression(M, t, result, fitted);

			v1 = result[1];
			v2 = result[2];
		}
			Catch(NEWMAT::Exception)
		{
			msg.asgType(ERMsg::ERREUR);

			if (NEWMAT::Exception::what() != NULL)
			{
				//TRACE( NEWMAT::Exception::what() );
				msg.ajoute(NEWMAT::Exception::what());
			}
		}

		return msg;
	}

	void CAdvancedNormalStation::ComputeTemperature(const CWeatherStation& dailyStation)
	{
		ERMsg msg;

		std::array<CStatistic, 12> statMin;
		std::array<CStatistic, 12> statMax;
		std::array<CStatistic, 12> statMinMax;
		std::array<std::array<CMatrix <double>, 2>, 12> matrix;

		CNormalsData normals;

		ASSERT(NB_FIELDS == 16);
		for (size_t m = 0; m < 12; m++)
		{
			double Tmean = float(m_dailyStat[H_TAIR][m][MEAN]);
			double Trange = float(m_dailyStat[H_TRNG][m][MEAN]);

			normals[m][TMIN_MN] = Tmean - Trange / 2;
			normals[m][TMAX_MN] = Tmean + Trange / 2;

			for (size_t v = 0; v < 2; v++)
			{
				matrix[m][v].resize(31 * m_monthStatArray.size(), 3);
				matrix[m][v].init(MISSING);
			}
		}

		normals.AdjustMonthlyMeans();

		std::set<int> years = dailyStation.GetYears();
		std::array<std::array<CStatisticVector, 12>, 2> annualStat;

		for (size_t m = 0; m < 12; m++)
		{
			annualStat[0][m].resize(dailyStation.size());
			annualStat[1][m].resize(dailyStation.size());
		}

		CTPeriod period = dailyStation.GetEntireTPeriod(CTM(CTM::DAILY));

		//calcul du STD_DEV, COEF_VAR
		for (size_t m = 0; m < 12; m++)
		{
			int p0 = 0;
			for (size_t y = 0; y < dailyStation.size(); y++)
			{
				int year = dailyStation[y].GetTRef().GetYear();
				if (dailyStation[y].HaveData())
				{
					size_t nbDayMin = m_monthStatArray[y].GetNbDaysPerMonthMin(m);

					CWVariablesCounter variables = dailyStation[y][m].GetVariablesCount();

					if (variables[H_TAIR].first >= nbDayMin)
					{
						ASSERT(m_monthStatArray[y][H_TAIR][m][NB_VALUE] >= nbDayMin);

						for (size_t d = 0; d < dailyStation[y][m].size(); d++)
						{
							CTRef TRef = dailyStation[y][m][d].GetTRef();


							double TminMonthNormal = normals.Interpole(TRef, TMIN_MN);
							double TmaxMonthNormal = normals.Interpole(TRef, TMAX_MN);

							if (dailyStation[TRef][H_TAIR].IsInit() && dailyStation[TRef][H_TRNG].IsInit())
							{
								double Tmin[3] = { MISSING, MISSING, MISSING };
								double Tmax[3] = { MISSING, MISSING, MISSING };
								for (size_t dd = 0; dd < 3; dd++)
								{
									CTRef shiftTRef = TRef - dd;
									if (period.IsInside(shiftTRef) &&
										dailyStation.IsYearInit(shiftTRef.GetYear()) &&
										dailyStation[TRef - dd][H_TAIR].IsInit() &&
										dailyStation[TRef - dd][H_TRNG].IsInit())
									{
										//remove interpolated mean
										Tmin[dd] = dailyStation[shiftTRef][H_TMIN][MEAN] - TminMonthNormal;
										Tmax[dd] = dailyStation[shiftTRef][H_TMAX][MEAN] - TmaxMonthNormal;
									}
								}

								annualStat[0][m][y] += Tmin[0];
								annualStat[1][m][y] += Tmax[0];

								statMin[m] += Tmin[0];
								statMax[m] += Tmax[0];
								statMinMax[m] += Tmin[0] * Tmax[0];

								//compute matrix for A1, A2, B1, B2 computation
								for (size_t dd = 0; dd < 3; dd++)
								{
									matrix[m][0][p0][dd] = Tmin[dd];
									matrix[m][1][p0][dd] = Tmax[dd];
								}
								p0++;
							}
						}
					}
				}
			}
		}

		for (size_t m = 0; m < 12; m++)
		{
			double Tmean = m_dailyStat[H_TAIR][m][MEAN];
			double Trange = m_dailyStat[H_TRNG][m][MEAN];
			m_Tmin[m] = Tmean - Trange / 2;
			m_Tmax[m] = Tmean + Trange / 2;
			m_minMaxRelation[m] = statMinMax[m][SUM] / sqrt(statMin[m][SUM²] * statMax[m][SUM²]);
			m_sigmaDelta[m] = statMin[m][STD_DEV];
			m_sigmaEpsilon[m] = statMax[m][STD_DEV];

			for (size_t v = 0; v < 2; v++)
			{
				ERMsg msgTmp = ComputeMatrix(matrix[m][v], m_TCorrelation[m][v][0], m_TCorrelation[m][v][1]);
				ASSERT(msgTmp);
			}
		}

	}

}