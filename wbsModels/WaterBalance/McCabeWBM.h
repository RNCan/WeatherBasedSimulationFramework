//
// McCabeWBM.h
//
// 2006-01-06, Markstro     Create Java code
// 2024-09-10, Saint-Amant  Convert to C++
//



#include <string>
#include <map>
#include <vector>


namespace McCabeWBM
{



class Parameter
{

public:

    Parameter(double value=0, double minn=0, double maxx=0)
    {
        m_value=value;
        m_min=minn;
        m_max=maxx;
    }

    double m_value;
    double m_min;
    double m_max;

};

typedef std::map<std::string,Parameter> Parameters;
Parameters GetDefaultParameters();

//************************************************************************************************
//Inputs

class Input
{
public:

    Input(int year=0, int month=0, double temp=0, double prcp=0)
    {
        m_year=year;
        m_month=month;
        m_temp=temp;
        m_prcp=prcp;
    }

    int m_year;
    int m_month;
    double m_temp; //Temperature (Degrees Celsius)
    double m_prcp;//Precipitation (mm)

};
typedef std::vector<Input> Inputs;

Inputs ReadInputFile(std::string filePath);

//************************************************************************************************
//Output


class Output
{
public:

    int m_year;
    int m_month;
    double m_temp; //Temperature (Degrees Celsius)
    double m_prcp;//Precipitation (mm)
    double m_pet;//Potential ET (mm)
    double m_precipPet;//Precipitation - Pot ET (mm)
    double m_soilMoist;//Soil Moisture Storage (mm)
    double m_aet; //Actual ET (mm)
    double m_petAet;//Potential ET - Actual ET (mm)
    double m_snow;//Snow Storage (mm)
    double m_surplus; //Over storage Surplus (mm)
    double m_totalRunoff;//total Runoff (direct and indirect) (mm)
    double m_directRunoff;//Direct Runoff (mm)
    double m_snowMelt;//Snow Melt (mm)




};

typedef std::vector<Output> Outputs;


class McCabeWaterBalanceModel
{

public:

    static Outputs Execute (Parameters params, Inputs inputs);
    static Output ComputeWaterBalance(Parameters params, Input input, double& prestor, double& remain, double& snowStor);
    static double getDayLength(int month, double latitude);
    static double getHammonPET(int month, double tc, double dl);
    static double getSnowMelt(double tc, double tsnow, double snstor, double train, double xmeltmax);
    static double getSnowProportion(double tc, double p, double train, double tsnow);


};

}


