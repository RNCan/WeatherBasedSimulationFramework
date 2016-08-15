//****************************************************************************
// File:	ModelInput.cpp
// Class:	CModelInputParam, CModelInput
//****************************************************************************
// 27/11/2014	Rémi Saint-Amant	Change in xml class
// 15/09/2008	Rémi Saint-Amant	Created from old file
//****************************************************************************

#include "stdafx.h"
#include <boost/algorithm/string.hpp>

#include "ModelBase/ModelInput.h"
#include "basic/UtilStd.h"
#include "WeatherBasedSimulationString.h"

using namespace std;

namespace WBSF
{

	//****************************************************************
	//CModelInputParam

	CModelInputParam::CModelInputParam(const string& name, /*int type,*/ const string& value)
	{
		m_name = name;
		m_value = value;
	}

	short CModelInputParam::GetListIndex()const
	{
		return short(GetNumVal());
	}

	void CModelInputParam::SetListIndex(short value)
	{
		m_value = ToString(value);
	}


	//****************************************************************
	//CModelInput


	const char* CModelInput::XML_FLAG = "ModelInput";
	const char* CModelInput::XML_FLAG_PARAM = "Parameter";
	const char* CModelInput::MEMBER_NAME[NB_MEMBER] = { "Name", "ModelName" };


	CModelInput::CModelInput()
	{

	}

	CModelInput::~CModelInput()
	{}


	CModelInput::CModelInput(const CModelInput& in)
	{
		operator =(in);
	}

	void CModelInput::Reset()
	{
		m_name.clear();
		m_extension.clear();
		clear();
	}

	CModelInput& CModelInput::operator =(const CModelInput& in)
	{
		if (this != &in)
		{
			CModelInputParamVector::operator=(in);
			m_name = in.m_name;
			m_extension = in.m_extension;
		}

		return *this;
	}

	bool CModelInput::operator ==(const CModelInput& in)const
	{
		bool bEqual = true;

		if (((CModelInputParamVector&)*this) != in)bEqual = false;
		if (m_name != in.m_name)bEqual = false;
		if (m_extension != in.m_extension)bEqual = false;

		return bEqual;
	}

	bool CModelInput::CompareParameter(const CModelInput& in)const
	{
		bool bEqual = true;

		if (((CModelInputParamVector&)*this) != in)bEqual = false;
		//if (!std::equal(cbegin(), cend(), in.cbegin()))bEqual = false;

		return bEqual;
	}


	ERMsg CModelInput::IsValid(const CModelInputParameterDefVector& def)const
	{
		ERMsg msg;

		const CModelInput& me = *this;

		size_t nSize1 = def.size();
		size_t nSize2 = me.size();


		size_t j = 0;
		for (size_t i = 0; i < nSize1 && j < nSize2; i++)
		{
			int type = def[i].GetType();
			if (type != CModelInputParameterDef::kMVTitle &&
				type != CModelInputParameterDef::kMVLine &&
				type != CModelInputParameterDef::kMVStaticText)
			{
				switch (def[i].GetType())
				{
				case CModelInputParameterDef::kMVInt:
				case CModelInputParameterDef::kMVReal:
				{
					float min = ToFloat(def[i].m_min);
					float max = ToFloat(def[i].m_max);


					if ((me[j].GetFloat() < min) || (me[j].GetFloat() > max))
					{
						msg.ajoute(FormatMsg(IDS_BSC_INVALID_LIST1, def[i].GetCaption(), ToString(min), ToString(max)));
					}
					break;
				}
				case CModelInputParameterDef::kMVListByPos:
				{
					if (!def[i].IsExtendedList())
					{
						int defaultPos = ToInt(def[i].m_listValues);
						StringVector listName = def[i].GetList();

						int min = 0;
						int max = (int)listName.size();

						if ((me[j].GetListIndex() < min) || (me[j].GetListIndex() >= max))
						{
							msg.ajoute(FormatMsg(IDS_BSC_INVALID_LIST1, def[i].GetCaption(), ToString(min), ToString(max)));
						}
					}
					break;
				}
				case CModelInputParameterDef::kMVListByString:
				case CModelInputParameterDef::kMVListByCSV:
				{
					if (!def[i].IsExtendedList())
					{
						StringVector listName = def[i].GetList();
						if (listName.Find(def[i].m_default) == UNKNOWN_POS)
						{
							msg.ajoute(FormatMsg(IDS_BSC_INVALID_LIST2, def[i].GetCaption(), listName.to_string()));
						}
					}
					break;
				}

				case CModelInputParameterDef::kMVBool:
				case CModelInputParameterDef::kMVString:
				case CModelInputParameterDef::kMVFile:
				case CModelInputParameterDef::kMVTitle:
				case CModelInputParameterDef::kMVLine: 
				case CModelInputParameterDef::kMVStaticText:break;
				default: ASSERT(false);//pas de vérification

				}//end switch
				//}
				/*else
				{
				msg.ajoute(FormatMsg(IDS_SIM_BADARG_MODELIN, ToString(i), def[i].GetTypeName()));
				}*/

				j++;
			}
		}

		//if (i != nSize1 || j != nSize2)
		if (j != nSize2)
		{
			msg.ajoute(FormatMsg(IDS_BSC_BAD_NBARG, ToString(j), ToString(nSize2)));
		}



		return msg;
	}


	//****************************************************************************
	// Abstrac:		Get model input file version
	//
	// Description:	return the version of a model input file
	//
	// Inpput:		FilePath : path of the file on disk
	//
	// Output:      File version. 1=old file, 2 = XML
	//
	// Note:
	//****************************************************************************
	short CModelInput::GetVersion(const string& filePath)
	{
		short version = -1;

		ifStream file;
		if (file.open(filePath))
		{
			string line;
			if (std::getline(file, line))
			{
				if (line.find("<?xml") != string::npos)
					version = 2;
				else version = 1;
			}
		}

		return version;
	}

	ERMsg CModelInput::Load(const string& filePath)
	{
		ERMsg msg;

		short version = GetVersion(filePath);

		if (version == 1)
		{
			//msg = LoadV1(filePath);
			msg.ajoute("ModelInput : Unsupported old format");
		}
		else
		{
			msg = zen::LoadXML(filePath, GetXMLFlag(), "1", *this);
			if (msg)
			{
				m_name = GetFileTitle(filePath);
				m_extension = GetFileExtension(filePath);
			}

			/*CStdFile file;
			msg = file.Open( filePath.c_str(), CStdFile::modeRead);
			if( msg )
			{
			string text;
			file.Read(text.GetBufferSetLength((int)file.GetLength()), (int)file.GetLength());
			text.ReleaseBuffer();

			XNode root;
			PARSEINFO pi;

			root.Load(text, &pi);
			if( pi.error_code == 0 )
			{
			SetXML(root);

			m_name = filePath.GetFileTitle();
			m_extension = filePath.GetFileExtension();

			}
			else
			{
			msg.ajoute(pi.error_string);
			}
			}*/
		}

		return msg;
	}
	/*
	ERMsg CModelInput::LoadV1(const string& filePath)
	{
	ERMsg msg;

	Reset();

	ifStream file;
	msg = file.open( filePath);
	if( msg )
	{
	m_name = GetFileTitle(filePath);
	m_extension = GetFileExtension(filePath);

	string line;

	std::getline(file, line);//read header
	std::getline(file, line);

	resize( ToInt( line) );

	for (int i=0; i<size(); i++)
	{
	at(i).Read(file);
	}

	file.close();
	}

	return msg;
	}
	*/

	//****************************************************************************
	// Abstract:	Save model input on disk
	//
	// Description:	Save model input on disk
	//
	// Input:       FilePath : the path of the file to save
	//
	// Output:       ERMsg : message on error
	//
	// Note:
	//****************************************************************************
	ERMsg CModelInput::Save(const string& filePath)
	{
		ERMsg msg;

		msg = zen::SaveXML(filePath, GetXMLFlag(), "1", *this);

		/*CFileEx file;
		msg = file.Open( filePath, CFile::modeCreate|CFile::modeWrite);
		if( msg )
		{
		m_name = UtilWin::GetFileTitle(filePath);
		//		m_id = GetModelIDFromExtension(UtilWin::GetFileExtension(filePath));
		m_extension=UtilWin::GetFileExtension(filePath);


		XNode root;
		GetXML(root);
		root[0]->AppendAttr("version", "2");

		const std::string& str = root[0]->GetXML();
		file.Write("<?xml version='1.0'?>", 21);
		file.Write( str.data(), str.size() );

		file.Close();
		}

		*/

		return msg;
	}


	string CModelInput::GetMember(size_t i)const
	{
		ASSERT(i >= 0 && i < NB_MEMBER);

		string str;
		switch (i)
		{
		case NAME:		str = m_name; break;
		case MODEL_NAME:str = m_extension; break;
		default: ASSERT(false);
		}

		return str;
	}



	void CModelInput::SetMember(size_t i, const string& str)
	{
		ASSERT(i >= 0 && i < NB_MEMBER);

		switch (i)
		{
		case NAME:		m_name = str; break;
		case MODEL_NAME:m_extension = str; break;
		default: ASSERT(false);
		}
	}

	/*void CModelInput::GetXML(XNode& root)const
	{
		XNode& xml = *root.AppendChild(XML_FLAG);

		for (int i = 0; i < size(); i++)
		{
			LPXNode pNode = xml.AppendChild("Parameter", at(i).GetStr().c_str());
			pNode->AppendAttr("Name", at(i).GetName().c_str());
		}
	}


	void CModelInput::SetXML(const XNode& root)
	{
		LPXNode pNode = root.Select(XML_FLAG);
		if (pNode)
		{
			XNode& xml = *pNode;
			ASSERT(xml.name == XML_FLAG);

			Reset();

			XNodes nodes = xml.GetChilds("Parameter");
			for (int i = 0; i < (int)nodes.size(); i++)
			{
				CModelInputParam param;
				param.SetName(nodes[i]->AppendAttr("Name")->value.data());
				push_back(param);
			}
		}
	}
*/
	string CModelInput::GetDescription(std::vector<int> pos)const
	{
		string str;
		if (pos.size() != size())
		{
			for (int i = 0; i < pos.size(); i++)
			{
				str += at(pos[i]).GetName() + "=" + at(pos[i]).GetStr() + "|";
			}
		}
		else
		{
			for (int i = 0; i < size(); i++)
			{
				str += at(i).GetName() + "=" + at(i).GetStr() + "|";
			}
		}


		return str;
	}

	CParameterVector CModelInput::GetParametersVector()const
	{
		CParameterVector out;
		for (size_t i = 0; i < size(); i++)
		{
			std::string name = at(i).GetName();
			std::string value = at(i).GetStr();

			out.push_back(CParameter(name, value));
		}

		return out;
	}


	void CModelInput::FilePath2SpecialPath(const string& appPath, const string& projectPath)
	{
		for (int i = 0; i < size(); i++)
		{
			bool bPath = at(i).m_value.find_first_of("\\/") != string::npos && at(i).m_value.find_first_of("\\/") != string::npos;
			if (bPath)
			{
				string newPath = WBSF::FilePath2SpecialPath(at(i).GetFilePath(), appPath, projectPath);
				at(i).SetFilePath(newPath);
			}
		}
	}

	void CModelInput::SpecialPath2FilePath(const string& appPath, const string& projectPath)
	{
		for (int i = 0; i < size(); i++)
		{
			bool bPath = at(i).m_value.find_first_of("\\/") != string::npos && at(i).m_value.find_first_of("\\/") != string::npos;
			if (bPath)
			{
				string newPath = WBSF::SpecialPath2FilePath(at(i).GetFilePath(), appPath, projectPath);
				at(i).SetFilePath(newPath);
			}
		}
	}

	int CModelInput::FindVariable(string name)const
	{
		int index = -1;
		for (int i = 0; i < size() && index == -1; i++)
			if (boost::iequals(name, at(i).GetName()))
				index = i;

		ASSERT(index >= -1 && index < size());
		return index;
	}
	//*********************************************************************************************************************
	vector<int> CModelInputVector::GetVariablePos()const
	{

		//a revoir, ne fonctionne pas avec les model input composé
		std::vector<int> out;

		for (int i = 1; i < size(); i++)
		{
			for (int j = 0; j < at(i).size(); j++)
			{
				if (at(i).at(j) != at(0).at(j))
				{
					if (std::find(out.begin(), out.end(), j) == out.end())
						out.push_back(j);
				}
			}
		}

		sort(out.begin(), out.end());

		return out;
	}

	CModelInputVector& CModelInputVector::operator*=(const CModelInputVector& in)
	{

		CModelInputVector modelInputVector(size()*in.size());
		for (size_t i = 0; i < size(); i++)
		{
			for (size_t j = 0; j < in.size(); j++)
			{
				modelInputVector[i*size() + j] = at(i);
				modelInputVector[i*size() + j].insert(modelInputVector[i*size() + j].end(), in[j].begin(), in[j].end());
			}
		}

		m_pioneer.insert(m_pioneer.end(), in.m_pioneer.begin(), in.m_pioneer.end());
		return *this;
	}

	CModelInputVector CModelInputVector::operator*(const CModelInputVector& in)const{ CModelInputVector out(*this);	return out *= in; }
	
}