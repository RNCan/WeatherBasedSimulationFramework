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
namespace WBSF
{

	class CBioSIMModelBase;
	class CSimulatedAnnealingVector;
	typedef CBioSIMModelBase* (*CreateObjectM)(void);



	class CModelFactory
	{
	public:

		static bool RegisterModel(CreateObjectM CreateObject){ _CreateObject = CreateObject; return true; }
		static CBioSIMModelBase* CreateObject(){ return _CreateObject ? _CreateObject() : 0; }

	protected:


		static CreateObjectM _CreateObject;
	};

	const CSimulatedAnnealingVector& GetSimulatedAnnealingVector();
	bool GetUseHxGrid();
	class CStaticDataStream;
	const CStaticDataStream& GetStaticDataStream();


}


