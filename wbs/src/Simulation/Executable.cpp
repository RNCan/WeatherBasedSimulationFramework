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
#include <boost/algorithm/string.hpp>
#include <omp.h>

#include "Basic/UtilStd.h"
#include "Basic/Registry.h"
#include "Basic/Timer.h"
#include "FileManager/FileManager.h"
#include "Simulation/Executable.h"
#include "Simulation/ExecutableFactory.h"

#include "WeatherBasedSimulationString.h"
#include "Basic/decode_html_entities_utf8.h"

using namespace std;
using namespace WBSF::DIMENSION;

namespace WBSF
{

	//****************************************************************
	CDimension CParentInfo::GetDimension()const
	{
		CDimension dimension;
		for (int d = 0; d < DIMENSION::NB_DIMENSION; d++)
		{
			switch (d)
			{
			case LOCATION: dimension[LOCATION] = m_locations.size(); break;
			case PARAMETER:dimension[PARAMETER] = m_parameterset.size(); break;
			case REPLICATION:dimension[REPLICATION] = m_nbReplications; break;
			case TIME_REF:dimension[TIME_REF] = m_period.GetNbRef(); break;
			case VARIABLE:	dimension[VARIABLE] = m_variables.size(); break;
			default: ASSERT(false);
			}
		}

		return dimension;
	}


	std::string CParentInfo::GetDimensionStr(int dim)const
	{
		//a quoi devrai ressembler la class CParentInfo
		//est-ce que ça devrait être que des strings????
		return to_string(GetDimensionList(dim), "|");
	}

	StringVector CParentInfo::GetDimensionList(int dim)const
	{
		StringVector list;
		switch (dim)
		{
		case LOCATION:
		{
			list.resize(m_locations.size());

			for (int i = 0; i < m_locations.size(); i++)
				list[i] = m_locations[i].m_name;
		}break;

		case PARAMETER:
		{
			list.resize(m_parameterset.size());
			for (size_t i = 0; i < m_parameterset.size(); i++)
			{
				if (m_parameterset[i].size() == m_parameterset.m_variation.size())
				{
					for (size_t j = 0; j < m_parameterset.m_variation.size(); j++)
					{
						if (m_parameterset.m_variation[j].m_bActive)
						{
							if (!list[i].empty())
								list[i] += ",";

							list[i] += m_parameterset[i][j].GetStr();
						}
					}
				}
			}
		}break;

		case REPLICATION: list.push_back(ToString(m_nbReplications)); break;
		case TIME_REF:	list.push_back(m_period.ToString()); break;

		case VARIABLE:
		{
			list.resize(m_variables.size());

			for (size_t i = 0; i < m_variables.size(); i++)
				list[i] = m_variables[i].m_name;//a changer pour m_title

		}break;

		default: ASSERT(false);
		}

		return list;
	}

	//****************************************************************
	//CExecutableVector

	const char* CExecutableVector::XML_FLAG = "ExecutableArray";

	CExecutableVector::CExecutableVector()
	{
		Reset();
	}

	CExecutableVector::CExecutableVector(const CExecutableVector& in)
	{
		Reset();
		operator =(in);
	}

	void CExecutableVector::Reset()
	{
		m_pParent = NULL;
		//est-ce que lon doit appeler le destructeur des object???
		clear();
	}

	CExecutableVector& CExecutableVector::operator =(const CExecutableVector& in)
	{
		if (&in != this)
		{
			register INT_PTR i = 0;
			register INT_PTR nSize(in.size());

			resize(nSize);
			for (i = 0; i < nSize; i++)
			{
				CExecutablePtr pItem = in[i]->CopyObject();
				pItem->SetParent(m_pParent);
				at(i) = pItem;
			}
		}

		ASSERT(in == *this);

		return *this;
	}

	bool CExecutableVector::operator == (const CExecutableVector& in)const
	{
		register INT_PTR i = 0;
		register INT_PTR nSize(size());

		if (nSize != in.size())
		{
			return false;
		}

		for (i = 0; i < nSize; i++)
		{
			if (at(i)->GetClassName() != in[i]->GetClassName())
				return false;

			if (!at(i)->CompareObject(*(in[i])))
				return false;
		}

		return true;
	}

	void CExecutableVector::writeStruc(zen::XmlElement& output)const
	{
		typedef CExecutableVector::const_iterator const_iter;
		std::for_each(begin(), end(),
			[&](const CExecutablePtr& pExec)
		{
			zen::XmlElement& newChild = output.addChild(pExec->GetClassName());
			pExec->writeStruc(newChild);
		}
		);
	}

	bool CExecutableVector::readStruc(const zen::XmlElement& input)
	{
		bool success = true;
		clear();

		auto iterPair = input.getChildren();
		//resize(std::distance(iterPair.first, iterPair.second));
		//CExecutableVector::iterator it = begin();
		for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
		{
			std::string className = iter->getNameAs<string>();
			CExecutablePtr pExec = CExecutableFactory::CreateObject(className);
			if (pExec)
			{
				pExec->SetParent(m_pParent);

				if (pExec->readStruc(*iter))//virtual call of readStruc
					push_back(pExec);
				else
					success = false;

			}
		}

		return success;
	}

	CExecutablePtr CreateExecutable(const std::string& className)
	{
		return CExecutableFactory::CreateObject(className);
	}



	void CExecutableVector::UpdateInternalName()
	{
		for (int i = 0; i < size(); i++)
			at(i)->UpdateInternalName();

	}


	int CExecutableVector::GetNbExecute(bool bTask)const
	{
		size_t nbExecute = 0;
		for (size_t i = 0; i < size(); i++)
			nbExecute += at(i)->GetNbExecute(bTask);

		return (int)nbExecute;
	}

	ERMsg CExecutableVector::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		typedef pair<int, size_t> PriorityPair;
		vector< PriorityPair >execList;
		size_t nbExec = 0;

		for (size_t i = size() - 1; i < size(); i--)
		{
			size_t e = at(i)->GetNbExecute(false);
			if (e > 0)
			{
				execList.push_back(PriorityPair(at(i)->GetPriority(), i));
				nbExec += e;
			}

		}

		sort(execList.begin(), execList.end());


		if (!execList.empty())
			callback.PushTask(FormatMsg(IDS_MSG_PUSH_LEVEL, GetParent()->m_name), execList.size(), 1);

		for (size_t ii = 0; ii < execList.size() && !callback.GetUserCancel(); ii++)
		{
			size_t i = execList[ii].second;
			ERMsg msgTmp = at(i)->ExecuteBasic(fileManager, callback);

			if (!execList.empty())
				msg += callback.StepIt(at(i)->GetNbExecute(false));

			msg += msgTmp;
			if (msgTmp)
			{
				//Execute child task
				msg += at(i)->ExecuteChild(fileManager, callback);

			}
		}

		if (!execList.empty())
			callback.PopTask();

		return msg;
	}



	CExecutablePtr CExecutableVector::FindItem(const std::string& iName)const
	{
		CExecutablePtr pItem;
		for (int i = 0; i < size() && !pItem; i++)
		{
			if (boost::iequals(at(i)->GetInternalName(), iName))
				pItem = at(i);
			else pItem = at(i)->FindItem(iName);
		}
		return pItem;
	}

	void CExecutableVector::SetItem(const std::string& iName, CExecutablePtr pItem)
	{
		for (int i = 0; i < size(); i++)
		{
			if (at(i)->GetInternalName() != iName)
			{
				at(i)->SetItem(iName, pItem);
			}
			else
			{
				pItem->SetParent(m_pParent);
				at(i) = pItem;
				break;
			}
		}
	}
	bool CExecutableVector::RemoveItem(const std::string& iName)
	{
		bool bRep = false;
		for (int i = 0; i < size() && !bRep; i++)
		{
			if (at(i)->GetInternalName() == iName)
			{
				erase(begin() + i);
				bRep = true;
			}
			else
			{
				bRep = at(i)->RemoveItem(iName);
			}
		}
		return bRep;
	}


	bool CExecutableVector::MoveItem(const std::string& iName, const std::string& iAfter, bool bCopy)
	{
		bool bRep = false;
		for (int i = 0; i < size() && !bRep; i++)
		{
			if (at(i)->GetInternalName() == iName)
			{
				int destIndex = 0;
				if (!iAfter.empty())
				{
					destIndex = -1;
					for (int j = 0; j < size(); j++)
					{
						if (at(j)->GetInternalName() == iAfter)
						{
							destIndex = j + 1;
							break;
						}
					}
				}

				if (destIndex >= 0 && destIndex < size() && i != destIndex)
				{
					CExecutablePtr elem = at(i);
					if (bCopy)
					{
						CExecutablePtr newElem = elem->CopyObject();
						newElem->UpdateInternalName();
						insert(begin() + destIndex, newElem);
					}
					else
					{
						insert(begin() + destIndex, elem);
						erase(begin() + (i < destIndex ? i : i + 1));
					}



					bRep = true;
				}
			}
			else bRep = at(i)->MoveItem(iName, iAfter, bCopy);
		}

		return bRep;
	}

	//**************************************************************
	//CExecuteCtrl

	CExecuteCtrl::CExecuteCtrl()
	{
		m_bRunEvenFar = false;
		m_bRunWithMissingYear = false;
		m_maxDistFromLOC = 300;
		m_maxDistFromPoint = 500;
		m_bKeepTmpFile = true;
		m_bUseHxGrid = false;
		m_listDelimiter = ',';
		m_decimalDelimiter = '.';
		m_bExportAllLines = false;
		m_nbMaxThreads = omp_get_num_procs();
		
	}

	void CExecuteCtrl::LoadDefaultCtrl()
	{
		CRegistry registry("ExecuteCtrl");
		m_maxDistFromLOC = registry.GetProfileInt("MaxDistFromLOC", m_maxDistFromLOC);
		m_maxDistFromPoint = registry.GetProfileInt("MaxDistFromPoint", m_maxDistFromPoint);
		m_bRunEvenFar = registry.GetProfileBool("RunEvenFar", m_bRunEvenFar);
		m_bRunWithMissingYear = registry.GetProfileBool("RunWithMissingYear", m_bRunWithMissingYear);
		m_bKeepTmpFile = registry.GetProfileBool("KeepTmpOutputFile", m_bKeepTmpFile);
		m_bUseHxGrid = registry.GetProfileBool("UseHxGrid", m_bUseHxGrid);
		m_bExportAllLines = registry.GetProfileBool("ExportAllLines", m_bExportAllLines);
		m_nbMaxThreads = min(omp_get_num_procs(), registry.GetProfileInt("NbMaxThreads", omp_get_num_procs()));
		
		CRegistry registryII;
		m_listDelimiter = registryII.GetListDelimiter();
		m_decimalDelimiter = registryII.GetDecimalDelimiter();

		m_timeFormat = CTRef::GetFormat();
	}

	void CExecutable::LoadDefaultCtrl()
	{
		CTRL.LoadDefaultCtrl();
	}

	//**************************************************************
	//CExecutable
	const char* CExecutable::MEMBERS_NAME[NB_MEMBERS] = { "Name", "InternalName", "Description", "Execute", "Export", /*"GraphArray", */"ExecutableArray" };
	CExecuteCtrl CExecutable::CTRL;


	CExecutable::CExecutable()
	{
		m_internalName = GenerateInternalName();
		m_pParent = NULL;
		m_executables.SetParent(this);
		Reset();
	}

	CExecutable::CExecutable(const CExecutable& in)
	{
		m_internalName = GenerateInternalName();
		m_pParent = NULL;
		m_executables.SetParent(this);
		operator=(in);
	}

	CExecutable::~CExecutable()
	{}

	void CExecutable::Reset()
	{
		m_bExecute = true;
		m_name.empty();
		m_description.empty();
		m_export.Reset();
		//m_graphArray.clear();
		m_executables.clear();
	}

	CExecutable& CExecutable::operator =(const CExecutable& in)
	{
		ASSERT(m_executables.GetParent() == this);

		if (&in != this)
		{
			m_bExecute = in.m_bExecute;
			m_name = in.m_name;
			m_internalName = in.m_internalName;
			m_description = in.m_description;
			m_export = in.m_export;
			//m_graphArray = in.m_graphArray;
			m_executables = in.m_executables;
		}

		ASSERT(in == *this);
		return *this;
	}

	bool CExecutable::operator == (const CExecutable& in)const
	{
		bool bEqual = true;
		if (m_bExecute != in.m_bExecute)bEqual = false;
		if (m_name != in.m_name)bEqual = false;
		if (m_internalName != in.m_internalName)bEqual = false;
		if (m_description != in.m_description)bEqual = false;
		if (m_export != in.m_export)bEqual = false;
		//if(m_graphArray!=in.m_graphArray)bEqual=false;

		if (m_executables != in.m_executables)bEqual = false;

		return bEqual;
	}



	CExecutablePtr CExecutable::GetParent()
	{
		return m_pParent ? m_pParent->GetExecutablePtr() : CExecutablePtr();
	}

	const CExecutablePtr CExecutable::GetParent()const
	{
		return m_pParent ? m_pParent->GetExecutablePtr() : CExecutablePtr();
	}

	CExecutablePtr CExecutable::GetExecutablePtr()
	{
		ASSERT(m_pParent);
		return m_pParent->FindItem(GetInternalName());
	}

	const CExecutablePtr CExecutable::GetExecutablePtr()const
	{
		ASSERT(m_pParent);
		return m_pParent->FindItem(GetInternalName());
	}

	//this fonction can be override to retrun more specific text
	std::string CExecutable::GetTitle()
	{
		return m_name;
	}

	std::string CExecutable::GenerateInternalName()
	{
		std::string internalName;
		for (int i = 0; i < 6; i++)
		{
			internalName += (char)Rand('a', 'z');
		}

		return internalName;
	}

	void CExecutable::UpdateInternalName()
	{
		m_internalName = GenerateInternalName();
		m_executables.UpdateInternalName();
	}

	int CExecutable::GetNbExecute(bool bTask)const
	{
		//int nbTask = ;
		int nbExecute = m_bExecute ? (bTask ? GetNbTask() : 1) : 0;
		nbExecute += m_executables.GetNbExecute(bTask);

		if (m_bExecute && bTask && m_export.m_bAutoExport)
			nbExecute++;

		return nbExecute;
	}


	//By default Execute do nothing
	ERMsg CExecutable::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;
		return msg;
	}

	ERMsg CExecutable::ExecuteBasic(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		if (GetExecute())
		{
			callback.DeleteMessages(true);
			callback.AddMessage(GetDefaultTaskTitle());
			callback.AddMessage(GetDescription());
			callback.AddMessage("");
			callback.AddMessage(GetCurrentTimeString());
			CTimer timer(TRUE);

			//delete old result
			msg = CResult::Remove(GetDBFilePath(GetPath(fileManager)));

			if (msg)
				msg = Execute(fileManager, callback);

			if (msg && m_export.m_bAutoExport)
			{
				msg = Export(fileManager, EXPORT_CSV, callback);

				//bnow execute script
				if (msg)
					ExecuteScript(fileManager, callback);
			}

			timer.Stop();
			callback.AddMessage("");
			callback.AddMessage("Time to execute task: " + SecondToDHMS(timer.Elapsed()) );
			callback.AddMessage(GetCurrentTimeString());
			callback.AddMessage("*******************************************");


			std::string logText = GetOutputString(msg, callback, false);
			std::string filePath = GetLogFilePath(GetPath(fileManager));

			msg += WriteOutputMessage(filePath, logText);
		}


		return msg;
	}


	ERMsg CExecutable::ExecuteChild(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		msg = m_executables.Execute(fileManager, callback);
		return msg;
	}

	CResultPtr CExecutable::GetResult(const CFileManager& fileManager)const
	{
		CResultPtr pDB(new CResult);
		pDB->SetFilePath(GetDBFilePath(GetPath(fileManager)));

		return pDB;
	}


	void CExecutable::InsertItem(CExecutablePtr pItem)
	{
		pItem->SetParent(this);
		m_executables.push_back(pItem);
	}

	CExecutablePtr CExecutable::FindItem(const std::string& iName)const
	{
		ASSERT(m_internalName != iName);
		return m_executables.FindItem(iName);
	}

	void CExecutable::SetItem(const std::string& iName, CExecutablePtr pItem)
	{
		ASSERT(m_internalName != iName);
		m_executables.SetItem(iName, pItem);
	}

	bool CExecutable::RemoveItem(const std::string& iName)
	{
		ASSERT(m_internalName != iName);
		return m_executables.RemoveItem(iName);
	}

	bool CExecutable::MoveItem(const std::string& iName, const std::string& iAfter, bool bCopy)
	{
		ASSERT(m_internalName != iName);
		return m_executables.MoveItem(iName, iAfter, bCopy);
	}


	std::string CExecutable::GetPath(const CFileManager& fileManager)const
	{
		if (m_pParent == NULL)
			return fileManager.GetTmpPath();

		return m_pParent->GetPath(fileManager);
	}

	std::string CExecutable::GetFilePath(const std::string& path, const std::string& ext)const
	{
		return path + m_internalName + ext;
	}

	std::string CExecutable::GetLogFilePath(const std::string& path)const
	{
		return GetFilePath(path, ".log");

	}

	std::string CExecutable::GetDBFilePath(const std::string& path)const
	{
		return GetFilePath(path, ".DBdata");

	}

	std::string CExecutable::GetOutputMessage(const CFileManager& fileManager)
	{
		return ReadOutputMessage(GetLogFilePath(GetPath(fileManager)));
	}

	std::string CExecutable::ReadOutputMessage(const std::string& filePath)
	{
		string logText;
		ifStream file;

		ERMsg msg = file.open(filePath);
		if (msg)
			logText = file.GetText();

		return logText;
	}

	ERMsg CExecutable::WriteOutputMessage(const std::string& filePath, const std::string& logText)
	{
		ofStream file;

		ERMsg msg;
		msg += CreateMultipleDir(WBSF::GetPath(filePath));
		msg += file.open(filePath);
		if (msg)
		{
			file.write(&(logText[0]), logText.length());
		}

		return msg;
	}


	std::string CExecutable::GetValidationMessage(const CFileManager& fileManager)
	{
		return "Not validate";
	}

	ERMsg CExecutable::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
	{
		ERMsg msg;
		if (m_pParent)
			msg = m_pParent->GetParentInfo(fileManager, info, filter);

		return msg;
	}


	CDimension CExecutable::GetDimension(const CFileManager& fileManager, bool bFromResult)const
	{
		CDimension dimension;
		if (bFromResult)
		{
			CResultPtr pResult = GetResult(fileManager);
			if (pResult && pResult->Open())
			{
				dimension = pResult->GetDimension();
			}
		}
		else
		{
			CParentInfo info;
			GetParentInfo(fileManager, info);
			dimension = info.GetDimension();
		}

		return dimension;
	}

	CTPeriod CExecutable::GetDefaultPeriod(const CFileManager& fileManager)const
	{
		CParentInfo info;
		GetParentInfo(fileManager, info, TIME_REF);
		return info.m_period;
	}

	std::string CExecutable::GetDefaultTaskTitle()
	{
		return 	"******* " + m_name + "(" + m_internalName + ") *******";
	}

	std::string CExecutable::GetExportFilePath(const CFileManager& fileManager, int format)const
	{
		string fileName = m_export.m_fileName;
		if (fileName.empty())
			fileName = "Export (" + m_name + ")";

		if (fileName.find("%d"))
		{
			CTRef TRef = CTRef::GetCurrentTRef();
			ReplaceString(fileName, "%d", TRef.GetFormatedString("%Y-%m-%d"));
		}


		if (GetFileExtension(fileName).empty())
		{
			switch (format)
			{
			case EXPORT_CSV: fileName += ".csv"; break;
			default: ASSERT(false);
			}
		}

		return fileManager.GetOutputPath() + fileName;
	}



	ERMsg CExecutable::Export(const CFileManager& fileManager, int format, CCallback& callback)
	{
		ASSERT(format == EXPORT_CSV);

		ERMsg msg;

		msg = ExportAsCSV(fileManager, callback);

		return msg;
	}


	ERMsg CExecutable::ExportAsCSV(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		char listDelimiter = CTRL.m_listDelimiter;
		char decimalDelimiter = CTRL.m_decimalDelimiter;
		CTRefFormat timeFormat = CTRL.m_timeFormat;

		CVariableDefineVector variables = m_export.m_variables;

		//if( bAsLoc )
		//{
		//	//verify that Name, Lat, lon and elevation is exported. if not we add it
		//	bool bOk[CLocation::NB_MEMBER] = {0};
		//	for(int j=0; j<variables.size(); j++)
		//	{
		//		if( variables[j].m_dimension == LOCATION )
		//			bOk[variables[j].m_field] = true;
		//	}

		//	if( !bOk[CLocation::ID] )
		//		variables.push_back(CVariableDefine(LOCATION, CLocation::ID));
		//	if( !bOk[CLocation::LAT] )
		//		variables.push_back(CVariableDefine(LOCATION, CLocation::LAT));
		//	if( !bOk[CLocation::LON] )
		//		variables.push_back(CVariableDefine(LOCATION, CLocation::LON));
		//	if( !bOk[CLocation::ELEV] )
		//		variables.push_back(CVariableDefine(LOCATION, CLocation::ELEV));
		//}


		//Open export file
		ofStream file;
		msg = file.open(GetExportFilePath(fileManager, EXPORT_CSV));
		if (!msg)
			return msg;

		CVariableSelectionVector statistic(m_export.m_statistic);

		//Open database
		CResultPtr pResult = GetResult(fileManager);
		if (pResult && pResult->Open())
		{
			callback.AddMessage(FormatMsg(IDS_SIM_EXPORT, GetExportFilePath(fileManager, EXPORT_CSV)));
			callback.PushTask("Export", pResult->GetNbRows());

			const CModelOutputVariableDefVector& outputVar = pResult->GetMetadata().GetOutputDefinition();
			const CLocationVector& loc = pResult->GetMetadata().GetLocations();
			const CModelInputVector& param = pResult->GetMetadata().GetParameterSet();
			StringVector SSI = loc.GetSSIHeader();
			//Write header
			{
				std::string line;
				for (size_t j = 0; j < variables.size(); j++)
				{
					if (j > 0)
						line += listDelimiter;

					if (variables[j].m_dimension == LOCATION)
					{
						string tmp = CLocation::GetMemberTitle(variables[j].m_field);
						if (variables[j].m_field == CLocation::SSI)
							tmp = SSI.to_string(",");//replace default name by real SSI name

						//tmp = pResult->GetFieldTitle(variables[j].m_dimension, variables[j].m_field, 0);
						std::replace(tmp.begin(), tmp.end(), ',', listDelimiter);

						line += tmp;
					}
					else if (variables[j].m_dimension == PARAMETER)
					{
						string tmp = pResult->GetFieldTitle(variables[j].m_dimension, variables[j].m_field, 0);
						if (tmp.empty())
							tmp = "P";

						std::replace(tmp.begin(), tmp.end(), ',', listDelimiter);
						line += tmp;
					}
					else if (variables[j].m_dimension == TIME_REF)
					{
						line += timeFormat.GetHeader(pResult->GetTM());
					}
					else if (variables[j].m_dimension == REPLICATION)
					{
						line += pResult->GetFieldTitle(variables[j].m_dimension, variables[j].m_field, 0);
					}
					else
					{
						std::string title;
						size_t col = pResult->GetCol(VARIABLE, variables[j].m_field);
						if (col != UNKNOWN_POS)
						{
							for (size_t s = statistic.find_first(), S = 0; s != UNKNOWN_POS; s = statistic.find_next(s), S++)
							{
								title = pResult->GetFieldTitle(variables[j].m_dimension, variables[j].m_field, s);

								if (S > 0)
									line += listDelimiter;

								line += title;

							}
						}
					}
				}

				file.write(line + "\n");
			}


			const CDBSectionIndexVector& index = pResult->GetDatabase().GetIndex();


			//Write data
			for (int i = 0; i < pResult->GetNbRows() && msg; i++)
			{

				size_t sectionNo = pResult->GetSectionNo(i);
				size_t lNo = pResult->GetMetadata().GetLno(sectionNo);
				size_t pNo = pResult->GetMetadata().GetPno(sectionNo);
				size_t rNo = pResult->GetMetadata().GetRno(sectionNo);
				bool bHaveData = index[sectionNo].HaveData();

				if (CTRL.m_bExportAllLines || bHaveData)//elimination by section
				{
					if (CTRL.m_bExportAllLines || pResult->HaveData(i))//no data for this entire row
					{
						bHaveData = false;//does have data for exported cols

						std::string line;
						for (size_t j = 0; j < variables.size(); j++)
						{
							if (j > 0)
								line += listDelimiter;

							std::string tmp;
							if (variables[j].m_dimension < VARIABLE)
							{
								switch (variables[j].m_dimension)
								{
								case LOCATION:

									if (variables[j].m_field == CLocation::SSI)
									{
										for (size_t k = 0; k < SSI.size(); k++)
										{
											if (k > 0)
												tmp += listDelimiter;
											tmp += loc[lNo].GetSSI(SSI[k]);
										}
									}
									else
									{
										tmp = loc[lNo].GetMember(variables[j].m_field);
									}

									break;
								case PARAMETER:
								{
									if (!param.m_variation.empty())
									{
										for (size_t i = 0; i < param.m_variation.size(); i++)
										{
											if (param.m_variation[i].m_bActive)
											{
												if (!tmp.empty())
													tmp += listDelimiter;

												if (i < param[pNo].size())
													tmp += param[pNo][i].GetStr();
											}
										}
									}
									else
									{
										tmp += "1";
									}
									break;
								}
								case REPLICATION: tmp = ToString(rNo + 1); break;
								case TIME_REF:	tmp = pResult->GetDataValue(i, TIME_REF, 0); break;
								default: ASSERT(false);
								}
							}
							else
							{
								size_t col = pResult->GetCol(VARIABLE, variables[j].m_field);
								if (col < pResult->GetNbCols())
								{
									bHaveData = bHaveData || pResult->GetData(i, col)[NB_VALUE] > 0;

									for (size_t s = statistic.find_first(), S = 0; s != UNKNOWN_POS; s = statistic.find_next(s), S++)
									{
										if (S > 0)
											tmp += listDelimiter;

										string tmp2 = pResult->GetDataValue(i, col, s);
										//if value contain list delimiter (like time format), remove it
										std::replace(tmp2.begin(), tmp2.end(), listDelimiter, listDelimiter != '-' ? '-' : '/');

										tmp += tmp2;
									}
								}
							}

							line += tmp.empty() ? " " : tmp;
						}//for all variable

						std::replace(line.begin(), line.end(), '.', decimalDelimiter);

						if (CTRL.m_bExportAllLines || bHaveData)//elimnation by line
							file.write(line + '\n');
					}
				}
				msg += callback.StepIt();
			}//for all rows

			callback.PopTask();
		}//if open

		file.close();

		return msg;
	}


	ERMsg CExecutable::ExecuteScript(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;


		if (!m_export.m_scriptName.empty())
		{
			std::string argument = "\"" + fileManager.Script().GetFilePath(m_export.m_scriptName);
			argument += "\" \"" + GetExportFilePath(fileManager, CExecutable::EXPORT_CSV) + "\"";
			ASSERT(false);
			//msg = CallApplication(CRegistry::SCRIPT, argument, NULL /*AfxGetMainWnd()*/, SW_HIDE, false);
		}

		return msg;
	}



	void CExecutable::CopyToClipBoard()
	{
		zen::XmlDoc doc("BioSIMComponent");
		doc.root().setAttribute("ClassName", GetClassName());
		writeStruc(doc.root());

		std::string str = zen::serialize(doc);

		SetClipboardText(str);
	}

	double CExecutable::GetExecutionTime(const std::string& name, CTM TM, bool bUseHxGrid)
	{
		double timePerUnit = 0;
		if (!name.empty())
		{
			CRegistry registry("ExecutionTime");
			std::string model = name + "_" + TM.GetTypeName() + (bUseHxGrid ? "[HxGrid]" : "");
			timePerUnit = registry.GetValue(model, 0.0);
		}

		return timePerUnit;
	}

	void CExecutable::SetExecutionTime(const std::string& name, double timePerUnit, CTM TM, bool bUseHxGrid)
	{
		CRegistry registry("ExecutionTime");
		std::string model = name + +"_" + TM.GetTypeName() + (bUseHxGrid ? "[HxGrid]" : "");
		registry.SetValue(model, timePerUnit);
	}


	CExecutablePtr CExecutable::CopyFromClipBoard()
	{
		CExecutablePtr pItem;

		try
		{
			std::string str = GetClipboardText();

			zen::XmlDoc doc = zen::parse(str);
			zen::XmlElement& root = doc.root();


			std::string className;
			if (root.getAttribute("ClassName", className))
			{
				pItem = CExecutableFactory::CreateObject(className);
				if (pItem)
				{
					//add if compatible ...
					pItem->SetParent(this);
					pItem->readStruc(root);
					pItem->UpdateInternalName();
				}
			}
		}
		catch (...)
		{
		}

		return pItem;
	}



	void CExecutable::writeStruc(zen::XmlElement& output)const
	{
		zen::XmlOut out(output);

		out[GetMemberName(NAME)](m_name);
		out[GetMemberName(INTERNAL_NAME)](m_internalName);
		out[GetMemberName(DESCRIPTION)](m_description);
		out[GetMemberName(EXECUTE)](m_bExecute);
		out[GetMemberName(EXPORT_DATA)](m_export);
		//	out[GetMemberName(GRAPHS)](m_graphArray);
		out[GetMemberName(EXECUTABLES)](m_executables);
	}

	bool CExecutable::readStruc(const zen::XmlElement& input)
	{
		zen::XmlIn in(input);

		in[GetMemberName(NAME)](m_name);
		in[GetMemberName(INTERNAL_NAME)](m_internalName);
		in[GetMemberName(DESCRIPTION)](m_description);
		in[GetMemberName(EXECUTE)](m_bExecute);
		in[GetMemberName(EXPORT_DATA)](m_export);
		//in[GetMemberName(GRAPHS)](m_graphArray);
		in[GetMemberName(EXECUTABLES)](m_executables);

		return true;
	}




	//****************************************************************
	//CProjectState

	const char* CProjectState::MEMBER_NAME[NB_MEMBERS] = { "ExpandedItems" };

	CProjectState::CProjectState()
	{
	}

	void CProjectState::clear()
	{
		m_expendedItems.clear();
	}



}