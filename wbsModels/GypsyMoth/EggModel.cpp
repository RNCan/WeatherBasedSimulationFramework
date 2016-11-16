#include "EggModel.h"
//#include "Basic/UtilMath.h"

using namespace std;

namespace WBSF
{


	void CEggModel::Reset()
	{
		m_eggState.clear();
	}

	CTRef CEggModel::GetFirstDay()const
	{
		//it's the first oviposition date
		return min(m_eggState.GetFirstTRef(PREDIAPAUSE), m_eggState.GetFirstTRef(HATCH));
	}

	CTRef CEggModel::GetFirstHatch()const
	{
		return m_eggState.GetFirstTRef(HATCH);
	}


	CTRef CEggModel::GetLastHatch()const
	{

		CTRef lastHatch;

		for (int i = int(m_eggState.size() - 2); i >= 0; i--)
		{
			if (m_eggState[i][HATCH] < MAXEGGS)
			{
				lastHatch = m_eggState.GetFirstTRef() + i + 1;
				break;
			}
		}
		return lastHatch;
	}

	//cumulatif
	CTRef CEggModel::GetMedian(int s)const
	{
		_ASSERTE(s >= 0 && s <= 4);

		CTRef median;

		for (int i = 0; i < (int)m_eggState.size(); i++)
		{
			double sum = 0;
			for (int j = s; j < NB_PHASES; j++)
				sum += m_eggState[i][j];

			if (sum > MAXEGGS / 2)
			{
				median = m_eggState.GetFirstTRef() + i;
				break;
			}
		}

		return median;
	}
}