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
#include "stdafx.h"
#include "See5global.h"

namespace WBSF
{

	CDecisionTreeBase::CDecisionTreeBase()
	{
		Reset();
	}

	void CDecisionTreeBase::Reset()
	{

		//***********************************************************************
		//									 
		//		Parameters etc						 
		//									 
		//***********************************************************************
		TRIALS = 1;	// number of trees to be grown 
		Trial = 0;		// trial number for boosting 

		RULES = 0;	// rule-based classifiers 
		RULESUSED = 0;	// list applicable rules 


		//***********************************************************************
		//									 
		//		Attributes and data					 
		//									 
		//***********************************************************************

		ClassAtt = 0;	// attribute to use as class 
		LabelAtt = 0;	// attribute to use as case ID 
		CWtAtt = 0;		// attribute to use for case weight 

		ClassName = NULL;	// class names 
		AttName = NULL;	// att names 
		AttValName = NULL;	// att value names 

		IgnoredVals = NULL;	// values of labels and atts marked ignore 
		IValsSize = 0;	// size of above 
		IValsOffset = 0;	// index of first free char 

		MaxAtt = 0;		// max att number 
		MaxClass = 0;	// max class number 
		AttExIn = 0;	// attribute exclusions/inclusions 
		LineNo = 0;	// input line number 
		ErrMsgs = 0;	// errors found 
		Delimiter = 0;	// character at end of name 
		TSBase = 0;	// base day for time stamps 

		MaxAttVal = NULL;	// number of values for each att 

		ClassThresh = NULL;	// thresholded class attribute 

		SpecialStatus = NULL;// special att treatment 

		AttDef = NULL;	// definitions of implicit atts 

		SomeMiss = Nil;	// att has missing values 
		SomeNA = Nil;	// att has N/A values 

		FileStem = "undefined";

		//***********************************************************************
		//									 
		//		Trees							 
		//									 
		//***********************************************************************

		Pruned = NULL;	// decision trees 

		TrialPred = NULL;	// predictions for each boost trial 

		Confidence = 0;	// set by classify() 
		Vote = NULL;	// total votes for classes 
		ClassSum = NULL;	// class weights during classification 
		MCost = NULL;	// misclass cost [pred][real] 

		//***********************************************************************
		//									 
		//		Rules							 
		//									 
		//***********************************************************************

		Active = Nil;	// rules that fire while classifying case 
		NActive = 0;	// number ditto 
		ActiveSpace = 0;	// space currently allocated for rules 
		RulesUsed = Nil;	// list of all rules used 
		NRulesUsed = 0;	// number ditto 

		RuleSet = NULL;	// rulesets 

		Default = 0;	// default class associated with ruleset or boosted classifier 

		MostSpec = NULL;	// used in RuleClassify() 


		//***********************************************************************
		//									 
		//		Misc							 
		//									 
		//***********************************************************************

		TRf = NULL;		// file pointer for tree and rule i/o 
		memset(Fn, 0, sizeof(char) * 500);	// file name 
	}


	bool CDecisionTreeBase::operator==(const CDecisionTreeBase& in)const
	{
		bool bEqual = true;
		if (TRIALS != in.TRIALS)bEqual = false;	// number of trees to be grown 
		if (Trial != in.Trial)bEqual = false;		// trial number for boosting 
		if (RULES != in.RULES)bEqual = false;	// rule-based classifiers 
		if (RULESUSED != in.RULESUSED)bEqual = false;	// list applicable rules 
		if (ClassAtt != in.ClassAtt)bEqual = false;	// attribute to use as class 
		if (LabelAtt != in.LabelAtt)bEqual = false;	// attribute to use as case ID 
		if (CWtAtt != in.CWtAtt)bEqual = false;		// attribute to use for case weight 
		//if(ClassName=NULL;	// class names 
		//if(AttName=NULL;	// att names 
		//if(AttValName=NULL;	// att value names 
		//if(IgnoredVals=NULL;	// values of labels and atts marked ignore 
		if (IValsSize != in.IValsSize)bEqual = false;	// size of above 
		if (IValsOffset != in.IValsOffset)bEqual = false;	// index of first free char 
		if (MaxAtt != in.MaxAtt)bEqual = false;		// max att number 
		if (MaxClass != in.MaxClass)bEqual = false;	// max class number 
		if (AttExIn != in.AttExIn)bEqual = false;	// attribute exclusions/inclusions 
		if (LineNo != in.LineNo)bEqual = false;	// input line number 
		if (ErrMsgs != in.ErrMsgs)bEqual = false;	// errors found 
		if (Delimiter != in.Delimiter)bEqual = false;	// character at end of name 
		if (TSBase != in.TSBase)bEqual = false;	// base day for time stamps 
		//if(MaxAttVal=NULL;	// number of values for each att 
		//if(ClassThresh=NULL;	// thresholded class attribute 
		//if(SpecialStatus=NULL;// special att treatment 
		//if(AttDef=NULL;	// definitions of implicit atts 
		//if(SomeMiss=Nil;	// att has missing values 
		//if(SomeNA=Nil;	// att has N/A values 
		if (strcmp(FileStem, in.FileStem) != 0)bEqual = false;
		//if(Pruned=NULL;	// decision trees 
		//if(TrialPred=NULL;	// predictions for each boost trial 
		if (Confidence != in.Confidence)bEqual = false;	// set by classify() 
		//if(Vote=NULL;	// total votes for classes 
		//if(ClassSum=NULL;	// class weights during classification 
		//if(MCost=NULL;	// misclass cost [pred][real] 
		//if(Active=Nil;	// rules that fire while classifying case 
		if (NActive != in.NActive)bEqual = false;	// number ditto 
		if (ActiveSpace != in.ActiveSpace)bEqual = false;	// space currently allocated for rules 
		//if(RulesUsed=Nil;	// list of all rules used 
		if (NRulesUsed != in.NRulesUsed)bEqual = false;	// number ditto 
		//if(RuleSet=NULL;	// rulesets 
		if (Default != in.Default)bEqual = false;	// default class associated with ruleset or boosted classifier 
		//if(MostSpec=NULL;	// used in RuleClassify() 
		//if(TRf=NULL;		// file pointer for tree and rule i/o 
		if (memcmp(Fn, in.Fn, sizeof(char) * 500) != 0)bEqual = false;

		return bEqual;
	}
}