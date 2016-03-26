//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 10-02-2009 	Remi Saint-Amant	Inherite from vector
// 01-01-2005	Remi Saint-Amant	Creation
//******************************************************************************
#include "stdafx.h"
#include <algorithm>

#include "Basic/ModelStat.h"
#include "Basic/UtilStd.h"
#include "Basic/WeatherDefine.h"
#include "Basic/CSV.h"


//size_t  operator ""s ("_z", unsigned long long n);
//size_t operator ""s _z(unsigned long long n){return n;}

//constexpr
using namespace std;

namespace WBSF
{

	CInitialPopulation::CInitialPopulation(CTRef peakDay, double sigma, size_t nbObjects, double initialPopulation, double age, size_t sex, bool bFertil, size_t generation)
	{
		Initialize(peakDay, sigma, nbObjects, initialPopulation, age, sex, bFertil, generation);
	}
	void CInitialPopulation::Initialize(CTRef peakDay, double sigma, size_t nbObjects, double initialPopulation, double age, size_t sex, bool bFertil, size_t generation)
	{
		ASSERT(nbObjects > 0);
		ASSERT(initialPopulation > 0);
		ASSERT((peakDay.GetJDay() - 3 * sigma) >= 0 && (peakDay.GetJDay() + 3 * sigma) <= 365); //all attack within first year

		clear();
		if (peakDay.IsInit())
		{
			size_t nbCreated = 0;
			double cumulCreated = 0;
			double scaleFactor = initialPopulation / nbObjects;

			for (CTRef TRef = peakDay - int(3 * sigma); TRef <= peakDay + int(3 * sigma) && nbCreated < nbObjects; TRef++)
			{
				cumulCreated += sigma>0 ? nbObjects / (sigma*pow((2 * PI), 0.5)) * exp(-0.5*Square((TRef - peakDay) / sigma)) : nbObjects;

				size_t nbObjectToCreate = max(size_t(0), Round<size_t>(cumulCreated - nbCreated));
				for (size_t i = 0; i < nbObjectToCreate; i++)
					push_back(CIndividualInfo(TRef, age, sex, bFertil, generation, scaleFactor));

				nbCreated += nbObjectToCreate;
			}

			ASSERT(size() == nbObjects);

			while (size() < nbObjects)
				push_back(CIndividualInfo(peakDay, age, sex, bFertil, generation, scaleFactor));


			ASSERT(size() == nbObjects);
		}
	}

	//**********************************************************************************************
	

	CModelStatVector::CModelStatVector(const CModelStatVector& in, CTM TM, size_t s)
	{
		Init(in,TM,s);
	}

	CModelStatVector::CModelStatVector(const CModelStatVector& in, const CTTransformation& TT, size_t s)
	{
		Init(in, TT, s);
	}

	CModelStatVector::CModelStatVector(const CTStatMatrix& in, size_t s)
	{
		Init(in, s);
	}

	void CModelStatVector::Init(const CModelStatVector& in, CTM TM, size_t s)
	{
		CTPeriod p1 = in.GetTPeriod();
		CTPeriod p2 = p1.Get(TM);
		Init(in, CTTransformation(p1, p2), s);
	}

	void CModelStatVector::Init(const CModelStatVector& in, const CTTransformation& TT, size_t s)
	{
		operator=(in);
		Transform(TT, s);
	}

	

	void CModelStatVector::Init(const CTStatMatrix& in, size_t s)
	{
		Init(in.m_period, in.size_x(), -999);
		for (size_t i = 0; i < in.size(); i++)
		{
			for (size_t j = 0; j < in[i].size(); j++)
			{
				if (in[i][j].IsInit())
					at(i).at(j) = in[i][j][s];
			}
		}
	}

	CModelStatVector& CModelStatVector::operator=(const CModelStatVector& in)
	{
		if (this != &in)
		{
			CModelStatVectorBase::operator =(in);
			m_firstTRef = in.m_firstTRef;
			m_nbStat = in.m_nbStat;
			m_missingValue = in.m_missingValue;
			m_header = in.m_header;
		}

		return *this;
	}


	CModelStatVector& CModelStatVector::swap(CModelStatVector& in)
	{
		if (this != &in)
		{
			CModelStatVectorBase::swap(in);
			std::swap(m_firstTRef, in.m_firstTRef);
			std::swap(m_nbStat, in.m_nbStat);
			std::swap(m_missingValue, in.m_missingValue);
			m_header.swap(in.m_header);

		}

		return *this;
	}
	__int32 CModelStatVector::GetFirstIndex(size_t s, double threshold, int nbDayBefore, const CTPeriod& pIn)const
	{
		const CModelStatVector& me = *this;

		CTPeriod p = pIn;
		if (!p.IsInit())
			p = GetTPeriod();

		ASSERT(GetTPeriod().IsInside(p));

		int fd = p.Begin() - m_firstTRef;
		int ld = p.End() - m_firstTRef;

		int firstDay = -1;
		for (int i = fd; i <= ld; i++)
		{
			if (me[i][s] > threshold)
			{
				firstDay = max(0, i - nbDayBefore);
				break;
			}
		}

		_ASSERTE(firstDay >= -1 && firstDay < size());

		return firstDay;
	}


	__int32 CModelStatVector::GetLastIndex(size_t s, double threshold, int nbDayAfter, const CTPeriod& pIn)const
	{
		const CModelStatVector& me = *this;
		int lastDay = -1;

		CTPeriod p = pIn;
		if (!p.IsInit())
			p = GetTPeriod();

		ASSERT(GetTPeriod().IsInside(p));

		int fd = p.Begin() - m_firstTRef;
		int ld = p.End() - m_firstTRef;
		for (int i = ld; i >= fd&&lastDay == -1; i--)
		{
			if (me[i][s] > threshold)
				lastDay = std::min(int(size() - 1), i + nbDayAfter);
		}

		_ASSERTE(lastDay >= -1 && lastDay < size());

		return lastDay;
	}

	CStatistic CModelStatVector::GetStat(size_t s, const CTPeriod& period)const
	{
		const CModelStatVector& me = *this;
		CStatistic stat;

		for (size_t i = 0; i < size(); i++)
		{
			if (period.IsInside(m_firstTRef + i))
				stat += me[i][s];
		}

		return stat;
	}

	void CModelStatVector::WriteStream(std::ostream& stream)const
	{
		const CModelStatVector& me = *this;

		__int64 version = 1;
		__int64 nbRows = (__int64)size();//GetNbUsed();
		__int64 nbCols = (nbRows>0) ? m_nbStat + 1 : 0;
		__int64 rawSize = nbRows*nbCols*sizeof(float);
		__int64 realSize = (__int64)size();
		__int64 nbStat = m_nbStat;
		__int64 Tref = GetFirstTRef().Get__int32();

		//CTRef ref(d.GetYear(),d.GetMonth(),d.GetDay());

		stream.write((char*)(&version), sizeof(version));
		stream.write((char*)(&rawSize), sizeof(rawSize));
		stream.write((char*)(&nbRows), sizeof(nbRows));
		stream.write((char*)(&nbCols), sizeof(nbCols));
		stream.write((char*)(&realSize), sizeof(realSize));
		stream.write((char*)(&nbStat), sizeof(nbStat));
		stream.write((char*)(&Tref), sizeof(Tref));

		for (size_t i = 0; i < size(); i++)
		{
			for (size_t j = 0; j < m_nbStat; j++)
			{
				ASSERT(me[i][j] < FLT_MAX);
				float value = float(me[i][j]);
				stream.write((char*)(&value), sizeof(value));
			}
		}
	}

	ERMsg CModelStatVector::ReadStream(std::istream& stream)
	{
		ERMsg msg;

		CModelStatVector& me = *this;

		__int64 version = -1;
		__int64 rawSize = 0;
		__int64 nbRows = 0;
		__int64 nbCols = 0;
		__int64 realSize = 0;
		__int64 nbStat = 0;
		__int64 Tref = 0;


		stream.read((char*)(&version), sizeof(version));
		stream.read((char*)(&rawSize), sizeof(rawSize));
		stream.read((char*)(&nbRows), sizeof(nbRows));
		stream.read((char*)(&nbCols), sizeof(nbCols));
		stream.read((char*)(&realSize), sizeof(realSize));
		stream.read((char*)(&nbStat), sizeof(nbStat));
		stream.read((char*)(&Tref), sizeof(Tref));
		resize((size_t)realSize, (size_t)nbStat, 0);
		m_firstTRef.Set__int32((int)Tref);

		for (size_t i = 0; i < size(); i++)
		{
			for (size_t j = 0; j < m_nbStat; j++)
			{
				float value = 0;
				stream.read((char*)(&value), sizeof(value));
				me[i][j] = value;
			}
		}


		_ASSERTE(version == 1);
		_ASSERTE(rawSize == nbRows*nbCols*sizeof(float));
		_ASSERTE(m_firstTRef.IsValid());

		return msg;
	}



	ERMsg CModelStatVector::Load(const std::string& filePath)
	{
		ERMsg msg;


		ifStream file;
		msg = file.open(filePath);

		//bool bHaveHeader
		//They have header or not????
		//	if (line.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") == string::npos)
		//	file.SeekToBegin();

		if (msg)
		{
			CSVIterator loop(file);
			m_header = loop.Header().to_string();
			assert(m_header.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != string::npos);

			CWeatherFormat format(m_header.c_str());
			size_t nbReference = format.GetTM().GetNbTimeReferences();
			m_nbStat = loop.Header().size() - nbReference;
			assert(m_nbStat > 0 && m_nbStat <= loop.Header().size());

			size_t line = 1;
			for (; loop != CSVIterator(); ++loop, line++)
			{
				//resize(size() + 1);
				if (!(*loop).empty())
				{
					CTRef TRef = format.GetTRef(*loop);
					if (msg)
					{
						if ((*loop).size() == nbReference + m_nbStat)
						{
							CModelStat row(m_nbStat);
							for (size_t j = nbReference; j < (*loop).size(); j++)
							{
								row[j - nbReference] = ToDouble((*loop)[j]);
							}

							Insert(TRef, row);
						}
						else
						{
							//msg.ajoute(FormatMsg(IDS_SIM_BAD_OUTPUTFILE_FORMAT, filePath, size()));
							msg.ajoute("Problem reading result file " + filePath + " at line " + ToString(line));
							msg.ajoute((*loop).GetLastLine());
						}
					}
				}//no empty
			}//for all lines

			file.close();
		}
		return msg;
	}

	ERMsg CModelStatVector::Save(const std::string& filePath)const
	{
		ERMsg msg;
		ofStream file;
		msg = file.open(filePath);
		if (msg)
		{
			//save header if any
			if (!m_header.empty())
			{
				assert(StringVector(m_header, ",").size() == m_nbStat);
				assert(GetFirstTRef().IsInit());

				CTemporal temporalRef(GetFirstTRef().GetTM());
				file << temporalRef.GetHeader(",");
				file << "," << m_header;
				file << std::endl;
			}


			CTRef d = GetFirstTRef();
			for (const_iterator it = begin(); it != end(); it++, d++)
			{
				if (d.GetTM().Mode() == CTM::FOR_EACH_YEAR)
				{
					if (d.GetTM().Type() != CTM::ATEMPORAL)
						file << d.GetYear();
					else
						file << d.m_ref;
				}

				switch (d.GetTM().Type())
				{
				case CTM::HOURLY: file << ',' << d.GetMonth() + 1 << ',' << d.GetDay() + 1 << ',' << d.GetHour(); break;
				case CTM::DAILY: file << ',' << d.GetMonth() + 1 << ',' << d.GetDay() + 1; break;
				case CTM::MONTHLY: file << ',' << d.GetMonth() + 1; break;
				case CTM::ANNUAL: break;
				case CTM::ATEMPORAL: break;
				default: _ASSERTE(false);
				}

				for (CModelStat::const_iterator it2 = it->begin(); it2 != it->end(); it2++)
					file << ',' << *it2;

				file << std::endl;
			}

			file.close();
		}

		return msg;
	}

	__int32 CModelStatVector::GetNbDay(size_t v, const CMathEvaluation& op, const CTPeriod& p, bool bConsecutive, double MISSING)const
	{
		_ASSERTE(GetFirstTRef().GetTM() == p.GetTM());

		const CModelStatVector& me = *this;
		CTRef firstDate = max(GetFirstTRef(), p.Begin());
		CTRef lastDate = min(GetLastTRef(), p.End());

		__int32 nbDay = 0;
		CTRef firstDWP = firstDate;
		for (CTRef d = firstDate; d <= lastDate; d++)
		{
			if (me[d][v] > MISSING)
			{
				if (bConsecutive)
				{
					if (!op.Evaluate(me[d][v]))
					{
						nbDay = max(nbDay, d - firstDWP);
						firstDWP = d + 1;
					}
					else if (d == lastDate)
					{
						//the last day, we verify the number of day
						nbDay = max(nbDay, d - firstDWP + 1);
					}
				}
				else
				{
					if (op.Evaluate(me[d][v]))
						nbDay++;
				}
			}
			else firstDWP = d + 1;
		}

		_ASSERTE(nbDay >= 0 && nbDay <= p.GetLength());

		return nbDay;
	}

	size_t CModelStatVector::Insert(CTRef r, const CModelStat& row)
	{
		ASSERT(size() == 0 || m_firstTRef.IsInit());

		if (!m_firstTRef.IsInit())
		{
			m_nbStat = row.size();
			m_firstTRef = r;
		}


		int index = r - m_firstTRef;
		ASSERT(index >= 0);

		if (index < 0)
		{
			insert(begin(), -index - 1, CModelStat(m_nbStat, m_missingValue));
			insert(begin(), row);
			m_firstTRef = m_firstTRef + index;
		}
		else if (index < int(size()))
		{
			insert(begin() + index, row);
		}
		else if (index == size())
		{
			push_back(row);
		}
		else
		{
			insert(end(), index - size(), CModelStat(m_nbStat, m_missingValue));
			push_back(row);
		}

		return GetRows();
	}

	void CModelStatVector::ConvertValue(float value1, float value2)
	{
		CModelStatVector& me = *this;
		for (size_t d = 0; d < size(); d++)
		{
			for (size_t v = 0; v < me[d].size(); v++)
			{
				if (me[d][v] == value1)
					me[d][v] = value2;
			}
		}
	}

	CInitialPopulation CModelStatVector::GetInitialPopulation(size_t var, size_t nbObjects, double initialPopulation, double age, size_t sex, bool bFertil, size_t generation, CTPeriod p)const
	{
		CInitialPopulation population;

		if (!p.IsInit())
			p = GetTPeriod();

		CStatistic stat = GetStat(var, p);

		if (stat[SUM] > 0)
		{
			//Get nbDay with activity
			double nbIndividuPerObject = initialPopulation / nbObjects;

			size_t nbCreated = 0;
			double cumulCreated = 0;
			CTPeriod period = GetTPeriod(var, 0, 0, 0, p);
			ASSERT(period.IsInit());

			for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
			{
				cumulCreated += nbObjects*at(TRef).at(var) / stat[SUM];

				size_t nbObjectToCreate = max(size_t(0), Round<size_t>(cumulCreated - nbCreated));
				for (size_t i = 0; i < nbObjectToCreate; i++)
					population.push_back(CIndividualInfo(TRef, age, sex, bFertil, generation, nbIndividuPerObject));

				nbCreated += nbObjectToCreate;
			}

			ASSERT(population.size() == nbObjects);
			//for(size_t i=size(); i<nbObjects; i++)
			//proportionVector.push_back( CProportionStat(peak, nbIndividuPerObject) );
		}

		return population;
	}


	//void CModelStatVector::Transform(const CTTransformation& TT, size_t s)
	//{
		//CTStatMatrix output(*this, TT);
		//CModelStatVector& me = *this;
		//CTPeriod pIn = TT.GetPeriodIn().IsInit() ? TT.GetPeriodIn() : GetTPeriod();
		//CTPeriod pOut = TT.GetPeriodOut();
		//output.Init(pOut, GetNbStat());

		//for (CTRef TRefIn = pIn.Begin(); TRefIn <= pIn.End(); TRefIn++)
		//{
		//	size_t c = TT.GetCluster(TRefIn);
		//	if (c != UNKNOWN_POS)
		//	{
		//		for (size_t v = 0; v < GetNbStat(); v++)
		//		{
		//			double var = me[TRefIn][v];
		//			ASSERT(var > -9999);
		//			CTRef TRefOut = TT.GetClusterTRef(c);
		//			output(TRefOut, v) += var;
		//		}
		//	}
		//}



		//CModelStatVector& me = *this;
		//CTPeriod pIn = TT.GetPeriodIn().IsInit() ? TT.GetPeriodIn() : GetTPeriod();
		//CTPeriod pOut = TT.GetPeriodOut();
		//output.Init(pOut, GetNbStat());

		//for (CTRef TRefIn = pIn.Begin(); TRefIn <= pIn.End(); TRefIn++)
		//{
		//	size_t c = TT.GetCluster(TRefIn);
		//	if (c != UNKNOWN_POS)
		//	{
		//		for (size_t v = 0; v < GetNbStat(); v++)
		//		{
		//			double var = me[TRefIn][v];
		//			ASSERT(var > -9999);
		//			CTRef TRefOut = TT.GetClusterTRef(c);
		//			output(TRefOut, v) += var;
		//		}
		//	}
		//}
	//}

	void CModelStatVector::Transform(const CTM& TM, size_t s)
	{
		if (TM != GetTM())
		{
			CTTransformation TT(GetTPeriod(), TM);
			Transform(TT,s);
		}
	}

	void CModelStatVector::Transform(const CTTransformation& TT, size_t s)
	{
		CTStatMatrix stat(*this, TT);

		CTPeriod p = stat.m_period;
		Init(p, stat.size_x(), -999);
		for (size_t i = 0; i < stat.size(); i++)
		{
			for (size_t j = 0; j < stat[i].size(); j++)
			{
				if (stat[i][j].IsInit())
					at(i).at(j) = stat[i][j][s];
			}
		}
	}


	//*************************************************************************************************
	CTStatMatrix::CTStatMatrix(CTPeriod period, size_t nbVars) :
		CTStatMatrixBase(period, nbVars)
	{
	}


	CTStatMatrix::CTStatMatrix(const CModelStatVector& in, const CTM& TM)
	{
		Init(in, TM);
	}

	CTStatMatrix::CTStatMatrix(const CModelStatVector& in, const CTTransformation& TT)
	{
		Init(in, TT);
	}
	//
	//CTStatMatrix(const CTStatMatrix& in, const CTM& TM)
	//{
	//}

	//CTStatMatrix(const CTStatMatrix& in, const CTTransformation& TT)
	//{
	//	operator=(in);
	//	Transform(TT);
	//}

	//void CTStatMatrix::Init(CTStatMatrix& in, const CTM& TM)
	//{
	//	CTPeriod p1 = input.GetTPeriod();
	//	CTPeriod p2 = p1.Transform(TM);
	//	Init(input, CTTransformation(p1, p2));
	//}

	//void CTStatMatrix::Init(CTStatMatrix& in, const CTTransformation& TT)
	//{
	//	operator=(in);
	//	Transform(TT);
	//}

	void CTStatMatrix::Init(const CModelStatVector& in, const CTM& TM)
	{
		CTPeriod p1 = in.GetTPeriod();
		CTPeriod p2 = p1.Get(TM);
		Init(in, CTTransformation(p1, p2));
	}

	void CTStatMatrix::Init(const CModelStatVector& in, const CTTransformation& TTin)
	{
		CTStatMatrix& me = *this;
		CTPeriod pIn = TTin.GetPeriodIn().IsInit() ? TTin.GetPeriodIn(): in.GetTPeriod();
		CTPeriod pOut = TTin.GetPeriodOut().IsInit() ? TTin.GetPeriodOut(): pIn;
		
		CTTransformation TT(pIn, pOut);
		CTStatMatrixBase::Init(pOut, in.GetNbStat());

		for (CTRef TRefIn = pIn.Begin(); TRefIn <= pIn.End(); TRefIn++)
		{
			size_t c = TT.GetCluster(TRefIn);
			if (c != UNKNOWN_POS)
			{
				for (size_t v = 0; v < in.GetNbStat(); v++)
				{
					double var = in[TRefIn][v];
					if (var != -999)
					{
						CTRef TRefOut = TT.GetClusterTRef(c);
						
						me[TRefOut][v] += var;
					}
				}
			}
		}
	}

	void CTStatMatrix::Transform(const CTM& TM)
	{
		if (TM.IsInit())
		{
			CTPeriod p1 = m_period;
			CTPeriod p2 = p1.Get(TM);
			Transform(CTTransformation(p1, p2));
		}
	}

	void CTStatMatrix::Transform(const CTTransformation& TT)
	{
		if (TT.IsInit())
		{
			CTStatMatrix& me = *this;
			CTPeriod pIn = TT.GetPeriodIn();
			CTPeriod pOut = TT.GetPeriodOut();

			CTStatMatrix output(pOut, size_x());

			for (CTRef TRefIn = pIn.Begin(); TRefIn <= pIn.End(); TRefIn++)
			{
				size_t c = TT.GetCluster(TRefIn);
				if (c != UNKNOWN_POS)
				{
					for (size_t v = 0; v < size_x(); v++)
					{
						CTRef TRefOut = TT.GetClusterTRef(c);
						output[TRefOut][v] += me[TRefIn][v];
					}
				}
			}

			operator=(output);
		}
	}

	void ModelInterface::Transform(const CTTransformation& TT, const CModelStatVector& in, CModelStatVector& out, size_t s)
	{
		CTStatMatrix tmp;
		Transform(TT, in, tmp);
		out.Init(tmp,s);
	}

	void ModelInterface::Transform(const CTTransformation& TT, const CModelStatVector& in, CTStatMatrix& out)
	{
		out.Init(in, TT);
		

		//CTPeriod pIn = TT.GetPeriodIn().IsInit() ? TT.GetPeriodIn():in.GetTPeriod();
		//CTPeriod pOut = TT.GetPeriodOut().IsInit()? TT.GetPeriodOut():pIn;
		
		//out.Init(pOut, NB_OUTPUT);

		//for (CTRef TRefIn = pIn.Begin(); TRefIn <= pIn.End(); TRefIn++)
		//{
		//	double DD = input[TRefIn][S_DD];
		//	ASSERT(DD > -9999);


		//	size_t c = TT.GetCluster(TRefIn);
		//	if (c != UNKNOWN_POS)
		//	{
		//		CTRef TRefOut = TT.GetClusterTRef(c);
		//		output(TRefOut, S_DD) += DD;
		//	}
		//}
	}

}//namespace WBSF
