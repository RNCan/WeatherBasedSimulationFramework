//***********************************************************************
//									 
//	 Source code for use with See5/C5.0 Release 2.07		 
//	 -----------------------------------------------		 
//		       Copyright RuleQuest Research 2009		 
//									 
//	This code is provided "as is" without warranty of any kind,	 
//	either express or implied.  All use is at your own risk.	 
//									 
//***********************************************************************

#pragma once 
#include "Geomatic/See5defns.h"

namespace WBSF
{

	//***********************************************************************
	//									 
	//		Parameters etc						 
	//									 
	//***********************************************************************
	class CDecisionTreeBase
	{

	public:

		CDecisionTreeBase();
		void Reset();
		bool operator==(const CDecisionTreeBase& in)const;
		bool operator!=(const CDecisionTreeBase& in)const{ return !operator==(in); }

		int		TRIALS;	// number of trees to be grown 
		int		Trial;		// trial number for boosting 

		Boolean		RULES;	// rule-based classifiers 
		Boolean		RULESUSED;	// list applicable rules 


		//***********************************************************************
		//									 
		//		Attributes and data					 
		//									 
		//***********************************************************************

		Attribute	ClassAtt;	// attribute to use as class 
		Attribute	LabelAtt;	// attribute to use as case ID 
		Attribute	CWtAtt;		// attribute to use for case weight 

		String		*ClassName;	// class names 
		String		*AttName;	// att names 
		String		**AttValName;	// att value names 

		char		*IgnoredVals;	// values of labels and atts marked ignore 
		int		IValsSize;	// size of above 
		int		IValsOffset;	// index of first free char 

		int		MaxAtt;		// max att number 
		int		MaxClass;	// max class number 
		int		AttExIn;	// attribute exclusions/inclusions 
		int		LineNo;	// input line number 
		int		ErrMsgs;	// errors found 
		int		Delimiter;	// character at end of name 
		int		TSBase;	// base day for time stamps 

		DiscrValue	*MaxAttVal;	// number of values for each att 

		ContValue	*ClassThresh;	// thresholded class attribute 

		char		*SpecialStatus;// special att treatment 

		Definition	*AttDef;	// definitions of implicit atts 

		Boolean		*SomeMiss;	// att has missing values 
		Boolean		*SomeNA;	// att has N/A values 

		String		FileStem;

		//***********************************************************************
		//									 
		//		Trees							 
		//									 
		//***********************************************************************

		Tree		*Pruned;	// decision trees 

		ClassNo		*TrialPred;	// predictions for each boost trial 

		float		Confidence;	// set by classify() 
		float		*Vote;	// total votes for classes 
		float		*ClassSum;	// class weights during classification 
		float		**MCost;	// misclass cost [pred][real] 

		//***********************************************************************
		//									 
		//		Rules							 
		//									 
		//***********************************************************************

		RuleNo		*Active;	// rules that fire while classifying case 
		RuleNo		NActive;	// number ditto 
		RuleNo		ActiveSpace;	// space currently allocated for rules 
		RuleNo		*RulesUsed;	// list of all rules used 
		RuleNo		NRulesUsed;	// number ditto 

		CRuleSet	*RuleSet;	// rulesets 

		ClassNo		Default;	// default class associated with ruleset or boosted classifier 

		CRule		*MostSpec;	// used in RuleClassify() 


		//***********************************************************************
		//									 
		//		Misc							 
		//									 
		//***********************************************************************

		FILE		*TRf;		// file pointer for tree and rule i/o 
		char		Fn[500];	// file name 
	};

}