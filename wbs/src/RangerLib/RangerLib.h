#pragma once

#include "RangerLib/globals.h"
#include "RangerLib/Utility/DataChar.h"
#include "RangerLib/Utility/DataShort.h"
#include "RangerLib/Utility/DataFloat.h"
#include "RangerLib/Utility/DataDouble.h"
#include "RangerLib/Forest/ForestClassification.h"
#include "RangerLib/Forest/ForestRegression.h"
#include "RangerLib/Forest/ForestSurvival.h"
#include "RangerLib/Forest/ForestProbability.h"



// Create forest object
inline Forest* CreateForest(TreeType treetype)
{
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
