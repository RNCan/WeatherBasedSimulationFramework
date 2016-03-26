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
#include "FileManager/LocationDirectoryManager.h"




namespace WBSF
{
	const char* CLocationDirectoryManager::SUB_DIR_NAME = "Loc\\";

	CLocationDirectoryManager::CLocationDirectoryManager(const std::string& projectPath) :
		CDirectoryManager(true, false, SUB_DIR_NAME, FILE_TITLE, ".csv", true)
	{
		SetLocalBasePath(projectPath);
	}


	CLocationDirectoryManager::~CLocationDirectoryManager(void)
	{
	}

	CLocationDirectoryManager::CLocationDirectoryManager(const CLocationDirectoryManager& DM)
	{
		operator=(DM);
	}

	CLocationDirectoryManager& CLocationDirectoryManager::operator=(const CLocationDirectoryManager& NDM)
	{
		CDirectoryManager::operator=(NDM);

		return *this;
	}

	/*ERMsg CLocationDirectoryManager::OpenFile(const std::string& fileName, CForecastFile& forecastFile, short openMode)const
	{
	ERMsg msg;

	if( forecastFile.IsOpen())
	forecastFile.Close();

	std::string filePath;
	msg = GetFilePath( fileName, filePath );
	if(msg)
	msg = forecastFile.Open( filePath, openMode );


	return msg;
	}

	ERMsg CLocationDirectoryManager::CreateNewDataBase(int directoryIndex, const std::string& fileName)const
	{
	ERMsg msg;
	std::string filePath = GetFilePath( fileName );
	if( !filePath.empty() )
	{
	std::string error;
	error.FormatMessage(IDS_FM_FILE_ALREADY_EXIST, fileName, UtilWin::GetPath(filePath) );
	msg.ajoute((LPCTSTR)error);

	return msg;
	}

	if( UtilWin::CreateMultipleDir(m_directoryArray[directoryIndex]) )
	{
	ASSERT( !UtilWin::FileExist(GetFilePath( m_directoryArray[directoryIndex], fileName )) );
	msg = CForecastFile::CreateDatabase(GetFilePath( m_directoryArray[directoryIndex], fileName ));
	}
	else
	{
	//a faire
	// deplacer CreateMultipleDie ver un autre fichier que UtilWin
	// et transferer les string du fichier ResString vers
	// les ressources associer
	}

	return msg;
	}
	*/

	void CLocationDirectoryManager::ConvertLoc2CSV()const
	{
		/*	StringVector fileArray;
			fileArray = CDirectoryManagerBase::GetFilesList(".loc", FILE_PATH);

			for(int i=0; i<fileArray.size(); i++)
			{
			//	TRY
			{
			CStdString newName = fileArray[i];
			newName.SetFileExtension( ".csv");
			newName = GenerateNewName( newName );

			int format = CLocationVector::GetFormat(fileArray[i]);
			if( format>=CLocationVector::CSV_FORMAT3)
			{
			//it's already a .csv file, only chnage extension
			//extra column will be keept
			rename( fileArray[i], newName );
			}
			else
			{
			std::string backupName = fileArray[i];
			SetFileExtension( backupName, ".backup");
			backupName = GenerateNewName( backupName );
			//very old format, open it and save it as CSV file
			CLocationVector loc;
			if( loc.Load(fileArray[i]) )
			{
			loc.Save(newName);
			}

			//try to rename whatever happen
			rename( fileArray[i], backupName );

			}
			}
			//CATCH_ALL(e)
			{
			//do nothing
			}
			//END_CATCH_ALL
			}
			*/
	}

	//void CLocationDirectoryManager::GetFilesList(StringVector& fileArray, bool fullPath)const
	//{
	//	CDirectoryManager::GetFilesList(fileArray, fullPath);
	//}

	ERMsg CLocationDirectoryManager::Get(const std::string& fileName, CLocationVector& locArray)const
	{
		ERMsg msg;

		std::string filePath;
		msg = GetFilePath(fileName, filePath);
		if (msg)
			msg = locArray.Load(filePath);


		return msg;

	}

	ERMsg CLocationDirectoryManager::Set(const std::string& fileName, CLocationVector& locArray)const
	{
		//ASSERT( !GetFileExtension(fileName).empty() );//Loc file must have extension now

		ERMsg msg;

		std::string filePath;
		filePath = GetFilePath(fileName);

		if (filePath.empty())
			filePath = GetFilePath(GetLocalPath(), fileName);

		ASSERT(!filePath.empty());

		msg = CreateMultipleDir(GetPath(filePath));
		if (msg)
		{
			msg = locArray.Save(filePath);
		}

		return msg;

	}
}