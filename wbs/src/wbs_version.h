//22-07-2019	1.1.4	Rémi Saint-Amant	ATM version 1.0.4: new circadian rhythm 
//25-01-2019	1.1.3	Rémi Saint-Amant	Add completeness in WGInputAnalysis
//											Add filter by name in weather editors
//											Simulate next year without error
//											Correctly simulate without shore when disabled
//											Optimization of internal database
//30-10-2018	1.1.2	Rémi Saint-Amant	Change mapping interpolation result
//24-10-2018	1.1.1	Rémi Saint-Amant	Change the default interpolation spatial drift param from Expo to Elev. 
//13-09-2018	1.1.0	Rémi Saint-Amant	Resolve confusion in vapor pressure units. 
//											thread safe correction in GetData() if hourly object
//											Correction of bug in generation or hourly prcp from daily
//											Remove "2" in the temporary name H_TMIN2, H_TAIR2, H_TMAX2 and H_SRAD2
//30-08-2018	1.0.1	Rémi Saint-Amant	bug correction in computation of slope and aspect from DEM
//29-08-2018	1.0.0	Rémi Saint-Amant	first version number of WBSF

#pragma once
static const char* WBSF_VERSION = "1.1.4";
