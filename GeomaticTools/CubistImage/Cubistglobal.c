//***********************************************************************
//									 
//	Source code for use with Cubist Release 2.07			 
//	--------------------------------------------			 
//		   Copyright RuleQuest Research 2010			 
//									 
//	This code is provided "as is" without warranty of any kind,	 
//	either express or implied.  All use is at your own risk.	 
//									 
//***********************************************************************


//***********************************************************************
//									 
//		General data for Cubist					 
//									 
//***********************************************************************

//#include "global.h"

Attribute	ClassAtt = 0;	// attribute to use as class 
Attribute	LabelAtt = 0;	// attribute containing case label 
Attribute	CWtAtt = 0;	// attribute containing case weight 

int		MaxAtt;		// max att number 
int		MaxDiscrVal = 3;	// max discrete values for any att 
int		Precision = 2;	// decimal places for target 
int		MaxLabel = 0;	// max characters in case label 
int		LineNo = 0;	// input line number 
int		ErrMsgs = 0;	// errors found 
int		AttExIn;	// attribute exclusions/inclusions 
int		TSBase = 0;	// base day for time stamps 
int		Delimiter;	// character at end of name 
int		NCPU = 1;		// number of CPUs 

float		ErrReduction = 1;	// effect of committee model 

CaseNo		MaxCase = -1;	// max data item number 

DataRec		*Case;		// data items 

DiscrValue	*MaxAttVal;	// number of values for each att 
DiscrValue	*Modal;		// most frequent value for discr att 

char		*SpecialStatus;	// special att treatment 

Definition	*AttDef;	// definitions of implicit atts 

String		Target;		// name of dependent att 
String		*AttName;	// att names 
String		**AttValName;	// att value names 

char		*IgnoredVals = 0;	// values of labels and ignored atts 
int		IValsSize = 0;	// size of above 
int		IValsOffset = 0;	// index of first free char 

String		FileStem = "undefined";
char		Fn[512];	// file name 

FILE		*Mf = 0;		// file for reading models 

ContValue	*AttMean = Nil;	// means of att values 
ContValue	*AttSD = Nil;		// std dev ditto 
ContValue	*AttMaxVal = Nil;	// max value in training data 
ContValue	*AttMinVal = Nil;	// min value ditto 
ContValue	Ceiling = 1E38;	// max allowable global prediction 
ContValue	Floor = -1E38;	// min allowable global prediction 

int		*AttPrec = Nil;	// Attribute precision  

DataRec		*Instance = Nil;	// training cases 
DataRec		Ref[2];		// reference point 
Index		KDTree = Nil;	// index for searching training cases 
CaseNo		MaxInstance = -1;	// highest instance 
float		*RSPredVal = Nil; // tabulated RS predictions 
float		*RSErrLim = Nil;	// tabulated RS error limits 
NNEnvRec	GNNEnv;		// global NN environment 

unsigned char	*Tested;	// used in BuildIndex 
CaseCount	*ValFreq;	// used in BuildIndex 

RRuleSet	*CubistModel;	// from .model file 

Boolean		USEINSTANCES;
float		EXTRAP = 0.1f;	// allowed extrapolation from models 
float		SAMPLE = 0;		// sample training proportion  
float		MAXD;			// max distance for close neighbors 
float		GlobalMean = 0;	// global mean on training data 
float		GlobalErrLim;	// estimated global error limit 
int		MEMBERS = 1;			// models in committee 
int		NN = 5;				// nearest neighbors to use 
int		KRInit;
