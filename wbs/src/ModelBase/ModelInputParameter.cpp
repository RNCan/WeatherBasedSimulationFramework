// ModelsInVar.cpp: implementation of the CModelInputParameterDef class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <boost/algorithm/string.hpp>
#include "Basic/CSV.h"
#include "Basic/UtilStd.h"
#include "ModelBase/ParametersVariations.h"
#include "ModelBase/SerializeRect.h"
#include "ModelBase/ModelInputParameter.h"


using namespace std;

namespace WBSF
{

	//const char* CModelInputParameterDefVector::XML_FLAG = "InputVariableArray";
	const char* CModelInputParameterDef::MEMBER_NAME[NB_MEMBER] = { "Name", "Caption", "Description", "Type", "Rect", "SeparatorPos", "DefaultValue", "MinValue", "MaxValue", "TextValue" };
	const short CModelInputParameterDef::TRANSLATED_MEMBER[NB_TRANSLATED_MEMBER] = { DESCRIPTION, CAPTION, TEXT_VALUE };

	const char* CModelInputParameterDef::TYPE_NAME[kMVNbType] = { "Bool", "Integer", "Real", "String", "File", "ListByPos", "ListByString", "ListByCSV", "Header", "Line" };
	//const char* CModelInputParameterDef::TypeSQLName[kMVNbType] = { "LOGICAL", "INTEGER", "DOUBLE", "TEXT", "TEXT", "INTEGER", "TEXT", "TEXT", "TEXT" };

	const int CModelInputParameterDef::MARGIN_HORZ = 4;
	const int CModelInputParameterDef::WIDTH_BUTTON_BROWSE = 30;
	const int CModelInputParameterDef::DEFAULT_WIDTH = 150;
	const int CModelInputParameterDef::DEFAULT_HEIGHT = 22;



	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////
	CModelInputParameterDef::CModelInputParameterDef(const std::string& name, const std::string& caption, short type, const string& defaultVal, const string& list, const tagRECT& rect)
	{
		m_name = name;
		m_caption = caption;// (m_type != kMVLine) ? caption : "";
		m_type = type;
		m_default = defaultVal;
		m_listValues = list;

		m_rect = rect;
		m_beginSecondField = (int)((m_rect.right - m_rect.left) * 2 / 3);
		m_min = type == kMVBool ? "0" : (type == kMVInt || type == kMVReal) ? "-999" : "";
		m_max = type == kMVBool ? "1" : (type == kMVInt || type == kMVReal) ? "999" : "";
	}

	CModelInputParameterDef::CModelInputParameterDef(const CModelInputParameterDef& in)
	{
		operator=(in);
	}


	CModelInputParameterDef::~CModelInputParameterDef()
	{}

	const CModelInputParameterDef& CModelInputParameterDef::operator=(const CModelInputParameterDef& in)
	{
		if (&in != this)
		{
			m_name = in.m_name;
			m_caption = in.m_caption;
			m_description = in.m_description;
			m_rect = in.m_rect;
			m_beginSecondField = in.m_beginSecondField;
			m_min = in.m_min;
			m_max = in.m_max;
			m_default = in.m_default;
			m_listValues = in.m_listValues;
			m_type = in.m_type;
		}

		return *this;
	}

	/*
	//ne pas effacer pour le load des vieux fichiers
	void CModelInputParameterDef::Serialize (CArchive& ar)
	{

	if ( ar.IsStoring() )
	{
	ar << m_name <<	m_caption << m_rect	<< m_beginSecondField;
	ar << m_min << m_max << m_default << m_listValues;

	ar << (long)0;
	ar << (long)m_type;
	}
	else
	{
	int v = ar.GetObjectSchema();
	ar >> m_name >>	m_caption >> m_rect >> m_beginSecondField;
	ar >> m_min >> m_max >> m_default >> m_listValues;
	long tmp1;
	ar >> tmp1; //m_bDefault = (tmp!=0);
	long tmp2;
	ar >> tmp2;m_type = (MInVarType )tmp2;

	if( m_type == kMVBool)
	m_default = tmp1?1.0f:0.0f;
	}

	}
	*/

	std::string CModelInputParameterDef::GetMember(size_t i)const
	{
		ASSERT(i < NB_MEMBER);

		string str;
		switch (i)
		{
		case NAME:			str = m_name; break;
		case CAPTION:		str = m_caption; break;
		case DESCRIPTION:   str = m_description; break;
		case TYPE:			str = ToString((short)m_type); break;
		case RECT:			str = ToString(m_rect); break;
		case SEPARATOR_POS:	str = ToString(m_beginSecondField); break;
		case DEFAULT_VALUE: str = m_default; break;
		case MIN_VALUE:		str = m_min; break;
		case MAX_VALUE:		str = m_max; break;
		case TEXT_VALUE:	str = m_listValues; break;
		default: ASSERT(false);

		}

		return str;
	}


	void CModelInputParameterDef::SetMember(size_t i, const std::string& str)
	{
		ASSERT(i >= 0 && i < NB_MEMBER);

		switch (i)
		{
		case NAME:			m_name = str; break;
		case CAPTION:		m_caption = str; break;
		case DESCRIPTION:   m_description = str; break;
		case TYPE:			m_type = ToShort(str); break;
		case RECT:			m_rect = ToRect(str); break;
		case SEPARATOR_POS:	m_beginSecondField = ToInt(str); break;
		case MIN_VALUE:		m_min = str; break;
		case MAX_VALUE:		m_max = str; break;
		case TEXT_VALUE:	m_listValues = str; break;
		case DEFAULT_VALUE:	m_default = str; break;
		default: ASSERT(false);
		}
	}

	bool CModelInputParameterDef::operator == (const CModelInputParameterDef& in)const
	{
		bool bEqual = true;

		if (m_name != in.m_name) bEqual = false;
		if (m_caption != in.m_caption) bEqual = false;
		if (m_rect.left != in.m_rect.left) bEqual = false;
		if (m_rect.right != in.m_rect.right) bEqual = false;
		if (m_rect.top != in.m_rect.top) bEqual = false;
		if (m_rect.bottom != in.m_rect.bottom) bEqual = false;
		if (m_type != in.m_type)bEqual = false;
		if (m_beginSecondField != in.m_beginSecondField)bEqual = false;
		if (m_min != in.m_min)bEqual = false;
		if (m_max != in.m_max)bEqual = false;
		if (m_default != in.m_default)bEqual = false;
		if (m_listValues != in.m_listValues)bEqual = false;


		return bEqual;
	}

	bool CModelInputParameterDef::operator != (const CModelInputParameterDef& in)const
	{
		return !(*this == in);
	}

	short CModelInputParameterDef::GetType(const std::string& inputString)
	{
		short type = -1;
		for (int i = 0; i < kMVNbType&&type == -1; i++)
			if (boost::iequals(inputString, TYPE_NAME[i]) == 0)
				type = i;

		assert(type != -1);
		if (type == -1)
			type = kMVString;

		return type;
	}

	ERMsg CModelInputParameterDef::IsValid(void)
	{
		assert(m_type < 0 || m_type >= kMVNbType);

		ERMsg msg;

		if (m_name.empty())
		{
			msg.ajoute("Empty name");
		}


		if (m_type == kMVReal)
		{
			if (ToDouble(m_min) >= ToDouble(m_max) ||
				ToDouble(m_default) < ToDouble(m_min) ||
				ToDouble(m_default) > ToDouble(m_max))
			{
				msg.ajoute("out of bouding");
			}
		}

		if (m_type == kMVListByPos)
		{
			StringVector tmp = Tokenize(m_listValues, ";|\r\n");
			if (tmp.empty() || ToInt(m_default) < 0 || ToInt(m_default) >= tmp.size())
				msg.ajoute("Invalid default");
		}

		if (m_type == kMVListByString)
		{
			StringVector tmp = Tokenize(m_listValues, ";|\r\n");
			if (tmp.Find(m_default, false) == UNKNOWN_POS)
				msg.ajoute("Invalid default");
		}

		if (m_type == kMVListByCSV)
		{
			//a faire
			//StringVector tmp = Tokenize(m_listValues, ";|\r\n");
			//if (tmp.Find(m_default, false) == UNKNOWN_POS)
			//msg.ajoute("Invalid default");
		}
		return msg;
	}



	tagRECT CModelInputParameterDef::GetItemRect(short ItemNo)const
	{
		ASSERT(ItemNo >= 0 && ItemNo < 3);
		ASSERT((ItemNo != 2) || (m_type == kMVFile)); // if ItemNo == 2 the it must be a kMVFile

		tagRECT rect;
		rect.top = m_rect.top;
		rect.bottom = m_rect.bottom - 1;

		switch (m_type)
		{

		case kMVBool:
		case kMVInt:
		case kMVReal:
		case kMVString:
		case kMVListByPos:
		case kMVListByString:
		case kMVListByCSV:
		case kMVFile:
		{
			switch (ItemNo)
			{
			case 0:
				rect.left = m_rect.left + MARGIN_HORZ;
				rect.right = m_rect.left + m_beginSecondField - MARGIN_HORZ;
				break;
			case 1:
				rect.left = m_rect.left + m_beginSecondField;
				rect.right = m_rect.right - MARGIN_HORZ;
				break;
			default: ASSERT(false);
			}

			break;
		}
		case kMVTitle:
		case kMVLine:
		case kMVStaticText:
		{
			switch (ItemNo)
			{
			case 0:
				rect.bottom = m_rect.top;
				rect.left = m_rect.left;
				rect.right = m_rect.left;
				break;
			case 1:
				rect.left = m_rect.left + MARGIN_HORZ;
				rect.right = m_rect.right - MARGIN_HORZ;
				if (m_type == kMVLine)
				{
					rect.top = (m_rect.top + m_rect.bottom) / 2;
					rect.bottom = rect.top + 1;
				}
				break;
			default: ASSERT(false);
			}

			break;
		}

		default: ASSERT(!"bad type");
		}

		return rect;
	}

	void CModelInputParameterDef::CleanList()
	{
		if (m_type == kMVListByPos || m_type == kMVListByString)
		{
			replace(m_listValues.begin(), m_listValues.end(), '\n', '|');
			m_listValues.erase( std::remove(m_listValues.begin(), m_listValues.end(), '\r'), m_listValues.end());
			m_listValues.erase( std::remove(m_listValues.begin(), m_listValues.end(), '\t'), m_listValues.end());
		}
	}

	int CModelInputParameterDef::GetNbItemList()const
	{
		int rep = -1;

		if (m_type == kMVListByPos || m_type == kMVListByString)
		{
			StringVector tmp = Tokenize(m_listValues, ";|\r\n");
			rep = (int)tmp.size();
		}


		return rep;
	}

	/*
	void CModelInputParameterDef::UpdateLanguage(const CTranslationArray& translation, short language)
	{
	if( m_type == kMVLine)
	return;

	for(int i=0; i<NB_TRANSLATED_MEMBER; i++)
	{
	if( m_type == kMVTitle && TRANSLATED_MEMBER[i]!=CAPTION)
	continue;

	string str = GetMember( TRANSLATED_MEMBER[i] );
	if( !str.empty() )
	{
	string newStr = translation.GetTranslation( str, language);
	if( !newStr.empty() )
	{
	SetMember( TRANSLATED_MEMBER[i], newStr);
	}
	}
	}
	}


	void CModelInputParameterDef::CreateTranslationArray(const CTranslationArray& oldTranslation, CTranslationArray& newTranslation)
	{
	if( m_type == kMVLine)
	return;

	for(int i=0; i<NB_TRANSLATED_MEMBER; i++)
	{
	//only caption is translate for header
	if( m_type == kMVTitle && TRANSLATED_MEMBER[i]!=CAPTION)
	continue;


	CTranslation translation;

	string str = GetMember( TRANSLATED_MEMBER[i] );
	if( !str.empty() )
	{
	translation[CRegistry::ENGLISH] = str;
	translation[CRegistry::FRENCH] = oldTranslation.GetTranslation( str, CRegistry::FRENCH);
	newTranslation.Add(translation);
	}
	}
	}
	*/

	bool CModelInputParameterDef::IsExtendedList()const
	{
		//StringVector list = Tokenize(m_listValues, ";|\r\n|");
		return m_type == kMVListByCSV;// && list.size() == 4 && list[0] == "FROM_CSV_FILE";

	}

	StringVector CModelInputParameterDef::GetList(const string& appPath, const string& projectPath)const
	{
		StringVector list = Tokenize(m_listValues, ";\r\n|");

		if (m_type == kMVListByCSV)
		{
			ASSERT(!appPath.empty() && !projectPath.empty());

			if (!list.empty())
			{
				string relFilePath = list[0];
				string filePath = SpecialPath2FilePath(relFilePath, appPath, projectPath);
				size_t col = 0;
				if (list.size() == 1)
					col = ToSizeT(list[1]) - 1;

				list.clear();

				ifStream file;
				if (file.open(filePath))
				{
					for (CSVIterator loop(file, ",;|\t"); loop != CSVIterator(); ++loop)
					{

						if (col < loop.Header().size() &&
							col < (*loop).size())
						{
							list.push_back((*loop)[col]);
						}
					}
				}
			}
		}

		return list;
	}
	//****************************************************************************

	const char* CModelInputParameterDefVector::XML_FLAG = "InputVariable";

	CParameterVector CModelInputParameterDefVector::GetParametersVector()const
	{
		CParameterVector out;

		for (size_t i = 0; i < size(); i++)
		{
			string type = ToString(at(i).GetType());
			string name = at(i).GetName();
			out.push_back(CParameter(type, name, ""));
		}


		return out;
	}

	CParametersVariationsDefinition CModelInputParameterDefVector::GetParametersVariations()const
	{
		CParametersVariationsDefinition out;
		for (size_t i = 0; i < size(); i++)
		{
			CParameterVariation p;
			p.m_name = at(i).m_name;
//			p.m_pos = i;
			p.m_bActive = false;
			p.m_type = at(i).m_type;
			p.m_min = ToFloat(at(i).m_min);
			p.m_max = ToFloat(at(i).m_max);
			p.m_step = 1;

			out.push_back(p);
		}


		return out;
	}

	//****************************************************************************

	/*void CModelInputParameterDefVector::UpdateLanguage(const CTranslationArray& translation, short language)
	{
	//CModelInputParameterDefVector& me = *this;
	for(int i=0; i<size(); i++)
	{
	at(i).UpdateLanguage(translation, language);
	}
	}

	void CModelInputParameterDefVector::CreateTranslationArray(const CTranslationArray& oldTranslation, CTranslationArray& newTranslation)
	{
	//CModelInputParameterDefVector& me = *this;
	for(int i=0; i<size(); i++)
	{
	//ASSERT( at(i) != NULL);
	at(i).CreateTranslationArray(oldTranslation, newTranslation);
	}
	}


	int CModelInputParameterDefVector::FindName(const std::string& name)const
	{
	int index = -1;
	for(int i=0; i<size(); i++)
	{
	if( at(i).GetName() == name )
	{
	index = i;
	break;
	}
	}

	return index;
	}

	*/

}