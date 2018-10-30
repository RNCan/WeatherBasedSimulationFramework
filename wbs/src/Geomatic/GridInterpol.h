//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Basic/UtilStd.h"
#include "Geomatic/UtilGDAL.h"
#include "Geomatic/GridInterpolBase.h"
#include "Geomatic/GDALBasic.h"



namespace WBSF
{

	class CProjectionTransformation;

	class CGridInterpol
	{
	public:

		enum TMethod { SPATIAL_REGRESSION, UNIVERSAL_KRIGING, INVERT_WEIGHTED_DISTANCE, THIN_PLATE_SPLINE, RANDOM_FOREST, NB_METHOD };
		static const char* GetMethodName(int method){ return METHOD_NAME[method]; }

		CGridInterpolBasePtr CreateNewGridInterpol()const;

		CGridInterpol();
		virtual ~CGridInterpol();
		void Reset();
		CGridInterpol& operator =(const CGridInterpol& in);
		const char* GetMethodName()const{ return GetMethodName(m_method); }



		ERMsg Initialise(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg OptimizeParameter(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg CreateSurface(CCallback& callback = DEFAULT_CALLBACK);
		void Finalize();
		//const CXValidationVector& GetXValidation()const{ return m_validation; }
		const CXValidationVector& GetOutput()const 
		{ 
			return (m_param.m_outputType == CGridInterpolParam::O_VALIDATION) ? m_validation : m_interpolation; 
		}


		CGridPointVectorPtr m_pts;

		std::string m_DEMFilePath;
		std::string m_TEMFilePath;

		bool m_bUseHxGrid;

		short m_method;
		CPrePostTransfo m_prePostTransfo;
		CGridInterpolParam m_param;
		CBaseOptions m_options;

		CGDALDatasetEx& GetOutputGrid(){ return m_outputGrid; }
		const CGridInterpolBase& GetGridInterpol()const { ASSERT(m_pGridInterpol.get()); return *m_pGridInterpol; }
		void CGridInterpol::ReadBlock(CBandsHolderMT& bandHolder, int xBlock, int yBlock, const CProjectionTransformation& PT, CGridPointVector& block);

	protected:

		ERMsg RunInterpolation(CCallback& callback);

		ERMsg RunHxGridOptimisation(const CGridInterpolParamVector& parameterset, std::vector<double>& optimisationR², CCallback& callback);
		ERMsg RunHxGridInterpolation(CCallback& callback);

		//ERMsg ExecuteXValidation(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg TrimDataset(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GenerateSurface(CCallback& callback = DEFAULT_CALLBACK);


		//computing tmp stuff
		CGDALDatasetEx m_inputGrid;
		CGDALDatasetEx m_outputGrid;
		CGridInterpolBasePtr m_pGridInterpol;

		static const char* METHOD_NAME[NB_METHOD];


		CXValidationVector m_validation;
		CXValidationVector m_interpolation;
		std::vector<size_t> m_trimPosition;

		//CProjectionTransformation m_PT;
		//CCritSec m_CS;
	};


}