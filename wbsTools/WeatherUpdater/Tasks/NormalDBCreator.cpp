#include "StdAfx.h"
#include "NormalDBCreator.h"
//#include "ormalStation.h"
#include "NormalsDatabase.h"
#include "dailyStation.h"


#include "SYShowMessage.h"
#include "CommonRes.h"
#include "FileManagerRes.h"
#include "BasicRes.h"
#include "Resource.h"
#include "AdvancedNormalStation.h"


//#include <shlwapi.h>
//#include <atlpath.h>

using namespace std; 
using namespace stdString; 
using namespace CFL;
//using namespace CFL;
//*********************************************************************

const char* CNormalDBCreator::ATTRIBUTE_NAME[NB_ATTRIBUTE]={"NbYears"};
const char* CNormalDBCreator::CLASS_NAME = "NormalDatabase";
const short CNormalDBCreator::NB_DAY_PER_MONTH_MIN=20;

CNormalDBCreator::CNormalDBCreator(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		StringVector options;
		options.push_back( GetString( IDS_STR_FILTER_NORMALS) );
		InitClass(options);
	}

	Reset();
}

void CNormalDBCreator::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString( IDS_SOURCENAME_CREATOR_NORMAL );

	CWeatherCreator::InitClass(option);
	//init static 
	ASSERT( GetParameters().size() < I_NB_ATTRIBUTE);

	StringVector header(IDS_PROPERTIES_CREATOR_NORMAL, "|;");
	ASSERT(header.size() == NB_ATTRIBUTE);

	GetParameters().push_back(CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[0], header[0]));
}

CNormalDBCreator::~CNormalDBCreator(void)
{
}


CNormalDBCreator::CNormalDBCreator(const CNormalDBCreator& in)
{
	operator=(in);
}

void CNormalDBCreator::Reset()
{
	CWeatherCreator::Reset();

	m_nbYearMin = 10;

}

CNormalDBCreator& CNormalDBCreator::operator =(const CNormalDBCreator& in)
{
	if( &in != this)
	{
		CWeatherCreator::operator =(in);
		m_nbYearMin = in.m_nbYearMin;

	}

	return *this;
}

bool CNormalDBCreator::operator ==(const CNormalDBCreator& in)const
{
	bool bEqual = true;

	if( CWeatherCreator::operator !=(in) )bEqual = false;
	if( m_nbYearMin != in.m_nbYearMin)bEqual = false;

	
	return bEqual;
}

bool CNormalDBCreator::operator !=(const CNormalDBCreator& in)const
{
	return !operator ==(in);
}


string CNormalDBCreator::GetValue(size_t type)const
{
	string str;
	
	ASSERT( NB_ATTRIBUTE == 1); 
	switch(type)
	{
	case I_NB_YEAR: str= ToString(m_nbYearMin); break;
	default: str = CWeatherCreator::GetValue(type); break;
	}

	return str;
}

void CNormalDBCreator::SetValue(size_t type, const string& str)
{
	ASSERT( NB_ATTRIBUTE == 1); 
	switch(type)
	{
	case I_NB_YEAR: m_nbYearMin=ToInt(str); break;
	default: CWeatherCreator::SetValue(type, str ); break;
	}

}


ERMsg CNormalDBCreator::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	callback.PushLevel();

	//load the WeatherUpdater
	CTask updaterTask; 
	msg = updaterTask.LoadFromDoc(m_updaterName);

	if( msg ) 
	{
		TRY
		{
			CUIWeather& weatherUpdater = dynamic_cast<CUIWeather&>(updaterTask.GetP());

			
			weatherUpdater.SetValue(CUIWeather::I_FIRST_YEAR, GetValue(I_FIRST_YEAR) );
			weatherUpdater.SetValue(CUIWeather::I_LAST_YEAR, GetValue(I_LAST_YEAR) );

			string outputFilePath = GetOutputFilePath(CNormalsDatabase::DATABASE_EXT);
			
			if( m_bDeleteOldDB )
				msg = CNormalsDatabase().DeleteDatabase(outputFilePath, callback);
			

			if( msg )
				msg = CreateDatabase(weatherUpdater, callback);

		}
		CATCH_ALL(e)
		{
			msg = SYGetMessage(*e);
			msg.ajoute( GetString( IDS_INVALID_INPUT_TASK) );
		}
		END_CATCH_ALL
	}

	callback.PopLevel();

	return msg;
}

ERMsg CNormalDBCreator::CreateDatabase(CUIWeather& weatherUpdater, CFL::CCallback& callback)const
{
	ERMsg msg;

	string outputFilePath = GetOutputFilePath(CNormalsDatabase::DATABASE_EXT);
	
	callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	callback.AddMessage(outputFilePath, 1);

	
	//SetFileExtension( outputFilePath, ".Normals");
		//Get the data for each station
	CNormalsDatabase normalDB;
	msg = normalDB.Open( outputFilePath, CNormalsDatabase::modeEdit);
	if( !msg)
		return msg;

	int firstYear = ToInt( weatherUpdater.GetValue( CUIWeather::I_FIRST_YEAR ));
	int lastYear = ToInt( weatherUpdater.GetValue( CUIWeather::I_LAST_YEAR ));
		
//	normalDB.SetBeginYear(firstYear);
	//normalDB.SetEndYear(lastYear);

	int nbStationAdded = 0;
	//find all station in the directories
	StringVector allStation;

	//weatherUpdater.GetNbStation();
	msg = weatherUpdater.PreProcess(callback);
	if( msg)
	{
		StringVector stationList;
		msg = weatherUpdater.GetStationList(stationList, callback);
		if( msg)
		{
			callback.SetCurrentDescription(GetString(IDS_CREATE_DATABASE));
			callback.SetNbStep(stationList.size());
			

			for (size_t i = 0; i<stationList.size() && msg; i++)
			{
				CDailyStation dailyStation;
				CAdvancedNormalStation station;

				ERMsg messageTmp = 	weatherUpdater.GetStation(stationList[i], dailyStation);

				if( messageTmp)
					messageTmp = station.FromDaily(dailyStation, m_nbYearMin);
	
				if( messageTmp)
				{
					string newName;
					int index = normalDB.GetStationIndex(station.m_name, true);
					while( index != -1)
					{
						newName = station.m_name + " " + ToString(i);
						index = normalDB.GetStationIndex(newName, true);
					}

					if( !newName.empty() )
					{
						station.m_name = newName;
					}

					messageTmp = normalDB.Add(station);
					if( messageTmp )
						nbStationAdded++;

				}

				if(! messageTmp )
					callback.AddMessage( messageTmp, 1);

				msg += callback.StepIt();
			}
		}
	}
	
	normalDB.Close();


	if( msg)
	{
		callback.AddMessage( GetString(IDS_STATION_ADDED) + ToString( nbStationAdded ) , 1);

		//open the file to create optimization now
		msg = normalDB.Open(outputFilePath);
		normalDB.Close();

		/*if(msg)
		{
			string outputFilePath2(outputFilePath);
			SetFileExtension(outputFilePath2, "OldNormals");

			CStdioFile fileIn( outputFilePath, CFile::modeRead);
			CStdioFile fileOut( outputFilePath2, CFile::modeWrite|CFile::modeCreate);
			
			string line;
			fileIn.ReadString(line);
			line.SetAt(29,'5');
			fileOut.WriteString( line.Left(11*8) + "\n");
			int i=0;
			while( fileIn.ReadString(line))
			{
				ASSERT( line.GetLength() == 15*8);
				if( (i%16) == 1) 
				{
					string line2;
					fileIn.ReadString(line2);
					i++;

					line.TrimRight();
					line = line.Left( line.GetLength()-1);
					line.TrimRight();
					line += " " + line2.Left(3);
					line += " 1";

					string tmp;
					tmp.Format( "%-88.88s", line );
					fileOut.WriteString( tmp.Left(11*8) + "\n"); 
				}
				else 
				{
					fileOut.WriteString( line.Left(11*8) + "\n"); 
				}
				//if( (i%16)/4 == 0) 
				//{
					//fileOut.WriteString( line.Left(11*8) + "\n"); 
				//}
				//else
				//{
				//	fileOut.WriteString( line.Left(11*8)+line.Right(4*8) + "\n");
				//}

				i++;
			}

			fileIn.Close();
			fileOut.Close();

		}*/
	}

	return msg;
}


/*ERMsg CNormalDBCreator::GetNormalStation(CDailyStation& dailyStation, CNormalsStation& normalStation, int nbYearMinimum)const
{
    ERMsg msg;

    //on vérifie la validité
    bool bValidTemperature= true;
    bool bValidPrecipitation= true;

//    msg = IsValidReference();
    
//    if( msg )
    msg = GetNormalValidity(dailyStation, nbYearMinimum, bValidTemperature, bValidPrecipitation);

    if( !msg )
        return msg;


    CMonthStatisticArray monthStatArray;
    GetMonthStatistic( dailyStation, monthStatArray );
	
	CMonthStatistic monthStat;
	CMonthStatistic dailyStat;

	for( int y=0; y<monthStatArray.size(); y++)
		for(int m=0; m<12; m++)
		{
			dailyStat[N_MIN][m] += monthStatArray[y][N_MIN][m];
			dailyStat[N_MAX][m] += monthStatArray[y][N_MAX][m];
			dailyStat[N_MEAN][m] += monthStatArray[y][N_MEAN][m];
			dailyStat[N_PPT][m] += monthStatArray[y][N_PPT][m];

			if( monthStatArray[y][N_MIN][m][CFL::NB_VALUE] > 0)
				monthStat[N_MIN][m] += monthStatArray[y][N_MIN][m][CFL::MEAN];
			if( monthStatArray[y][N_MAX][m][CFL::NB_VALUE] > 0)
				monthStat[N_MAX][m] += monthStatArray[y][N_MAX][m][CFL::MEAN];
			if( monthStatArray[y][N_MEAN][m][CFL::NB_VALUE] > 0)
				monthStat[N_MEAN][m] += monthStatArray[y][N_MEAN][m][CFL::MEAN];

			const CDailyYear& data = dailyStation(y);
			int nbDay = GetNbDayPerMonth(m+1, data.GetYear());
			if( monthStatArray[y][N_PPT][m][CFL::NB_VALUE] > 0)
				monthStat[N_PPT][m] += monthStatArray[y][N_PPT][m][CFL::MEAN]*nbDay;
		}

	double SumMin2[12] = {0};
	double SumMax2[12] = {0};
	double SumMin[12] = {0};
	double SumMax[12] = {0};
	double SumMinMax[12] = {0};
	int nbSum[12] = {0};
	

    double SDMean[12] = {0};
	double SDPrec[12] = {0};
	int nbPrec[12] = {0};
	int nbMean[12] = {0};
	
	
	//calcul du STD_DEV et du CV
	for(int y=0; y<dailyStation.GetNbYear(); y++)
    {
		const CDailyYear& data = dailyStation(y);
        for(int m=0; m<12; m++)
        {
			if( data.GetNbDayMin(m, true) > NB_DAY_PER_MONTH_MIN )
			{
				//double tmp = monthStatArray[y][MEAN][m][CFL::MEAN] - monthStat[MEAN][m][CFL::MEAN];
				ASSERT( monthStatArray[y][N_MEAN][m][CFL::NB_VALUE] > 0);

				double tmp = monthStatArray[y][N_MEAN][m][CFL::MEAN] - dailyStat[N_MEAN][m][CFL::MEAN];
				tmp *= tmp;

				SDMean[m] += tmp;
				nbMean[m]++;

				//***************** 
				int fd = GetJDay(1, m+1, data.GetYear() )-1;
				int nbDay = GetNbDayPerMonth(m+1, data.GetYear() );
				for(int i=fd; i<fd+nbDay; i++)
				{
					
					if( data[i].m_min > -999 && data[i].m_max > -999)
					{
						nbSum[m]++;
						
						double min = data[i].m_min - monthStatArray[y][N_MIN][m][CFL::MEAN];
						
						SumMin2[m] += CFL::Square(min);
						SumMin[m] += min;
						double max = data[i].m_max - monthStatArray[y][N_MAX][m][CFL::MEAN];
						
						SumMax2[m]  += CFL::Square(max);
						SumMax[m] += max;
						SumMinMax[m] += min*max;
					}
				}
			}


			if( data.GetNbDayMin(m, false) > NB_DAY_PER_MONTH_MIN)
			{
                //Ici, on estime la précipitation mensuel.
                //Dans le cas ou il y à des données manquante, la valeur estimer sera différente
                //de la valeur réel
				int nbDay = GetNbDayPerMonth(m+1, data.GetYear());
				double monthlyPrec = monthStatArray[y][N_PPT][m][CFL::MEAN]*nbDay;
				double tmp =  0;
				//tmp = monthlyPrec - monthStat[N_PPT][m][CFL::MEAN];//pour avoir des déviation standars au lieu des coéficient de variations
                if( monthStat[N_PPT][m][CFL::MEAN] > 0)
                    tmp = (monthlyPrec/monthStat[N_PPT][m][CFL::MEAN]) - 1;

				tmp*= tmp;

				SDPrec[m] += tmp;
				nbPrec[m] ++;
			}
        }
    }

    CNormalsData data;
		
	ASSERT(CNormalsData::NB_FIELD == 10);
	for(int m=0; m<12; m++)
	{
        if( bValidTemperature )
        {
			data[m].MinExt() = float(dailyStat[N_MIN][m][CFL::LOWEST]);
			data[m].MinMean() = float(dailyStat[N_MIN][m][CFL::MEAN]);
            data[m].Mean() = float(dailyStat[N_MEAN][m][CFL::MEAN]);
			data[m].MaxMean() = float(dailyStat[N_MAX][m][CFL::MEAN]);
			data[m].MaxExt() = float(dailyStat[N_MAX][m][CFL::HIGHEST]);
            
            ASSERT( nbMean[m] >= nbYearMinimum );
		    data[m].SD() = float( sqrt( SDMean[m]/(nbMean[m]-1) ));
			//data[m].r_minMax() = float((nbSum[m]*SumMinMax[m]-SumMax[m]*SumMin[m])/ sqrt( (nbSum[m]*SumMax2[m]-Square(SumMax[m]))*(nbSum[m]*SumMin2[m]-Square(SumMin[m]))));
			data[m].r_minMax() = float(SumMinMax[m]/sqrt( SumMin2[m]*SumMax2[m]));
			
        }

		if( bValidPrecipitation )
		{
			data[m].Snow() = 0;
            data[m].Precipitation() = float(monthStat[N_PPT][m][CFL::MEAN]);
            
        
			ASSERT( nbPrec[m] >= nbYearMinimum);
            data[m].CV() = float( sqrt( SDPrec[m]/(nbPrec[m]-1) ));
		}
	}
	
    //Omn assigne la station
    //on pourrait vérifier ici s'il y a plusieur station avec le même nom
    //Si oui s'il on le même numéro on prend celle qui à le plus de données
    //sinon on change le nom en ajoutant le no de station
    ((CStation&)normalStation) = dailyStation;

    normalStation.SetData( data );

    return msg;
}

ERMsg CNormalDBCreator::GetNormalValidity(CDailyStation& dailyStation, int nbYearMin, bool& bValidTemperature, bool& bValidPrecipitation)const
{
	ERMsg msg;

	int nSize = dailyStation.GetNbYear();
    if( nSize > 0 )
    {
		
        int totalTemperature[12]={0};
        int totalPrecipitation[12]={0};

        for(int y=0; y<nSize; y++)
        {
			const CDailyYear& data = dailyStation(y);
			
            for(int m=0; m<12; m++)
            {
				
			    if( data.GetNbDayMin( m, true ) > NB_DAY_PER_MONTH_MIN )
			    {
				    totalTemperature[m]++;
			    }
            
                if( data.GetNbDayMin( m, false ) > NB_DAY_PER_MONTH_MIN)
			    {
				    totalPrecipitation[m] ++;
			    }
            }
        }
    

        bValidTemperature= true;
        bValidPrecipitation= true;
   
        for(int m=0; m<12; m++)
        {
            if( totalTemperature[m] < nbYearMin )
            {
                bValidTemperature = false;
            }

            if( totalPrecipitation[m]  < nbYearMin )
            {
                bValidPrecipitation = false;
            }
        }
    
    }
  
	if( !(bValidTemperature || bValidPrecipitation) )
	{
		msg.asgType( ERMsg::ERREUR );
	}

	return msg;
}

void CNormalDBCreator::GetMonthStatistic( CDailyStation& dailyStation, CMonthStatisticArray& monthStat)const
{
	int nSize = dailyStation.GetNbYear();
	monthStat.RemoveAll();
	monthStat.SetSize( nSize );


    for(int y=0; y<nSize; y++)
    {
		const CDailyYear& data = dailyStation(y);
        for(int m=0; m<12; m++)
        {
			if( data.GetNbDayMin( m, true ) > NB_DAY_PER_MONTH_MIN )
			{
				int fd = GetJDay(1, m+1, data.GetYear() )-1;
				int nbDay = GetNbDayPerMonth(m+1, data.GetYear() );
				for(int i=fd; i<fd+nbDay; i++)
				{
					int month = GetMonth(i+1, data.GetYear() );
					if( data[i].m_min > -999)
						monthStat[y][N_MIN][m] += data[i].m_min;

					if( data[i].m_max > -999)
						monthStat[y][N_MAX][m] += data[i].m_max;

					if( data[i].m_min > -999 && data[i].m_max > -999)
						monthStat[y][N_MEAN][m] += (data[i].m_min+data[i].m_max)/2;
				}
 
			}
            
            if( data.GetNbDayMin( m, false ) > NB_DAY_PER_MONTH_MIN)
			{
				int fd = GetJDay(1, m+1, data.GetYear() )-1;
				int nbDay = GetNbDayPerMonth(m+1, data.GetYear() );
				for(int i=fd; i<fd+nbDay; i++)
				{
					if( data[i].m_ppt > -999)
						monthStat[y][N_PPT][m] += data[i].m_ppt;
				}
//                monthStat[m].GetTotalSnow() += m_weather[y].GetMonth(m).GetTotalSnow();
  //              monthStat[m].GetNbSnow() += m_weather[y].GetMonth(m).GetNbSnow();
                //monthStat[m].m_nbMonthSnow++;
            }
            
        }
    }
}
*/
/*
void CNormalDBCreator::GetParameterValue(CParamInfo& param)const
{
	CWeatherCreator::GetParameterValue(param);

	StringVector array;
	
	array.Copy(param.GetParam());

	array.push_back( m_selection.ToString());

	param.SetParam(array);

}


void CNormalDBCreator::SetParameterValue(const CParamInfo& param)
{
	CWeatherCreator::SetParameterValue(param);

	const StringVector& array = param.GetParam();

	m_selection.FromString(array[I_PROVINCE]);
}
*/


bool CNormalDBCreator::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CNormalDBCreator& info = dynamic_cast<const CNormalDBCreator&>(in);
	return operator==(info);
}

CParameterBase& CNormalDBCreator::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CNormalDBCreator& info = dynamic_cast<const CNormalDBCreator&>(in);
	return operator=(info);
}

//Interface attribute index to attribute index
/*short CNormalDBCreator::IA2A(short IA)const
{
	//this interface is simple
	//there are only a title in the begining
	return 	IA - CWeatherCreator::I_NB_ATTRIBUTE;
}

//attribute index to Interface attribute index
short CNormalDBCreator::A2IA(short A)const
{
	//this interface is simple
	//there are only a title in the begining
	return 	A + CWeatherCreator::I_NB_ATTRIBUTE;
}
*/