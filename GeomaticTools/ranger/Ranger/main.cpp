/*-------------------------------------------------------------------------------
 This file is part of Ranger.

 Ranger is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Ranger is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Ranger. If not, see <http://www.gnu.org/licenses/>.

 Written by:

 Marvin N. Wright
 Institut für Medizinische Biometrie und Statistik
 Universität zu Lübeck
 Ratzeburger Allee 160
 23562 Lübeck
 Germany

 http://www.imbs-luebeck.de
 #-------------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <stdexcept> 
#include <string>

#include "globals.h"
#include "ArgumentHandler.h"
#include "Forest/ForestClassification.h"
#include "Forest/ForestRegression.h"
#include "Forest/ForestSurvival.h"
#include "Forest/ForestProbability.h"


//--file "D:\Travaux\Ranger\Training\exemple_train_remi.csv" -o "D:\Travaux\Ranger\Training\exemple_train_remi" --write --depvarname pcover_L --impmeasure 1 --treetype 3 --memmode 2 --seed 1 --verbose 
//--file "D:\Travaux\Ranger\input\test.csv" -o "D:\Travaux\Ranger\Output\test" --predict "D:\Travaux\Ranger\Training\training.forest"  --verbose 
//

//--ntree 500 --file "U:\GIS\#documents\TestCodes\Ranger\Training\exemple_train_remi.csv" -o "U:\GIS\#documents\TestCodes\Ranger\Training\exemple_train_remi" --write --depvarname pcover_L --impmeasure 1 --treetype 3 --memmode 2 --seed 1234 --verbose 
//--file "U:\GIS\#documents\TestCodes\Ranger\input\test.csv" -o "U:\GIS\#documents\TestCodes\Ranger\Output\test" --predict "U:\GIS\#documents\TestCodes\Ranger\Training\exemple_train_remi.classification.forest"  --memmode 2 --verbose
//--seed 451 --treetype 1 --ntree 500 --write --impmeasure 1 --memmode 0 --file "U:\GIS\#projets\LAQ\LAI\ANALYSE\20170815_Map_demo\test_code_remi_v1\BKP_9616_050_S7\RUN_RF\_TRAIN_for_Ranger_v1.csv" --depvarname value --outprefix "U:\GIS\#projets\LAQ\LAI\ANALYSE\20170815_Map_demo\test_code_remi_v1\BKP_9616_050_S7\RUN_RF\test4_pv" --verbose 



// Create forest object
static Forest* CreateForest(TreeType treetype){
	Forest* forest = NULL;
	switch (treetype) {
	case TREE_CLASSIFICATION:
		forest = new ForestClassification;
		break;
	case TREE_REGRESSION:
		forest = new ForestRegression;
		break;
	case TREE_SURVIVAL:
		forest = new ForestSurvival;
		break;
	case TREE_PROBABILITY:
		forest = new ForestProbability;
		break;
	}

	return forest;
}

int main(int argc, char **argv) {
	ArgumentHandler arg_handler(argc, argv);
	Forest* forest = 0;
	try {
		// Handle command line arguments
		if (arg_handler.processArguments() != 0) {
			return 0;
		}
		arg_handler.checkArguments();

		
		// Verbose output to logfile if non-verbose mode
		std::ostream* verbose_out;
		if (arg_handler.verbose) {
			verbose_out = &std::cout;
		}
		else {
			std::ofstream* logfile = new std::ofstream();
			logfile->open(arg_handler.outprefix + ".log");
			if (!logfile->good()) {
				throw std::runtime_error("Could not write to logfile.");
			}
			verbose_out = logfile;
		}

		
		// Call Ranger
		*verbose_out << "Starting Ranger." << std::endl;
		if (arg_handler.predict.empty())
		{
			if (arg_handler.treetype == TREE_CLASSIFICATION && arg_handler.probability)
				arg_handler.treetype = TREE_PROBABILITY;

			forest = CreateForest(arg_handler.treetype);

			Data* training = forest->initCpp_grow(arg_handler.depvarname, arg_handler.memmode, arg_handler.file, arg_handler.mtry,
				arg_handler.ntree, verbose_out, arg_handler.seed, arg_handler.nthreads,
				/*arg_handler.predict,*/ arg_handler.impmeasure, arg_handler.targetpartitionsize, arg_handler.splitweights,
				arg_handler.alwayssplitvars, arg_handler.statusvarname, arg_handler.replace, arg_handler.catvars,
				arg_handler.savemem, arg_handler.splitrule, arg_handler.caseweights, /*arg_handler.predall,*/ arg_handler.fraction,
				arg_handler.alpha, arg_handler.minprop, arg_handler.holdout, /*arg_handler.predictiontype,*/
				arg_handler.randomsplits);

			
			forest->run_grow(training);
			if (arg_handler.write) {
				std::string tree_type_str = GetTreeTypeStr(arg_handler.treetype);
				forest->saveToFile(arg_handler.outprefix + tree_type_str + ".forest");
			}
			forest->writeOutput(training, arg_handler.outprefix);
			*verbose_out << "Finished Ranger." << std::endl;

			delete training;
		}
		else
		{
			TreeType treetype = GetTreeType(arg_handler.predict);
			forest = CreateForest(treetype);

			Data* data = forest->initCpp_predict(arg_handler.memmode, arg_handler.file,
				verbose_out, arg_handler.seed, arg_handler.nthreads,
				arg_handler.predict, arg_handler.predall, arg_handler.predictiontype);

			if (verbose_out) {
				*verbose_out << "Predicting .." << std::endl;
			}

			
			forest->run_predict(data);

			std::string filename = arg_handler.outprefix + ".prediction";
			forest->writePredictionFile(filename);

			delete data;
		}

		delete forest;
	}
	catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << " Ranger will EXIT now." << std::endl;
		delete forest;
		return -1;
	}

	return 0;
}

