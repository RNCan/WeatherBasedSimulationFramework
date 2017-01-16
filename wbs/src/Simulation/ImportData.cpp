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
#include <time.h>


#include "Basic/ModelStat.h"
#include "Basic/Registry.h"
#include "FileManager/FileManager.h"
#include "Simulation/ExecutableFactory.h"
#include "Simulation/ImportData.h"

#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF::WEATHER;
using namespace WBSF::DIMENSION;

namespace WBSF
{




	//**************************************************************
	//CColumnLinkVector

	//static const char* XML_FLAG;
	bool CColumnLinkVector::HaveDimension(int dim)const
	{
		bool bRep = false;
		for (int i = 0; i < size(); i++)
		{
			if (at(i).m_dimension == dim)
			{
				bRep = true;
				break;
			}
		}

		return bRep;
	}

	void CColumnLinkVector::GetLocation(const StringVector& columnList, CLocation& LOC)const
	{
		ASSERT(columnList.size() == size());

		for (int i = 0; i < size(); i++)
		{
			if (at(i).m_dimension == LOCATION)
			{
				if (at(i).m_field < CLocation::SITE_SPECIFIC_INFORMATION)
				{
					LOC.SetMember(at(i).m_field, columnList[i]);
				}
				else
				{
					std::string others = LOC.GetMember(CLocation::SITE_SPECIFIC_INFORMATION);
					others += others.empty() ? "" : ",";
					others += columnList[i];
					LOC.SetMember(CLocation::SITE_SPECIFIC_INFORMATION, others);
				}
			}
		}
	}


	std::string CColumnLinkVector::GetOtherHeader(const StringVector& header)const
	{
		ASSERT(header.size() == size());
		std::string others;

		for (int i = 0; i < size(); i++)
		{
			if (at(i).m_dimension == LOCATION)
			{
				if (at(i).m_field == CLocation::SITE_SPECIFIC_INFORMATION)
				{
					others += others.empty() ? "" : ",";
					others += header[i];
				}
			}
		}

		return others;
	}

	size_t CColumnLinkVector::GetReplication(const StringVector& columnList)const
	{
		ASSERT(columnList.size() == size());

		int replication = -1;
		for (int i = 0; i < size(); i++)
		{
			if (at(i).m_dimension == REPLICATION)
			{
				replication = ToInt(columnList[i]);
				break;
			}
		}

		return replication;
	}

	CTRef CColumnLinkVector::GetTRef(const StringVector& columnList)const
	{
		ASSERT(columnList.size() == size());

		CTRef TRef;
		TRef.m_mode = CTRef::OVERALL_YEARS;//over all year by default. FOR_EACH_YEAR if year is prensent.

		for (int i = 0; i < size(); i++)
		{
			if (at(i).m_dimension == TIME_REF)
			{
				if (at(i).m_field == CTRefFormat::NB_FORMAT)
					//at(i).m_field == CTRef::DATE_YMD ||
					//at(i).m_field == CTRef::DATE_DMY ||
					//at(i).m_field == CTRef::DATE_MD ||
					//at(i).m_field == CTRef::DATE_DM )

				{
					TRef.FromFormatedString(columnList[i], "", "/- :");
				}
				else
				{
					ASSERT(CTRef::ANNUAL == CTRefFormat::YEAR);
					ASSERT(CTRef::DAILY == CTRefFormat::DAY);
					TRef.m_type = (TRef.m_type == CTRef::UNKNOWN) ? at(i).m_field : max(size_t(TRef.m_type), at(i).m_field);
					switch (at(i).m_field)
					{
					case CTRefFormat::HOUR: TRef.m_hour = ToInt(columnList[i]); break;
					case CTRefFormat::DAY: TRef.m_day = ToInt(columnList[i]); break;
					case CTRefFormat::MONTH: TRef.m_month = ToInt(columnList[i]); break;
					case CTRefFormat::YEAR:
						TRef.m_year = ToInt(columnList[i]);
						TRef.m_mode = CTRef::FOR_EACH_YEAR;
						break;
					case CTRefFormat::REFERENCE: TRef.m_ref = ToInt(columnList[i]); break;
					case CTRefFormat::JDAY: TRef.SetJDay(TRef.m_year, ToInt(columnList[i])); break;
					default: ASSERT(false);
					}
				}
			}
		}


		return TRef;
	}

	void CColumnLinkVector::GetVariable(const StringVector& columnList, CModelStat& outputVar)const
	{
		ASSERT(columnList.size() == size());

		for (int i = 0; i < size(); i++)
		{
			if (at(i).m_dimension == VARIABLE)
			{
				outputVar.push_back(ToDouble(columnList[i]));
			}
		}

	}

	CTM CColumnLinkVector::GetTM()const
	{
		bool bHave[CTRefFormat::NB_FORMAT + 1] = { 0 };

		for (int i = 0; i < size(); i++)
		{
			if (at(i).m_dimension == TIME_REF)
			{
				bHave[at(i).m_field] = true;
			}
		}

		CTM TM = -1;

		//if( bHave[CTRef::DATE_YMD] || bHave[CTRef::DATE_DMY])
		//	TM=CTM(CTRef::DAILY, CTRef::FOR_EACH_YEAR);
		//else if( bHave[CTRef::DATE_MD] || bHave[CTRef::DATE_DM])
		//	TM=CTM(CTRef::DAILY, CTRef::OVERALL_YEARS);
		if (bHave[CTRefFormat::NB_FORMAT])
		{
			assert(false);
		}
		else
		{
			if (bHave[CTRefFormat::HOUR] && ((bHave[CTRefFormat::MONTH] && bHave[CTRefFormat::DAY]) || bHave[CTRefFormat::JDAY]))
				TM = CTM(CTRef::HOURLY, bHave[CTRefFormat::YEAR] ? CTRef::FOR_EACH_YEAR : CTRef::OVERALL_YEARS);
			else if ((bHave[CTRefFormat::MONTH] && bHave[CTRefFormat::DAY]) || bHave[CTRefFormat::JDAY])
				TM = CTM(CTRef::DAILY, bHave[CTRefFormat::YEAR] ? CTRef::FOR_EACH_YEAR : CTRef::OVERALL_YEARS);
			else if (bHave[CTRefFormat::MONTH])
				TM = CTM(CTRef::MONTHLY, bHave[CTRefFormat::YEAR] ? CTRef::FOR_EACH_YEAR : CTRef::OVERALL_YEARS);
			else TM = CTM(CTRef::ANNUAL, bHave[CTRefFormat::YEAR] ? CTRef::FOR_EACH_YEAR : CTRef::OVERALL_YEARS);
		}

		ASSERT(TM.IsInit());

		return TM;
	}

	void CColumnLinkVector::GetOutputDefinition(CModelOutputVariableDefVector& def)const
	{
		def.clear();
		for (size_t i = 0; i < size(); i++)
		{
			if (at(i).m_dimension == VARIABLE)
			{
				def.push_back(CModelOutputVariableDef(at(i).m_name, at(i).m_name, "", "", CTM(CTM::ATEMPORAL), 5,"",at(i).m_field));
			}
		}
	}

	//**************************************************************
	//CImportData

	const char* CColumnLink::XML_FLAG = "ColumnLink";
	const char* CColumnLink::MEMBER_NAME[NB_MEMBER] = { "Name", "Dimension", "Field" };
	///const char* CArrayEx<CColumnLink>::XML_FLAG = "ColumnLinkArray";

	const char* CImportData::XML_FLAG = "ImportData";
	const char* CImportData::MEMBER_NAME[NB_MEMBER_EX] = { "FileName", "ColumnLinkArray" };
	const int CImportData::CLASS_NUMBER = CExecutableFactory::RegisterClass(CImportData::GetXMLFlag(), &CImportData::CreateObject);


	CImportData::CImportData()
	{
		Reset();
	}

	CImportData::CImportData(const CImportData& in)
	{
		operator=(in);
	}


	CImportData::~CImportData()
	{}

	void CImportData::Reset()
	{
		CExecutable::Reset();

		m_name = "ImportData";

		m_fileName.empty();
		m_columnLinkArray.clear();


		//parameter for optimisation
		ResetOptimization();
	}

	void CImportData::ResetOptimization()const
	{
		CImportData& me = const_cast<CImportData&>(*this);

		me.m_bInfoLoaded = false;
		me.m_locArray.clear();
		me.m_period.Reset();
		me.m_nbReplications = 0;
		me.m_varDefArray.clear();
		me.m_parameterSet.clear();
	}

	ERMsg CImportData::LoadOptimisation(const CFileManager& fileManager)const
	{
		ERMsg msg;
		CImportData& me = const_cast<CImportData&>(*this);

		if (me.m_bInfoLoaded == false)
		{

			CResultPtr pResult = me.GetResult(fileManager);
			msg = pResult->Open();
			if (msg)
			{
				me.m_bInfoLoaded = true;

				me.m_locArray = pResult->GetMetadata().GetLocations();
				me.m_period = pResult->GetMetadata().GetTPeriod();
				me.m_nbReplications = pResult->GetMetadata().GetNbReplications();
				me.m_varDefArray = pResult->GetMetadata().GetOutputDefinition();
				me.m_parameterSet = pResult->GetMetadata().GetParameterSet();
			}
		}

		return msg;
	}




	CImportData& CImportData::operator =(const CImportData& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);
			m_fileName = in.m_fileName;
			m_columnLinkArray = in.m_columnLinkArray;
			ResetOptimization();
			//m_lastUpdate = in.m_lastUpdate;
			//m_bTemporalData=in.m_bTemporalData;
		}

		return *this;
	}

	bool CImportData::operator == (const CImportData& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator!=(in))bEqual = false;
		if (m_fileName != in.m_fileName) bEqual = false;
		if (m_columnLinkArray != in.m_columnLinkArray) bEqual = false;
		//if( m_bTemporalData != in.m_bTemporalData) bEqual = false;

		return bEqual;
	}


	/*
	std::string CImportData::GetMember(int i, LPXNode& pNode)const
	{
	ASSERT( i>=0 && i<NB_MEMBER);

	std::string str;
	switch(i)
	{
	case FILE_NAME: str = m_fileName ; break;
	//case TEMPORAL_DATA: str = ToString(m_bTemporalData); break;
	case COLUMN_LINK: m_columnLinkArray.GetXML(pNode); break;
	default: str = CExecutable::GetMember(i, pNode);
	}

	return str;
	}

	void CImportData::SetMember(int i, const std::string& str, const LPXNode pNode)
	{
	ASSERT( i>=0 && i<NB_MEMBER);
	switch(i)
	{
	case FILE_NAME: m_fileName = str; break;
	//case TEMPORAL_DATA: m_bTemporalData=ToBool(str); break;
	case COLUMN_LINK: m_columnLinkArray.SetXML(pNode); break;
	default: CExecutable::SetMember(i, str, pNode);
	}
	}
	*/
	/*CTPeriod CImportData::GetDefaultPeriod(CFileManager& fileManager)const
	{
	CTPeriod p;

	//1- open the TGInput to get first and last year
	CTGInput TGInput;
	int firtYear = 0;
	int lastYear = 0;

	if( LoadTGInput(fileManager, TGInput) )
	{
	firtYear = TGInput.GetFirstYear();
	lastYear = TGInput.GetLastYear();
	}

	//2- Get the model to know the kind of output model
	CModel model;
	int TType = CTRef::UNKNOWN;
	if( fileManager.Model().Get(m_modelName, model) )
	{
	TType = model.GetOutputTType();
	//est-ce que l'on remplace ca par m_outputtype???
	//ou on élimine m_outputtype et m_outputFormat

	//a faire: mettre le output type du model et le bon
	//2- create the begin and end date
	CTRef begin(TType, firtYear,0,0,0);
	CTRef end(TType, lastYear, LAST_MONTH, LAST_DAY, LAST_HOUR);

	p = CTPeriod(begin,end);
	}



	//3- create period
	return p;
	}
	*/
	/*
	ERMsg CImportData::Validation(CFileManager& fileManager)const
	{
	ERMsg msg;
	msg = ValidationDB(fileManager);
	if( msg )
	{
	//      switch( CTRL.m_outputType )
	//      {
	//case ASCII:
	//	msg = ValidationOutput(fileManager);
	//	break;
	//case MDB:
	//          msg = ValidationOutputOLEDB(fileManager);
	//	break;
	//case HOMEDB:
	msg = ValidationOutputHomeDB(fileManager);
	//			break;
	//		default: ASSERT(false);
	//      }

	}

	return msg;
	}
	*/
	//ERMsg CImportData::GetValidation()

	std::string CImportData::GetOptFilePath(const CFileManager& fileManager)const
	{
		//std::string filePath( + m_fileName);
		return GetFilePath(GetPath(fileManager), ".opt");
	}

	bool CImportData::IsUpToDate(const CFileManager& fileManager)const
	{
		bool bIsUptodate = false;

		std::string refFilePath(fileManager.Input().GetFilePath(m_fileName));
		std::string optFilePath = GetOptFilePath(fileManager);

		if (GetFileStamp(optFilePath) > GetFileStamp(refFilePath))
		{
			bIsUptodate = true;
		}

		return bIsUptodate;
	}

	ERMsg CImportData::UpdateData(const CFileManager& fileManager, CCallback& callback)const
	{
		ERMsg msg;

		if (!IsUpToDate(fileManager))
		{
			msg = ImportFile(fileManager, callback);
		}

		return msg;
	}

	ERMsg CImportData::ReadData(ifStream& file, CModelStatVector& data, CLocation& lastLOC, size_t& lastReplication, CCallback& callback)const
	{
		ERMsg msg;


		data.clear();

		bool bHaveData[NB_DIMENSION] = { 0 };
		for (int i = 0; i < NB_DIMENSION; i++)
			bHaveData[i] = m_columnLinkArray.HaveDimension(i);

		ASSERT(bHaveData[VARIABLE]);

		data.Reset();
		lastLOC.Reset();
		lastReplication = -1;


		ios::pos_type pos = file.tellg();
		string line;

		while (std::getline(file, line) && msg)
		{
			StringVector columnList(line, ",;\t");
			ASSERT(columnList.size() == m_columnLinkArray.size());


			//Get lcoation
			CLocation LOC;
			if (bHaveData[LOCATION])
			{
				m_columnLinkArray.GetLocation(columnList, LOC);
				if (!lastLOC.IsInit())
					lastLOC = LOC;
			}

			//Get replication
			size_t replication = 0;

			if (bHaveData[REPLICATION])
			{
				replication = m_columnLinkArray.GetReplication(columnList);
				if (lastReplication == -1)
					lastReplication = replication;
			}

			if (LOC != lastLOC || replication != lastReplication)
				break;


			CTM TM(CTM::ANNUAL, CTM::OVERALL_YEARS);
			CTRef TRef(0, 0, 0, 0, TM);
			if (bHaveData[TIME_REF])
				TRef = m_columnLinkArray.GetTRef(columnList);

			CModelStat outputVar;
			m_columnLinkArray.GetVariable(columnList, outputVar);


			if (true /*outputVar.size() ==*/)
			{
				data.Insert(TRef, outputVar);
			}
			else
			{
				msg.ajoute(FormatMsg(IDS_SIM_BAD_OUTPUTFILE_FORMAT, m_fileName, line));
			}

			pos = file.tellg();

			msg += callback.SetCurrentStepPos((size_t)pos);
		}

		file.seekg(pos);


		return msg;
	}


	ERMsg CImportData::ImportFile(const CFileManager& fileManager, CCallback& callback)const
	{
		ERMsg msg;

		ResetOptimization();

		std::string filePath = fileManager.Input().GetFilePath(m_fileName);
		//if( !IsUpToDate() )
		//{
		//msg = ImportFile(fileManager, callback);
		ifStream file;
		msg = file.open(filePath);
		if (msg)
		{
			//Generate output path
			std::string outputPath = GetPath(fileManager);

			//Generate DB file path
			std::string DBFilePath = GetDBFilePath(outputPath);

			//create output database and open it
			CResult db;

			msg = db.Open(DBFilePath, std::fstream::binary | std::fstream::out | std::fstream::trunc);
			if (!msg)
				return msg;


			callback.PushTask(FormatMsg(IDS_SIM_IMPORT, m_fileName), file.length());
			//callback.SetNbStep(file.length());


			//read file header
			string header;
			std::getline(file, header);

			StringVector columnHeader(header, ",;\t");
			if (columnHeader.size() == m_columnLinkArray.size())
			{

				//beginning reading
				CLocationVector locArray;
				size_t nbReplication = 1;

				CModelStatVector data;
				CLocation lastLOC;
				size_t lastReplication = 0;
				size_t firstReplication = 1;

				//locArray.SetOtherHeader(m_columnLinkArray.GetOtherHeader(header));

				msg = ReadData(file, data, lastLOC, lastReplication, callback);

				if (!lastLOC.IsInit())
					locArray.push_back(CLocation());

				//we keep in mind the first replication number
				//to init loc correcly. We assume the the first line begin 
				//with the first replication number
				if (lastReplication != 0)
					firstReplication = lastReplication;


				while (msg && !data.empty())
				{
					bool bAddLOC = true;
					if (lastReplication != 0)
					{
						nbReplication = max(nbReplication, lastReplication - firstReplication + 1);
						if (lastReplication != firstReplication)
							bAddLOC = false;//don't add loc because it's a replication
					}

					if (bAddLOC && lastLOC.IsInit())
						locArray.push_back(lastLOC);

					db.AddSection(data);
					msg = ReadData(file, data, lastLOC, lastReplication, callback);
					//msg += callback.StepIt(0);
				}

				if (locArray.empty())
					locArray.push_back(CLocation("default", "0"));

				CModelOutputVariableDefVector outputVariable;
				m_columnLinkArray.GetOutputDefinition(outputVariable);

				CDBMetadata& metadata = db.GetMetadata();
				metadata.SetLocations(locArray);
				metadata.SetNbReplications(nbReplication);
				metadata.SetModelName(GetFileTitle(m_fileName));
				metadata.SetOutputDefinition(outputVariable);
				//				metadata.SetDataTM(vector<CTM>(outputVariable.size()));


				db.Close();
			}

			callback.PopTask();
		}//if msg




		return msg;
	}

	ERMsg CImportData::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ASSERT(GetNbExecute() > 0);

		ERMsg msg;

		if (!IsUpToDate(fileManager))
			msg = ImportFile(fileManager, callback);

		return msg;
	}


	double CImportData::GetModelTime()
	{
		CRegistry option("SimulationTime");

		return option.GetValue("ImportedFile", 0.0);
	}

	void CImportData::SetModelTime(double timePerUnit)
	{
		//double timePerUnit = timer.Elapsedms()/(fileSize/1000);

		CRegistry option("SimulationTime");
		option.SetValue("ImportedFile", timePerUnit);
	}

	//CResultPtr CImportData::GetResult(const CFileManager& fileManager)const
	//{
	//	//CImportDataDB* pDB = new CImportDataDB;
	//	CResult* pDB = new CResult;
	//	pDB->SetFilePath( GetDBFilePath(GetPath(fileManager)));
	//	
	//	CResultPtr ptr( pDB );
	//
	//	return ptr;
	//}

	/*ERMsg CImportData::GetDimensionList(const CFileManager& fileManager, int dim, StringVector& list)const
	{
	ASSERT( dim>=0 && dim<DIMENSION::NB_DIMENSION);

	ERMsg msg;
	list.clear();


	switch( dim)
	{
	case DIMENSION::LOCATION:
	{
	CResultPtr pResult = GetResult(fileManager);
	if( pResult->Open() )
	{
	constCLOCArray& loc = pResult->GetMetadata().GetLOC();


	msg = fileManager.LOC().Get(m_LOCName, loc);
	if(msg)
	{
	list.resize(loc.size());

	for(int i=0; i<loc.size(); i++)
	list[i] = loc[i].GetName() + " (" + loc[i].GetID() + ")";
	}

	}break;

	case DIMENSION::PARAMETER: ASSERT(false); //Is it used???
	case DIMENSION::REPLICATION:
	{
	list.push_back( ToString(m_nbReplications) );
	}break;

	case DIMENSION::TIME_REF:
	{
	ASSERT( !m_modelName.empty() );

	CModel model;
	msg = fileManager.Model().Get(m_modelName, model);
	if(msg)
	{
	list.push_back( ToString(model.GetTM()) );
	CTPeriod p = GetDefaultPeriod(fileManager);
	list.push_back( p.ToString().c_str() );
	}
	}break;

	case DIMENSION::VARIABLE:
	{
	//Variable list
	ASSERT( !m_modelName.empty() );

	CModel model;
	msg = fileManager.Model().Get(m_modelName, model);
	if(msg)
	{
	const CModelOutputVariableDefVector& variablesDef = model.GetOutputDefinition();
	list.resize(variablesDef.size());

	for(int i=0; i<variablesDef.size(); i++)
	list[i] = variablesDef[i].GetName();

	}
	}break;
	default: ASSERT(false);
	}

	return msg;

	}
	*/
	
	ERMsg CImportData::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter )const
	{
		ERMsg msg;
		msg = UpdateData(fileManager);
		if (msg)
			msg = LoadOptimisation(fileManager);
		
		if (msg)
		{
			if (filter[LOCATION])
				info.m_locations = m_locArray;

			if (filter[PARAMETER])
				info.m_parameterset = m_parameterSet;

			if (filter[REPLICATION])
				info.m_nbReplications = m_nbReplications;

			if (filter[TIME_REF])
				info.m_period = m_period;

			if (filter[VARIABLE])
				info.m_variables = m_varDefArray;
		}

		return msg;
	}

	void CImportData::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(FILE_NAME)](m_fileName);
		out[GetMemberName(COLUMN_LINK)](m_columnLinkArray);
	}

	bool CImportData::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(FILE_NAME)](m_fileName);
		in[GetMemberName(COLUMN_LINK)](m_columnLinkArray);

		return true;
	}

}