// OptionPropSheet.cpp : implementation file
//

#include "stdafx.h"

#include "DPTProjet.h"
#include "resource.h"
#include "CSVFile.h"
#include "UtilTime.h"
//#include "Registry.h"




using namespace std;
using namespace stdString;
using namespace CFL;


//"Unite", "Defaut", 
const char* CFieldDefinition::MEMBERS_NAMES[NB_MEMBRES] = { "Titre", "Description", "Min", "Max", "Liste", "Couleur", "Largeur", "Position" };

const char* CStudyDefinition::FIELD_XML = "Champ";
const char* CStudyDefinition::MEMBERS_NAMES[NB_MEMBRES] = { "Titre", "Responsable", "Description", "Metadonnee", "Validation", "PremierNoVial", "DernierNoVial", "LargeurVial", "Position", "FacteursObservations", "SeparateurListe", "SymboleDecimal", "Champs" };

const char* CDPTProjectProperties::MEMBERS_NAMES[NB_MEMBRES] = { "EtudeActive", "LigneActive", "ColonneActive", "Editable", "LargeurEtiquette" };

const char* CDPTProject::PROJECT_XML = "Etude";
const char* CDPTProject::MEMBERS_NAMES[NB_MEMBRES] = {"Proprietees", "Etudes", "Donnees" };


//****************************************************************************************************************************
//CFieldDefinition


static bool is_digits(const std::string &str)
{
	return std::all_of(str.begin(), str.end(), ::isdigit); // C++11
}

string CFieldDefinition::GetCode(const string& value)const
{
	string str;
	if (!value.empty())
	{
		if (!m_codesList.empty())
		{
			if (m_codesList != m_lastCodesBuild)
			{
				GetCodes(const_cast<CFieldDefinition*>(this)->m_codes);
				const_cast<CFieldDefinition*>(this)->m_lastCodesBuild = m_codesList;
			}

			std::map<std::string, std::string>::const_iterator it = m_codes.find(value);
			if (it != m_codes.end())
				str = it->second;
		}
	}

	return str;
}

bool CFieldDefinition::IsValid(const std::string value)const
{
	bool bValid = true;
	if (!value.empty())
	{
		if (m_codesList.empty())
		{
			if (!m_min.empty() && !m_max.empty())
			{
				double dMin = ToDouble(m_min);
				double dMax = ToDouble(m_max);
				double dValue = ToDouble(value);
				bValid = dValue >= dMin && dValue <= dMax;
			}
		}
		else
		{
			if (m_codesList != m_lastCodesBuild)
			{
				GetCodes(const_cast<CFieldDefinition*>(this)->m_codes);
				const_cast<CFieldDefinition*>(this)->m_lastCodesBuild = m_codesList;
			}
			
			bValid = m_codes.find(value) != m_codes.end();
		}
	}

	return bValid;
}


void CFieldDefinition::GetCodes(const std::string& str, std::map<std::string, std::string>& codes)
{
	codes.clear();

	StringVector codesList(str, "{}");
	for (StringVector::const_iterator it = codesList.begin(); it!=codesList.end(); it++)
	{
		StringVector code(*it, "|");
		if (code.size() == 2 && is_digits(code[0]))
		{
			codes[code[0]] = code[1];
		}
	}

}
//****************************************************************************************************************************
//CStudyDefinition

ERMsg CStudyDefinition::VerifyNames()const
{
	ERMsg msg;

	set<string> names;

	for (CFieldsDefinitions::const_iterator it = m_fields.begin(); it != m_fields.end(); it++)
	{
		if (names.find(it->first) == names.end())
		{
			names.insert(it->first);
		}
		else
		{
			msg.ajoute(FormatMsg(IDS_NAME_ALREADY_EXIST, "champs", it->first));
		}
	}

	return msg;
}
//
//ERMsg CStudyDefinition::BuildPositions()
//{
//	ERMsg msg = VerifyNames();
//	if (!msg)
//		return msg;
//
//	vector<pair<size_t, string>> positions(m_fields.size());
//
//	for (CFieldsDefinitions::const_iterator it = m_fields.begin(); it != m_fields.end(); it++)
//		positions.push_back(pair<size_t, string>(it->second.m_position, it->first));
//
//	std::sort(positions.begin(), positions.end());
//
//	for (size_t j = 0; j < positions.size(); j++)
//	{
//		ASSERT(j == m_fields[positions[j].second].m_position);
//		m_fields[positions[j].second].m_position = (int)j;//assure good positions
//	}
//
//	return msg;
//}


bool CStudyDefinition::HaveObservationsDates()const
{
	bool bYear = m_fields.find("Annee") != m_fields.end() || m_fields.find("Year") != m_fields.end();
	bool bMonth = m_fields.find("Mois") != m_fields.end() || m_fields.find("Month") != m_fields.end();
	bool bDay = m_fields.find("Jour") != m_fields.end() || m_fields.find("Day") != m_fields.end();
		
	return bYear && bMonth && bDay;
}


//****************************************************************************************************************************
//CStudyData

CStudyData::CStudyData()
{
	m_bModifier=false;
	//m_TRefPos.fill(UNKNOWN_POS);
}

//CTRef CStudyData::GetTRef(size_t row)const
//{
//	CTRef TRef;
//
//	if (m_TRefPos.empty())
//	{
//		CStudyData& me = const_cast<CStudyData&>(*this);
//		
//		std::set<std::string>::const_iterator itYear = m_header.find("Annee");
//		if (itYear == m_header.end())
//			itYear = m_header.find("Year");
//
//		std::set<std::string>::const_iterator itMonth = m_header.find("Mois");
//		if (itMonth == m_header.end())
//			itMonth = m_header.find("Month");
//
//		std::set<std::string>::const_iterator  itDay = m_header.find("Jour");
//		if (itDay == m_header.end())
//			itDay = m_header.find("Day");
//
//
//		if (itYear != m_header.end() &&
//			itMonth != m_header.end() &&
//			itDay != m_header.end())
//		{
//			me.m_TRefPos.push_back(itYear);
//			me.m_TRefPos.push_back(itMonth);
//			me.m_TRefPos.push_back(itDay);
//	
//		}
//		else
//		{
//			me.m_TRefPos.push_back(m_header.end());
//		}
//	}
//
//	if (m_TRefPos[0] != m_header.end())
//	{
//		int year = ToInt(*m_TRefPos[0]);
//		int month = ToInt(*m_TRefPos[1]);
//		int day = ToInt(*m_TRefPos[2]);
//
//		TRef = CTRef(year, month - 1, day - 1);
//	}
//
//	return TRef;
//}

ERMsg GetFieldsPositions(const StringVector& header, const CFieldsDefinitions& fields, vector<size_t>& positions)
{
	ERMsg msg;
	positions.clear();
	//set<size_t> fieldsDone;

	//for (size_t j = 0; j < header.size(); j++)
	for (StringVector::const_iterator it = header.begin(); it != header.end(); it++)
	{
		string::size_type pos = 0;
		string name = stdString::TrimConst(*it);//stdString::trim(stdString::Tokenize(*it, "[", pos));
		if (name != "Vial")
		{
			CFieldsDefinitions::const_iterator itField = fields.find(name);
			if (itField == fields.end())
			{
				msg.ajoute(FormatMsg(IDS_FIELDS_MISMATCH1, name));
			}
			else
			{
				positions.push_back(itField->second.m_position);
			}
		}
		/*else
		{
			positions.push_back(UNKNOWN_POS);
		}*/
	}

	//for (int i = 0; i != fields.size(); i++)
	for (CFieldsDefinitions::const_iterator it = fields.begin(); it != fields.end(); it++)
	{
		if (find(positions.begin(), positions.end(), it->second.m_position) == positions.end())
		{
			msg.ajoute(FormatMsg(IDS_FIELDS_MISMATCH2, it->first));
		}
	}


	ASSERT(!msg || positions.size() == fields.size());
	return msg;
}

ERMsg CStudiesData::LoadCSV(const CDPTProject& doc)
{
	ASSERT(!doc.GetFilePath().empty());
	ERMsg msg; 
	
	clear();


	CStudiesData& me = *this;
	

	const CStudiesDefinitions& studies = doc.m_studiesDefinitions;
	for (CStudiesDefinitions::const_iterator defIt = studies.begin(); defIt != studies.end()&&msg; defIt++)
	{
		string filePath = doc.GetProjectFilePath(defIt->first);
		
		if (CFL::FileExists(filePath))
		{
			ifStream fichier;
			msg = fichier.open(filePath);

			if (msg)
			{
				const CStudyDefinition& study = defIt->second;
				const CFieldsDefinitions& fields = study.m_fields;
				CStudyData& data = me[defIt->first];

				CSVIterator loop(fichier);
				//data.m_header.insert(loop.Header().begin(), loop.Header().end());


				vector<size_t> positions;
				msg = GetFieldsPositions(loop.Header(), fields, positions);
				
				
				if (msg)
				{
					//ASSERT(doc.m_positions.size() == positions.size());
					

					size_t nbVials = ToSizeT(study.m_lastVial) - ToSizeT(study.m_firstVial) + 1;
					data.resize(nbVials, positions.size() );

					
					size_t i = 0;
					for (; loop != CSVIterator() && i < data.size_y(); ++loop, i++)
					{
						for (size_t j = 0; j < data.size_x() && j < (*loop).size()-1; j++)//j==0 -> key
						{
							//if (positions[j] != UNKNOWN_POS)
							//{
							ASSERT(positions[j] < data.size_x());
							data[i][positions[j]] = (*loop)[j+1];
							//}
						}
					}//pour toutes les lignes
				}
				else
				{
					msg.ajoute(FormatMsg(IDS_DATA_FILE_MISMATCH, CFL::GetFileName(filePath), defIt->first));
				}

				fichier.close();
			}//overture avec succes.
		}//si le fichier existe
	}//pout tous les projets

	return msg;
}



ERMsg CStudiesData::SaveCSV(const CDPTProject& project)const
{
	ERMsg msg;
	for (CStudiesData::const_iterator it = begin(); it != end(); it++)
	{
		const CStudyData& data = it->second;

		//if (data.Modified())
		{
			const CStudiesDefinitions& studies = project.m_studiesDefinitions;
			CStudiesDefinitions::const_iterator defIt = studies.find(it->first);
			if (defIt != studies.end())//erased table
			{

				const CStudyDefinition& study = defIt->second;


				const CFieldsDefinitions & fields = study.m_fields;
				ASSERT(fields.size() == data.size_x());

				string filePath = project.GetProjectFilePath(it->first);
				ofStream fichier;
				msg += fichier.open(filePath);

				if (msg)
				{
					set<pair<size_t, string>> positions;

					for (CFieldsDefinitions::const_iterator it2 = fields.begin(); it2 != fields.end(); it2++)
						positions.insert(pair<size_t, string>(it2->second.m_position, it2->first));

					ASSERT(positions.size() == fields.size());
					//ecrire l'entête;
					//for (CFieldsDefinitions::const_iterator it2 = fields.begin(); it2 != fields.end(); it2++)

					fichier << "Vial";
					for (set<pair<size_t, string>>::const_iterator it2 = positions.begin(); it2 != positions.end(); it2++)
					{
						fichier << study.m_separateurListe;
						fichier << it2->second;
					}

					fichier << "\n";

					int permierVial = ToInt(study.m_firstVial);
					for (size_t y = 0; y < data.size_y(); y++)
					{
						fichier << (permierVial + y);
						for (size_t x = 0; x < data.size_x(); x++)
						{
							fichier << study.m_separateurListe;
							fichier << data[y][x];
						}

						fichier << "\n";
					}

					fichier.close();



					string backupFilePath = project.GetBackupFilePath(it->first);
					CFL::CreateMultipleDir(GetPath(backupFilePath));
					CopyFile(CString(filePath.c_str()), CString(backupFilePath.c_str()), FALSE);
				}
			}
		}
	}

	return msg;
}


//****************************************************************************************************************************
//CDPTProject
//
//ERMsg CDPTProject::OuvrirXML(std::string& filePath)
//{
//	ERMsg msg;
//
//	SetFilePath(filePath);
//
//	msg = zen::LoadXML(filePath, NOM_XML, *this);
//	//msg = m_studies.OuvrirXML(filePath);
//	if (msg)
//	{
//		BuildPositions();
//		msg = m_studiesData.OuvrirCSV(*this);
//	}
//
//	return msg;
//
//}
//
//ERMsg CDPTProject::SauvegardeXML(std::string& filePath)const
//{
//	ERMsg msg;
//
//	msg = zen::SaveXML(filePath, NOM_XML, *this);
//	msg += m_studiesData.SauvegardeCSV(*this);
//	
//	const_cast<CDPTProject*>(this)->SetFilePath(filePath);
//
//	return msg;
//}

ERMsg CDPTProject::VerifyNames()const
{
	ERMsg msg;

	set<string> names;

	for (CStudiesDefinitions::const_iterator it = m_studiesDefinitions.begin(); it != m_studiesDefinitions.end(); it++)
	{
		if (names.find(it->first) == names.end())
		{
			names.insert(it->first);
		}
		else
		{
			msg.ajoute( FormatMsg(IDS_NAME_ALREADY_EXIST, "projets", it->first));
		}
	}

	return msg;
}

//ERMsg CDPTProject::BuildPositions()
//{
//	ERMsg msg = VerifyNames();
//	if (!msg)
//		return msg;
//
//	vector<pair<size_t, string>> positions;
//	
//	for (CStudiesDefinitions::const_iterator it = m_studiesDefinitions.begin(); it != m_studiesDefinitions.end(); it++)
//		positions.push_back(pair<size_t, string>(it->second.m_position, it->first));
//
//	std::sort(positions.begin(), positions.end());
//
//	for (size_t j = 0; j < positions.size(); j++)
//	{
//		m_studiesDefinitions[positions[j].second].m_position = (int)j;//assure good positions
//	}
//
//
//	return msg;
//}

std::string CDPTProject::GetBackupFilePath(const std::string& name)const
{
	ASSERT(!m_filePath.empty());

	string dateStr = CFL::GetCurrentTimeString("%Y-%m-%d (%H.%M)");
	return CFL::GetPath(m_filePath) + "Copies\\" + CFL::GetFileTitle(m_filePath) + " " + name + " " + dateStr + ".csv";
}
