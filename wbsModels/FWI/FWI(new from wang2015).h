#pragma once

#include <deque>

class CFWIInput
{
public:

	CFWIInput(int _month = 0, int _day = 0, double _temp = 0, double _rhum = 0, double _wind = 0, double _prcp = 0)
	{
		month = _month;
		day = _day;
		temp = _temp;
		rhum = _rhum;
		wind = _wind;
		prcp = _prcp;
	}

	int month;
	int day;
	double temp;
	double rhum;
	double wind;
	double prcp;
};

typedef std::deque<CFWIInput> CFWIInputVector;
class CFWIOutput
{
public:
	CFWIOutput(double _ffmc = 0, double _dmc = 0, double _dc = 0, double _isi = 0, double _bui = 0, double _fwi = 0)
	{
		ffmc = _ffmc;
		dmc = _dmc;
		dc = _dc;
		isi = _isi;
		bui = _bui;
		fwi = _fwi;
	}

	double ffmc;
	double dmc;
	double dc;
	double isi;
	double bui;
	double fwi;
};

typedef std::deque<CFWIOutput> CFWIOutputVector;


void ComputeFWI(const CFWIInputVector& input, CFWIOutputVector& output, double ffmc0 = 85, double dmc0 = 6, double dc0 = 15);
