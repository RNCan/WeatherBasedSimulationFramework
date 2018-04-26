//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "basic/zenXml.h"



#include "Basic/UtilTime.h"
#include "Basic/Callback.h"
#include "Basic/modelStat.h"
#include "Simulation/SectionData.h"
#include "Simulation/Executable.h"
#include "Simulation/ATM.h"


namespace WBSF
{
	class CFileManager;
	class CResult;


	class CDispersalParamters
	{
	public:

		//static public member 
		enum TMember{ WORLD_PARAMETERS, ATM_PARAMETERS, NB_MEMBERS };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return MEMBERS_NAME[i]; }

		//public member
		CATMWorldParamters m_world;
		CFlyerParameters m_flyers;


		CDispersalParamters()
		{
			clear();
		}

		void clear()
		{
			m_world.clear();
			m_flyers.clear();
		}


		CDispersalParamters& operator =(const CDispersalParamters& in)
		{
			if (&in != this)
			{
				m_world = in.m_world;
				m_flyers = in.m_flyers;
			}

			return *this;
		}

		bool operator ==(const CDispersalParamters& in)const
		{
			bool bEqual = true;

			if (m_world != in.m_world)bEqual = false;
			if (m_flyers != in.m_flyers)bEqual = false;

			return bEqual;
		}

		bool operator !=(const CDispersalParamters& in)const{ return !operator ==(in); }

	protected:


		static const char* MEMBERS_NAME[NB_MEMBERS];
	};



	//*******************************************************************************
	//

	class CDispersal : public CExecutable
	{
	public:

		enum TMember{ ATM_PARAMETERS = CExecutable::NB_MEMBERS, NB_MEMBERS, NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS };

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CDispersal()); }

		//public members
		CDispersalParamters m_parameters;


		CDispersal();
		CDispersal(const CDispersal& in);
		virtual ~CDispersal();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CDispersal(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CDispersal&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CDispersal&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);
		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter = CParentInfoFilter())const;

		ERMsg GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const;
		void GetOutputDefinition(CModelOutputVariableDefVector& outputVar)const;

		void Reset();
		CDispersal& operator =(const CDispersal& in);
		bool operator == (const CDispersal& in)const;
		bool operator != (const CDispersal& in)const{ return !operator==(in); }

		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_STATISTIC; }
		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);
		//ERMsg Execute2(const CFileManager& fileManager, CCallback& callback);

		static CGeoPoint GetNewPosition(const CGeoPoint& pt, double U, double V);



	protected:

		void ClassReset();
		void GetInputDBInfo(CResultPtr& pResult, CDBMetadata& info);



		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;


	};
}



namespace zen
{


	template <> inline
		void writeStruc(const WBSF::CDispersalParamters& in, XmlElement& output)
	{
		XmlOut out(output);

		out[WBSF::CDispersalParamters::GetMemberName(WBSF::CDispersalParamters::WORLD_PARAMETERS)](in.m_world);
		out[WBSF::CDispersalParamters::GetMemberName(WBSF::CDispersalParamters::ATM_PARAMETERS)](in.m_flyers);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CDispersalParamters& out)
	{
		XmlIn in(input);
		in[WBSF::CDispersalParamters::GetMemberName(WBSF::CDispersalParamters::WORLD_PARAMETERS)](out.m_world);
		in[WBSF::CDispersalParamters::GetMemberName(WBSF::CDispersalParamters::ATM_PARAMETERS)](out.m_flyers);

		return true;
	}
}