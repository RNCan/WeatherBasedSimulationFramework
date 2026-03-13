#include "auxiliary.h"
#include "lmoments.h"
#include "spei.h"
#include "pdfs.h"
#include <vector>


// spei()
// Calculates the Standardized Precipitation-Evapotransporation Index
// from a series of climatic balance (precipitation minus etp). The
// SPEI is the standardized value of the climatic balance (P-ETP),
// computed following a Log Logistic probability distribution.
void spei(double dataSeries[], int n, double speiSeries[])
{
	enum TLogLogisticParams{ BETA, ALPHA, GAMMA, NB_LOSTATPARAM };
	enum TSeasons{ SEASONS = 12 };

	//int i, j, k, nSeason = 0 ;
	double beta[NB_LOSTATPARAM] = { 0 };
	double logLogisticParams[SEASONS][NB_LOSTATPARAM] = { 0 };
	
	// Loop through all seasons defined by seasons
	for (int j = 0; j < SEASONS; j++)
	{
		// Extract and sort the seasonal series
		std::vector<double> seasonSeries;
		for (int i = j; i < n; i += SEASONS)
			seasonSeries.push_back( dataSeries[i] );

		upward(seasonSeries.data(), (int)seasonSeries.size());
		// Compute probability weighted moments
		//pwm(seasonSeries, nSeason, beta, -0.35, 0, 0);
		pwm(seasonSeries.data(), (int)seasonSeries.size(), beta, 0, 0, 0);
		// Fit a Log Logistic probability function
		logLogisticFit(beta, logLogisticParams[j]);
		//printf("\nSeason %u", jndice);
		//printf("\nLogLogistic beta param.: %.4f", logLogisticParams[jndice][0]);
		//printf("\nLogLogistic alpha param.: %.4f", logLogisticParams[jndice][1]);
		//printf("\nLogLogistic gamma param.: %.4f\n", logLogisticParams[jndice][2]);
		// Calculate the standardized values
		for (int i = j; i < n; i += SEASONS)
		{
			speiSeries[i] = logLogisticCDF(dataSeries[i], logLogisticParams[j]);
			speiSeries[i] = -standardGaussianInvCDF(speiSeries[i]);
		}
	}
}
