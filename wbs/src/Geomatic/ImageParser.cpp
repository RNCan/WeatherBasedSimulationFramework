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
#include "Geomatic/ImageParser.h"
#include "Basic/OpenMP.h"

using namespace std;

namespace WBSF
{

	//************************************************************************
	//CFunctionContext

	//************************************************************************
	//CImageParser
	/*
	CMTParserVariable::CMTParserVariable(std::string name, double noData)
	{
	m_name=name;
	m_value=0;
	m_noData=noData;
	}

	CMTParserVariable::CMTParserVariable(const CMTParserVariable& in)
	{
	operator = (in);
	}

	CMTParserVariable& CMTParserVariable::operator = (const CMTParserVariable& in)
	{
	if( &in != this)
	{
	m_name=in.m_name;
	m_value=in.m_value;
	m_noData=in.m_noData;
	}

	return *this;
	}

	bool CMTParserVariable::operator == (const CMTParserVariable& in)const
	{
	return m_name==in.m_name;
	}

	bool CMTParserVariable::operator != (const CMTParserVariable& in)const
	{
	return !operator==(in);
	}
	*/
	CImageBandPos GetImageBand(const CMTParserVariable& var)
	{
		int posI = 0;
		int posB = 0;

		ASSERT(var.m_name[0] == 'i');
		string::size_type pos = var.m_name.find('b');

		if (pos != string::npos)
		{
			posI = ToInt(var.m_name.substr(1, pos)) - 1;
			posB = ToInt(var.m_name.substr(pos + 1)) - 1;
		}

		return pair<int, int>(posI, posB);
	}


	//************************************************************************
	//CImageParser

	CImageParser::CImageParser()
	{}

	CImageParser::CImageParser(const CImageParser& in)
	{
		operator = (in);
	}

	CImageParser& CImageParser::operator = (const CImageParser& in)
	{
		if (&in != this)
		{
			m_bManageSrcNoData = in.m_bManageSrcNoData;
			m_formula = in.m_formula;
			m_parser = in.m_parser;
			m_vars = in.m_vars;
		}

		return *this;
	}

	string CImageParser::ReplaceOldStyle(string formula)
	{
		string newFormula;

		string::size_type pos = 0;
		bool bContinue = true;
		//int newPos = formula.Find("GetPixel(", pos);
		//int newPos = formula.Find("Pixel(", pos);

		while (bContinue)
		{
			string::size_type pos2 = formula.find("][", pos);
			if (pos2 != string::npos)
			{
				string::size_type pos1 = formula.find_last_of("[", pos2);
				string::size_type pos3 = formula.find("]", pos2 + 2);
				//ASSERT( newPos2-newPos1 <= 3 );//up to 999 image
				//ASSERT( newPos3-newPos2 <= 5 );//up tyo 99 999 bands
				if (pos1 != string::npos && pos3 != string::npos)
				{
					newFormula += formula.substr(pos, pos1 - pos);

					string strVar = formula.substr(pos1, pos3 - pos1 + 1);
					string::size_type subPos = 0;

					string posI = Tokenize(strVar, "[]", subPos, true);
					if (subPos != string::npos)
					{
						string posB = Tokenize(strVar, "[]", subPos, true);
						//newFormula += formula.substr(pos, pos-pos1);
						newFormula += "i" + posI + "b" + posB;
						pos = pos3 + 1;
					}
					else
					{
						//????? probably invalid formula...will be detected later
						newFormula += formula.substr(pos);
						bContinue = false;
					}
				}
				else
				{
					//????? probably invalid formula...will be detected later
					newFormula += formula.substr(pos);
					bContinue = false;
				}
			}
			else
			{
				newFormula += formula.substr(pos);
				bContinue = false;
			}
		}

		return newFormula;
	}




	/*mu::value_type CImageParser::GetPixel(int nBulkIdx, int nThreadIdx, mu::value_type v1in, mu::value_type v2in)
	{
	int v1 = (int)v1in;
	int v2 = (int)v2in;
	if( v1>=0 && v1<m_variablePos.size() &&
	v2>=0 && v2<m_variablePos[v1].size() )
	{
	int pos = m_variablePos[v1][v2];
	m_vars[pos].
	m_pBandHolder.get()->GetPixel();
	}


	return nBulkIdx + nThreadIdx + v1;
	}
	*/
	//static double * pVal1[2] = {0};
	/*mu::value_type* CImageParser::VariableFactory(const mu::char_type* pName, void* pData)
	{
	CMTParserVariable var(pName);

	CImageParser* pMe = static_cast<CImageParser*>(pData);
	pMe->m_vars.push_back(var);

	//pVal1[pMe->m_vars.size()-1] = &(pMe->m_vars.back().m_value);
	return &(pMe->m_vars.back().m_value);
	}
	*/
	string GetOldStyle(const string& name)
	{
		ASSERT(name[0] == 'i');
		string::size_type pos = name.find('b');
		ASSERT(pos != string::npos);

		return string("[") + name.substr(1, pos - 1) + "][" + name.substr(pos + 1) + "]";
	}

	ERMsg CImageParser::Compile(const CGDALDatasetExVector& inputDSVector, int nbThread)
	{
		ERMsg msg;


		if (m_formula.empty())
			return msg;


		m_optimizedFormula = ReplaceOldStyle(m_formula);
		bool bOldStyle = m_optimizedFormula != m_formula;

		NoDataVector noData(inputDSVector.size());
		StringVector filePaths;
		for (size_t i = 0; i < inputDSVector.size(); i++)
		{
			for (size_t j = 0; j < inputDSVector[i].GetRasterCount(); j++)
				noData[i].push_back(inputDSVector[i].GetNoData(j));

			filePaths.push_back(inputDSVector[i].GetFilePath());
		}

		try
		{
			m_parser.resize(nbThread);

			for (int t = 0; t < nbThread; t++)
			{
				GetNoDataFct* pNoDataFct = new GetNoDataFct;
				pNoDataFct->SetNoData(noData);
				m_parser[t].defineFunc(pNoDataFct);

				GetPixelFct* pGetPixelFct = new GetPixelFct;
				pGetPixelFct->SetImagesFilePath(filePaths);
				m_parser[t].defineFunc(pGetPixelFct);


				m_parser[t].enableAutoVarDefinition(true);
				m_parser[t].compile(convert(m_optimizedFormula).c_str());//posible que ca bne fonctionne pas a vérifier????
			}
		}
		catch (MTParserException e)
		{
			msg.ajoute(UTF8(e.m_description.c_str()));
		}

		if (msg)
		{
			//Set variable
			m_vars.resize(nbThread);
			for (int t = 0; t < m_parser.size() && msg; t++)
				m_vars[t].resize(m_parser[t].getNbUsedVars());

			for (int t = 0; t < m_parser.size() && msg; t++)
			{
				for (unsigned int v = 0; v < m_parser[t].getNbUsedVars(); v++)
				{
					m_vars[t][v].m_name = UTF8(m_parser[t].getUsedVar(v));

					CImageBandPos IBpos = GetImageBand(m_vars[t][v]);

					if (IBpos.first >= 0 && IBpos.first < (int)inputDSVector.size())
					{
						if (IBpos.second >= 0 && IBpos.second < (int)inputDSVector[IBpos.first].GetRasterCount())
						{
							//init noData value
							m_parser[t].redefineVar(m_parser[t].getUsedVar(v).c_str(), &(m_vars[t][v].m_value));
							m_vars[t][v].m_noData = MISSING_NO_DATA;
							if (inputDSVector[IBpos.first].HaveNoData(IBpos.second))
								m_vars[t][v].m_noData = inputDSVector[IBpos.first].GetNoData(IBpos.second);

						}
						else
						{
							msg.ajoute(string("ERROR: band index (1 .. ") + to_string(inputDSVector[IBpos.first].GetRasterCount()) + ") is incorrecte in expression: " + (bOldStyle ? GetOldStyle(m_vars[t][v].m_name) : m_vars[t][v].m_name));
						}
					}
					else
					{
						msg.ajoute(string("ERROR: image index (1 .. ") + to_string(inputDSVector.size()) + ") is incorrecte in expression: " + (bOldStyle ? GetOldStyle(m_vars[t][v].m_name) : m_vars[t][v].m_name));
					}
				}
			}
		}

		return msg;
	}


	double CImageParser::Evaluate(const vector<float>& vars)
	{


		int thread = omp_get_thread_num();
		double value = MISSING_NO_DATA;
		bool bMissingValue = false;

		ASSERT(vars.size() == m_vars[thread].size());
		for (size_t i = 0; i < m_vars[thread].size() && (m_bManageSrcNoData || !bMissingValue); i++)
		{
			m_vars[thread][i].m_value = vars[i];
			bMissingValue = !m_vars[thread][i].IsValid();
		}

		if (m_bManageSrcNoData || !bMissingValue)
		{
			try
			{
				value = m_parser[thread].evaluate();
			}
			catch (MTParserException& e)
			{
				string msg = UTF8(e.m_description.c_str());
			}
		}

		return value;
	}




	//***************************************************************************

	MTDOUBLE GetPixelFct::evaluate(unsigned int nbArgs, const MTDOUBLE *pArg)
	{

		ASSERT(nbArgs == 2);
		size_t imageNo = size_t(pArg[0]) - 1;
		size_t bandNo = size_t(pArg[1]) - 1;

		MTDOUBLE val = 0;
		/*
		//convert
		if( imageNo>=0 && imageNo<m_DS.size() )
		{
		if( !m_DS[imageNo].IsOpen() )
		{
		ERMsg msg = m_DS[imageNo].Open(m_filePaths[imageNo] );
		if( !msg )
		{
		MTExcepData data( MTPARSINGEXCEP_InternalError );
		MTParserException exceps;
		string desc;
		desc.Format( "Unable to open image %s", m_filePaths[imageNo] );
		//SYGetMessage(msg);
		exceps.add(__LINE__,  _T(__FILE__), 0, data, desc.c_str());
		throw(exceps);
		}

		}

		if( bandNo>=0 && bandNo<m_DS[imageNo].GetRasterCount() )
		{
		val = (MTDOUBLE) m_DS[imageNo]->RasterIO(>GetPixel(bandNo, m_x);
		}
		else
		{
		MTExcepData data( MTPARSINGEXCEP_InternalError );
		MTParserException exceps;
		string desc;
		desc.Format( "Invalid argument in function GetPixel(%d,%d)", imageNo+1, bandNo+1);

		exceps.add(__LINE__,  _T(__FILE__), 0, data, desc.c_str());
		throw(exceps);
		}
		}
		else
		{
		MTExcepData data( MTPARSINGEXCEP_InternalError );
		MTParserException exceps;
		string desc;
		desc.Format( "Invalid argument in function GetPixel(%d,%d)", imageNo+1, bandNo+1);

		exceps.add(__LINE__,  _T(__FILE__), 0, data, desc.c_str());
		throw(exceps);
		}
		*/

		return val;
	}



	//*****************************************************************************************************************
}