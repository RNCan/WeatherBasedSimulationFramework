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


//--file "D:\Travaux\Ranger\Training\training.csv" -o "D:\Travaux\Ranger\Training\training" --write --depvarname class --impmeasure 1 --treetype 1 --verbose --splitweights "D:\Travaux\Ranger\Training\weight.csv"
//--file "D:\Travaux\Ranger\input\test.csv" -o "D:\Travaux\Ranger\Output\test" --predict "D:\Travaux\Ranger\Training\training.forest"  --verbose 



int main(int argc, char **argv) 
{

	ArgumentHandler arg_handler(argc, argv);
	Forest* forest = 0;
	try {

		// Handle command line arguments
		if (arg_handler.processArguments() != 0) {
			return 0;
		}
		arg_handler.checkArguments();

		// Create forest object
		switch (arg_handler.treetype) {
		case TREE_CLASSIFICATION:
			if (arg_handler.probability) {
				forest = new ForestProbability;
			}
			else {
				forest = new ForestClassification;
			}
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
			Data* training = forest->initCpp_grow(arg_handler.depvarname, arg_handler.memmode, arg_handler.file, arg_handler.mtry,
				arg_handler.ntree, verbose_out, arg_handler.seed, arg_handler.nthreads,
				/*arg_handler.predict,*/ arg_handler.impmeasure, arg_handler.targetpartitionsize, arg_handler.splitweights,
				arg_handler.alwayssplitvars, arg_handler.statusvarname, arg_handler.replace, arg_handler.catvars,
				arg_handler.savemem, arg_handler.splitrule, arg_handler.caseweights, /*arg_handler.predall,*/ arg_handler.fraction,
				arg_handler.alpha, arg_handler.minprop, arg_handler.holdout, /*arg_handler.predictiontype,*/
				arg_handler.randomsplits);

			forest->run(training);
			if (arg_handler.write) {
				forest->saveToFile(arg_handler.outprefix + ".forest");
			}
			forest->writeOutput(training, arg_handler.outprefix);
			*verbose_out << "Finished Ranger." << std::endl;

			delete training;
		}
		else
		{
			Data* data = forest->initCpp_predict(/*arg_handler.depvarname, */arg_handler.memmode, arg_handler.file,/*, arg_handler.mtry,
				arg_handler.ntree, */verbose_out, /*arg_handler.seed,*/ arg_handler.nthreads,
				arg_handler.predict, /*arg_handler.impmeasure, arg_handler.targetpartitionsize, arg_handler.splitweights,
				arg_handler.alwayssplitvars, arg_handler.statusvarname, arg_handler.replace, arg_handler.catvars,*/
				/*arg_handler.savemem,*/ /*arg_handler.splitrule, arg_handler.caseweights, */arg_handler.predall, /*arg_handler.fraction,
				arg_handler.alpha, arg_handler.minprop, arg_handler.holdout, */arg_handler.predictiontype/*,
				arg_handler.randomsplits*/);

			if (verbose_out) {
				*verbose_out << "Predicting .." << std::endl;
			}
			forest->run(data);

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

//#include "DataShort.h"
//
//int main(int argc, char **argv)
//{
//	try
//	{
//		ArgumentHandler arg(argc, argv);
//
//		// Handle command line arguments
//		if (arg.processArguments() != 0) {
//			return 0;
//		}
//		arg.checkArguments();
//		
//		//std::string forest_filename;
//
//		arg.memmode = MEM_DOUBLE;
//		arg.treetype = TREE_CLASSIFICATION;
//		arg.seed = 0;
//		//arg.nthreads = 1;
//		arg.savemem = false;
//
//		std::ostream* verbose_out;
//		if (arg.verbose)
//		{
//			verbose_out = &std::cout;
//		}
//		else
//		{
//			std::ofstream* logfile = new std::ofstream();
//			logfile->open(arg.outprefix + ".log");
//			if (!logfile->good()) {
//				throw std::runtime_error("Could not write to logfile.");
//			}
//			verbose_out = logfile;
//		}
//
//
//		ForestClassification forest;
//		forest.set_verbose(verbose_out);
//		//forest.verbose_out = verbose_out;
//
//		
//
//
//		// Call other init function
//		forest.init_predict(/*arg.depvarname, &data, arg.mtry, arg.ntree, arg.seed,*/ arg.nthreads, /*arg.impmeasure,*/
//			/*arg.targetpartitionsize, arg.statusvarname, true, arg.replace, arg.catvars,*/
//			arg.savemem, /*arg.splitrule, */arg.predall, /*arg.fraction, arg.alpha, arg.minprop, arg.holdout,*/ arg.predictiontype/*,
//			arg.randomsplits*/);
//
//
//		forest.loadFromFile(arg.predict);
//
//
//		// Initialize data with memmode
//		DataShort data;
//		//DataDouble data;
//
//		// Load data
//		*verbose_out << "Loading input file: " << arg.file << "." << std::endl;
//		bool rounding_error = data.loadFromFile(arg.file);
//		if (rounding_error) {
//			*verbose_out << "Warning: Rounding or Integer overflow occurred. Use FLOAT or DOUBLE precision to avoid this."
//				<< std::endl;
//		}
//		forest.run(&data);
//		//forest.writeOutput(arg.outprefix);
//		std::string filename = arg.outprefix + ".prediction";
//		forest.writePredictionFile(filename);
//	}
//	catch (std::exception& e) 
//	{
//		std::cerr << "Error: " << e.what() << " Ranger will EXIT now." << std::endl;
//		//delete forest;
//		return -1;
//	}
//	return 0;
//
//	
//}