// Functions for computing probability distribution
// functions commonly used in extreme value analysis

// Function prototypes
void gammaFit(double L[], double gammaParams[]);
double gammaStandardize(double value, double params[]);
void pearsonIIIFit(double L[], double pearsonIIIParams[]);
double pearsonIIIStandardize(double value, double params[]);
void logLogisticFit(double pwm[], double logLogisticParams[]);
double logLogisticCDF(double value, double params[]);
double standardGaussianInvCDF(double prob);
