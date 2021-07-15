//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 28-11-2018	Rémi Saint-Amant	Change in NormalsWeith size NBVarH by NB_CATEGORIES
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 15-09-2015	Rémi Saint-Amant	Created from old file
//******************************************************************************
#include "stdafx.h"
#include <math.h>

#include "Basic/NormalsData.h"
#include "Basic/WeatherCorrection.h"
#include "Basic/UtilTime.h"
#include "Basic/WeatherCorrection.h"

#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::WEATHER;
using namespace WBSF::NORMALS_DATA;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::GRADIENT;


namespace WBSF
{



	//***********************************************************************************
	//CNormalsMonth

	CNormalsMonth::CNormalsMonth()
	{
		Reset();
	}


	double CNormalsMonth::GetP(double& sigma_gamma, double& sigma_zeta)const
	{
		double Rnx = at(TMNMX_R);
		double sigma_delta = at(DEL_STD);
		double sigma_epsilon = at(EPS_STD);
		double A1 = at(TACF_A1);
		double A2 = at(TACF_A2);
		double B1 = at(TACF_B1);
		double B2 = at(TACF_B2);

		//Régnière (2007) Equ [4] 
		double a[20] = { 0 };
		double b[20] = { 0 };
		a[0] = 1;
		a[1] = A1;
		b[0] = 1;
		b[1] = B1;
		for (int j = 2; j < 20; j++)
		{
			a[j] = A1*a[j - 1] + A2*a[j - 2];
			b[j] = B1*b[j - 1] + B2*b[j - 2];
		}
		double sum_a2 = 0;
		double sum_b2 = 0;
		double sum_ab = 0;
		for (int j = 0; j < 20; j++)
		{
			sum_a2 += a[j] * a[j];
			sum_b2 += b[j] * b[j];
			sum_ab += a[j] * b[j];
		}

		//Régnière (2007) Equ. [7]
		double P = Rnx * (sum_a2 / sum_ab);
		//Restrict |P| to [0,.95]
		if (P < -0.95) P = -0.95;
		if (P > 0.95) P = 0.95;


		//Régnière (2007) Equ [5] and [6]
		sigma_gamma = sigma_delta / pow(sum_a2, 0.5);
		sigma_zeta = sigma_delta / (1 - abs(P)) * pow((1 / sum_b2 - P*P / sum_a2), 0.5);

		return P;

	}

	bool CNormalsMonth::IsValid()const
	{
		bool bValid = true;
		for (int v = 0; v < NB_FIELDS&&bValid; v++)
			bValid = IsValid(v);

		return bValid;
	}

	bool CNormalsMonth::IsValid(size_t f)const
	{
		ASSERT(f >= 0 && f < NB_FIELDS);
		return IsMissing(at(f)) || ((at(f) >= GetLimitN(f, 0)) && (at(f) <= GetLimitN(f, 1)));
	}

	ERMsg CNormalsMonth::FromString(const std::string& str)
	{
		ERMsg msg;

		ASSERT(str.length() == LINE_LENGTH1);
		if (str.length() != LINE_LENGTH1)
		{
			msg.ajoute(FormatMsg(IDS_SIM_BAD_NORMAL_RECORD, WBSF::ToString(LINE_LENGTH1), WBSF::ToString(str.length())));
		}

		for (size_t i = 0; i < NB_FIELDS; i++)
		{
			int iostat = sscanf(str.substr(i*RECORD_LENGTH, RECORD_LENGTH).c_str(), "%f", &at(i));
			if (iostat < 1)
				at(i) = MISSING;
		}

		return msg;
	}

	CWVariables CNormalsMonth::GetVariables()const
	{
		using namespace HOURLY_DATA;
		static const size_t VARIABLES[NB_FIELDS] = { H_TAIR, H_TAIR, H_TAIR, H_TAIR, H_TAIR, H_TAIR, H_TAIR, H_TAIR, H_TAIR, H_PRCP, H_PRCP, H_TDEW, H_TDEW, H_TDEW, H_WNDS, H_WNDS };

		CWVariables vars;
		for (size_t f = 0; f < NB_FIELDS; f++)
			vars[VARIABLES[f]] = vars[VARIABLES[f]] || !IsMissing(at(f));

		if (vars[H_TAIR])
		{
			vars[H_TMIN] = true;
			vars[H_TMAX] = true;
		}
			
		if (vars[H_TDEW])
		{
			vars[H_RELH] = true;
		}

		return vars;
	}

	//***********************************************************************************
	//CNormalsData

	CNormalsData::CNormalsData()
	{
		Reset();
	}

	CNormalsData::CNormalsData(const CNormalsData& in)
	{
		operator =(in);
	}

	void CNormalsData::Reset()
	{
		for (size_t m = 0; m < 12; m++)
			at(m).Reset();

		m_bAdjusted = false;
	}

	void CNormalsData::Init(float v)
	{
		for (size_t m = 0; m < 12; m++)
			at(m).Init(v);
	}


	CNormalsData::~CNormalsData()
	{
	}

	void CNormalsData::GetVector(size_t j, float values[12])const
	{
		_ASSERTE(j >= 0 && j < NB_FIELDS);
		for (size_t m = 0; m < 12; m++)
			values[m] = at(m)[j];
	}


	void FindMonthIndex(size_t d, size_t& im, size_t& im1)
	{
		assert(d < 366);

		if (d < MidDayInMonth(0))
		{
			im = 0;
			im1 = 11;
		}
		else if (d >= MidDayInMonth(11))
		{
			im = 0;
			im1 = 11;
		}
		else
		{
			im = -1;
			for (int i = 1; i < 12; ++i)
			{
				if (d < MidDayInMonth(i))
				{
					im = i;
					im1 = i - 1;
					break;
				}

			}
		}

		assert(im < 12);
		assert(im1 < 12);
	}

	CNormalsData& CNormalsData::operator =(const CNormalsData& in)
	{
		if (&in != this)
		{
			CNormalsDataBase::operator = (in);
			m_bAdjusted = in.m_bAdjusted;
		}

		return *this;
	}
	//interpole a day for a climatic variable
	//d is the index of the day
	//v is the index of the climatic variable
	double CNormalsData::Interpole(CTRef TRef, size_t v)const
	{
		return Interpole(TRef.GetJDay(), v);
	}

	double CNormalsData::Interpole(size_t m, size_t d, size_t v)const
	{
		return Interpole(GetJDay(1995, m, d), v);
	}

	double CNormalsData::Interpole(size_t d, size_t v)const
	{
		//assert( d>=-2 && d<366);
		assert(d < 366);//mmmm
		assert(v < NB_FIELDS);
		assert(m_bAdjusted);

		size_t im = 0;
		size_t im1 = 0;
		FindMonthIndex(d, im, im1);


		double dx = (MidDayInMonth(im) - MidDayInMonth(im1));
		dx = dx >= 0 ? dx : dx + 365;
		_ASSERTE(dx >= 29.5 && dx <= 31);

		double slope = (at(im)[v] - at(im1)[v]) / dx;

		double ptx = (d - MidDayInMonth(im1));
		ptx = ptx >= 0 ? ptx : ptx + 365;
		_ASSERTE(ptx >= 0 && ptx < 31);

		return at(im1)[v] + slope*ptx;
	}


	void CNormalsData::AdjustMonthlyMeans()
	{
		assert(!m_bAdjusted);
		if (m_bAdjusted)
			return;

		m_bAdjusted = true;
		double meanTMin[12] = { 0 };
		double meanTMax[12] = { 0 };

		for (size_t m = 0; m < 12; m++)
		{
			meanTMin[m] = at(m)[TMIN_MN];
			meanTMax[m] = at(m)[TMAX_MN];
		}

		for (size_t k = 0; k < 5; k++)
		{
			size_t jd = 0;
			for (size_t m = 0; m < 12; m++)
			{
				double meanMin = 0;
				double meanMax = 0;

				for (size_t j = 0; j < GetNbDayPerMonth(m); j++)
				{
					meanMin += Interpole(jd, TMIN_MN);
					meanMax += Interpole(jd, TMAX_MN);
					jd++;
				}

				meanMin /= GetNbDayPerMonth(m);
				meanMax /= GetNbDayPerMonth(m);

				double deltaMin = meanTMin[m] - meanMin;
				double deltaMax = meanTMax[m] - meanMax;

				at(m)[TMIN_MN] += float(deltaMin);
				at(m)[TMAX_MN] += float(deltaMax);
			}
		}

	}

	CWVariables CNormalsData::GetVariables()const
	{
		CWVariables vars;
		for (int m = 0; m < 12; m++)
			vars |= at(m).GetVariables();

		return vars;
	}


	bool CNormalsData::HaveField(size_t f)const
	{
		ASSERT(f < NB_FIELDS);

		bool bRep = false;

		for (size_t m = 0; m < 12; m++)
		{
			if (!IsMissing(at(m)[f]))
			{
				bRep = true;
				break;
			}
		}

		return bRep;
	}

	bool CNormalsData::HaveNoDataField(size_t f)const
	{
		ASSERT(f < NB_FIELDS);

		bool bRep = false;

		for (size_t m = 0; m < 12; m++)
		{
			if (IsMissing(at(m)[f]))
			{
				bRep = true;
				break;
			}
		}

		return bRep;
	}

	bool CNormalsData::IsValidField(size_t f)const
	{
		ASSERT(f < NB_FIELDS);

		bool bValid = true;
		for (size_t m = 0; m < 12 && bValid; m++)
			bValid = at(m).IsValid(f);

		return bValid;
	}


	CNormalsData& CNormalsData::Copy(const CNormalsData& data, CWVariables variables)
	{
		assert(!m_bAdjusted);
		assert(!data.m_bAdjusted);



		if (this != &data)
		{
			bitset<NB_CATEGORIES> categories = GetCategories(variables);

			for (size_t m = 0; m < 12; m++)
			{
				for (size_t f = 0; f < NB_FIELDS; f++)
				{
					size_t c = GetCategoryN(f);
					if (categories[c])
//					if (variables[F2V(f)])
					{
						ASSERT(data.at(m).IsValid(f));
						at(m)[f] = data.at(m)[f];
					}
				}
			}
		}

		return *this;
	}


	CNormalsData& CNormalsData::operator +=(const CNormalsData& data)
	{
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t j = 0; j < NB_FIELDS; j++)
			{
				if (!IsMissing(data.at(m)[j]))
				{
					if (!IsMissing(at(m)[j]))
						at(m)[j] += data.at(m)[j];
					else at(m)[j] = data.at(m)[j];
				}
			}
		}

		return *this;

	}

	CNormalsData& CNormalsData::operator /=(float value)
	{
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t j = 0; j < NB_FIELDS; j++)
			{
				if (!IsMissing(at(m)[j]))
					at(m)[j] /= value;
			}
		}

		return *this;
	}


	bool CNormalsData::operator ==(const CNormalsData& data)const
	{
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t j = 0; j<NB_FIELDS; j++)
			{
				double fDiff = at(m)[j] - data.at(m)[j];
				if (fabs(fDiff) > 0.0001)
					return false;
			}
		}

		return true;
	}

	bool CNormalsData::operator !=(const CNormalsData& data)const
	{
		return !((*this) == data);
	}


	//**************************************************

	void CNormalsData::SetInverseDistanceMean(CWVariables variables, const CNormalDataVector& normalVector, const CNormalWeight& weight)
	{
		CNormalsData& me = *this;

		//average normals

		bitset<NB_CATEGORIES> categories = GetCategories(variables);

		if (!normalVector.empty())
		{
			for (size_t f = 0; f < NB_FIELDS; f++)
			{
				size_t c = GetCategoryN(f);
				//if (variables[v])
				if (categories[c])
				{
					ASSERT(normalVector.size() == weight[c].size());
					for (size_t m = 0; m < 12; m++)
					{
						if (f >= TACF_A1 && f <= TACF_B2)
						{
							//only take the nearest
							me[m][f] = normalVector[0][m][f];
						}
						else
						{
							me[m][f] = 0;
							for (size_t i = 0; i < normalVector.size(); i++)
							{
								ASSERT(WEATHER::HaveValue(normalVector[i][m][f]));
								ASSERT(normalVector[i][m].IsValid(f));

								me[m][f] += float(weight[c][i] * normalVector[i][m][f]);
							}

							if (f == PRCP_TT && me[m][f] < 0)
								me[m][f] = 0;

							me[m][f] = Round(me[m][f], GetNormalDataPrecision(f));
						}
					}
				}
			}
		}
	}


	ERMsg CNormalsData::IsValid()const
	{
		ERMsg msg;

		CWVariables vars = GetVariables();

		if (!vars.any())
		{
			msg.ajoute(GetString(IDS_SIM_NORMAL_WITHOUT_DATA));
		}

		for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS; f++)
		{
			size_t v = F2V(f);
			if (vars[v] && HaveNoDataField(f))
			{
				msg.ajoute(FormatMsg(IDS_SIM_NORMAL_WITH_NODATA, GetFieldTitle(f)));
			}

			if (!IsValidField(f))
			{
				msg.ajoute(FormatMsg(IDS_SIM_NORMAL_NOTVALID, GetFieldTitle(f)));
			}
		}

		return msg;

	}


	ERMsg CNormalsData::LoadV2(std::istream& stream)
	{
		ERMsg msg;

		for (size_t m = 0; m < 12 && msg; m++)
		{
			if (!stream.eof())
			{
				string line;
				std::getline(stream, line);
				msg = at(m).FromString(line);
			}
			else
			{
				msg.ajoute("Bad Normals Database");
			}

		}

		return msg;
	}

	std::ostream& CNormalsData::operator << (std::ostream& stream)const
	{
		stream.write(reinterpret_cast<char const*>(data()), 12* NORMALS_DATA::NB_FIELDS*sizeof(float));
		return stream;
	}

	std::istream& CNormalsData::operator >> (std::istream& stream)
	{
		stream.read(reinterpret_cast<char *>(data()), 12 * NORMALS_DATA::NB_FIELDS * sizeof(float));
		return stream;
	}

}//namespace WBSF