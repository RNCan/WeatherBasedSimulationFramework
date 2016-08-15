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
// 01-10-1998	Rémi Saint-Amant	Initiale version 
//******************************************************************************
#include "stdafx.h"
#include "FileManager/BaseFile.h"

using namespace std;

namespace WBSF
{
	//****************************************************************************
	// Sommaire:    Constructeur par défaut.
	//
	// Description: Crée la classe et assigne le path par default
	//
	// Entrée:		const std::string& sPath: le path 
	//
	// Sortie:
	//
	// Note:		mère des classes qui jouent sur le disque
	//****************************************************************************
	CBaseFile::CBaseFile(const std::string& path)
	{
		SetPath(path);
	}

	//****************************************************************************
	// Sommaire:     Destructeur de la classe
	//
	// Description:  Libères la mémoire si la BD est encore ouverte
	//
	// Entrée:
	//
	// Sortie:
	//
	// Note:
	//****************************************************************************
	CBaseFile::~CBaseFile()
	{}

	//****************************************************************************
	// Sommaire:    Spécifie un nouveau path
	//
	// Description: Change le path par default de BD. 
	//
	// Entrée:		const std::string& path: le nouveau path
	//
	// Sortie:		bool: true: si le path est changer
	//				false: Si c'est le même path
	//
	// Note:
	//****************************************************************************
	bool CBaseFile::SetPath(const std::string& path)
	{
		//ASSERT(pathlength() >= 2); // path must be valid

		std::string tmpPath = SimplifyFilePath(TrimConst(path));

		if (!IsPathEndOk(tmpPath))
			tmpPath += '\\';


		bool bRep = false;
		if (tmpPath != m_path)
		{
			m_path = tmpPath;
			bRep = true;
		}

		return bRep;
	}

	string CBaseFile::GetFilePath(const string& fileName, const std::string& fileExtention)const
	{
		std::string filePath;

		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath(fileName.c_str(), drive, dir, fname, ext);
		if (strlen(drive) == 0 || strlen(dir) == 0)
			filePath = m_path;
		else
		{
			filePath = drive;
			filePath += dir;
		}

		filePath += fname;

		if (strlen(ext) == 0)
			filePath += fileExtention;
		else
			filePath += ext;



		return filePath;
	}

}