//*********************************************************************
// File: SimulatedAnnealingVector.cpp
//
// Class:	CSimulatedAnnealingVector : Base of Simulated Ennealing in model
//
// Description: CSimulatedAnnealingVector is an vector of model to simulate 
//				different location at the same time
//*********************************************************************
// 25/02/2015   Rémi Saint-Amant    replace CSVFile by CSV
// 02/02/2011   Rémi Saint-Amant    Use of HxGrid in SimulatedAnnealing
// 09/02/2009   Rémi Saint-Amant    Creation.
//*********************************************************************

#include "stdafx.h"
#include <algorithm>
#include <streambuf>

#include "Basic/CSV.h"
#include "Basic/utilStd.h"
#include "ModelBase/SimulatedAnnealingVector.h"
#include "ModelBase/EntryPoint.h"

using namespace std;

namespace WBSF
{

	//************************************************************
	//CSimulatedAnnealingVector

	CSimulatedAnnealingVector::CSimulatedAnnealingVector()
	{}

	CSimulatedAnnealingVector::~CSimulatedAnnealingVector()
	{
		Reset();
	}

	void CSimulatedAnnealingVector::Reset()
	{
		for (int i = 0; i < (int)size(); i++)
		{
			delete at(i);
		}

		clear();
	}

	//double CSimulatedAnnealingVector::GetFValue(__int64 sizeArray, const double* paramArray, CStatisticXYVector& stat)
	//{
	//	_ASSERTE( stat.size() == 1 );
	//	//stat.clear();
	//	//stat.resize(size());
	//	//loop overs model
	//	for(int i=0; i<(int)size(); i++)
	//	{
	//		//stat[i].Reset();
	//		at(i)->GetFValue(sizeArray, paramArray, stat[0]);
	//	}
	//
	//	return at(0)->MakeFinalComputation(stat);
	//}


	void CSimulatedAnnealingVector::resize(size_t newSize)
	{
		Reset();

		for (size_t i = 0; i < newSize; i++)
			push_back(CModelFactory::CreateObject());
	}

	//
	//class rangebuf : public std::streambuf 
	//{
	//public:
	//
	//	rangebuf(std::streampos start,size_t size,std::streambuf* sbuf) :
	//		m_size(size), m_sbuf(sbuf), m_buffer(NULL)
	//	{
	//		m_sbuf->pubseekpos(start, std::ios_base::in);
	//	}
	//
	//	virtual streambuf::int_type underflow()
	//	{
	//		size_t r = m_sbuf->sgetn(m_buffer, std::min<std::streamsize>(sizeof(m_sbuf), m_size));
	//		m_size -= r;
	//		setg(m_buffer, m_buffer, m_buffer + r);
	//		return gptr() == egptr()? traits_type::eof(): traits_type::to_int_type(*gptr());
	//	}
	//
	//	std::streamsize m_size;
	//	std::streambuf* m_sbuf;
	//	char* m_buffer;
	//};



	ERMsg CSimulatedAnnealingVector::ReadStream(istream& stream)
	{
		ERMsg msg;

		size_t fieldPos = (size_t)read_value<__int64>(stream);
		if (fieldPos < 0 && fieldPos>2000)
		{
			msg.ajoute("Invalid location field position (" + ToString(fieldPos + 1) + ")");
			return msg;
		}

		size_t locSize = (size_t)read_value<__int64 >(stream);

		if (locSize <= 0 && locSize > 1000000)
		{
			msg.ajoute("Invalid dataset size (" + ToString(locSize) + ")");
			return msg;
		}
		resize(locSize);


		StringVector locName(size());
		for (size_t i = 0; i < size(); i++)
		{
			msg += at(i)->Init(stream);
			locName[i] = at(i)->GetInfo().m_loc.m_ID;
		}


		//CCSVFile result;

		stringstream s(ReadBuffer(stream));
		for (CSVIterator loop(s); loop != CSVIterator() && msg; loop++)
		{
			//result.ReadStream(stream);

			if (fieldPos < loop.Header().size())
			{
				//for (size_t i = 0; i < (*loop).size() && msg; i++)
				//{
				//Find the proper location
				string name = (*loop)[fieldPos];
				StringVector::iterator it = std::find(locName.begin(), locName.end(), name);
				if (it != locName.end())
				{
					//add result entry for this location
					size_t pos = it - locName.begin();
					at(pos)->AddResult(loop.Header(), *loop);
				}
				else
				{
					msg.ajoute("Unable to find location with ID = " + name);
				}
				//}


				//look to see if they are at least one value
				bool bEmptyResult = true;
				for (size_t i = 0; i < size() && bEmptyResult; i++)
				{
					bEmptyResult = at(i)->GetSAResult().empty();
				}

				if (bEmptyResult)
				{
					msg.ajoute("No result in the input file match the location list");
				}
			}
			else
			{
				msg.ajoute("Invalid location list field.");
			}
		}

		return msg;
	}


}