//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Basic/NormalsData.h"
#include "Basic/Location.h"

namespace WBSF
{

	class CWeatherCorrections;


	//**************************************************************************************
	class CNormalsStation : public CLocation, public CNormalsData
	{
	public:

		CNormalsStation();
		CNormalsStation(const CNormalsStation& in);

		virtual ~CNormalsStation();

		CNormalsStation& operator=(const CNormalsStation& in);
		bool operator==(const CNormalsStation& in)const;
		bool operator!=(const CNormalsStation& in)const{ return !(operator==(in)); }

		void clear(){ Reset(); }
		void Reset();


		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CNormalStationHead>(*this);
			ar & boost::serialization::base_object<CNormalsData>(*this);
		}


		ERMsg LoadHeaderV2(std::istream& stream);
		ERMsg LoadV2(std::istream& stream);

		float GetDelta(size_t type)const;

		ERMsg IsValid() const;
		void ApplyCorrections(const CWeatherCorrections& correction);
	};

	typedef std::shared_ptr<CNormalsStation> CNormalsStationPtr;

	class CNormalsStationVector : public std::vector < CNormalsStation >
	{
	public:
		
		void ApplyCorrections(const CWeatherCorrections& correction);
		void GetNormalVector(const CLocation& target, CWVariables variables, CNormalDataVector& normalVector)const;
		void GetWeight(const CLocation& target, CWVariables variables, CNormalWeight& weight, bool bTakeElevation, bool bTakeShoreDistance)const;
		void GetInverseDistanceMean(const CLocation& target, CWVariables variables, CNormalsStation& normalsStation, bool bTakeElevation, bool bTakeShoreDistance )const;
		ERMsg MergeStation(CNormalsStation& station, size_t mergeType)const;
		void GetMean(size_t mergeType, CNormalsData& data)const;

	};

}