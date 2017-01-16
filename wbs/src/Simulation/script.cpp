//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include "Script.h"
#include "Simulation/ExecutableFactory.h"
#include "Basic/Registry.h"
#include "FileManager/FileManager.h"

using namespace std;

namespace WBSF
{
	//*******************************************************************************
	const char* CScript::XML_FLAG = "Script";
	const char* CScript::MEMBERS_NAME[NB_MEMBERS_EX] = { "Script", "Input", "Output" };
	const int CScript::CLASS_NUMBER = CExecutableFactory::RegisterClass(CScript::GetXMLFlag(), &CScript::CreateObject);

	CScript::CScript()
	{
		ClassReset();
	}

	CScript::CScript(const CScript& in)
	{
		operator=(in);
	}

	CScript::~CScript()
	{}

	void CScript::Reset()
	{
		CExecutable::Reset();
		ClassReset();
	}

	void CScript::ClassReset()
	{
		m_name = "Script";
		m_scriptFileName.clear();
		m_inputFileName.clear();
		m_outputFileName.clear();
	}

	CScript& CScript::operator =(const CScript& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);
			m_scriptFileName = in.m_scriptFileName;
			m_inputFileName = in.m_inputFileName;
			m_outputFileName = in.m_outputFileName;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CScript::operator == (const CScript& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator !=(in))bEqual = false;
		if (m_scriptFileName != in.m_scriptFileName)bEqual = false;
		if (m_inputFileName != in.m_inputFileName)bEqual = false;
		if (m_outputFileName != in.m_outputFileName)bEqual = false;


		return bEqual;
	}

	void CScript::GetInputDBInfo(CResultPtr& pResult, CDBMetadata& info)
	{
		info.SetLocations(pResult->GetMetadata().GetLocations());
		info.SetNbReplications(pResult->GetMetadata().GetNbReplications());
	}


	ERMsg CScript::GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const
	{
		GetOutputDefinition(outputVar);

		return ERMsg();
	}

	void CScript::GetOutputDefinition(CModelOutputVariableDefVector& outputVar)const
	{
		outputVar.clear();
		//outputVar.push_back(CModelOutputVariableDef("Latitude", "Latitude", "Current position", CTM(CTM::HOURLY)));
		//outputVar.push_back(CModelOutputVariableDef("Longitude", "Longitude", "Current position", CTM(CTM::HOURLY)));
		//outputVar.push_back(CModelOutputVariableDef("Height", "Height", "Current position", CTM(CTM::HOURLY)));
	}


	void CScript::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(SCRIPT_NAME)](m_scriptFileName);
		out[GetMemberName(INPUT)](m_inputFileName);
		out[GetMemberName(OUTPUT)](m_outputFileName);
		
	}

	bool CScript::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		
		in[GetMemberName(SCRIPT_NAME)](m_scriptFileName);
		in[GetMemberName(INPUT)](m_inputFileName);
		in[GetMemberName(OUTPUT)](m_outputFileName);


		return true;
	}


	ERMsg CScript::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		string filePath;
		msg = GetFM().Script().GetFilePath(m_scriptFileName, filePath);
		if (msg)
		{
			string arg1 = !m_inputFileName.empty() ? "\"" + m_inputFileName+ "\"" : "";
			string arg2 = !m_outputFileName.empty() ? "\"" + m_outputFileName + "\"" : "";
			msg = CallApplication(CRegistry::R_SCRIPT, arg1 + " " + arg2, NULL, SW_SHOW, false, true);
		}

		//CResultPtr pResult = m_pParent->GetResult(fileManager);

		//msg = pResult->Open();
		//if (!msg)
		//	return msg;


		//size_t nbSection = pResult->GetNbSection();
		//const CModelOutputVariableDefVector& varDef = pResult->GetMetadata().GetOutputDefinition();

		////Generate output path
		//string outputPath = GetPath(fileManager);

		////Generate DB file path
		//string DBFilePath = GetDBFilePath(outputPath);

		////open outputDB
		//CResult result;
		//msg = result.Open(DBFilePath, std::fstream::binary | std::fstream::out | std::fstream::trunc);
		//if (!msg)
		//	return msg;

		////init output info
		//CDBMetadata& metadata = result.GetMetadata();
		////GetInputDBInfo(pResult, metadata);


		//CTPeriod period = pResult->GetTPeriod();
		//const CLocationVector& locations = metadata.GetLocations();

		//for (size_t s = 0, f = 0; s<nbSection&&msg; s++)
		//{
		//	size_t nbRow = pResult->GetNbRows(s);

		//	CNewSectionData section;
		//	section.resize(nbRow, 3, pResult->GetFirstTRef(s));

		//	for (size_t r = 0; r<nbRow&&msg; r++, f++)
		//	{
		//		for (size_t i = 0; i<3; i++)
		//		{
		//		}

		//		msg += callback.StepIt(1.0 / nbRow);
		//	}

		//	result.AddSection(section);
		//}

		//result.Close();

		return msg;
	}

}