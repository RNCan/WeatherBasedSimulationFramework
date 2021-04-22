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


#include "Basic/UtilStd.h"
#include "Basic/Timer.h"
#include "Basic/hxGrid.h"
#include "Geomatic/Projection.h"
#include "Geomatic/UniversalKriging.h"
#include "Geomatic/IWD.h"
#include "Geomatic/ThinPlateSpline.h"
#include "Geomatic\RandomForest.h"
#include "Geomatic/GridInterpol.h"
#include "Geomatic/ProjectionTransformation.h"
#include "Geomatic/GDAL.h"
#include "WeatherBasedSimulationString.h"
#include "Basic/Shore.h"
#include "Basic/OpenMP.h"


namespace WBSF
{

	static const __int8 DIRECT_ACCESS = 0;
	static const __int8 NB_THREAD_PROCESS = 1;


	static const char mutexName[] = "hxGridMutex";
	const char* CGridInterpol::METHOD_NAME[NB_METHOD] = { "Spatial Regression", "Universal Kriging", "Inverse Weighted Distance", "Thin Plate Spline", "RandomForest" };

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	CGridInterpol::CGridInterpol()
	{
		Reset();
	}

	CGridInterpol::~CGridInterpol()
	{
		Reset();
	}

	void CGridInterpol::Reset()
	{
		m_method = INVERT_WEIGHTED_DISTANCE;
		m_pts.reset();
		m_DEMFilePath.clear();
		m_TEMFilePath.clear();
		m_bUseHxGrid = false;

		if (m_inputGrid.IsOpen())
			m_inputGrid.Close(m_options);
		if (m_outputGrid.IsOpen())
			m_outputGrid.Close(m_options);
	}

	CGridInterpol& CGridInterpol::operator =(const CGridInterpol& in)
	{
		if (this != &in)
		{
			Reset();

			m_method = in.m_method;
			m_pts = in.m_pts;

			m_DEMFilePath = in.m_DEMFilePath;
			m_TEMFilePath = in.m_TEMFilePath;
			m_param = in.m_param;
			m_prePostTransfo = in.m_prePostTransfo;
			m_bUseHxGrid = in.m_bUseHxGrid;
		}

		return *this;
	}

	CGridInterpolBasePtr CGridInterpol::CreateNewGridInterpol()const
	{
		CGridInterpolBasePtr pNewGridInterpol;

		switch (m_method)
		{
		case SPATIAL_REGRESSION:pNewGridInterpol.reset(new CSpatialRegression); break;
		case UNIVERSAL_KRIGING:pNewGridInterpol.reset(new CUniversalKriging); break;
		case INVERT_WEIGHTED_DISTANCE:pNewGridInterpol.reset(new CIWD); break;
		case THIN_PLATE_SPLINE: pNewGridInterpol.reset(new CThinPlateSpline); break;
		case RANDOM_FOREST: pNewGridInterpol.reset(new CRandomForest); break;
		default: _ASSERTE(false);
		}

		if (pNewGridInterpol.get())
		{
			ASSERT(m_inputGrid.IsOpen());

			CGridInterpolInfo info;
			info.m_cellSizeX = m_inputGrid.GetExtents().XRes();//m_inputGrid->GetRasterXSize();
			info.m_cellSizeY = m_inputGrid.GetExtents().YRes();//m_inputGrid->GetRasterYSize();
			info.m_noData = m_inputGrid.GetNoData(0);
			info.m_bMulti = m_options.m_bMulti;
			info.m_nbCPU = m_options.m_CPU;

			pNewGridInterpol->SetInfo(info);
			pNewGridInterpol->SetParam(m_param);
			pNewGridInterpol->SetPrePostTransfo(m_prePostTransfo);
			pNewGridInterpol->SetDataset(m_pts);
		}

		return pNewGridInterpol;
	}


	ERMsg CGridInterpol::Initialise(CCallback& callback)
	{
		ASSERT(!m_inputGrid.IsOpen());

		ERMsg msg;

		msg += m_inputGrid.OpenInputImage(m_DEMFilePath);

		if (!msg)
			return msg;

		CProjectionPtr pPrj = m_inputGrid.GetPrj();
		if (pPrj->IsProjected())
		{
			CProjectionTransformation PT(m_pts->GetPrjID(), pPrj->GetPrjID());
			m_pts->Reproject(PT);
		}

		//eliminate vmiss, outside and duplicate(kriging) points
		if (msg)
			msg = TrimDataset(callback);


		if (m_prePostTransfo.HaveTransformation())
		{
			string tmp = FormatMsg(IDS_MAP_PREPOST_TRANSFO, m_prePostTransfo.GetDescription());
			callback.AddMessage(tmp);
		}

		if (msg)
		{
			m_pGridInterpol = CreateNewGridInterpol();
		}

		return msg;
	}

	void CGridInterpol::Finalize()
	{
		m_inputGrid.Close(m_options);
		if (m_pGridInterpol)
			m_pGridInterpol->Cleanup();

		Reset();

		//CGridInterpolBase::FreeMemoryCache();
		//CUniversalKriging::FreeMemoryCache();
		//annClose();
	}


	ERMsg CGridInterpol::CreateSurface(CCallback& callback)
	{
		_ASSERTE(m_pGridInterpol.get());//call initialize first
		_ASSERTE(m_inputGrid.IsOpen());
		_ASSERTE(!m_outputGrid.IsOpen());

		ERMsg msg;

		m_inputGrid.UpdateOption(m_options);
		m_options.m_dstNodata = m_param.m_noData;
		m_options.m_outputType = GDT_Float32;
		if (m_options.m_extents.m_yBlockSize > 0 && (m_options.m_extents.m_yBlockSize % 256) == 0)
		{
			//input is tiled, so output will be tiled
			//add tiled
			m_options.m_createOptions.push_back("TILED=YES");
			m_options.m_createOptions.push_back("BLOCKXSIZE=" + to_string(m_options.m_extents.m_yBlockSize));//use y block to avoid problem (256x)
			m_options.m_createOptions.push_back("BLOCKYSIZE=" + to_string(m_options.m_extents.m_yBlockSize));
		}

		msg = m_outputGrid.CreateImage(m_TEMFilePath, m_options);
		if (!msg)
			return msg;

		msg = GenerateSurface(callback); //surface-generation step
		m_outputGrid.Close(m_options);

		return msg;
	}

	ERMsg CGridInterpol::TrimDataset(CCallback& callback)
	{
		_ASSERTE(m_inputGrid.IsOpen());


		ERMsg msg;

		if (m_pts->empty())
		{
			msg.ajoute(GetString(IDS_MAP_NORESULT));
			return msg;
		}

		string comment = FormatMsg(IDS_MAP_TRIM_DATA, GetFileTitle(m_TEMFilePath), GetFileTitle(m_DEMFilePath));

		callback.PushTask(comment, m_pts->size());
		//callback.SetNbStep(m_pts->size());


		bool bAccepDuplicate = (m_method != UNIVERSAL_KRIGING && m_method != THIN_PLATE_SPLINE);
		bool bAccepOutside = true;

		int nAtVMISS = 0;
		int nNotInBound = 0;
		int nDuplicate = 0;

		//int newobs = 0;

		//add a buffer around the map to avoid side effects
		// add 50 km if projected or 0.4 degree if not
		double bufferSize = m_inputGrid.GetPrj()->IsProjected() ? 50000 : 0.4;

		CGeoRect rect = m_inputGrid.GetExtents();
		rect.InflateRect(bufferSize, bufferSize, bufferSize, bufferSize);

		size_t oldNbObs = m_pts->size();
		for (CGridPointVector::reverse_iterator it = m_pts->rbegin(); it != m_pts->rend() && msg;)//; it++
		{
			if (it->m_event > VMISS)
			{
				if (rect.PtInRect(*it) || bAccepOutside)
				{
					bool bNotIn = true;
					if (!bAccepDuplicate)
					{
						int d = (int)std::distance(it, m_pts->rend());
#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti)
						for (int j = 1; j < d; j++)
						{
							double dist = it->GetDistance(*(it + j));
							if (dist < 0.001)
								bNotIn = false;
						}
					}

					if (bNotIn)
					{
						it++;//go to next element
					}
					else
					{
						m_trimPosition.push_back(m_pts->size() - std::distance(m_pts->rbegin(), it) - 1);
						it = CGridPointVector::reverse_iterator(m_pts->erase((++it).base()));
						nDuplicate++;
					}
				}
				else
				{
					m_trimPosition.push_back(m_pts->size() - std::distance(m_pts->rbegin(), it) - 1);
					it = CGridPointVector::reverse_iterator(m_pts->erase((++it).base()));
					nNotInBound++;
				}
			}
			else
			{
				m_trimPosition.push_back(m_pts->size() - std::distance(m_pts->rbegin(), it) - 1);
				it = CGridPointVector::reverse_iterator(m_pts->erase((++it).base()));
				nAtVMISS++;
			}

			msg += callback.StepIt();
		}

		size_t newObs = m_pts->size();
		if ((nAtVMISS != 0 || nNotInBound != 0 || nDuplicate != 0))
		{
			string comment = FormatMsg(IDS_MAP_STATION_ELIMINATE, ToString(oldNbObs).c_str(), ToString(newObs).c_str());
			if (newObs == 0)
			{
				msg.ajoute(comment);
				comment = FormatMsg(IDS_MAP_PREPOST_TRANSFO, m_prePostTransfo.GetDescription());
				msg.ajoute(comment);
			}
			else callback.AddMessage(comment);

			comment = FormatMsg(IDS_MAP_ELIMINATE_RAISON, ToString(nNotInBound), ToString(nAtVMISS), ToString(nDuplicate));
			if (newObs == 0)
				msg.ajoute(comment);
			else callback.AddMessage(comment);
		}
		else
		{
			callback.AddMessage("Samples size = " + ToString(newObs));
		}

		if (newObs < m_param.m_nbPoints && m_method != SPATIAL_REGRESSION)
		{
			string  error = FormatMsg(IDS_MAP_NOT_ENOUGH_POINT, ToString(newObs), ToString(m_param.m_nbPoints));
			msg.ajoute(error);
		}


		callback.PopTask();

		return msg;
	}

	ERMsg CGridInterpol::GenerateSurface(CCallback& callback)
	{
		_ASSERTE(m_pGridInterpol.get());

		ERMsg msg;

		string comment = FormatMsg(IDS_MAP_DOMAP, GetFileTitle(m_TEMFilePath), GetFileTitle(m_DEMFilePath));
		callback.PushTask(comment, size_t(m_inputGrid->GetRasterXSize())*m_inputGrid->GetRasterYSize());

		CTimer timer;

		timer.Start();

		if (m_bUseHxGrid)
		{
			msg = RunHxGridInterpolation(callback);
		}
		else //not using HxGrid
		{
			msg = RunInterpolation(callback);
		}


		timer.Stop();

		callback.AddMessage("Interpolation = " + SecondToDHMS(timer.Elapsed()) + " s");

		if (msg)
		{
			string comment = FormatMsg(IDS_MAP_GENERATE_SUCCESS, m_TEMFilePath);
			callback.AddMessage(comment);
		}


		callback.PopTask();

		return msg;
	}


	ERMsg CGridInterpol::OptimizeParameter(CCallback& callback)
	{
		ERMsg msg;



		CGridInterpolParamVector parameterset;
		m_pGridInterpol->GetParamterset(parameterset);

		if (parameterset.size() > 1)
		{
			string comment = GetString(IDS_MAP_OPTIMISATION);

			callback.PushTask(comment, parameterset.size());


			CTimer timer;
			timer.Start();

			std::vector<double> optimisationR²;

			if (m_bUseHxGrid)
			{
				msg = RunHxGridOptimisation(parameterset, optimisationR², callback);
			}
			else
			{
				optimisationR².resize(parameterset.size(), 0);


				for (size_t i = 0; i < parameterset.size() && msg; i++)
				{
					//initialize with this parameters set
					m_pGridInterpol->SetParam(parameterset[i]);
					msg = m_pGridInterpol->Initialization(callback);
					if (msg)
					{
						optimisationR²[i] = m_pGridInterpol->GetOptimizedR²(callback);

						if (m_param.m_bOutputVariogramInfo)
						{
							CVariogram variogram;

							if (m_pGridInterpol->GetVariogram(variogram))
							{
								if (i == 0)
									callback.AddMessage("nbLags\tLadDist\tType\tNugget\tSill\tRange\tVariogram R²\tMixed R²");

								string tmp = FormatA("%4d\t%8.3lf\t%16.16s\t% 7.4lf\t% 7.4lf\t% 7.4lf\t% 7.4lf\t% 7.4lf", parameterset[i].m_nbLags, parameterset[i].m_lagDist, variogram.GetModelName(), variogram.GetNugget(), variogram.GetSill(), variogram.GetRange(), max(-9.999, variogram.GetR2()), max(-9.999, optimisationR²[i]));
								callback.AddMessage(tmp);
							}
						}
					}

					msg += callback.StepIt();
				}
			}

			timer.Stop();

			if (msg)
			{
				_ASSERTE(optimisationR².size() == parameterset.size());

				//find the best R² of correlation
				double bestR² = -999;
				int bestIndex = 0;
				for (size_t i = 0; i < optimisationR².size(); i++)
				{
					if (optimisationR²[i] > bestR²)
					{
						bestIndex = (int)i;
						bestR² = optimisationR²[i];
					}
				}

				m_param = parameterset[bestIndex];

				callback.AddMessage("Optimization = " + SecondToDHMS(timer.Elapsed()) + " s");
				callback.AddMessage(m_pGridInterpol->GetFeedbackOnOptimisation(parameterset, optimisationR²));


			}

			callback.PopTask();
		}//else, if we have only one parameter set, then we take it directly
		else if (parameterset.size() == 1)
		{
			m_param = parameterset[0];
		}


		if (msg)
		{
			//if we have transformation, we have to change the noData values
			m_pGridInterpol->SetParam(m_param);

			//Initialise with good parameter
			msg = m_pGridInterpol->Initialization(callback);
			if (msg)
			{
				//save variogram
				if (m_method== CGridInterpol::UNIVERSAL_KRIGING&&m_param.m_bOutputVariogramInfo)
				{
					string filePath = GetPath(m_TEMFilePath) + GetFileTitle(m_TEMFilePath) + "_variogram.csv";
					CVariogram variogram;
					if (m_pGridInterpol->GetVariogram(variogram))
						msg += variogram.Save(filePath);
				}

				//compute calibration X-Validation
				CXValidationVector m_calibration;
				m_pGridInterpol->GetXValidation(CGridInterpolParam::O_CALIBRATION, m_calibration, callback);


				CStatisticXY stat = m_calibration.GetStatistic(m_param.m_noData);

				string comment = FormatMsg(IDS_MAP_METHOD_CHOOSE, GetMethodName(), ToString(stat[NB_VALUE]), ToString(stat[COEF_D], 4));
				callback.AddMessage(comment);
				callback.AddMessage(m_pGridInterpol->GetFeedbackBestParam());


				//Compute validation
				CXValidationVector m_validation;
				m_pGridInterpol->GetXValidation(CGridInterpolParam::O_VALIDATION, m_validation, callback);

				stat = m_validation.GetStatistic(m_param.m_noData);
				comment = FormatMsg(IDS_MAP_VALIDATION_MSG, ToString(stat[NB_VALUE]), ToString(stat[COEF_D], 4));
				callback.AddMessage(comment);



				//compute output
				m_pGridInterpol->GetXValidation((CGridInterpolParam::TOutputType)m_param.m_outputType, m_output, callback);
				//replace noData by VMISS
				for (CXValidationVector::iterator it = m_output.begin(); it != m_output.end(); it++)
				{
					if (fabs(it->m_observed - m_param.m_noData) < 0.1)
						it->m_observed = VMISS;

					if (fabs(it->m_predicted - m_param.m_noData) < 0.1)
						it->m_predicted = VMISS;
				}

				//push back removed point with VMISS data
				if (!m_trimPosition.empty())
				{
					CXvalTuple empty(VMISS, VMISS);

					for (vector<size_t>::const_reverse_iterator p = m_trimPosition.rbegin(); p != m_trimPosition.rend(); p++)
					{
						m_output.insert(m_output.begin() + *p, empty);
					}
				}

			}//if init
		}

		return msg;
	}


	ERMsg CGridInterpol::RunInterpolation(CCallback& callback)
	{
		ERMsg msg;

		//m_options.m_BLOCK_THREADS = 2;


		//Load band holders
		CBandsHolderMT bandHolder(3, m_options.m_memoryLimit, 1/*m_options.m_IOCPU*/, 1/*m_options.m_BLOCK_THREADS*/);
		msg += bandHolder.Load(m_inputGrid, m_options.m_bQuiet, m_options.m_extents);

		if (!msg)
			return msg;


		//get projection
		vector< CProjectionTransformation> PT(m_options.m_CPU);
		for (size_t i = 0; i < PT.size(); i++)
			PT[i] = CProjectionTransformation(m_inputGrid.GetPrj(), CProjectionManager::GetPrj(PRJ_WGS_84));
		//m_PT = GetReProjection(pPts->GetPrjID(), PRJ_WGS_84);


		bool bHaveExposition = m_pts->HaveExposition();
		bool bUseShore = m_param.m_bUseShore;
		//get extents and compute blocks index
		CGeoExtents extents = m_options.GetExtents();
		vector<pair<int, int>> XYindex = extents.GetBlockList();




		//run over all blocks
		//omp_set_nested(1);//for IOCPU
//#pragma omp parallel for num_threads( m_options.m_BLOCK_THREADS ) if (m_options.m_bMulti )
		for (__int64 xy = 0; xy < (__int64)XYindex.size() && msg; xy++)//for all blocks
		{
			//#pragma omp flush(msg)
						//if (msg && !callback.GetUserCancel())
						//{
			int xBlock = XYindex[xy].first;
			int yBlock = XYindex[xy].second;
			CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
			

#pragma omp critical(GDAL_READ)
			bandHolder[0].LoadBlock(blockExtents);


			vector<CDataWindowPtr> input = bandHolder[0].GetWindow();

			//execute over all pixels of the blocks
			vector<float> output(blockExtents.m_ySize*blockExtents.m_xSize);
			
#pragma omp parallel for /*schedule(static, 1)*/ num_threads( m_options.m_CPU ) if (m_options.m_bMulti)
			for (int y = 0; y < blockExtents.m_ySize/*&&msg && !callback.GetUserCancel()*/; y++)
			{
				size_t no = omp_get_thread_num();
#pragma omp flush(msg)
				if (msg)
				{
					for (int x = 0; x < blockExtents.m_xSize&&msg && !callback.GetUserCancel(); x++)
					{
						int pos_xy = y * blockExtents.m_xSize + x;
						if (input[0]->IsValid(x, y))
						{
							CGridPoint pt;
							((CGeoPoint&)pt) = blockExtents.XYPosToCoord(CGeoPointIndex(x, y));
							pt.m_z = input[0]->at(x, y);

							if (bHaveExposition)
							{
								input[0]->GetSlopeAndAspect(x, y, pt.m_slope, pt.m_aspect);

								if (IsGeographic(pt.GetPrjID()))
								{
									pt.m_latitudeDeg = pt.m_y;
								}
								else
								{
									CGeoPoint ptGeo(pt);
									ptGeo.Reproject(PT[no]);
									pt.m_latitudeDeg = ptGeo.m_y;
								}
							}
							if (bUseShore)
							{
								if (IsGeographic(pt.GetPrjID()))
								{
									pt.m_shore = CShore::GetShoreDistance(pt);
								}
								else
								{
									CGeoPoint ptGeo(pt);
									ptGeo.Reproject(PT[no]);
									pt.m_shore = CShore::GetShoreDistance(ptGeo);
								}
							}

							pt.m_event = m_pGridInterpol->Evaluate(pt);
							output[pos_xy] = (float)m_outputGrid.PostTreatment(pt.m_event);
						}
						else
						{
							output[pos_xy] = (float)m_param.m_noData;
						}

#pragma omp atomic 
						m_options.m_xx++;
					}//x

					if (msg)
					{
						if (omp_get_thread_num() == 0)//this line is essential to avoid very slow performence. Stanges!
							msg += callback.SetCurrentStepPos(m_options.m_xx);
#pragma omp flush(msg)
					}
				}//if msg
			}//y


			CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
			GDALRasterBand *pBand = m_outputGrid.GetRasterBand(0);
#pragma omp critical(GDAL_WRITE)
			pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
			//pBand->FlushBlock(xBlock, yBlock);
			//}// if msg
		}//for all blocks




		return msg;
	}



	//******************************************************************************
	//HxGrid general section
	//===============================================================
	//===============================================================
	//***************************************************************
	class TGridInterpolSessionData
	{
	public:

		TGridInterpolSessionData()
		{
			m_pGlobalDataStream = NULL;
			Reset();
		}

		~TGridInterpolSessionData()
		{
			Reset();
		}

		void Reset()
		{
			m_pGridInterpol = NULL;
			m_pOutputGrid = NULL;

			m_msg = ERMsg::OK;		//for error
			m_nbTaskCompleted = 0;//for progression

			if (m_pGlobalDataStream)
			{
				ULONG test = m_pGlobalDataStream->AddRef();
				test = m_pGlobalDataStream->Release();

				m_pGlobalDataStream->Release();
				m_pGlobalDataStream = NULL;
			}

			m_optimisationR².clear();
		}

		CGridInterpolBase* m_pGridInterpol;

		ERMsg m_msg;		//for error
		size_t m_nbTaskCompleted;//for progression
		IGenericStream* m_pGlobalDataStream;
		CCriticalSection m_CS;

		//part for optimisation
		std::vector<double> m_optimisationR²;//optimisation R²

		//part for interpolation
		CGDALDatasetEx* m_pOutputGrid;//output grid
	};


	//the current session
	static TGridInterpolSessionData gSession;


	IGenericStream* CreateGlobalData(const CGridInterpol& gridInterpol)
	{

		IGenericStream* pGlobalDataStream = new TGenericStream(100000);//CreateGenericStream();

		std::ostringstream io((char*)pGlobalDataStream->GetCurPointer());
		io << gridInterpol.m_method;

		gridInterpol.GetGridInterpol().WriteStream(io);

		return pGlobalDataStream;
	}

	void __cdecl GetDataCallback(const char* dataDesc, IGenericStream** stream)
	{
		ASSERT(strcmp(dataDesc, CGridInterpolBase::DATA_DESCRIPTOR) == 0);

		(*stream) = new TGenericStream(gSession.m_pGlobalDataStream->GetLength());
		(*stream)->Write(gSession.m_pGlobalDataStream->GetBasePointer(), gSession.m_pGlobalDataStream->GetLength());
	}

	//******************************************************************************
	//HxGrid optimisation section
	//===============================================================
	//===============================================================

	void __cdecl FinalizeOptimisation(IGenericStream* outStream)
	{
		ASSERT(outStream->GetPos() == 0);

		DWORD sessionId = 0;
		outStream->Read(&sessionId, sizeof(sessionId));

		try
		{
			ASSERT(gSession.m_pGridInterpol);

			__int32 parameterIndex = 0;
			outStream->Read(&parameterIndex, sizeof(parameterIndex));

			double R² = 0;
			outStream->Read(&R², sizeof(R²));

			//because gSession is a share composant, we have to protect it
			gSession.m_CS.Enter();
			gSession.m_optimisationR²[parameterIndex] = R²;
			gSession.m_nbTaskCompleted++;
			gSession.m_CS.Leave();

		}
		catch (...)
		{
			ASSERT(false);
			//oups...
		}
	}

	IGenericStream* CreateOptimisationStream(DWORD sessionId, __int32 i, const CGridInterpolParam& param)
	{
		//Get info stream in XML
		//ostringstream stream;
		//CCommunicationStream::WriteInputStream(info, weather, stream);
		IGenericStream* pStream = new TGenericStream(0);//CreateGenericStream();

		zen::XmlDoc doc;
		doc.setEncoding("Windows-1252");
		writeStruc(param, doc.root());

		std::string str = serialize(doc);

		pStream->Write(&sessionId, sizeof(sessionId));
		pStream->Write(&DIRECT_ACCESS, sizeof(DIRECT_ACCESS));
		pStream->Write(&i, sizeof(i));


		__int32 infoSize = (__int32)str.size();
		ASSERT(str.size() == infoSize);

		ASSERT(infoSize == (DWORD)infoSize);
		pStream->Write(&infoSize, sizeof(infoSize));
		pStream->Write(str.c_str(), (DWORD)infoSize);


		//XNode::DeleteNode(pRoot);

		return pStream;
	}


	ERMsg CGridInterpol::RunHxGridOptimisation(const CGridInterpolParamVector& parameterset, std::vector<double>& optimisationR², CCallback& callback)
	{
		ERMsg msg;


		CMultiAppSync appSync;
		if (!appSync.Enter(mutexName))
		{
			string error = GetString(IDS_CMN_HXGRID_ALREADY_USED);
			msg.ajoute(error);
			return msg;
		}

		//CoFreeUnusedLibrariesEx(0,0);
		CoFreeUnusedLibraries();
		//Initialise critical section for writing with HxGrid
		//InitializeCriticalSection(&CS);


		//Get a random number as ID, then, if more tnah one application call the same DLL
		//Initialisation in DLL will be made correcly
		//I don't know if it's a glitch oif it usefull
		//What's appen when 2 compute run the same programm at the same time???
		DWORD sessionId = Rand();

		//resize for result
		gSession.Reset();
		gSession.m_pGridInterpol = m_pGridInterpol.get();
		gSession.m_pGlobalDataStream = CreateGlobalData(*this);
		gSession.m_optimisationR².resize(parameterset.size(), 0);


		if (DIRECT_ACCESS)
		{
			//CoFreeUnusedLibraries();
			gSession.m_pGlobalDataStream->Seek(0);

			wstring str = UTF16(GetApplicationPath()) + L"GridInterpolTask.dll";
			HINSTANCE hDLL = LoadLibraryW(str.c_str());
			ASSERT(hDLL);


			typedef bool(__cdecl *TRunTaskProc)(IAgent* agent, DWORD sessionId, IGenericStream* inStream, IGenericStream* outStream);
			typedef void(__cdecl *TEndSession)(IAgent* agent, DWORD sessionId);
			TRunTaskProc RunTaskXValidation = (TRunTaskProc)GetProcAddress(hDLL, "RunTaskXValidation");
			TEndSession EnsSession = (TEndSession)GetProcAddress(hDLL, "EndSession");

			ASSERT(RunTaskXValidation);
#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti)
			for (int i = 0; i < (int)parameterset.size(); i++)
			{
				if (msg)
				{
					IGenericStream* inStream = CreateOptimisationStream(sessionId, i, parameterset[i]);
					inStream->Seek(0);
					//intialize with this parameters set

					IGenericStream* outStream = new TGenericStream(0);//CreateGenericStream();
					if (RunTaskXValidation((IAgent*)gSession.m_pGlobalDataStream, sessionId, inStream, outStream))
					{

						outStream->Seek(0);
						FinalizeOptimisation(outStream);
					}
					inStream->Release();
					outStream->Release();
					callback.SetCurrentStepPos(gSession.m_nbTaskCompleted);
					msg += callback.StepIt(0);
#pragma omp flush(msg)
				}
			}

			EnsSession(NULL, sessionId);

			BOOL bFree = ::FreeLibrary(hDLL);
			ASSERT(bFree);
		}
		else
		{
			IGridUser* pGridUser = CreateGridUserObject(IGridUser::VERSION);
			if (pGridUser == NULL)
			{
				string error = GetString(IDS_CMN_UNABLE_CREATE_HXGRID);
				msg.ajoute(error);
				return msg;
			}

			DWORD refCount = pGridUser->AddRef();
			refCount = pGridUser->Release();

			pGridUser->CompressStream(gSession.m_pGlobalDataStream);
			pGridUser->BindGetDataCallback(GetDataCallback);

			int loop = 0;
			for (size_t i = 0; i < parameterset.size(); i++)
			{
				IGenericStream* inStream = CreateOptimisationStream(sessionId, (__int32)i, parameterset[i]);
				//intialize with this parameters set

				DWORD notUsedID = 0;
				string DLLName = GetApplicationPath() + "GridInterpolTask.dll";// +"," + "Powell.dll";//don't put full path. Doesn't works
				while (msg && pGridUser->RunTask(DLLName.c_str(), "RunTaskXValidation", inStream, FinalizeOptimisation, &notUsedID, false) != S_OK)
				{
					//we can't add more task to the queue, then we wait
					callback.SetCurrentStepPos(gSession.m_nbTaskCompleted);
					msg += callback.StepIt(0);
					msg += gSession.m_msg;

					pGridUser->WaitForCompletionEvent(100);
					msg += GetConnectionStatus(pGridUser, loop);
				}


				callback.SetCurrentStepPos(gSession.m_nbTaskCompleted);
				msg += callback.StepIt(0);

			}


			bool bComplet = false;
			while (msg && !bComplet)
			{
				pGridUser->IsComplete(&bComplet);

				callback.SetCurrentStepPos(gSession.m_nbTaskCompleted);
				msg += callback.StepIt(0);
				msg += gSession.m_msg;

				Sleep(100);
				msg += GetConnectionStatus(pGridUser, loop);
			}

			if (!msg)
				pGridUser->CancelTasks();


			refCount = pGridUser->AddRef();
			refCount = pGridUser->Release();

			pGridUser->Release();
			FreeGridUserLibrary();

			//#endif			
		}

		//transfer optimized value to output vector
		optimisationR² = gSession.m_optimisationR²;


		//delete critical section
		//DeleteCriticalSection(&CS);

		return msg;
	}



	//**********************************************************************************
	//HxGrid interpolation section
	//===============================================================
	//===============================================================
	void __cdecl FinalizeInterpolation(IGenericStream* outStream)
	{
		ASSERT(outStream->GetPos() == 0);
		ASSERT(gSession.m_pOutputGrid);

		try
		{
			DWORD sessionId = 0;
			outStream->Read(&sessionId, sizeof(sessionId));

			ASSERT(gSession.m_pGridInterpol != NULL);

			__int32 xIndex = 0;
			__int32 yIndex = 0;
			__int32 width = 0;
			__int32 height = 0;
			outStream->Read(&xIndex, sizeof(xIndex));
			outStream->Read(&width, sizeof(width));
			outStream->Read(&yIndex, sizeof(yIndex));
			outStream->Read(&height, sizeof(height));


			unsigned __int64 size = 0;
			outStream->Read(&size, sizeof(size));
			ASSERT(width*height == size);
			CGridLine lineOut(size);

			outStream->Read(lineOut.data(), (DWORD)size * sizeof(CGridLine::value_type));
			//ReadStream(outStream, lineOut );

			if (gSession.m_pGridInterpol->GetPrePostTransfo().HaveTransformation())
			{
				for (size_t j = 0; j < lineOut.size(); j++)
					lineOut[j] = (float)gSession.m_pGridInterpol->GetPrePostTransfo().InvertTransform(lineOut[j], gSession.m_pGridInterpol->GetParam().m_noData);
			}

			gSession.m_CS.Enter();
			//gSession.m_pOutputGrid->WriteLine(lineOut, lineIndex);
			//CGeoExtents extents = gSession.m_pOutputGrid->GetExtents();
			//CGeoRectIndex outputRect = extents.GetPosRect();
			GDALRasterBand *pBand = gSession.m_pOutputGrid->GetRasterBand(0);
			pBand->RasterIO(GF_Write, xIndex, yIndex, width, height, &(lineOut[0]), width, height, GDT_Float32, 0, 0);

			gSession.m_nbTaskCompleted++;
			gSession.m_CS.Leave();



		}
		catch (...)
		{
			ASSERT(false);
			//oups...
		}


	}

	IGenericStream* CreateInterpolationStream(DWORD sessionId, __int32 xIndex, __int32 width, __int32 yIndex, __int32 height, const CGridPointVector& block)
	{
		//Get info stream in XML
		//ostringstream stream;
		//CCommunicationStream::WriteInputStream(info, weather, stream);
		IGenericStream* pStream = new TGenericStream(0);//CreateGenericStream();

		pStream->Write(&sessionId, sizeof(sessionId));
		pStream->Write(&DIRECT_ACCESS, sizeof(DIRECT_ACCESS));

		pStream->Write(&xIndex, sizeof(xIndex));
		pStream->Write(&width, sizeof(width));
		pStream->Write(&yIndex, sizeof(yIndex));
		pStream->Write(&height, sizeof(height));

		//pStream->Write(&i, sizeof(i));

		//WriteStream(pStream, line);
		unsigned __int64 size = (unsigned __int64)block.size();
		pStream->Write(&size, sizeof(size));
		pStream->Write(block.data(), (DWORD)block.size() * sizeof(CGridPointVector::value_type));


		return pStream;
	}



	void CGridInterpol::ReadBlock(CBandsHolderMT& bandHolder, int xBlock, int yBlock, const CProjectionTransformation& PT, CGridPointVector& block)
	{
		//CProjection const & prj = m_inputGrid.GetPrj();
		//CProjectionTransformation PT(prj, CProjectionManager::GetPrj(PRJ_WGS_84) );

		const CGeoExtents& blockExtents = bandHolder.GetExtents();
		bandHolder.LoadBlock(blockExtents);

		vector<CDataWindowPtr> input;
		bandHolder.GetWindow(input);

		for (int y = 0; y < blockExtents.m_ySize; y++)
		{
			for (int x = 0; x < blockExtents.m_xSize; x++)
			{
				int pos_xy = y * blockExtents.m_xSize + x;
				if (input[0]->IsValid(x, y))
				{
					CGridPoint pt;
					((CGeoPoint&)pt) = blockExtents.XYPosToCoord(CGeoPointIndex(x, y));
					pt.m_z = input[0]->at(x, y);
					if (m_pts->HaveExposition())
						input[0]->GetSlopeAndAspect(x, y, pt.m_slope, pt.m_aspect);

					if (!IsGeographic(m_pts->GetPrjID()))
					{
						CGeoPoint ptGeo(pt);
						ptGeo.Reproject(PT);
						pt.m_latitudeDeg = ptGeo.m_y;
					}
				}
			}
		}
	}


	ERMsg CGridInterpol::RunHxGridInterpolation(CCallback& callback)
	{
		ERMsg msg;


		CMultiAppSync appSync;
		if (!appSync.Enter(mutexName))
		{
			string error = GetString(IDS_CMN_HXGRID_ALREADY_USED);
			msg.ajoute(error);
			return msg;
		}



		CBandsHolderMT bandHolder(3, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
		msg += bandHolder.Load(m_inputGrid, m_options.m_bQuiet, m_options.m_extents);

		if (!msg)
			return msg;

		CProjectionTransformation PT(m_inputGrid.GetPrj(), CProjectionManager::GetPrj(PRJ_WGS_84));

		CGeoExtents inputExtents = m_inputGrid.GetExtents();
		CGeoExtents extents = m_options.GetExtents();
		vector<pair<int, int>> XYindex = extents.GetBlockList();


		CoFreeUnusedLibraries();
		DWORD sessionId = Rand();


		//We have to recreate global data even if they have already created to take care of new parameters
		gSession.Reset();
		gSession.m_pGridInterpol = m_pGridInterpol.get();
		gSession.m_pOutputGrid = &m_outputGrid;
		gSession.m_pGlobalDataStream = CreateGlobalData(*this);



		if (DIRECT_ACCESS)
		{
			//CoFreeUnusedLibraries();
			gSession.m_pGlobalDataStream->Seek(0);

			string str = GetApplicationPath() + "GridInterpolTask.dll";
			HINSTANCE hDLL = LoadLibraryA(str.c_str());
			ASSERT(hDLL);

			typedef bool(__cdecl *TRunTaskProc)(IAgent* agent, DWORD sessionId, IGenericStream* inStream, IGenericStream* outStream);
			typedef void(__cdecl *TEndSession)(IAgent* agent, DWORD sessionId);
			TRunTaskProc RunTaskInterpolation = (TRunTaskProc)GetProcAddress(hDLL, "RunTaskInterpolation");
			TEndSession EnsSession = (TEndSession)GetProcAddress(hDLL, "EndSession");

			ASSERT(RunTaskInterpolation);



#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti)
			for (int xy = 0; xy < (int)XYindex.size(); xy++)//for all blocks
			{
#pragma omp flush(msg)
				if (msg)
				{
					int xBlock = XYindex[xy].first;
					int yBlock = XYindex[xy].second;

					CGridPointVector blockIn;
					ReadBlock(bandHolder, xBlock, yBlock, PT, blockIn);

					CGeoRectIndex rect = extents.GetBlockRect(xBlock, yBlock);
					IGenericStream* inStream = CreateInterpolationStream(sessionId, rect.m_x, rect.Width(), rect.m_y, rect.Height(), blockIn);
					inStream->Seek(0);

					IGenericStream* outStream = new TGenericStream(0);
					if (RunTaskInterpolation((IAgent*)gSession.m_pGlobalDataStream, sessionId, inStream, outStream))
					{
						outStream->Seek(0);
						FinalizeInterpolation(outStream);
					}

					inStream->Release();
					outStream->Release();


#pragma omp flush(msg)
					if (msg)
					{
						msg += callback.SetCurrentStepPos(gSession.m_nbTaskCompleted);
#pragma omp flush(msg)
					}
				}
			}

			EnsSession(NULL, sessionId);

			BOOL bFree = ::FreeLibrary(hDLL);
			ASSERT(bFree);
		}
		else
		{
			IGridUser* pGridUser = CreateGridUserObject(IGridUser::VERSION);

			if (pGridUser == NULL)
			{
				msg.ajoute(GetString(IDS_CMN_UNABLE_CREATE_HXGRID));
				return msg;
			}

			DWORD refCount = pGridUser->AddRef();
			refCount = pGridUser->Release();



			pGridUser->CompressStream(gSession.m_pGlobalDataStream);
			pGridUser->BindGetDataCallback(GetDataCallback);


			int loop = 0;//verify the connection with server
			CGridPointVector blockIn;

			for (int xy = 0; xy < (int)XYindex.size() && msg; xy++)//for all blocks
			{
				int xBlock = XYindex[xy].first;
				int yBlock = XYindex[xy].second;


				//		timerRead.Start();

				ReadBlock(bandHolder, xBlock, yBlock, PT, blockIn);

				//apply transformation if any
				//if( m_prePostTransfo.HaveTransformation() )
				//{
				//	for(int j=0; j<blockIn.size(); j++)
				//		blockIn[j].m_event = m_prePostTransfo.Transform(blockIn[j].m_event, m_options.m_srcNodata);
				//}

				CGeoRectIndex rect = extents.GetBlockRect(xBlock, yBlock);
				IGenericStream* inStream = CreateInterpolationStream(sessionId, rect.m_x, rect.Width(), rect.m_y, rect.Height(), blockIn);

				//	timerRead.Stop();

				DWORD unusedID = 0;
				string DLLName = GetApplicationPath() + "GridInterpolTask.dll";
				while (msg && pGridUser->RunTask(DLLName.c_str(), "RunTaskInterpolation", inStream, FinalizeInterpolation, &unusedID, false) != S_OK)
				{
					//we can't add more task to the queue, then we wait
					msg += callback.SetCurrentStepPos(gSession.m_nbTaskCompleted);

					if (!gSession.m_msg)
						msg += gSession.m_msg;

					pGridUser->WaitForCompletionEvent(100);
					msg += GetConnectionStatus(pGridUser, loop);
				}

				if (msg)
				{
					msg += callback.SetCurrentStepPos(gSession.m_nbTaskCompleted);
				}
			}


			//wait until the end of completion
			bool bComplet = false;
			while (msg && !bComplet)
			{
				pGridUser->IsComplete(&bComplet);

				msg += callback.SetCurrentStepPos(gSession.m_nbTaskCompleted);

				if (!gSession.m_msg)
					msg += gSession.m_msg;


				Sleep(100);
				msg += GetConnectionStatus(pGridUser, loop);
			}

			if (!msg)
				pGridUser->CancelTasks();

			refCount = pGridUser->AddRef();
			refCount = pGridUser->Release();

			pGridUser->Release();
			FreeGridUserLibrary();
		}//if DIRECT_ACCESS


		return msg;
	}


}