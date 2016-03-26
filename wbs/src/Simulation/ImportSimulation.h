//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once

#include "Simulation/Executable.h"


namespace WBSF
{

	class CModelStat;

	class CColumnLink
	{
	public:

		enum TMember{ NAME, DIMENSION_REF, DIMENASION_FIELD, NB_MEMBER };

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }

		CColumnLink()
		{
			m_name.empty();
			m_dimensionRef = DIMENSION::VARIABLE;
			m_dimensionField = -1;
		}
		
		std::string GetMember(size_t i)const
		{
			ASSERT(i >= 0 && i < NB_MEMBER);

			std::string str;
			switch (i)
			{
			case NAME: str = m_name; break;
			case DIMENSION_REF: str = ToString(m_dimensionRef); break;
			case DIMENASION_FIELD: str = ToString(m_dimensionField); break;
			default: ASSERT(false);
			}

			return str;
		}

		void SetMember(size_t i, const std::string& str)
		{
			ASSERT(i >= 0 && i < NB_MEMBER);
			switch (i)
			{
			case NAME: m_name = str; break;
			case DIMENSION_REF: m_dimensionRef = ToInt(str); break;
			case DIMENASION_FIELD: m_dimensionField = ToInt(str); break;
			default: ASSERT(false);
			}
		}


		bool operator == (const CColumnLink& in)const
		{
			bool bRep = true;
			if (m_name != in.m_name)bRep = false;
			if (m_dimensionRef != in.m_dimensionRef)bRep = false;
			if (m_dimensionField != in.m_dimensionField)bRep = false;

			return bRep;
		}

		bool operator != (const CColumnLink& in)const{ return !operator==(in); }

		std::string m_name;
		int m_dimensionRef;
		int m_dimensionField;

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];
	};



	class CColumnLinkVector : public std::vector < CColumnLink >
	{
	public:

		bool HaveDimension(int dim)const;
		void GetLocation(const StringVector& columnList, CLocation& LOC)const;
		size_t GetReplication(const StringVector& columnList)const;
		CTRef GetTRef(const StringVector& columnList)const;
		void GetVariable(const StringVector& columnList, CModelStat& outputVar)const;
		void GetOutputDefinition(CModelOutputVariableDefVector& def)const;
		CTM GetTM()const;

		std::string GetOtherHeader(const StringVector& header)const;
	};



	class CImportSimulation : public CExecutable
	{
	public:

		enum TMember{
			FILE_NAME = CExecutable::NB_MEMBERS, COLUMN_LINK,
			NB_MEMBER, NB_MEMBER_EX = NB_MEMBER - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBER_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CImportSimulation()); }

		//*** public member ***
		std::string m_fileName;
		CColumnLinkVector m_columnLinkArray;
		//*********************


		CImportSimulation();
		CImportSimulation(const CImportSimulation& in);
		virtual ~CImportSimulation();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CImportSimulation(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CImportSimulation&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CImportSimulation&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);

		void Reset();

		CImportSimulation& operator =(const CImportSimulation& in);
		bool operator == (const CImportSimulation& in)const;
		bool operator != (const CImportSimulation& in)const{ return !operator==(in); }

		//std::string GetMember(int i, LPXNode& pNode=NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode=NULL_ROOT);
		//virtual void GetXML(LPXNode& pRoot)const{XGetXML(*this, pRoot);}
		//virtual void SetXML(const LPXNode pRoot){XSetXML(*this, pRoot);}

		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg UpdateData(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK)const;

		virtual ERMsg GetLocationList(const CFileManager& fileManager, CLocationVector& loc)const;
		virtual ERMsg GetParameterList(const CFileManager& fileManager, CModelInputVector& parameters)const;
		virtual ERMsg GetReplication(const CFileManager& fileManager, size_t& nbReplication)const;
		virtual ERMsg GetDefaultPeriod(const CFileManager& fileManager, CTPeriod& period)const;
		virtual ERMsg GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const;

		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_FLOAT; }

	protected:

		std::string GetOptFilePath(const CFileManager& fileManager)const;
		bool IsUpToDate(const CFileManager& fileManager)const;

		ERMsg ReadData(ifStream& file, CModelStatVector& data, CLocation& lastLOC, size_t& lastReplication, CCallback& callback)const;

		ERMsg ImportFile(const CFileManager& fileManager, CCallback& callback)const;
		double GetModelTime();
		void SetModelTime(double timePerUnit);




		//for optimisation we load information in memory
		void ResetOptimization()const;
		ERMsg LoadOptimisation(const CFileManager& fileManager)const;
		bool m_bInfoLoaded;
		CLocationVector m_locArray;
		CTPeriod m_period;
		size_t m_nbReplications;
		CModelOutputVariableDefVector m_varDefArray;
		//	short m_dataTM;//Le TM de la dimention VARIABLE. -1 si non temporel
		CModelInputVector m_parameterSet;


		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER_EX];
		static const int CLASS_NUMBER;

	};

	//
	//namespace zen
	//{
	//	template <> inline
	//	void writeStruc(const CImportSimulation& in, XmlElement& output)
	//	{
	//		writeStruc( (const CExecutable&)in, output );
	//
	//
	//		XmlOut out(output);
	//		
	//		out[CImportSimulation::GetMemberName(CImportSimulation::FILE_NAME)](in.m_fileName);
	//		out[CImportSimulation::GetMemberName(CImportSimulation::COLUMN_LINK)](in.m_columnLinkArray);
	//	}
	//
	//	template <> inline
	//	bool readStruc(const XmlElement& input, CImportSimulation& out)
	//	{
	//		XmlIn in(input);
	//		
	//		readStruc( input, (CExecutable&)out );
	//		in[CImportSimulation::GetMemberName(CImportSimulation::FILE_NAME)](out.m_fileName);
	//		in[CImportSimulation::GetMemberName(CImportSimulation::COLUMN_LINK)](out.m_columnLinkArray);
	//
	//		
	//		return true;
	//	}
	//}
}



namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CColumnLink& in, XmlElement& output)
	{
		output.setValue(in.m_name);
		output.setAttribute(WBSF::CColumnLink::GetMemberName(WBSF::CColumnLink::DIMENSION_REF), in.m_dimensionRef);
		output.setAttribute(WBSF::CColumnLink::GetMemberName(WBSF::CColumnLink::DIMENASION_FIELD), in.m_dimensionField);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CColumnLink& out)
	{


		input.getValue(out.m_name);
		input.getAttribute(WBSF::CColumnLink::GetMemberName(WBSF::CColumnLink::DIMENSION_REF), out.m_dimensionRef);
		input.getAttribute(WBSF::CColumnLink::GetMemberName(WBSF::CColumnLink::DIMENASION_FIELD), out.m_dimensionField);


		return true;
	}
}
