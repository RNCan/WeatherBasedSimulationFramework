//***********************************************************************
// 2.0.0  22/01/2018		Rémi Saint-Amant	Update with new framework
// 1.4.2  21/03/2013		Rémi Saint-Amant	add fuzzy threshold2
// 1.4.1  21/03/2013		Rémi Saint-Amant	Correction of problem with shift
// 1.4.0  18/03/2013		Rémi Saint-Amant	Add mode class. New GDALBase
// 1.3.0  26/03/2012		Rémi Saint-Amant	
// 1.2.0  20/03/2012		Rémi Saint-Amant	used of dynamic_bitset. Optimisation of memory
//***********************************************************************
// -nbPixel 1000 1000 -multi -overwrite -co COMPRESS=LZW "D:\Travail\LucGuindon\Dissolve\Input\out\shore_ne.tif" "D:\Travail\LucGuindon\Dissolve\Input\out\shore_ne_seave.tif"
//-NbPixel 10 20 -multi -Threshold 5 95 -PixelOff 0 -FuzzyZone MEAN 50 -overwrite "D:\Travail\LucGuindon\RegressionTree\Output\RT_0405.tif" "D:\Travail\LucGuindon\RegressionTree\Output\RT_0405_seiveNew.tif"
//-nbPixel 100 100 -multi -overwrite -co COMPRESS=LZW "D:\Travail\LucGuindon\Dissolve\Input\out\shore_ne.tif" "D:\Travail\LucGuindon\Dissolve\Input\out\shore_ne_seave.tif"

#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <boost\dynamic_bitset.hpp>
#include <deque>
#include <list>
#include <memory>
#include <array>
#include <iostream>

#include "Geomatic/GDALBasic.h"
#include "Basic/statistic.h"
#include "Basic/UtilMath.h"
#include "Basic/Timer.h"
#include "Basic/OpenMP.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;

namespace WBSF
{

	static const char* version = "2.0.0";


	enum TFilePath { INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
	enum TDefaultAction{ KEPT, REJECT };
	enum TType { BY_THRESHOLD, BY_CLASS };
	enum bandsStat{ B_LOWEST, B_MEAN, B_STD_DEV, B_HIGHEST, NB_STATS };
	static const short BANDS_STATS[NB_STATS] = { LOWEST, MEAN, STD_DEV, HIGHEST };

	class CSieveOption : public CBaseOptions
	{
	public:
		CSieveOption()
		{


			m_pixelOffValue = 0;
			m_n1 = 1;
			m_n2 = 1;
			m_th1 = 0;
			m_th2 = 0;
			m_FZStat = -1;
			m_FZTh1 = MISSING_DATA;
			m_FZTh2 = MISSING_DATA;
			m_b8Connection = true;
			m_replacedBy = MISSING_DATA;
			m_bDebug = false;
			m_bExportStats = false;
			m_defaultAction = KEPT;
			m_type = BY_THRESHOLD;




			static const COptionDef OPTIONS[14] =
			{
				{ "-Type", 1, "ByThreshold or ByClass", false, "Type of computation for zone 2, 3 and 4. ByThreshold by default." },
				{ "-PixelOff", 1, "value", false, "Value used to separate pixel \"ON\" and \"OFF\". 0 by default." },
				{ "-nbPixel", 2, "nb1 nb2", false, "Set size of first and second threshold in pixels. By default, nb1=2 and nb2=2" },
				{ "-4", 0, "", false, "Four connectedness should be used when determining connection. That is diagonal pixels are not considered directly connected." },
				{ "-8", 0, "", false, "Eight connectedness should be used when determining connection. That is diagonal pixels are considered directly connected. This is the default." },
				{ "-ReplacedBy", 0, "", false, "replacement value when pixel is reject. PixelOff by default." },
				{ "-DefaultAction", 1, "Kept or Reject", false, "Default action to do in the fuzzyZone. KEPT by default." },//(if type is ByThreshold) or when class is not explicitly keep or reject if type is ByClass. 
				{ "-Threshold", 2, "th1 th2", false, "Select first and second threshold. By Default, th1=0 and th2=0. Used when type is by threshold." },
				//{"-FuzzyZone",2,"FZStat FZThreshold",false,"Select the statistic and the statistic threshold. If type is ByThreshold, then DefaultAction is take If the statistic is greather than the threshold. If type is ByClass, then DefaultAction is taken if the statistic equal the threshold. By default, MEAN over threshold1 is selected. Used when type is by threshold."},
				{ "-FuzzyZone", 3, "FZStat FZTh1 FZTh2", false, "Select the statistic and the statistic thresholds for fuzzy zone. DefaultAction will be take If the statistic is greather or equal than FZTh1 and is lower or equal than FZTh2. By default, MEAN over th1 th2 is selected if type is ByThreshold and MODE 0 0 if type is ByClass." },
				{ "-Class", 1, "\"action: class 1, class 2, ..., class n\"", true, "Select a action (Kept or Reject) to be apply over a class or a combinaison of class. A action is taked if at least one pixel is found to every class. If no action is taked, then DefaultAction will be selected. Used when type is by class." },
				//{"-ClassOut",1,"\"class 1,class 2, ..., class n\"",true,"Select some class or combinaison of class to be reject. If a class is not keep or reject, then DefaultAction will be selected. Used when type is by class."},
				//class mode when class is not explicitly keep or reject
				{ "-Debug", 0, "", false, "Export extra grid with 2 bands. The zone(0..5) and the statistic of the FuzzyZone." },
				{ "-ExportStats", 0, "", false, "Export all statistics of the FuzzyZone." },
				{ "srcfile", 0, "", false, "Input images file path (no limits)." },
				{ "dstfile", 0, "", false, "Output image file path." }
			};


			for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
				AddOption(OPTIONS[i]);
		}

		virtual ERMsg ParseOption(int argc, char* argv[])
		{
			ERMsg msg = CBaseOptions::ParseOption(argc, argv);

			if (m_FZStat == -1)
			{
				if (m_type == BY_THRESHOLD)
				{
					m_FZStat = MEAN;
					m_FZTh1 = m_th1;
					m_FZTh2 = m_th2;
				}
				else
				{
					m_FZStat = MODE;
					m_FZTh1 = 0;
					m_FZTh2 = 0;
				}
			}

			//compute the threshold
			//if( m_FZThreshold==MISSING_DATA )
			//{
			//	if( m_type==BY_THRESHOLD )
			//		m_FZThreshold=m_th1;
			//	else 
			//		m_FZThreshold=-9999;
			//}

			if (m_replacedBy == MISSING_DATA)
				m_replacedBy = m_pixelOffValue;

			if (msg && m_filesPath.size() != 2)
				msg.ajoute("Invalid argument line. 2 files are needed: the source image to clean and the destination image.");

			//if(msg && !m_classIn.empty() && !m_classOut.empty())
			//{
			//	msg.ajoute("Only -ClassIn or -ClassOut can be used at a time" );
			//}

			//if(msg && !m_fuzzyClassIn.empty() && !m_fuzzyClassOut.empty())
			//{
			//	msg.ajoute("Only -FuzzyClassIn or -FuzzyClassOut can be used at a time" );
			//}

			return msg;
		}
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[])
		{
			ERMsg msg;

			if (IsEqual(argv[i], "-Type"))
			{
				string str(argv[++i]);
				//ToUpper(str);

				if (IsEqualNoCase(str, "BYTHRESHOLD"))
					m_type = BY_THRESHOLD;
				else if (IsEqualNoCase(str, "BYCLASS"))
					m_type = BY_CLASS;
				else msg.ajoute("Bad type. Tybe must be ByThreshold or ByClass");
			}
			else if (IsEqual(argv[i], "-PixelOff"))
			{
				m_pixelOffValue = atof(argv[++i]);
			}
			else if (IsEqual(argv[i], "-Threshold"))
			{
				m_th1 = atof(argv[++i]);
				m_th2 = atof(argv[++i]);

			}
			else if (IsEqual(argv[i], "-nbPixel"))
			{
				m_n1 = atoi(argv[++i]);
				m_n2 = atoi(argv[++i]);
				if (m_n1 < 0 || m_n2 < 0 || m_n2 < m_n1)
				{
					msg.ajoute("invalid nbPixel. nbPixel must be greater than zero and nbPixel1 must be lower or equal then nbPixel2.");
				}
			}
			else if (IsEqual(argv[i], "-FuzzyZone"))
			{
				string statName = argv[++i];
				Trim(statName);

				m_FZStat = -1;
				for (int j = 0; j < NB_STAT_TYPE_EX; j++)
				{
					if (IsEqualNoCase(statName, CStatisticEx::GetName(j)))
					{
						m_FZStat = j;
						break;
					}
				}

				m_FZTh1 = atof(argv[++i]);
				m_FZTh2 = atof(argv[++i]);
				if (m_FZStat == -1)
				{
					msg.ajoute("Invalid FZStat. See documentation.");
				}
			}
			else if (IsEqual(argv[i], "-Class"))
			{
				string str(argv[++i]);

				pair<size_t, std::vector<int>> combinaison;
				string::size_type pos = 0;
				string type = WBSF::Tokenize(str, ":", pos, true);
				
				if (IsEqualNoCase(type, "KEPT"))
					combinaison.first = KEPT;
				else if (IsEqualNoCase(type, "REJECT"))
					combinaison.first = REJECT;
				else msg.ajoute("Bad type of class action. Action must be Kept or Reject in: " + str);

				//string fuzzyClassIn(argv[++i]);

				while (pos != string::npos)
				{
					string aClass = TrimConst(Tokenize(str, ",", pos, true));
					if (!aClass.empty())
						combinaison.second.push_back(ToInt(aClass));
				}

				if (!combinaison.second.empty())
					m_class.push_back(combinaison);
				else msg.ajoute("Bad class definition: " + str);
			}
			else if (IsEqual(argv[i], "-DefaultAction"))
			{
				string str(argv[++i]);
				WBSF::MakeUpper(str);
				if (str == "KEPT")
					m_defaultAction = KEPT;
				else if (str == "REJECT")
					m_defaultAction = REJECT;
				else msg.ajoute("Bad default action. Default action must be Kept or Reject");
			}
			else if (IsEqual(argv[i], "-4"))
			{
				m_b8Connection = false;
			}
			else if (IsEqual(argv[i], "-8"))
			{
				m_b8Connection = true;
			}
			else if (IsEqual(argv[i], "-ReplacedBy"))
			{
				m_replacedBy = atof(argv[++i]);
			}
			else if (IsEqual(argv[i], "-Debug"))
			{
				m_bDebug = true;
			}
			else if (IsEqual(argv[i], "-ExportStats"))
			{
				m_bExportStats = true;
			}
			else
			{
				//Look to see if it's a know base option
				msg = CBaseOptions::ProcessOption(i, argc, argv);
			}

			return msg;
		}

		virtual string GetUsage()const
		{
			string usage = "\nThis software clean isolated pixel group base on there connected number and there values.\n\n";

			//or out of the validity range 
			usage +=
				"If the pixel itself is \"ON\" (diffenrent then PixelOff), then the number of connected pixel \"ON\" is computed and classed as: (zone in parenthesis):\n"
				"If -type is ByThreashold:\n\n"
				"                     Th1          Th2            \n"
				"         |------------|------------|------------|\n"
				"         |              reject(1)               |\n"
				"nbPixel1 |------------|------------|------------|\n"
				"         |reject(2)   |fuzzyZone(3)|  kept(4)   |\n"
				"nbPixel2 |------------|------------|------------|\n"
				"         |               kept(5)                |\n"
				"         |------------|------------|------------|\n"
				"\n"
				"When the number of connected pixel \"ON\" is :\n"
				" -Smaller nbPixel1 then the pixel is reject (zone 1)\n"
				" -Bigger or equal to nbPixel1 but smaller nbPixel2 then\n"
				"   -If no pixel are upper then th1 then the pixel is reject (zone 2)\n"
				"   -If one pixel is upper the th2 then the pixel is kept (zone 4)\n"
				"   -If at least one pixel is upper th1 but no pixel is upper th2 (zone 3) then\n"
				"     -If FZStat >= FZTh1 and FZStat <= FZTh2  then DefaultAction\n"
				"     -otherwise NOT DefaultAction\n"

				" -Bigger or equal to nbPixel2 then the pixel is kept (zone 5)\n\n";


			"If -type is ByClass:\n\n"
				"         |--------------------------------------|\n"
				"         |              reject(1)               |\n"
				"nbPixel1 |--------------------------------------|\n"
				"         | reject(2)  |fuzzyZone(3)|   Kept(4)  |\n"
				"nbPixel2 |--------------------------------------|\n"
				"         |               kept(5)                |\n"
				"         |--------------------------------------|\n"
				"When the number of connected pixel \"ON\" is :\n"
				" -Smaller nbPixel1 then the pixel is reject (zone 1)\n"
				" -Bigger or equal to nbPixel1 but smaller nbPixel2 then\n"
				"   -If a combinaison is in -Class(Reject) then the pixel is reject (zone 2)\n"
				"   -If a combinaison is in -Class(Kept) then the pixel is kept (zone 4)\n"
				"   -If no pixel fall in oine combination (zone 3) then:\n"
				"     -If FZStat >= FZTh1 and FZStat <= FZTh2  then DefaultAction\n"
				"     -Otherwise NOT DefaultAction\n"
				" -Bigger or equal to nbPixel2 then the pixel is kept (zone 5)\n\n"
				" Note: combinaisons are evaluated in the same order than user enter it.\n";

			usage += CBaseOptions::GetUsage();


			return usage;
		}

		virtual string GetHelp()const
		{
			string help = CBaseOptions::GetHelp();

			help += "\n\n"
				"  Statistic available: Lowest, Mean, Sum, Sum², StandardDeviation,\n"
				"                       StandardError, CoeficientOfVariation, Variance,\n"
				"                       Highest, TotalSumOfSquares, QuadraticMean, NbValue,\n"
				"                       MeanAbsoluteDeviation, Skewness, Kurtosis, Median, Mode\n";

			return help;
		}

		size_t m_type;
		int m_n1;
		int m_n2;
		double m_th1;
		double m_th2;
		int m_FZStat;
		double m_FZTh1;
		double m_FZTh2;
		double m_pixelOffValue;
		double m_replacedBy;
		bool m_b8Connection;
		bool m_bDebug;
		bool m_bExportStats;

		std::vector<std::pair<size_t, std::vector<int>>> m_class;
		size_t m_defaultAction;
	};





	class CSeiveContext
	{
	public:

		CSeiveContext(const CSieveOption& option)
		{
			m_options = option;
		}

		void SeiveWindow(const CDataWindowPtr& window)
		{
			int windowSize = GetWindowSize();
			if (m_visited.size() != windowSize)
			{
				m_visited.resize(windowSize);
				m_pixelOn.resize(windowSize);
				for (size_t i = 0; i < m_visited.size(); i++)
				{
					m_visited[i].resize(windowSize);
					m_pixelOn[i].resize(windowSize);
				}
			}
			else
			{
				for (size_t i = 0; i < m_visited.size(); i++)
				{
					for (size_t j = 0; j < m_visited[i].size(); j++)
					{
						m_visited[i][j] = false;
						m_pixelOn[i][j] = false;
					}
				}

				for (int i = 0; i < 3; i++)
					m_stat[i].Reset();

				m_class.clear();
			}

			int halfWindow = GetWindowSize() / 2;
			SeiveWindow(window, halfWindow, halfWindow);
		}

		void SeiveWindow(const CDataWindowPtr& window, int x, int y)
		{

			bool bValid = false;

			DataType value = window->at(x, y);
			if (window->IsValid(value) && value != m_options.m_pixelOffValue)
			{
				MarkPixel(x, y, value);

				int windowSize = GetWindowSize();
				for (int yy = -1; yy < 2 && !bValid; yy++)
				{
					int curY = y + yy;
					for (int xx = -1; xx < 2 && !bValid; xx++)
					{
						int curX = x + xx;

						if (!(xx == 0 && yy == 0)) //exclude current pixel
							if (curY >= 0 && curY < windowSize && curX >= 0 && curX < windowSize)//IsInside
								if (m_options.m_b8Connection || (abs(xx) != abs(yy)))//is connected
									if (!IsMarked(curX, curY))//never visited
										SeiveWindow(window, curX, curY);

						//bool IsItself =  (xx==0 && yy==0);
						//bool bInside = curY>=0 && curY<window->YSize() && curX>=0 && curX<window->XSize();
						//bool bAceptCorner = m_options.m_b8Connection || (abs(xx) != abs(yy));
						//bool bIsVisited = IsMarked(curX,curY);
						//
						//if( !IsItself && //exclude current pixel
						//	bInside && //does the pixel is in the dataset X
						//	bAceptCorner && //accept corner
						//	!bIsVisited)//if the pixel was never visited
						//{
						//	//recurse with new position
						//	bValid = SeiveWindow(window, curX, curY);
						//}
					}
				}
			}

			//return bValid;
		}

		int GetWindowSize()const{ return 2 * m_options.m_n2 + 1; }

		void MarkPixel(int x, int y, double value)
		{
			ASSERT(y >= 0 && y < (int)m_visited.size());
			ASSERT(x >= 0 && x < (int)m_visited[y].size());
			ASSERT(m_visited[y][x] == false);//this pixel never be marked


			int cat = -1;
			if (value != m_options.m_pixelOffValue)
			{
				if (m_options.m_type == BY_THRESHOLD)
				{
					cat = value <= m_options.m_th1 ? 0 : value < m_options.m_th2 ? 1 : 2;
				}
				else
				{
					cat = 1;//unknown by default
					vector<int>::const_iterator pos = std::find(m_class.begin(), m_class.end(), (int)value);
					if (pos == m_class.end())
						m_class.push_back((int)value);

					//for(size_t i=0; i<m_options.m_class.size(); i++)
					//{
					//	if(m_options.m_class[i].second.size()==1)
					//	{
					//		if(m_options.m_class[i].second[0]==value)
					//			cat = m_options.m_class[i].first==REJECT?0:2;
					//	}
					//}
				}
			}

			if (cat != -1)
				m_stat[cat] += value;

			m_visited[y][x] = true;
			m_pixelOn[y][x] = value != m_options.m_pixelOffValue;

			//return IsValid();
		}

		int GetZone()const
		{
			int zone = 0;

			CStatisticEx stat;
			for (int i = 0; i < 3; i++)
				stat += m_stat[i];

			if (stat[NB_VALUE] < m_options.m_n1)
			{
				zone = 1;
			}
			else if (stat[NB_VALUE] < m_options.m_n2)
			{
				if (m_options.m_type == BY_THRESHOLD)
				{
					if (m_stat[2][NB_VALUE] > 0)
						zone = 4;
					else if (m_stat[1][NB_VALUE]>0)
						zone = 3;
					else
						zone = 2;
				}
				else
				{
					zone = 3;
					for (size_t i = 0; i < m_options.m_class.size() && zone == 3; i++)
					{
						bool bMatch = true;
						for (size_t j = 0; j < m_options.m_class[i].second.size() && bMatch; j++)
						{
							vector<int>::const_iterator pos = std::find(m_class.begin(), m_class.end(), m_options.m_class[i].second[j]);
							if (pos == m_class.end())
								bMatch = false;
						}
						if (bMatch)
						{
							zone = m_options.m_class[i].first == REJECT ? 2 : 4;
						}
					}
				}
			}
			else
			{
				zone = 5;
			}

			return zone;
		}

		bool IsValid()const
		{
			int zone = GetZone();
			bool bValid = zone >= 3;

			if (zone == 3) //in fuzzy zone
				bValid = GetFuzzyZoneResult();

			return bValid;
		}


		double GetFuzzyZoneStat(int s = -1)const
		{
			CStatisticEx stat;
			for (int i = 0; i < 2; i++)
				stat += m_stat[i];

			return stat[s == -1 ? m_options.m_FZStat : s];
		}

		bool GetFuzzyZoneResult()const
		{

			bool bResult = m_options.m_defaultAction == REJECT;
			double FZStat = GetFuzzyZoneStat();
			if (FZStat >= m_options.m_FZTh1 && FZStat <= m_options.m_FZTh2)
				bResult = m_options.m_defaultAction == KEPT;

			return bResult;
		}

		bool IsMarked(int x, int y)const{ return m_visited[y][x]; }
		bool IsPixelOn(int x, int y)const{ return m_pixelOn[y][x]; }

	protected:

		CStatisticEx m_stat[3];
		std::vector< boost::dynamic_bitset<size_t> > m_visited;
		std::vector< boost::dynamic_bitset<size_t> > m_pixelOn;
		std::vector< int > m_class;

		CSieveOption m_options;
	};


	class CSeive
	{
	public:

		ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS)
		{
			ERMsg msg;

			if (!m_options.m_bQuiet)
			{
				cout << "Output :" << m_options.m_filesPath[1] << endl;
				cout << "From   :" << m_options.m_filesPath[0] << endl;
				if (!m_options.m_maskName.empty())
					cout << "Mask:    " << m_options.m_maskName << endl;

				cout << "Type:  :" << (m_options.m_type == BY_THRESHOLD ? "ByThreshold" : "ByClass") << endl;
				if (m_options.m_type == BY_THRESHOLD)
				{
					string strTh1 = ToString(m_options.m_th1, 2);
					string strTh2 = ToString(m_options.m_th2, 2);
					while (strTh1.length() < 7)
					{
						if (strTh1.length() % 2 == 0)
							strTh1.insert(strTh1.begin(), 1, ' ');
						else
							strTh1.insert(strTh1.end(), 1, ' ');
					}
					while (strTh2.length() < 7)
					{
						if (strTh2.length() % 2 == 0)
							strTh1.insert(strTh2.begin(), 1, ' ');
						else
							strTh1.insert(strTh2.end(), 1, ' ');
					}

					printf("Using this rules:\n");
					printf("All pixel different of %lg will be considarate as \"ON\"\n", m_options.m_pixelOffValue);
					printf("All connected pixel \"ON\" will be counted and classed as:\n");
					printf("                    %7.7s      %7.7s          \n", strTh1.c_str(), strTh2.c_str());
					printf("          |------------|------------|------------|\n");
					printf("          |              reject(1)               |\n");
					printf("<%8d |------------|------------|------------|\n", m_options.m_n1);
					printf("          |reject(2)   |fuzzyZone(3)|  kept(4)   |\n");
					printf("<%8d |------------|------------|------------|\n", m_options.m_n2);
					printf("          |               kept(5)                |\n");
					printf("          |------------|------------|------------|\n");
					printf("\n");
					printf("DefaultAction = %s\n", m_options.m_defaultAction == KEPT ? " Kept " : "Reject");
					printf("In FuzzyZone, DefaultAction will take place if: %s >= %lg AND %s <= %lg\n", CStatisticEx::GetName(m_options.m_FZStat), m_options.m_FZTh1, CStatisticEx::GetName(m_options.m_FZStat), m_options.m_FZTh2);
					printf("When a pixel is rejected, it will be replaced by: %lg\n", m_options.m_replacedBy);
				}
				else
				{
					printf("Using this rules:\n");
					printf("All pixel different of %lg will be considarate as \"ON\"\n", m_options.m_pixelOffValue);
					printf("All connected pixel \"ON\" will be counted and classed as:\n");
					printf("          |--------------------------------------|\n");
					printf("          |              reject(1)               |\n");
					printf("<%8d |--------------------------------------|\n", m_options.m_n1);
					printf("          |reject(2)   |fuzzyZone(3)|  kept(4)   |\n");
					printf("<%8d |--------------------------------------|\n", m_options.m_n2);
					printf("          |               kept(5)                |\n");
					printf("          |--------------------------------------|\n");
					printf("\n");
					printf(" Type :\tClass or class combinaison\n");
					for (size_t c = 0; c < m_options.m_class.size(); c++)
					{
						printf("%s:\t\"%d", m_options.m_class[c].first == KEPT ? " Kept " : "Reject", m_options.m_class[c].second[0]);
						for (size_t cc = 1; cc < m_options.m_class[c].second.size(); cc++)
						{
							printf(",%d", m_options.m_class[c].second[cc]);
						}
						printf("\"\n");
					}
					printf("DefaultAction = %s\n", m_options.m_defaultAction == KEPT ? " Kept " : "Reject");
					printf("In FuzzyZone, DefaultAction will take place if: %s >= %lg AND %s <= %lg\n", CStatisticEx::GetName(m_options.m_FZStat), m_options.m_FZTh1, CStatisticEx::GetName(m_options.m_FZStat), m_options.m_FZTh2);
					printf("When a pixel is rejected, it will be replaced by: %lg\n", m_options.m_replacedBy);
				}
			}


			if (!m_options.m_bQuiet)
				cout << "Open input image..." << endl;

			//Get input file path in
			msg += inputDS.OpenInputImage(m_options.m_filesPath[INPUT_FILE_PATH], m_options);
			if (msg)
				inputDS.UpdateOption(m_options);

			if (msg && !m_options.m_maskName.empty())
			{
				if (!m_options.m_bQuiet)
					cout << "Open mask..." << endl;

				msg += maskDS.OpenInputImage(m_options.m_maskName);
			}

			if (msg)
			{

				if (!m_options.m_bQuiet)
					cout << "Open output image..." << endl;


				//CSieveOption option = m_options;
				msg += outputDS.CreateImage(m_options.m_filesPath[OUTPUT_FILE_PATH], m_options);
			}

			//Open debug image if needed
			if (msg && m_options.m_bDebug)
			{
				if (!m_options.m_bQuiet)
					cout << "Open debug image..." << endl;

				CSieveOption option = m_options;
				option.m_nbBands = 2;
				option.m_outputType = GDT_Float32;
				option.m_dstNodata = -999999;//(float)::GetDefaultNoData(GDT_Float32);

				string debugFilePath = m_options.m_filesPath[OUTPUT_FILE_PATH];
				SetFileTitle(debugFilePath, GetFileTitle(debugFilePath) + "_debug");
				msg += debugDS.CreateImage(debugFilePath, option);
			}

			//Open stats image if needed
			if (msg && m_options.m_bExportStats)
			{
				if (!m_options.m_bQuiet)
					cout << "Open stats image..." << endl;

				CSieveOption option = m_options;
				option.m_nbBands = NB_STATS;
				option.m_outputType = GDT_Float32;
				option.m_dstNodata = -999999;//(float)GetDefaultNoData(GDT_Float32);

				string debugFilePath = m_options.m_filesPath[OUTPUT_FILE_PATH];
				SetFileTitle(debugFilePath, GetFileTitle(debugFilePath) + "_stats");
				msg += statsDS.CreateImage(debugFilePath, option);
			}

			if (msg && m_options.m_n2 > inputDS->GetRasterYSize() / 2)
			{
				msg.ajoute("nbPixel2 (" + ToString(m_options.m_n2) + ") can't be greather then Ysize/2 (" + ToString(inputDS->GetRasterYSize() / 2) + ")");
			}

			return msg;
		}


		ERMsg Execute()
		{
			ERMsg msg;


			//init image input and image output
			CGDALDatasetEx inputDS;
			CGDALDatasetEx maskDS;
			CGDALDatasetEx outputDS;
			CGDALDatasetEx debugDS;
			CGDALDatasetEx statsDS;


			//Open input image
			msg += OpenAll(inputDS, maskDS, outputDS, debugDS, statsDS);


			if (!msg)
				return msg;

			//Init band hoder
			int windowSize = m_options.m_n2 * 2 + 1;
			CBandsHolderMT bandHolder(windowSize, m_options.m_memoryLimit, m_options.m_IOCPU);

			if (maskDS.IsOpen())
				bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

			msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

			if (!msg)
				return msg;

			//CTimer timerRead;
			//CTimer timerProcess;
			//CTimer timerWrite;

			//Get xy size
			double noDataIn = inputDS.GetNoData(0);
			double noDataOut = outputDS.GetNoData(0);

			int nbCells = m_options.GetExtents().m_xSize*m_options.GetExtents().m_ySize;
			int xx = 0;

			CGeoExtents extents = bandHolder.GetExtents();

			if (outputDS.IsOpen() && !m_options.m_bQuiet)
				cout << "Create output images (" << outputDS->GetRasterXSize() << " C x " << outputDS->GetRasterYSize() << " R x " << outputDS->GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;

			for (int yBlock = 0; yBlock < extents.YNbBlocks(); yBlock++)
			{
				for (int xBlock = 0; xBlock < extents.XNbBlocks(); xBlock++)
				{
					CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

					m_options.m_timerRead.Start();
					bandHolder.LoadBlock(xBlock, yBlock);
					m_options.m_timerRead.Stop();

					if (!bandHolder.IsEmpty())
					{
						vector<vector<float>> output;
						if (outputDS.IsOpen())
						{
							float noData = (float)outputDS.GetNoData().at(0);
							output.resize(blockSize.m_y);
							for (size_t j = 0; j < output.size(); j++)
								output[j].resize(blockSize.m_x, noData);

						}


						array<vector<vector<float>>, 2> debugData;

						if (debugDS.IsOpen())
						{
							float noData = -999999;//(float)GetDefaultNoData(GDT_Float32);
							for (int i = 0; i < 2; i++)
							{
								debugData[i].resize(blockSize.m_y);
								for (size_t j = 0; j < debugData[i].size(); j++)
									debugData[i][j].resize(blockSize.m_x, noData);
							}
						}

						array<vector<vector<float>>, NB_STATS> statsData;

						if (statsDS.IsOpen())
						{
							float noData = -999999;//(float)GetDefaultNoData(GDT_Float32);
							for (int i = 0; i < NB_STATS; i++)
							{
								statsData[i].resize(blockSize.m_y);
								for (size_t j = 0; j < statsData[i].size(); j++)
									statsData[i][j].resize(blockSize.m_x, noData);
							}
						}


						m_options.m_timerProcess.Start();

						CDataWindowPtr blockInput = bandHolder.GetWindow(0);
#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_CPU ) if (m_options.m_bMulti) 
						for (int y = 0; y<blockSize.m_y; y++)
						{

							CSeiveContext context(m_options);

							for (int x = 0; x < blockSize.m_x; x++)
							{
								double value = blockInput->at(x, y);
								if (fabs(value - noDataIn) > 0.00001)
								{
									if (fabs(value - m_options.m_pixelOffValue) > 0.00001)
									{
										CDataWindowPtr input = bandHolder.GetWindow(0, x, y, windowSize);

										context.SeiveWindow(input);
										bool bValid = context.IsValid();

										if (outputDS.IsOpen())
											output[y][x] = (float)(bValid ? value : m_options.m_replacedBy);

										if (debugDS.IsOpen())
										{
											debugData[0][y][x] = (float)context.GetZone();
											if (int(debugData[0][y][x]) == 3) //in fuzzy zone
												debugData[1][y][x] = (float)context.GetFuzzyZoneStat();
										}

										if (statsDS.IsOpen())
										{
											if (int(debugData[0][y][x]) == 3) //in fuzzy zone
											{
												for (int s = 0; s < NB_STATS; s++)
													statsData[s][y][x] = (float)context.GetFuzzyZoneStat(BANDS_STATS[s]);
											}
										}
									}
									else
									{
										if (outputDS.IsOpen())
											output[y][x] = (float)value;

										if (debugDS.IsOpen())
											debugData[0][y][x] = 0;//zone 0

									}//!m_options.m_pixelOffValue )
								}//if !noData

								if (!m_options.m_bQuiet && ++xx%int(ceil(nbCells / 80.0)) == 0)
									cout << ".";
							}//x
						}//y

						//write band to output image
						CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
						if (outputDS.IsOpen())
						{
							ASSERT(outputRect.Height() == (int)output.size());
							GDALRasterBand *pBand = outputDS.GetRasterBand(0);

							for (int y = 0; y < outputRect.Height(); y++)
								pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y + y, outputRect.Width(), 1, &(output[y][0]), outputRect.Width(), 1, GDT_Float32, 0, 0);
						}

						if (debugDS.IsOpen())
						{
							for (size_t b = 0; b < debugDS.GetRasterCount(); b++)
							{
								GDALRasterBand *pBand = debugDS.GetRasterBand(b);
								for (int y = 0; y < outputRect.Height(); y++)
									pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y + y, outputRect.Width(), 1, &(debugData[b][y][0]), outputRect.Width(), 1, GDT_Float32, 0, 0);
							}
						}

						if (statsDS.IsOpen())
						{
							for (size_t b = 0; b < statsDS->GetRasterCount(); b++)
							{
								GDALRasterBand *pBand = statsDS.GetRasterBand(b);
								for (int y = 0; y < outputRect.Height(); y++)
									pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y + y, outputRect.Width(), 1, &(statsData[b][y][0]), outputRect.Width(), 1, GDT_Float32, 0, 0);
							}
						}
					}//band !empty
					else
					{
						for (int x = 0; x < blockSize.m_x*blockSize.m_y; x++)
							if (!m_options.m_bQuiet && ++xx%int(ceil(nbCells / 80.0)) == 0)
								cout << ".";
					}
				}//xBlock
			}//yBlock

			//close all images
			inputDS.Close();
			maskDS.Close();
			
			m_options.m_timerProcess.Start();
			outputDS.Close(m_options);
			debugDS.Close(m_options);
			statsDS.Close(m_options);
			m_options.m_timerProcess.Stop();
			m_options.PrintTime();


			return msg;
		}

		CSieveOption m_options;

	};
}


int _tmain(int argc, _TCHAR* argv[])
{
	std::locale::global(std::locale(""));
	GDALAllRegister();

	CTimer timer(true);

	WBSF::CSeive seive;
	ERMsg msg = seive.m_options.ParseOptions(argc, argv);

	if( !seive.m_options.m_bQuiet || msg )
	{
		cout << endl;
		cout << "Seive version" << WBSF::version << "(" << __DATE__ << ")" << endl;
	}
	
	if( msg ) 
		msg = seive.Execute();

	if( !msg)
	{
		WBSF::PrintMessage(msg);
		return -1;
	}
	

	if( !seive.m_options.m_bQuiet )
		cout << endl << "Total time = " << WBSF::SecondToDHMS(timer.Elapsed()) << endl; 

	return 0; 

}
