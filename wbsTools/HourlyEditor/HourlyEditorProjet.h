// OptionPropSheet.h : header file
//

#pragma once
#include <string>
#include <array>
#include <boost\multi_index_container.hpp>

#include "UtilStd.h"
#include "UtilZen.h"
#include "Mtrx.h"
#include "UtilTime.h"


//****************************************************************************************************************************
//CFieldDefinition
class CFieldDefinition
{
public:

	
	enum TMembre{ TITLE, DESCRIPTION, VAL_MIN, VAL_MAX, LISTE, COULEUR, LARGEUR, POSITION, NB_MEMBRES };
	//static const char* NOM_XML;
	static const char* MEMBERS_NAMES[NB_MEMBRES];
	
	//std::string m_nom;
	std::string m_title;
	//std::string m_unite;
	std::string m_description;
	//std::string m_defaut;
	std::string m_min;
	std::string m_max;
	std::string m_codesList;
	COLORREF m_backColor;
	int m_largeur;
	int m_position;

	CFieldDefinition()
	{
		m_backColor = RGB(250, 250, 250);
		m_largeur = 40;
		m_position = 0;
	}

	CFieldDefinition(const CFieldDefinition& in)
	{
		operator=(in);
	}

	CFieldDefinition& operator = (const CFieldDefinition& in)
	{
		if (&in != this)
		{
			//m_nom = in.m_nom;
			m_title = in.m_title;
			//m_unite = in.m_unite;
			m_description = in.m_description;
			//m_defaut = in.m_defaut;
			m_min = in.m_min;
			m_max = in.m_max;
			m_codesList = in.m_codesList;
			m_backColor = in.m_backColor;
			m_largeur = in.m_largeur;
			m_position = in.m_position;
		}

		ASSERT(*this == in);

		return *this;
	}

	bool operator == (const CFieldDefinition& in)const
	{
		bool bEqual = true;
		
		//if (m_nom != in.m_nom) bEqual = false;
		if (m_title != in.m_title) bEqual = false;
		///if (m_unite != in.m_unite) bEqual = false;
		if (m_description != in.m_description) bEqual = false;
		//if (m_defaut != in.m_defaut) bEqual = false;
		if (m_min != in.m_min) bEqual = false;
		if (m_max != in.m_max) bEqual = false;
		if (m_codesList != in.m_codesList) bEqual = false;
		if (m_backColor != in.m_backColor) bEqual = false;
		if (m_largeur != in.m_largeur) bEqual = false;
		if (m_position != in.m_position) bEqual = false;
		
		return bEqual;
	}

	bool operator != (const CFieldDefinition& in)const{ return !operator==(in); }

	bool IsValid(const std::string value)const;
	
	void GetCodes(std::map<std::string, std::string>& codes)const{ GetCodes(m_codesList, codes); }
	static void GetCodes(const std::string& str, std::map<std::string, std::string>& codes);
	
	std::string GetCode(const std::string& value)const;
protected:

	std::string m_lastCodesBuild;
	std::map<std::string, std::string> m_codes;
};



namespace zen
{
	inline std::string ToHex(COLORREF color)
	{
		std::stringstream s; 
		s << std::hex << color;
		return s.str();
	}

	inline COLORREF FromHex(std::string str)
	{
		COLORREF color;
		std::stringstream s(str);
		s >> std::hex >> color;
		
		return color;
	}

	template <> inline
	void writeStruc(const CFieldDefinition& in, XmlElement& output)
	{
		XmlOut out(output);
		
		out[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::TITLE]](in.m_title);
		//out[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::UNITE]](in.m_unite);
		out[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::DESCRIPTION]](in.m_description);
		//out[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::DEFAUT]](in.m_defaut);
		out[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::VAL_MIN]](in.m_min);
		out[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::VAL_MAX]](in.m_max);
		out[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::LISTE]](in.m_codesList);
		out[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::COULEUR]](ToHex(in.m_backColor));
		out[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::LARGEUR]](in.m_largeur);
		//out[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::POSITION]](in.m_position);
		
	}

	template <> inline
	bool readStruc(const XmlElement& input, CFieldDefinition& out)
	{
		XmlIn in(input);
		std::string hex;
		in[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::TITLE]](out.m_title);
		//in[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::UNITE]](out.m_unite);
		in[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::DESCRIPTION]](out.m_description);
		//in[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::DEFAUT]](out.m_defaut);
		in[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::VAL_MIN]](out.m_min);
		in[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::VAL_MAX]](out.m_max);
		in[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::LISTE]](out.m_codesList);
		in[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::COULEUR]](hex); out.m_backColor = FromHex(hex);
		in[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::LARGEUR]](out.m_largeur);
		//in[CFieldDefinition::MEMBERS_NAMES[CFieldDefinition::POSITION]](out.m_position);

		return true;
	}
}


//****************************************************************************************************************************
//CFieldsDefinitions
typedef std::map<std::string, CFieldDefinition> CFieldsDefinitions;


//****************************************************************************************************************************
//CStudyDefinition
class CStudyDefinition
{

public:

	enum TMembre{ TITLE, RESPONSABLE, DESCRIPTION, METADATA, VALIDATION, PREMIER_NO_VIAL, DERNIER_NO_VIAL, LARGEUR_VIAL, POSITION, FACTEURS_OBSERVATIONS, SEPARATEUR_LISTE, SYMBOLE_DECIMAL, CHAMPS, NB_MEMBRES };
	static const char* FIELD_XML;
	static const char* MEMBERS_NAMES[NB_MEMBRES];

	//std::string m_nom;
	std::string m_title;
	std::string m_responsable;
	std::string m_description;
	std::string m_metadata;
	std::string m_validation;

	std::string m_firstVial;
	std::string m_lastVial;
	int m_largeurVial;
	int m_position;
	std::string m_observationsFactors;
	std::string m_separateurListe;
	std::string m_symboleDecimal;

	CFieldsDefinitions m_fields;


	CStudyDefinition()
	{
		m_largeurVial = 50;
		m_position = -1;
		m_observationsFactors = "100;90;80;70";
		m_separateurListe = ",";
		m_symboleDecimal = ".";
	}



	CStudyDefinition(const CStudyDefinition& in)
	{
		operator=(in);
	}

	CStudyDefinition& operator = (const CStudyDefinition& in)
	{
		if (&in != this)
		{
			m_title = in.m_title;
			m_firstVial = in.m_firstVial;
			m_lastVial = in.m_lastVial;
			m_responsable = in.m_responsable;
			m_description = in.m_description;
			m_metadata = in.m_metadata;
			m_validation = in.m_validation;
			m_largeurVial = in.m_largeurVial;
			m_position = in.m_position;
			m_observationsFactors = in.m_observationsFactors;
			m_separateurListe = in.m_separateurListe;
			m_symboleDecimal = in.m_symboleDecimal;
			m_fields = in.m_fields;

		}

		ASSERT(*this == in);

		return *this;
	}

	bool operator == (const CStudyDefinition& in)const
	{
		bool bEqual = true;

		if (m_title != in.m_title) bEqual = false;
		if (m_firstVial != in.m_firstVial) bEqual = false;
		if (m_lastVial != in.m_lastVial) bEqual = false;
		if (m_responsable != in.m_responsable) bEqual = false;
		if (m_description != in.m_description) bEqual = false;
		if (m_metadata != in.m_metadata) bEqual = false;
		if (m_validation != in.m_validation) bEqual = false;
		if (m_largeurVial != in.m_largeurVial) bEqual = false;
		if (m_position != in.m_position) bEqual = false;
		if (m_observationsFactors != in.m_observationsFactors) bEqual = false;
		if (m_separateurListe != in.m_separateurListe)bEqual = false;
		if (m_symboleDecimal != in.m_symboleDecimal)bEqual = false;
		if (m_fields != in.m_fields) bEqual = false;

		return bEqual;
	}

	bool operator != (const CStudyDefinition& in)const{ return !operator==(in); }

	size_t GetNbVials()const{ return (!m_lastVial.empty() && !m_firstVial.empty()) ? (stdString::ToSizeT(m_lastVial) - stdString::ToSizeT(m_firstVial) + 1):0; }
	ERMsg VerifyNames()const;

	bool HaveObservationsDates()const;

};


typedef std::map<std::string, CStudyDefinition> CStudiesDefinitions;

//
//class  CStudiesDefinitions : public std::map<std::string, CStudyDefinition>
//{
//public:
//
//	bool operator == (const CStudiesDefinitions& in)const
//	{
//		return map_compare(*this, in);
//	}
//
//	bool operator != (const CStudiesDefinitions& in)const{ return !operator==(in); }
//	
//};


namespace zen
{
	template <> inline
		void writeStruc(const CFieldsDefinitions& in, XmlElement& output)
	{
			std::set<std::pair<size_t, std::string>> positions;

			for (CFieldsDefinitions::const_iterator it = in.begin(); it != in.end(); it++)
				positions.insert(std::pair<size_t, std::string>(it->second.m_position, it->first));

			for (auto it = positions.begin(); it != positions.end(); it++)
//			for (auto it = in.begin(); it != in.end(); it++)
			{
				XmlElement& newChild = output.addChild(CStudyDefinition::FIELD_XML);
				newChild.setAttribute("ID", it->second);
				zen::writeStruc(in.at(it->second), newChild);
			}
	}

	template <> inline
		bool readStruc(const XmlElement& input, CFieldsDefinitions& out)
	{
			out.clear();

			int pos = 0;
			auto iterPair = input.getChildren(CStudyDefinition::FIELD_XML);
			for (auto it = iterPair.first; it != iterPair.second; ++it)
			{
				CFieldDefinition field;

				std::string ID;
				it->getAttribute("ID", ID);
				if (zen::readStruc(*it, field))
				{
					out[ID] = field;
					out[ID].m_position = pos;
					pos++;
				}
			}

			return true;
	}

	template <> inline
		void writeStruc(const CStudyDefinition& in, XmlElement& output)
	{
			XmlOut out(output);

			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::TITLE]](in.m_title);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::RESPONSABLE]](in.m_responsable);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::DESCRIPTION]](in.m_description);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::METADATA]](in.m_metadata);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::VALIDATION]](in.m_validation);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::PREMIER_NO_VIAL]](in.m_firstVial);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::DERNIER_NO_VIAL]](in.m_lastVial);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::LARGEUR_VIAL]](in.m_largeurVial);
			//out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::POSITION]](in.m_position+1);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::FACTEURS_OBSERVATIONS]](in.m_observationsFactors);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::SEPARATEUR_LISTE]](in.m_separateurListe);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::SYMBOLE_DECIMAL]](in.m_symboleDecimal);
			out[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::CHAMPS]](in.m_fields);
	}

	template <> inline
		bool readStruc(const XmlElement& input, CStudyDefinition& out)
	{
			
			XmlIn in(input);

			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::TITLE]](out.m_title);
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::RESPONSABLE]](out.m_responsable);
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::DESCRIPTION]](out.m_description);
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::METADATA]](out.m_metadata);
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::VALIDATION]](out.m_validation);
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::PREMIER_NO_VIAL]](out.m_firstVial);
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::DERNIER_NO_VIAL]](out.m_lastVial);
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::LARGEUR_VIAL]](out.m_largeurVial);
			//in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::POSITION]](out.m_position); out.m_position--;//zero base
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::FACTEURS_OBSERVATIONS]](out.m_observationsFactors);
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::SEPARATEUR_LISTE]](out.m_separateurListe);
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::SYMBOLE_DECIMAL]](out.m_symboleDecimal);
			in[CStudyDefinition::MEMBERS_NAMES[CStudyDefinition::CHAMPS]](out.m_fields);

			//ERMsg msg = out.BuildPositions();
			//if (!msg)
				//throw msg;

			return true;
	}

	
}


//****************************************************************************************************************************
//CStudyData

typedef CFL::CMatrix<std::string> CCSVData;
class CStudyData : public CCSVData
{
public:

	//std::vector<std::string> m_header;

	CStudyData();

	bool Modified()const{ return m_bModifier; }
	void Modified(bool in){ m_bModifier = in; }

	bool operator == (const CStudyData& in)const{ return CCSVData::operator==(in); }
	bool operator != (const CStudyData& in)const{ return !operator==(in); }

//	CTRef GetTRef(size_t row)const;
	

protected:

	bool m_bModifier;
	//std::vector<std::set<std::string>::const_iterator> m_TRefPos;
};

typedef std::map<std::string, CStudyData> CCSVDataMap;


class CDPTProject;

class CStudiesData : public CCSVDataMap
{
public:

	ERMsg LoadCSV(const CDPTProject& studies);
	ERMsg SaveCSV(const CDPTProject& studies)const;

	bool operator == (const CStudiesData& in)const{ return map_compare(*this, in); }
	bool operator != (const CStudiesData& in)const{ return !operator==(in); }
};


//*********************************************************************************************************************
//CDPTProjectProperties
class CDPTProjectProperties
{
public:

	enum TMembre{ LAST_STUDY, LAST_ROW, LAST_COL, EDITABLE, TAB_WIDTH, NB_MEMBRES };
	static const char* MEMBERS_NAMES[NB_MEMBRES];
	
	
	std::string m_studyName;
	long m_curRow;
	int m_curCol;
	BOOL m_bEditable;
	int m_tabWidth;

	CDPTProjectProperties()
	{
		m_bEditable = false;
		m_curRow=-1;
		m_curCol=-1;
		m_tabWidth = 300;
	}

	CDPTProjectProperties(const CDPTProjectProperties& in)
	{
		operator=(in);
	}

	CDPTProjectProperties& operator = (const CDPTProjectProperties& in)
	{
		if (&in != this)
		{
			m_studyName = in.m_studyName;
			m_curRow = in.m_curRow;
			m_curCol = in.m_curCol;
			m_bEditable = in.m_bEditable;
			m_tabWidth = in.m_tabWidth;
		}

		ASSERT(*this == in);

		return *this;
	}

	bool operator == (const CDPTProjectProperties& in)const
	{
		bool bEqual = true;
		if (m_studyName != in.m_studyName)bEqual = false;
		if (m_curRow != in.m_curRow)bEqual = false;
		if (m_curCol != in.m_curCol)bEqual = false;
		if (m_bEditable != in.m_bEditable)bEqual = false;
		if (m_tabWidth != in.m_tabWidth)bEqual = false;

		return bEqual;
	}

	bool operator != (const CDPTProjectProperties& in)const{ return !operator==(in); }
};

namespace zen
{

template <> inline
void writeStruc(const CDPTProjectProperties& in, XmlElement& output)
{
	XmlOut out(output);
	out[CDPTProjectProperties::MEMBERS_NAMES[CDPTProjectProperties::LAST_STUDY]](in.m_studyName);
	out[CDPTProjectProperties::MEMBERS_NAMES[CDPTProjectProperties::LAST_ROW]](in.m_curCol);
	out[CDPTProjectProperties::MEMBERS_NAMES[CDPTProjectProperties::LAST_COL]](in.m_curRow);
	out[CDPTProjectProperties::MEMBERS_NAMES[CDPTProjectProperties::EDITABLE]](in.m_bEditable);
	out[CDPTProjectProperties::MEMBERS_NAMES[CDPTProjectProperties::TAB_WIDTH]](in.m_tabWidth);
	
	
	
}

template <> inline
bool readStruc(const XmlElement& input, CDPTProjectProperties& out)
{
	XmlIn in(input);
	
	in[CDPTProjectProperties::MEMBERS_NAMES[CDPTProjectProperties::LAST_STUDY]](out.m_studyName);
	in[CDPTProjectProperties::MEMBERS_NAMES[CDPTProjectProperties::LAST_ROW]](out.m_curCol);
	in[CDPTProjectProperties::MEMBERS_NAMES[CDPTProjectProperties::LAST_COL]](out.m_curRow);
	in[CDPTProjectProperties::MEMBERS_NAMES[CDPTProjectProperties::EDITABLE]](out.m_bEditable);
	in[CDPTProjectProperties::MEMBERS_NAMES[CDPTProjectProperties::TAB_WIDTH]](out.m_tabWidth);

	return true;
}

}


//*********************************************************************************************************************
class CDPTProject
{
public:

	enum TMembre{ PROPERTIES, DEFINITIONS, DATA, NB_MEMBRES };
	static const char* PROJECT_XML;
	static const char* MEMBERS_NAMES[NB_MEMBRES];
	
	CStudiesDefinitions m_studiesDefinitions;
	CStudiesData m_studiesData;
	CDPTProjectProperties m_properties;


	CDPTProject()
	{}

	~CDPTProject()
	{}

	CDPTProject(const CDPTProject& in)
	{
		operator=(in);
	}

	CDPTProject& operator = (const CDPTProject& in)
	{
		if (&in != this)
		{
			m_studiesDefinitions = in.m_studiesDefinitions;
			m_studiesData = in.m_studiesData;
			m_properties = in.m_properties;
			m_filePath = in.m_filePath;
		}

		ASSERT(*this == in);
 
		return *this;
	}

	bool operator == (const CDPTProject& in)const
	{ 
		bool bEqual = true;
		
		if (m_studiesDefinitions != in.m_studiesDefinitions)bEqual = false;
		if (m_studiesData != in.m_studiesData)bEqual = false;
//		if (m_properties != in.m_properties)bEqual = false;

		return bEqual;
	}

	bool operator != (const CDPTProject& in)const{ return !operator==(in); }
	

	std::string GetFilePath()const{ return m_filePath; }
	void SetFilePath(const std::string& in){ m_filePath = in; }

	std::string GetProjectFilePath(const std::string& name)const{ ASSERT(!m_filePath.empty());  return CFL::GetPath(m_filePath) + CFL::GetFileTitle(m_filePath) + " " + name + ".csv"; }
	std::string GetBackupFilePath(const std::string& name)const;
	
	ERMsg VerifieVialNo()const;
	ERMsg VerifyNames()const;
	//ERMsg BuildPositions();

	
protected:

	std::string m_filePath;
};

typedef std::shared_ptr<CDPTProject> CDPTProjectPtr;
typedef std::weak_ptr<CDPTProject> CDPTProjectWeakPtr;

namespace zen
{
	template <> inline
		void writeStruc(const CStudiesDefinitions& in, XmlElement& output)
	{
			std::set<std::pair<size_t, std::string>> positions;

			for (CStudiesDefinitions::const_iterator it = in.begin(); it != in.end(); it++)
				positions.insert(std::pair<size_t, std::string>(it->second.m_position, it->first));

			//std::sort(positions.begin(), positions.end());

			//for (size_t j = 0; j < positions.size(); j++)
			for (auto it = positions.begin(); it != positions.end(); it++)
//			for (auto it = in.begin(); it != in.end(); it++)
			{
				XmlElement& newChild = output.addChild(CDPTProject::PROJECT_XML);
				newChild.setAttribute("ID", it->second);
				zen::writeStruc(in.at(it->second), newChild);
			}

	}

	template <> inline
		bool readStruc(const XmlElement& input, CStudiesDefinitions& out)
	{
			out.clear();

			int pos = 0;
			auto iterPair = input.getChildren(CDPTProject::PROJECT_XML);
			for (auto it = iterPair.first; it != iterPair.second; ++it)
			{
				CStudyDefinition study;

				std::string ID;
				it->getAttribute("ID", ID);
				if (zen::readStruc(*it, study))
				{
					out[ID] = study;
					out[ID].m_position = pos;
					pos++;
				}
			}

			return true;
	}

	template <> inline
		void writeStruc(const CDPTProject& in, XmlElement& output)
	{
		XmlOut out(output);
		out[CDPTProject::MEMBERS_NAMES[CDPTProject::PROPERTIES]](in.m_properties);
		out[CDPTProject::MEMBERS_NAMES[CDPTProject::DEFINITIONS]](in.m_studiesDefinitions);
			
		ERMsg msg = in.m_studiesData.SaveCSV(in);
		if (!msg)
			throw msg;
		
	}

	template <> inline
		bool readStruc(const XmlElement& input, CDPTProject& out)
	{
			XmlIn in(input);
			in[CDPTProject::MEMBERS_NAMES[CDPTProject::PROPERTIES]](out.m_properties);
			in[CDPTProject::MEMBERS_NAMES[CDPTProject::DEFINITIONS]](out.m_studiesDefinitions);
			
			if (out.m_studiesDefinitions.find(out.m_properties.m_studyName) == out.m_studiesDefinitions.end() )
				out.m_properties.m_studyName.clear();
			
			ERMsg msg = out.m_studiesData.LoadCSV(out);
			if (!msg)
				throw msg;

			for (CStudiesDefinitions::iterator it = out.m_studiesDefinitions.begin(); it != out.m_studiesDefinitions.end(); it++)
			{
				CStudyData& data = out.m_studiesData[it->first];
				CStudyDefinition& study = it->second;
				if (data.size_y() != study.GetNbVials() && data.size_x() != study.m_fields.size())
				{
					data.resize(study.GetNbVials(), study.m_fields.size());
				}

				ASSERT(data.size_y() == study.GetNbVials() && data.size_x() == study.m_fields.size());
			}

			//build position
			//out.BuildPositions();

			return true;
	}

}
