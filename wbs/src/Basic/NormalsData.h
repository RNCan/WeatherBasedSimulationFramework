//***************************************************************************
// File:        DailyData.h
//
// class:		CNormalsMonth: all normal variable for one month
//				CNormalWeight: weight of all stations for all variables
//				CNormalsData: date for all variables for 12 months
//
// Abstract:    
//
// Description: 
//
// Note:        
//***************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "Basic/WeatherDefine.h"

namespace WBSF
{

	//***********************************************************************************
	//CNormalWeight

	typedef std::array<std::vector<double>, NORMALS_DATA::NB_CATEGORIES> CNormalWeight;

	//***********************************************************************************
	//CNormalsMonth

	typedef public std::array<float, NORMALS_DATA::NB_FIELDS> CNormalMonthBase;
	class CNormalsMonth : public CNormalMonthBase
	{
	public:

		CNormalsMonth();


		void Reset(){ Init(WEATHER::MISSING); }
		void Init(float v){ fill(v); }
		ERMsg FromString(const std::string& str);
		//std::string ToString()const;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CNormalMonthBase>(*this);
		}
		friend class boost::serialization::access;

		double GetP()const{ double junk; return GetP(junk, junk); }
		double GetP(double& sigma_gamma, double& sigma_zeta)const;

		bool IsValid()const;
		bool IsValid(size_t f)const;

		CWVariables GetVariables()const;

	};



	//***********************************************************************************
	//CNormalsData

	class CWeatherCorrections;

	class CNormalsData;
	typedef std::vector<CNormalsData> CNormalDataVector;
	typedef std::array<CNormalsMonth, 12> CNormalsDataBase;

	class CNormalsData : public CNormalsDataBase
	{
	public:

		CNormalsData();
		CNormalsData(const CNormalsData& in);
		~CNormalsData();

		void Reset();
		void Init(float v);

		CNormalsData& operator =(const CNormalsData& in);
		CNormalsData& operator +=(const CNormalsData& in);
		CNormalsData& operator /=(float value);
		bool operator ==(const CNormalsData& in)const;
		bool operator !=(const CNormalsData& in)const;

		CNormalsData& Copy(const CNormalsData& data, CWVariables variables);

		//std::ostream& operator << ( std::ostream& stream);
		//std::istream& operator >> ( std::istream& stream);
		//friend std::ostream& operator << ( std::ostream& stream, CNormalsData& data){	return data << stream;}
		//friend std::istream& operator >> ( std::istream& stream, CNormalsData& data){	return data >> stream;}

		friend class boost::serialization::access;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_bAdjusted & boost::serialization::base_object<CNormalsDataBase>(*this);
		}

		ERMsg LoadV2(std::istream& stream);

		ERMsg IsValid()const;
		//Get a vector for a climatic variable
		void GetVector(size_t i, float values[12])const;

		//adjust the mean (only once) when interpolate
		bool IsAdjusted()const{ return m_bAdjusted; }
		void AdjustMonthlyMeans();
		//interpole a day for a climatic variable
		double Interpole(CTRef TRef, size_t v)const;
		double Interpole(size_t m, size_t d, size_t v)const;
		double Interpole(size_t d, size_t v)const;

		bool HaveField(size_t f)const;
		bool HaveNoDataField(size_t f)const;
		bool IsValidField(size_t f)const;
		CWVariables GetVariables()const;

		void SetInverseDistanceMean(CWVariables variables, const CNormalDataVector& normalVector, const CNormalWeight& weight);

	protected:


		bool m_bAdjusted;
	};


}