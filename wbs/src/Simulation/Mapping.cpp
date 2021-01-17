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

#include "Basic/Statistic.h"
#include "Geomatic/GridInterpolBase.h"
#include "Geomatic/GridInterpol.h"
#include "FileManager/FileManager.h"
#include "Simulation/ExecutableFactory.h"
#include "Simulation/Mapping.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

#include "WeatherBasedSimulationString.h"
#include "Basic/QGISPalette.h"

using namespace WBSF::DIMENSION;
using namespace std;

#include "gdal.h"

namespace WBSF
{




	//**********************************************************************
	// Construction/Destruction
	//**********************************************************************
	const char* CMapping::XML_FLAG = "Mapping";
	const char* CMapping::MEMBERS_NAME[NB_MEMBERS_EX] = { "Method", "DEMName", "TEMName", "PrePostTransfo", "XValOnly", "CreateLegendOnly", CGridInterpolParam::GetXMLFlag(), "QGISOptions" };
	const int CMapping::CLASS_NUMBER = CExecutableFactory::RegisterClass(CMapping::GetXMLFlag(), &CMapping::CreateObject);

	CMapping::CMapping()
	{
		m_pParam.reset(new CGridInterpolParam);
		m_pPrePostTransfo.reset(new CPrePostTransfo);
		Reset();
	}

	CMapping::~CMapping()
	{
	}

	void CMapping::Reset()
	{
		CExecutable::Reset();
		m_name = "Mapping";
		m_method = CGridInterpol::UNIVERSAL_KRIGING;
		m_pParam->Reset();

		m_DEMName.clear();
		m_TEMName = "%c%v%t";
		m_pPrePostTransfo->Reset();

		m_XValOnly = false;
		m_createLengendOnly = false;
		m_createStyleFile.LoadFromRegistry();
	}

	CMapping::CMapping(const CMapping& in)
	{
		m_pParam.reset(new CGridInterpolParam);
		m_pPrePostTransfo.reset(new CPrePostTransfo);
		operator=(in);
	}




	CMapping& CMapping::operator =(const CMapping& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);

			m_DEMName = in.m_DEMName;
			m_TEMName = in.m_TEMName;
			m_method = in.m_method;
			*m_pParam = *in.m_pParam;
			*m_pPrePostTransfo = *in.m_pPrePostTransfo;
			m_XValOnly = in.m_XValOnly;
			m_createLengendOnly = in.m_createLengendOnly;
			m_createStyleFile = in.m_createStyleFile;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CMapping::operator == (const CMapping& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator !=(in))bEqual = false;
		if (m_method != in.m_method)bEqual = false;
		if (m_DEMName != in.m_DEMName)bEqual = false;
		if (m_TEMName != in.m_TEMName)bEqual = false;
		if (*m_pParam != *in.m_pParam)bEqual = false;
		if (*m_pPrePostTransfo != *in.m_pPrePostTransfo)bEqual = false;
		if (m_XValOnly != in.m_XValOnly)bEqual = false;
		if (m_createLengendOnly != in.m_createLengendOnly)bEqual = false;
		if (m_createStyleFile != in.m_createStyleFile)bEqual = false;

		return bEqual;
	}

	ERMsg CMapping::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
	{
		ERMsg msg;

		msg = m_pParent->GetParentInfo(fileManager, info, filter);

		if (filter[VARIABLE])
		{
			info.m_variables = CleanVariables(info.m_variables);
		}

		return msg;
	}

	CModelOutputVariableDefVector CMapping::CleanVariables(const CModelOutputVariableDefVector& outputVarIn)const
	{
		CModelOutputVariableDefVector outputVarOut;

		size_t vSize = GetNbMapVariableIndex(outputVarIn);

		for (size_t i = 0; i < vSize; i++)
		{
			outputVarOut.push_back(CModelOutputVariableDef(outputVarIn[i].m_name + "Sim", outputVarIn[i].m_title + " -Sim-", outputVarIn[i].m_units, "Variable simulated by the model", outputVarIn[i].m_TM, outputVarIn[i].m_precision, outputVarIn[i].m_equation, outputVarIn[i].m_climaticVariable));
			outputVarOut.push_back(CModelOutputVariableDef(outputVarIn[i].m_name + "Map", outputVarIn[i].m_title + " -Map-", outputVarIn[i].m_units, "Variable generate by the spatial interpolation", outputVarIn[i].m_TM, outputVarIn[i].m_precision, outputVarIn[i].m_equation, outputVarIn[i].m_climaticVariable));
		}


		return outputVarOut;
	}

	ERMsg CMapping::CreateStyleFile(const std::string& TEMFilePath, CTM TM)const
	{
		ERMsg msg;

		CCreateStyleOptions options = m_createStyleFile;
		

		string pal_file_path = GetApplicationPath() + "Palette\\Palettes.xml";
		CQGISPalettes palettes;
		msg = palettes.load(pal_file_path);
		if (!msg)
			return msg;

		if (palettes.find(options.m_palette_name) == palettes.end())
		{
			msg.ajoute("Create QGIS style file, palette not found: " + options.m_palette_name);
			return msg;
		}

		//open rater to get statistic
		CGDALDatasetEx dataset;
		msg = dataset.OpenInputImage(TEMFilePath);
		if (msg)
		{
			//dataset.ComputeStats(TRUE);

			GDALRasterBand* pBand = dataset.GetRasterBand(0);

			double dfMin, dfMax, dfMean, dfStdDev;
			pBand->ComputeStatistics(false, &dfMin, &dfMax, &dfMean, &dfStdDev, GDALDummyProgress, NULL);

			if (options.m_min_max_type == CCreateStyleOptions::BY_MINMAX)
			{
				options.m_min = dfMin;
				options.m_max = dfMax;
			}
			else if (options.m_min_max_type == CCreateStyleOptions::BY_STDDEV)
			{
				options.m_min = dfMean - options.m_var_factor * dfStdDev;
				options.m_max = dfMean + options.m_var_factor * dfStdDev;
			}
			else if (options.m_min_max_type == CCreateStyleOptions::BY_USER)
			{
				if (options.m_min >= options.m_max)
				{
					msg.ajoute("Create QGIS style file: Invalid user min/max. minimum must be smaller than maximum" + options.m_palette_name);
					return msg;
				}

			}

			string file_path = TEMFilePath + ".qml";
			palettes[options.m_palette_name].CreateStyleFile(file_path, options, TM);
		}

		return msg;
	}


	void CMapping::GetInputDBInfo(CResultPtr& pResult, CDBMetadata& info)const
	{
		info = pResult->GetMetadata();

		const CModelOutputVariableDefVector& inputVar = pResult->GetMetadata().GetOutputDefinition();
		CModelOutputVariableDefVector outputVarOut = CleanVariables(inputVar);
		info.SetOutputDefinition(outputVarOut);
	}


	ERMsg CMapping::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ASSERT(GetParent());
		ASSERT(GetExecute());


		ERMsg msg;

		//open input DB
		CResultPtr pResult = m_pParent->GetResult(fileManager);

		msg = pResult->Open();
		if (!msg)
			return msg;

		//Generate output path
		string outputPath = GetPath(fileManager);

		//Generate DB file path
		string DBFilePath = GetDBFilePath(outputPath);

		//open outputDB
		CResult result;
		msg = result.Open(DBFilePath, std::fstream::binary | std::fstream::out | std::fstream::trunc);
		if (msg)
		{
			//init output info
			CDBMetadata& metadata = result.GetMetadata();
			GetInputDBInfo(pResult, metadata);

			CTPeriod p = pResult->GetTPeriod();
			const CModelOutputVariableDefVector& outputDef = pResult->GetMetadata().GetOutputDefinition();
			size_t maxSize = outputDef.size();
			const CModelInputVector& parameterSet = pResult->GetMetadata().GetParameterSet();

			size_t tSize = GetNbMapTimeIndex(pResult);
			size_t vSize = GetNbMapVariableIndex(outputDef);
			size_t pSize = GetNbMapParameterIndex(parameterSet);
			size_t rSize = GetNbReplicationIndex(pResult);
			std::vector<CTM> TM = pResult->GetMetadata().GetDataTM();
			ASSERT(TM.size() == vSize);

			//Xvalidation of all maps 
			std::vector<CXValidationVector> XValVector(pSize*tSize*vSize*rSize);

			callback.PushTask("Create maps (" + ToString(pSize*tSize*vSize*rSize) + ")", pSize*tSize*vSize*rSize);
			for (size_t p = 0; p < pSize&&msg; p++)
			{
				for (size_t r = 0; r < rSize&&msg; r++)
				{
					for (size_t t = 0; t < tSize&&msg; t++)
					{
						for (size_t v = 0; v < vSize&&msg; v++)
						{
							callback.AddMessage("*********************************************************************");
							size_t index = p * (tSize*vSize) + (t*vSize) + v;

							CGridInterpol gridInterpol;
							msg = InitGridInterpol(fileManager, pResult, p, r, t, v, gridInterpol, callback);
							if (msg)
							{
								//skip when no data without returning error
								if (gridInterpol.m_pts->size() > 0)
								{
									if (msg)
										msg += gridInterpol.Initialise(callback);

									if (msg && (m_XValOnly||!m_createLengendOnly))
										msg += gridInterpol.OptimizeParameter(callback);

									if (msg && !m_XValOnly && !m_createLengendOnly)
										msg += gridInterpol.CreateSurface(callback);

									if (msg && (!m_XValOnly|| m_createLengendOnly) && m_createStyleFile.m_create_style_file)
										msg += CreateStyleFile(gridInterpol.m_TEMFilePath, TM[v]);

									if (msg && (m_XValOnly || !m_createLengendOnly))
										XValVector[index] = gridInterpol.GetOutput();
								}
								else
								{
									callback.AddMessage(FormatMsg(IDS_SIM_SKIP_MAPPING, GetTEMName(pResult, p, r, t, v)));
								}

								gridInterpol.Finalize();
								callback.AddMessage("*********************************************************************");
								callback.AddMessage("");

								msg += callback.StepIt();
							}//if msg
						}//for all variables
					}//for all temporal steps
				}//for all replication
			}//for all parameters

			callback.PopTask();
			if (msg)
			{
				//Save output to file
				const CLocationVector& loc = metadata.GetLocations();
				for (int i = 0; i < loc.size(); i++)
				{
					CNewSectionData section;
					section.resize(tSize, vSize * 2, p.Begin());//obs and sim

					for (size_t p = 0; p < pSize; p++)
					{
						for (size_t r = 0; r < rSize; r++)
						{
							for (size_t t = 0; t < tSize; t++)
							{
								for (size_t v = 0; v < vSize; v++)
								{
									size_t index = p * (tSize*vSize) + (t*vSize) + v;

									//ASSERT( XValVector[index].size() == loc.size() );
									if (i < XValVector[index].size())
									{
										if (XValVector[index][i].m_observed > VMISS)
											section[t][v * 2] += XValVector[index][i].m_observed;
										if (XValVector[index][i].m_predicted > VMISS)
											section[t][v * 2 + 1] += XValVector[index][i].m_predicted;
									}
								}//for all variables
							}//for all temporal steps

							result.AddSection(section);
						}//for all replications
					}//for all parameters
				}//for all locations
			}//if msg

			result.Close();
		}//if msg


		return msg;
	}


	size_t CMapping::GetNbMapVariableIndex(const CModelOutputVariableDefVector& outputDef)const
	{
		return outputDef.size();
	}


	size_t CMapping::GetNbMapParameterIndex(const CModelInputVector& parameterSet)const
	{
		return parameterSet.size();
	}

	size_t CMapping::GetNbMapTimeIndex(CResultPtr& pResult)const
	{
		CTPeriod p = pResult->GetTPeriod();
		return p.GetNbRef();
	}

	std::string CMapping::GetTEMName(CResultPtr& pResult, size_t p, size_t r, size_t t, size_t v)const
	{
		string name = m_TEMName;
		string now = GetCurrentTimeString("%x");

		size_t nbReplications = pResult->GetMetadata().GetNbReplications();
		const CModelInputVector& parameterSet = pResult->GetMetadata().GetParameterSet();
		ASSERT(p >= 0 && p < parameterSet.size());

		const CModelOutputVariableDefVector& varArray = pResult->GetMetadata().GetOutputDefinition();
		ASSERT(v >= 0 && v < varArray.size());

		CTPeriod period = pResult->GetTPeriod();
		CTRef ref = period.Begin() + t;


		string cName = PurgeFileName(GetName());
		string vName = PurgeFileName(varArray[v].m_name);
		std::vector<size_t> pos = parameterSet.GetVariablePos();
		string pName = PurgeFileName(parameterSet[p].GetDescription(pos));
		string rName = ToString(r + 1);
		string tName = PurgeFileName(ref.GetFormatedString().c_str());
		string dName = PurgeFileName(now);


		if (name.find("%c") != string::npos)
		{
			ReplaceString(name, "%c", cName);
		}
		else
		{
			if (m_TEMName.empty())
				name = cName;
		}


		if (name.find("%v") >= 0)
		{
			ReplaceString(name, "%v", vName);
		}
		else
		{
			if (m_TEMName.empty())
				name += vName;
		}


		if (name.find("%t") >= 0)
		{
			ReplaceString(name, "%t", tName);
		}
		else
		{
			if (!tName.empty() && period.GetNbRef() > 1)
				name += tName;
		}

		if (name.find("%p") >= 0)
		{
			ReplaceString(name, "%p", pName);
		}
		else
		{
			if (!pName.empty() && parameterSet.size() > 1)
				name += pName;
		}

		if (name.find("%r") >= 0)
		{
			ReplaceString(name, "%r", rName);
		}
		else
		{
			if (!rName.empty() && nbReplications > 1)
				name += rName;
		}

		if (name.find("%d") >= 0)
		{
			ReplaceString(name, "%d", dName);
		}


		return name;
	}

	std::string CMapping::GetTEMFilePath(const CFileManager& fileManager, const std::string& TEMName)const
	{
		//string DEMName = m_DEMName;
		//string mapExt = GetFileExtension(DEMName);
		////when using GDAL, for the moment we transform flt to bil
		//MakeLower(mapExt);

		//if (mapExt == ".flt")
		//	mapExt = ".bil";

		////if( mapExt== ".aux")
		////mapExt = "";
		//if (IsEqualNoCase(Right(DEMName, 4), ".aux"))
		//	mapExt.clear();
		//else if (IsEqualNoCase(Right(DEMName, 8), ".aux.xml"))
		//	mapExt.clear();



		string mapExt = GetFileExtension(TEMName);
		if (mapExt.empty())
			mapExt = ".tif";//tif by default

		return fileManager.GetOutputMapFilePath(GetFileName(TEMName), mapExt);
	}


	ERMsg CMapping::InitGridInterpol(const CFileManager& fileManager, CResultPtr& pResult, size_t p, size_t r, size_t t, size_t v, CGridInterpol& mapInput, CCallback& callback)
	{
		ERMsg msg;

		const CLocationVector& locArray = pResult->GetMetadata().GetLocations();


		CGridPointVectorPtr pts(new CGridPointVector);
		pts->reserve(locArray.size());

		callback.PushTask("Compute event", locArray.size());

		bool bHaveData = false;
		for (size_t i = 0; i < locArray.size() && msg; i++)
		{
			CStatistic value;
			CStatistic::SetVMiss(VMISS);
			size_t nbReplication = pResult->GetMetadata().GetNbReplications();

			CNewSectionData section;
			pResult->GetSection(i, p, r, section);

			if (section[t][v][MEAN] > VMISS)
				bHaveData = true;

			CGridPoint pt(locArray[i].m_lon, locArray[i].m_lat, locArray[i].m_alt, locArray[i].GetSlope(), locArray[i].GetAspect(), section[t][v][MEAN], locArray[i].m_lat, locArray[i].GetShoreDistance(), locArray[i].GetPrjID());
			pts->push_back(pt);

			msg += callback.StepIt();
		}

		if (msg)
		{
			if (bHaveData)
				mapInput.m_pts = pts;
			else
				mapInput.m_pts.reset(new CGridPointVector);

			string TEMName = GetTEMName(pResult, p, r, t, v);


			mapInput.m_DEMFilePath = fileManager.MapInput().GetFilePath(m_DEMName);
			mapInput.m_method = m_method;
			mapInput.m_param = *m_pParam;
			mapInput.m_prePostTransfo = *m_pPrePostTransfo;
			mapInput.m_options.m_format = "GTiff";
			mapInput.m_options.m_bOverwrite = true;
			mapInput.m_options.m_outputType = 6; // GDT_Float32;
			mapInput.m_options.m_CPU = CTRL.m_nbMaxThreads;
			mapInput.m_options.m_IOCPU = min(CTRL.m_nbMaxThreads, 4);
			mapInput.m_options.m_BLOCK_THREADS = 1;// CTRL.m_nbMaxThreads;
			mapInput.m_options.m_bMulti = true;
			mapInput.m_options.m_nbBands = 1;
			mapInput.m_options.m_dstNodata = m_pParam->m_noData;

			msg = mapInput.m_options.ParseOptions(m_pParam->m_GDALOptions);

			if (msg)
			{
				string fileExt = GetFileExtension(TEMName);

				if (fileExt.empty())
					fileExt += GetDriverExtension(mapInput.m_options.m_format);
			}

			mapInput.m_TEMFilePath = GetTEMFilePath(fileManager, TEMName);

			callback.AddMessage("");
			callback.AddMessage("Input: " + mapInput.m_DEMFilePath, 1);
			callback.AddMessage("Output: " + mapInput.m_TEMFilePath, 1);
		}


		callback.PopTask();


		return msg;
	}


	int CMapping::GetNbTask()const
	{
		ASSERT(m_method >= 0 && m_method < CGridInterpol::NB_METHOD);
		ASSERT(CGridInterpol::NB_METHOD == 5);
		ASSERT(m_pParent);

		static int NB_TASK_PER_TYPE[CGridInterpol::NB_METHOD] = { 4, 3, 4, 3, 3 };
		int nbTask = NB_TASK_PER_TYPE[m_method];

		//remove one task if only Xvalidation
		if (this->m_XValOnly)
			nbTask--;

		//Get the number of variable of the parent;
		CDimension d = m_pParent->GetDimension(GetFM());
		return int(nbTask*d[VARIABLE] * d[TIME_REF] * d[PARAMETER]);
	}

	void CMapping::SetPrePostTransfo(const CPrePostTransfo & in) { *m_pPrePostTransfo = in; }

	void CMapping::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(METHOD)](m_method);
		out[GetMemberName(DEM_NAME)](m_DEMName);
		out[GetMemberName(TEM_NAME)](m_TEMName);
		out[GetMemberName(PREPOST_TRANSFO)](*m_pPrePostTransfo);
		out[GetMemberName(XVAL_ONLY)](m_XValOnly);
		out[GetMemberName(CREATE_LEGEND_ONLY)](m_createLengendOnly);
		out[GetMemberName(SI_PARAMETER)](*m_pParam);
		out[GetMemberName(QGIS_OPTIONS)](m_createStyleFile);
	}

	bool CMapping::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);


		in[GetMemberName(METHOD)](m_method);
		in[GetMemberName(DEM_NAME)](m_DEMName);
		in[GetMemberName(TEM_NAME)](m_TEMName);
		in[GetMemberName(PREPOST_TRANSFO)](*m_pPrePostTransfo);
		in[GetMemberName(XVAL_ONLY)](m_XValOnly);
		in[GetMemberName(CREATE_LEGEND_ONLY)](m_createLengendOnly);
		in[GetMemberName(SI_PARAMETER)](*m_pParam);
		in[GetMemberName(QGIS_OPTIONS)](m_createStyleFile);

		return true;
	}

}