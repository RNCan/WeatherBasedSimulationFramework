//***************************************************************************
// File:        ModelInput.h
//
// Class:       CModelInputParam	: model input parameter
//				CModelInput			: Array of model input
//
// Abstract:    Manage model input parameters 
//
// Description: A container class with XML serialization for model input
//
// Note:        
//***************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "Basic/UtilStd.h"
//#include "XMLite.h"
#include "Basic/UtilZen.h"
#include "ModelBase/InputParam.h"
#include "ModelBase/ModelInputParameter.h"
#include "ModelBase/ParametersVariations.h"


namespace WBSF
{

	class CModelInputParam
	{
	public:


		std::string m_name;
		std::string m_value;

		CModelInputParam(const std::string& name = "", /*int type=-1,*/ const std::string& value = "");

		std::string GetName()const{ return m_name; }
		void SetName(std::string name){ m_name = name; }


		bool IsExtendedList()const;
		double GetNumVal()const{ return ToDouble(m_value); }
		bool GetBool()const{ return bool(GetNumVal() != 0); }
		short GetShort()const{ return short(GetNumVal()); }
		long GetLong()const{ return long(GetNumVal()); }
		float GetFloat()const{ return float(GetNumVal()); }
		double GetDouble()const{ return GetNumVal(); }
		const std::string& GetStr()const{ return m_value; }

		std::string GetExportString(const std::string& varName)const{ return varName + "|" + m_value; }



		const std::string& GetFilePath()const{ return m_value; }

		void SetValue(bool value){	m_value = ToString(value);	}
		void SetValue(__int32 value)		{			m_value = ToString(value);		}
		void SetValue(float value)		{			m_value = ToString(value, -1);		}
		void SetValue(double value)		{			m_value = ToString(value, -1);		}

		void SetValue(const std::string& value){ m_value = value; }
		void SetStr(const std::string& value)		{			m_value = value;		}
		short GetListIndex()const;
		void SetListIndex(short value);

		void SetFilePath(const std::string& value)		{			m_value = value;		}
		void SetValue(short type, const std::string& value)
		{
			ASSERT(type >= 0 && type < CModelInputParameterDef::kMVNbType);
			m_value = value;
		}

		bool operator ==(const CModelInputParam& in)const
		{
			bool bEqual = true;

			if (m_value != in.m_value)bEqual = false;
			return bEqual;
		}

		bool operator !=(const CModelInputParam& in)const{ return !operator==(in); }

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_name & m_value;
		}
		friend boost::serialization::access;

	};

	

	typedef std::vector<CModelInputParam> CModelInputParamVector;
	class CModelInput : public CModelInputParamVector
	{
	public:

		enum TMember { NAME, MODEL_NAME, NB_MEMBER };

		static const char* XML_FLAG;
		static const char* XML_FLAG_PARAM;
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }

		static short GetVersion(const std::string& filePath);

		CModelInput();
		CModelInput(const CModelInput& in);
		~CModelInput();

		void Reset();

		CModelInput& operator =(const CModelInput& in);
		bool operator ==(const CModelInput& in)const;
		bool operator !=(const CModelInput& in)const{ return !operator ==(in); }

		bool CompareParameter(const CModelInput& modelInput)const;

		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& value);
		//void GetXML(XNode& xml)const;
		//void SetXML(const XNode& xml);

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);


		const std::string& GetName()const{ return m_name; }
		void SetName(const std::string& name){ m_name = name; }

		const std::string& GetExtension()const{ return m_extension; }
		void SetExtension(const std::string& ext){ m_extension = ext; }


		void FilePath2SpecialPath(const std::string& appPath, const std::string& projectPath);
		void SpecialPath2FilePath(const std::string& appPath, const std::string& projectPath);


		int FindVariable(std::string name)const;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CModelInputParamVector>(*this) & m_name & m_extension;
		}
		friend boost::serialization::access;
		std::string GetDescription(std::vector<size_t> pos)const;

		CParameterVector GetParametersVector()const;

		ERMsg IsValid(const CModelInputParameterDefVector& def)const;

	protected:


		std::string GetModelIDFromExtension(const std::string& ext);

		std::string m_name;
		std::string m_extension;



		static const char* MEMBER_NAME[NB_MEMBER];
	};

	typedef std::vector<CModelInput> CModelInputVectorBase;
	class CModelInputVector : public CModelInputVectorBase
	{
	public:

		CModelInputVector(size_t size = 0) :CModelInputVectorBase(size){}


		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CModelInputVectorBase>(*this) & m_pioneer & m_variation;
		}
		friend boost::serialization::access;

		CModelInput m_pioneer;	//intial model input
		CParametersVariationsDefinition m_variation; //variation definition
		CModelInputVector& operator*=(const CModelInputVector& in);
		CModelInputVector operator*(const CModelInputVector& in)const;

		std::vector<size_t>  GetVariablePos()const;
	};

}


namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CModelInputParam& in, XmlElement& output)
	{
		output.setAttribute("Name", in.m_name);
		output.setValue(in.m_value);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CModelInputParam& out)
	{
		input.getAttribute("Name", out.m_name);
		input.getValue(out.m_value);

		return true;
	}


	template <> inline
		void writeStruc(const WBSF::CModelInput& in, XmlElement& output)
	{
		writeStruc3(in, output, WBSF::CModelInput::XML_FLAG_PARAM);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CModelInput& out)
	{
		return readStruc3(input, out, WBSF::CModelInput::XML_FLAG_PARAM);
	}
}