#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	namespace LeafAreaIndex
	{
		//profile from:
		//Leaf Area Index Review and Determination for the Greater Athabasca
		//Oil Sands Region of  Northern Alberta, Canada
		//February 19, 2009

		enum TForestCovertype { HARDWOODS, MIXED_WOODS, WHITE_SPRUCE, BLACK_SPRUCE_FORESTS, BLACK_SPRUCE_FENS, JACK_PINE_FORESTS, NB_FOREST_COVER_TYPE };

		class CLAIProfile
		{
		public:

			size_t m_forestCovertype;
			size_t m_DOY;
			double m_LAI_Mean;
			double m_LAI_SD;
			double m_LAI_Median;
		};


		class CLeafAreaIndex
		{
			public:
			
				static const CLAIProfile LAI_PROFILE[NB_FOREST_COVER_TYPE * 12];
				static double ComputeLAI(size_t Jday, size_t forestCovertype, double quatile);

				static size_t GetProfileIndex(size_t forestCovertype, size_t i)
				{
					return forestCovertype * 12+i;
				}
		};

	}

}