//***************************************************************************
// File:        ModelInputParameter.h
//
// class:		CModelInputParameterDef
//
// Abstract:    Manage input parameters definition for model dialog
//
// Description: A container class with XML serialization for model input 
//				definition. CModelInputParameterDef define type and position.
//
// Note:        
//***************************************************************************

#pragma once

#include "Basic/UtilStd.h"
#include "ModelBase/SerializeRect.h"
#include "ModelBase/InputParam.h"

namespace WBSF
{
	class CModelInputParameterDef
	{
	public:

		enum TMember{ NAME, CAPTION, DESCRIPTION, TYPE, RECT, SEPARATOR_POS, DEFAULT_VALUE, MIN_VALUE, MAX_VALUE, TEXT_VALUE, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }



		enum TParameter { kMVBool, kMVInt, kMVReal, kMVString, kMVFile, kMVListByPos, kMVListByString, kMVListByCSV, kMVTitle, kMVLine, kMVNbType };
		static short GetType(const std::string& inputString);

		static const int MARGIN_HORZ;
		static const int WIDTH_BUTTON_BROWSE;
		static const int DEFAULT_WIDTH;
		static const int DEFAULT_HEIGHT;


		short m_type;
		std::string m_name;
		std::string m_caption;
		std::string m_description;
		tagRECT   m_rect;
		int		m_beginSecondField;


		std::string m_min;
		std::string m_max;
		std::string m_default;
		std::string m_listValues;

		//const CRect& rect = CRect(0, 0, 0, 0), 
		CModelInputParameterDef(const std::string& name = "", const std::string& caption = "", short type = kMVString, const std::string& defaultVal = "", const std::string& list = "", const tagRECT& rect = { 0, 0, 0, 0 });

		CModelInputParameterDef(const CModelInputParameterDef& in);


		~CModelInputParameterDef();

		const CModelInputParameterDef& operator=(const CModelInputParameterDef& in);
		bool operator == (const CModelInputParameterDef& in)const;
		bool operator != (const CModelInputParameterDef& in)const;
		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);

		ERMsg IsValid(void);
		tagRECT GetItemRect(short ItemNo)const;
		int GetNbItemList()const;
		void CleanList();

		const std::string& GetName()const{ return m_name; }
		void SetName(const std::string& name){ m_name = name; }

		//const std::string& GetTitle()const{return m_title.empty()?m_name:m_title;}
		const std::string& GetDescription()const{ return m_description; }
		void SetDescription(const std::string& in){ m_description = in; }

		const std::string& GetCaption()const{ return	m_caption; }
		void SetCaption(const std::string& caption){ if (m_type != kMVLine) m_caption = caption; }
		const tagRECT& GetRect()const{ return	m_rect; }
		void SetRect(const tagRECT& rect){ m_rect = rect; }
		short GetType()const{ return m_type; }
		void SetType(short type){ ASSERT(type >= 0 && type < kMVNbType); m_type = type; }
		int GetEndFirstField()const{ return m_beginSecondField - MARGIN_HORZ; }
		int GetBeginSecondField()const{ return m_beginSecondField; }
		void SetBeginSecondField(int val){ m_beginSecondField = val; }

		///***********************///*********************///*****************
		bool IsAVariable()const{ return m_type != kMVTitle && m_type != kMVLine; }

		bool IsExtendedList()const;


		static const char* GetTypeName(short type)
		{
			ASSERT(type >= 0 && type < kMVNbType);
			return type == -1 ? TYPE_NAME[kMVNbType] : TYPE_NAME[type];
		}

		const char* GetTypeName()const{ return GetTypeName(short(m_type)); }

		StringVector GetList(const std::string& appPath = "", const std::string& projectPath = "")const;


		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_type &m_name &m_caption &m_description &m_rect &m_BeginSecondField &m_min &m_max &m_default &m_listValues;
		}

		bool IsNumeric()const
		{
			assert(kMVNbType == 10);

			return
				m_type == CModelInputParameterDef::kMVBool ||
				m_type == CModelInputParameterDef::kMVInt ||
				m_type == CModelInputParameterDef::kMVReal ||
				m_type == CModelInputParameterDef::kMVListByPos ||
				m_type == CModelInputParameterDef::kMVListByString ||
				m_type == CModelInputParameterDef::kMVListByCSV;
		}

		bool IsString()const
		{
			ASSERT(kMVNbType == 10);

			return
				m_type == CModelInputParameterDef::kMVString ||
				m_type == CModelInputParameterDef::kMVFile ||
				m_type == CModelInputParameterDef::kMVListByString ||
				m_type == CModelInputParameterDef::kMVListByCSV;
		}

	protected:

		enum{ NB_TRANSLATED_MEMBER = 3 };

		static const short TRANSLATED_MEMBER[NB_TRANSLATED_MEMBER];
		static const char* MEMBER_NAME[NB_MEMBER];
		static const char* TYPE_NAME[kMVNbType];
		//static const char* TypeSQLName[kMVNbType];
	};


	
	class CParametersVariationsDefinition;

	typedef std::vector<CModelInputParameterDef> CModelInputParameterDefVectorBase;
	class CModelInputParameterDefVector : public CModelInputParameterDefVectorBase
	{
	public:

		static const char* XML_FLAG;
		CParameterVector GetParametersVector()const;
		CParametersVariationsDefinition GetParametersVariations()const;

	};

}

namespace zen
{

	template <> inline
		void writeStruc(const WBSF::CModelInputParameterDef& def, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::NAME)](def.m_name);
		out[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::CAPTION)](def.m_caption);
		out[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::DESCRIPTION)](def.m_description);
		out[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::TYPE)](def.m_type);
		out[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::RECT)](def.m_rect);
		out[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::SEPARATOR_POS)](def.m_beginSecondField);
		out[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::DEFAULT_VALUE)](def.m_default);
		out[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::MIN_VALUE)](def.m_min);
		out[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::MAX_VALUE)](def.m_max);
		out[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::TEXT_VALUE)](def.m_listValues);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CModelInputParameterDef& def)
	{
		XmlIn in(input);

		in[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::NAME)](def.m_name);
		in[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::CAPTION)](def.m_caption);
		in[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::DESCRIPTION)](def.m_description);
		in[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::TYPE)](def.m_type);
		in[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::RECT)](def.m_rect);
		in[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::SEPARATOR_POS)](def.m_beginSecondField);
		in[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::DEFAULT_VALUE)](def.m_default);
		in[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::MIN_VALUE)](def.m_min);
		in[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::MAX_VALUE)](def.m_max);
		in[WBSF::CModelInputParameterDef::GetMemberName(WBSF::CModelInputParameterDef::TEXT_VALUE)](def.m_listValues);


		return true;
	}

	template <> inline
		void writeStruc(const WBSF::CModelInputParameterDefVector& value, XmlElement& output)
	{
		writeStruc3(value, output, WBSF::CModelInputParameterDefVector::XML_FLAG);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CModelInputParameterDefVector& value)
	{
		return readStruc3(input, value, WBSF::CModelInputParameterDefVector::XML_FLAG);
	}
}
