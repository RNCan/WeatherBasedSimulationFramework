//***********************************************************************
//									 
//	Source code for use with See5/C5.0 Release 2.07			 
//	-----------------------------------------------			 
//		      Copyright RuleQuest Research 2009			 
//									 
//	This code is provided "as is" without warranty of any kind,	 
//	either express or implied.  All use is at your own risk.	 
//									 
//***********************************************************************

#pragma once 

#include <vector>
#include "basic/ERMsg.h"
#include "Geomatic/See5global.h"

namespace WBSF
{

#define	MAXLINEBUFFER	10000

#define FailSyn(Msg)	 {DefSyntaxError(Msg); return false;}
#define FailSem(Msg)	 {DefSemanticsError(Fi, Msg, OpCode); return false;}

	typedef  union  _xstack_elt
	{
		DiscrValue  _discr_val;
		ContValue   _cont_val;
		String      _string_val;
	}XStackElt;

#define	cval		_cont_val
#define	sval		_string_val
#define	dval		_discr_val


	//typedef short AttributeBandRef[2][4];

	typedef std::vector<AttValue> CDecisionTreeBlock;
	class CDecisionTreeBaseEx : public CDecisionTreeBase
	{
	public:

		CDecisionTreeBaseEx();

		void LoadModel(String filePath);
		int GetNbVirtualBands()const;

		bool IsValidVirtualPixel(int Att, DataRec Case, ContValue noData);
		bool IsValidPixel(DataRec Case, ContValue noData);
		bool IsValidPixel(DataRec Case, std::vector<double>& noData);

		Boolean ReadName(FILE *f, String s, int n, char ColonOpt);
		void GetNames(FILE *Nf);
		void ExplicitAtt(FILE *Nf);
		int Which(String Val, const String *List, int First, int Last);



		DataRec GetDataRec(FILE *Df, Boolean Train);
		int StoreIVal(String S);
		void CheckValue(DataRec DVec, Attribute Att)const;


		//***********************************************************************
		//									 
		//	Routines to handle implicitly-defined attributes		 
		//									 
		//***********************************************************************

		//
		//char	*Buff;			// buffer for input characters 
		//int	BuffSize, BN;		// size and index of next character 
		//
		//EltRec	*TStack;		// expression stack model 
		//int	TStackSize,w TSN;	// size of stack and index of next entry 
		//
		//int	DefSize, DN;		// size of definition and next element 
		//
		//Boolean PreviousError;		// to avoid parasytic errors 
		//
		//AttValue _UNK,			// quasi-constant for unknown value 
		//	 _NA;			// ditto for not applicable 




		void ImplicitAtt(FILE *Nf);
		void ReadDefinition(FILE *f);
		void Append(char c);
		Boolean Expression();
		Boolean Conjunct();
		Boolean SExpression();
		Boolean AExpression();
		Boolean Term();
		Boolean Factor();
		Boolean Primary();
		Boolean Atom();
		Boolean Find(String S);
		int FindOne(const String *Alt);
		Attribute FindAttName();
		void DefSyntaxError(String Msg);
		void DefSemanticsError(int Fi, String Msg, int OpCode);
		void Dump(char OpCode, ContValue F, String S, int Fi);
		void DumpOp(char OpCode, int Fi);
		Boolean UpdateTStack(char OpCode, ContValue F, String S, int Fi);
		AttValue EvaluateDef(Definition D, DataRec Case)const;

		void ReadFilePrefix(String Extension);
		void ReadHeader();
		Tree GetTree(String Extension);
		CRuleSet GetRules(String Extension);
		CRuleSet InRules();
		CRule InRule();
		Condition InCondition();

		void ConstructRuleTree(CRuleSet RS);
		void SetTestIndex(Condition C);
		RuleTree GrowRT(RuleNo *RR, int RRN, CRule *Rule);
		int DesiredOutcome(CRule R, int TI);
		int SelectTest(RuleNo *RR, int RRN, CRule *Rule);
		int ReadProp(char *Delim);
		String RemoveQuotes(String S);
		Set MakeSubset(Attribute Att);
		void BinRecoverDiscreteNames();
		Tree BinInTree();
		CRuleSet BinInRules();
		void StreamIn(String S, int n);
		Tree Leaf(double *Freq, ClassNo NodeClass, CaseCount Cases, CaseCount Errors);
		void GetMCosts(FILE *Cf);
		ClassNo TreeClassify(DataRec Case, Tree DecisionTree);
		void FollowAllBranches(DataRec Case, Tree T, float Fraction);
		void FindLeaf(DataRec Case, Tree T, Tree PT, float Fraction);
		ClassNo RuleClassify(DataRec Case, CRuleSet RS);
		int FindOutcome(DataRec Case, Condition OneCond);
		Boolean Satisfies(DataRec Case, Condition OneCond);
		Boolean Matches(CRule R, DataRec Case);
		void CheckActiveSpace(int N);
		void MarkActive(RuleTree RT, DataRec Case);
		ClassNo BoostClassify(DataRec Case, int MaxTrial);
		ClassNo SelectClass(ClassNo Default, Boolean UseCosts);
		ClassNo Classify(DataRec Case);
		float Interpolate(Tree T, ContValue Val);
		FILE *GetFile(String Extension, String RW);
		void CheckFile(String Extension, Boolean Write);
		char ProcessOption(int Argc, char *Argv[], char *Options);

		void Error(int ErrNo, String S1, String S2)const;
		//void XError(int ErrNo, String S1, String S2){Error(ErrNo, S1, S2);}
		void SetTSBase(int y);
		int TStampToMins(String TS);
		void FreeLastCase(DataRec DVec);
		void FreeGlobals();
		void FreeNames();
		void FreeTree(Tree T);
		void FreeRule(CRule R);
		void FreeRuleTree(RuleTree RT);
		void FreeRules(CRuleSet RS);
		void FreeVector(void **V, int First, int Last);


		Tree InTree();


		int InChar(FILE *f);
		//temp buffer
		char	LineBuffer[MAXLINEBUFFER];
		char	*LBp;


		char	*Buff;			/* buffer for input characters */
		int	BuffSize, BN;		/* size and index of next character */

		EltRec	*TStack;		/* expression stack model */
		int	TStackSize, TSN;	/* size of stack and index of next entry */

		int	DefSize, DN;		/* size of definition and next element */

		Boolean PreviousError;		/* to avoid parasytic errors */

		AttValue _UNK,			/* quasi-constant for unknown value */
			_NA;			/* ditto for not applicable */


		char	*LastExt;
		int	OptNo;

		Condition	*Test;
		int		NTest;
		int		TestSpace;
		int		*TestOccur;
		int		*RuleCondOK;
		Boolean		*TestUsed;


		Boolean	BINARY;
		int	Entry;

		char	PropName[20];
		char	*PropVal;
		char	*Unquoted;
		int	PropValSize;

		//	std::vector<AttributeBandRef> m_attributeBandeRef;

	};





	typedef std::vector < CDecisionTreeBaseEx > CDecisionTreeVector;
	class CDecisionTree : public CDecisionTreeVector
	{
	public:

		virtual ~CDecisionTree(){ FreeMemory(); }
		ERMsg Load(const std::string& filePath, size_t numThread, size_t numIOThread = 1, bool bQuiet = false);
		void FreeMemory();


	};



	void *Pmalloc(CDecisionTreeBaseEx& DT, size_t Bytes);
	void *Prealloc(CDecisionTreeBaseEx& DT, void *Present, size_t Bytes);
	void *Pcalloc(CDecisionTreeBaseEx& DT, size_t Number, unsigned int Size);

	int Denominator(ContValue Val);
	int GetInt(String S, int N);
	int DateToDay(String DS);	//  Day 1 is 0000/03/01  
	int TimeToSecs(String TS);

}