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

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif


//***********************************************************************
//									 
//		Constants, macros etc.					 
//									 
//***********************************************************************


#define	 SEE5

#define	 Nil	   0		// null pointer 
#define	 False	   0
#define	 True	   1
#define	 None	   -1 
#define	 Epsilon   1E-4

#define  EXCLUDE   1		// special attribute status: do not use 
#define  SKIP	   2		// do not use in classifiers 
#define  DISCRETE  4		// ditto: collect values as data read 
#define  ORDERED   8		// ditto: ordered discrete values 
#define  DATEVAL   16		// ditto: YYYY/MM/DD or YYYY-MM-DD 
#define  STIMEVAL  32		// ditto: HH:MM:SS 
#define	 TSTMPVAL  64		// date time 

				// unknown and N/A values are represented by
				// unlikely floating-point numbers
				// (octal 01600000000 and 01) 
#define	 DT_UNKNOWN   01600000000	// 1.5777218104420236e-30 
#define	 DT_NA	   01		// 1.4012984643248171e-45 

#define	 BrDiscr   1
#define	 BrThresh  2
#define	 BrSubset  3

#define  Alloc(DT, N,T)		(T *) Pmalloc(DT, (N)*sizeof(T))
#define  AllocZero(DT, N,T)		(T *) Pcalloc(DT, N, sizeof(T))
#define  Realloc(DT, V,N,T)		V = (T *) Prealloc(DT, V, (N)*sizeof(T))

#define	 Bit(b)			(1 << (b))
#define	 In(b,s)		((s[(b) >> 3]) & Bit((b) & 07))
#define	 SetBit(b,s)		(s[(b) >> 3] |= Bit((b) & 07))

#define	 ForEach(v,f,l)		for(v=f ; v<=l ; ++v) 

#define	 StatBit(DT, a,b)		(DT.SpecialStatus[a]&(b))
#define	 Exclude(DT, a)		StatBit(DT, a,EXCLUDE)
#define	 Skip(DT, a)		StatBit(DT, a,EXCLUDE|SKIP)
#define  Discrete(DT, a)		(DT.MaxAttVal[a] || StatBit(DT,a,DISCRETE))
#define  Continuous(DT, a)		(! DT.MaxAttVal[a] && ! StatBit(DT, a,DISCRETE))
#define	 Ordered(DT, a)		StatBit(DT, a,ORDERED)
#define	 DateVal(DT, a)		StatBit(DT, a,DATEVAL)
#define	 TimeVal(DT, a)		StatBit(DT, a,STIMEVAL)
#define	 TStampVal(DT, a)		StatBit(DT, a,TSTMPVAL)

#define  Space(s)		(s==' ' || s=='\n' || s=='\r' || s=='\t')
#define  SkipComment		while ( ( c = InChar(f) ) != '\n' && c != EOF )

#define	 FreeUnlessNil(p)	if((p)!=Nil) free(p),p=Nil
#define	 Free(p)	 	free(p),p=Nil

//#define	 assert(x)

#ifdef WIN32
#define  rint(x)		((x)<0 ? (double)((int)((x)-0.5)) :\
					 (double)((int)((x)+0.5)))
#define	 finite(x)		_finite(x)
#define	 strdup(x)		_strdup(x)
#endif

#define	 P1(x)			(rint((x)*10) / 10)
#define	 Of			stdout
#define	 Goodbye(x)		throw("error")
#define	 CharWidth(s)		((int) strlen(s))


#define	 NOFILE		 0
#define	 BADCLASSTHRESH	 1
#define	 LEQCLASSTHRESH	 2
#define	 BADATTNAME	 3
#define	 EOFINATT	 4
#define	 SINGLEATTVAL	 5
#define	 BADATTVAL	 6
#define	 BADNUMBER	 7
#define	 BADCLASS	 8
#define	 BADCOSTCLASS	 9
#define	 BADCOST	10
#define	 NOMEM		11
#define	 TOOMANYVALS	12
#define	 BADDISCRETE	13
#define	 NOTARGET	14
#define	 BADCTARGET	15
#define	 BADDTARGET	16
#define	 LONGNAME	17
#define	 HITEOF		18
#define	 MISSNAME	19
#define	 BADDATE	20
#define	 BADTIME	21
#define	 BADTSTMP	22
#define	 DUPATTNAME	23
#define	 UNKNOWNATT	24
#define	 BADDEF1	25
#define	 BADDEF2	26
#define	 BADDEF3	27
#define	 BADDEF4	28
#define	 SAMEATT	29
#define	 MODELFILE	30
#define	 CWTATTERR	31


//***********************************************************************
//									 
//		Type definitions					 
//									 
//***********************************************************************


typedef  unsigned char	Boolean, BranchType, *Set, Byte;
typedef	 char		*String;

typedef  int	CaseNo;			// data item number 
typedef  float	CaseCount;		// count of (partial) items 

typedef  int	ClassNo,		// class number, 1..MaxClass 
		DiscrValue,		// discrete attribute value (0 = ?) 
		Attribute;		// attribute number, 1..MaxAtt 

typedef	 float	ContValue;		// continuous attribute value 
#define	 PREC	 7			// precision 


typedef  union	 _def_val
	 {
	    String	_s_val;		// att val for comparison 
	    ContValue	_n_val;		// number for arith 
	 }
	 DefVal;


typedef  struct  _def_elt
	 {
	    short	_op_code;	// type of element 
	    DefVal	_operand;	// string or numeric value 
	 }
	 DefElt, *Definition;


typedef  struct  _elt_rec
	 {
	    int		Fi,		// index of first char of element 
			Li;		// last ditto 
	    char	Type;		// 'B', 'S', or 'N' 
	 }
	 EltRec;


#define	 DefOp(DE)	DE._op_code
#define	 DefSVal(DE)	DE._operand._s_val
#define	 DefNVal(DE)	DE._operand._n_val

#define	 OP_ATT			 0	// opcodes 
#define	 OP_NUM			 1
#define	 OP_STR			 2
#define	 OP_MISS		 3
#define	 OP_AND			10
#define	 OP_OR			11
#define	 OP_EQ			20
#define	 OP_NE			21
#define	 OP_GT			22
#define	 OP_GE			23
#define	 OP_LT			24
#define	 OP_LE			25
#define	 OP_SEQ			26
#define	 OP_SNE			27
#define	 OP_PLUS		30
#define	 OP_MINUS		31
#define	 OP_UMINUS		32
#define	 OP_MULT		33
#define	 OP_DIV			34
#define	 OP_MOD			35
#define	 OP_POW			36
#define	 OP_SIN			40
#define	 OP_COS			41
#define	 OP_TAN			42
#define	 OP_LOG			43
#define	 OP_EXP			44
#define	 OP_INT			45
#define	 OP_END			99


typedef  union  _attribute_value
	 {
	    DiscrValue	_discr_val;
	    ContValue	_cont_val;
	 }
	 AttValue, *DataRec;

#define  CVal(Case,Attribute)   Case[Attribute]._cont_val
#define  DVal(Case,Attribute)   Case[Attribute]._discr_val
#define  XDVal(Case,Att)	(Case[Att]._discr_val & 077777777)
#define  SVal(Case,Attribute)   Case[Attribute]._discr_val
#define  Class(Case)		(*Case)._discr_val
#define  Weight(Case)		(*(Case-1))._cont_val

#define	 Unknown(Case,Att)	(DVal(Case,Att)==DT_UNKNOWN)
#define	 UnknownVal(AV)		(AV._discr_val==DT_UNKNOWN)
#define	 NotApplic(Case,Att)	(DVal(Case,Att)==DT_NA)
#define	 NotApplicVal(AV)	(AV._discr_val==DT_NA)


typedef  struct _treerec	*Tree;
typedef  struct _treerec
	 {
	    BranchType	NodeType;
	    ClassNo	Leaf;		// best class at this node 
	    CaseCount	Cases,		// no of items at this node 
			*ClassDist,	// class distribution of items 
	    		Errors;		// no of errors at this node 
	    Attribute	Tested; 	// attribute referenced in test 
	    int		Forks;		// number of branches at this node 
	    ContValue	Cut,		// threshold for continuous attribute 
		  	Lower,		// lower limit of soft threshold 
		  	Upper,		// upper limit ditto 
			Mid;		// 50% point 
	    Set         *Subset;	// subsets of discrete values  
	    Tree	*Branch;	// Branch[x] = subtree for outcome x 
	 }
	 TreeRec;


typedef  int	RuleNo;			// rule number 

typedef  struct _condrec
	 {
	    BranchType	NodeType;	// test type (see tree nodes) 
	    Attribute	Tested;		// attribute tested 
	    int		Forks;		// possible branches 
	    ContValue	Cut;		// threshold (if relevant) 
	    Set		Subset;		// subset (if relevant) 
	    int		TestValue,	// specified outcome of test 
			TestI;		// rule tree index of this test 
	 }
	 CondRec, *Condition;


typedef  struct _rulerec
	 {
	    RuleNo	RNo;		// rule number 
	    int		TNo,		// trial number 
	    		Size;		// number of conditions 
	    Condition	*Lhs;		// conditions themselves 
	    ClassNo	Rhs;		// class given by rule 
	    CaseCount	Cover,		// number of cases covered by rule 
			Correct;	// number on which correct 
	    float	Prior;		// prior probability of RHS 
	    int		Vote;		// unit = 0.001 
	 }
	 RuleRec, *CRule;


typedef  struct _ruletreerec *RuleTree;
typedef  struct _ruletreerec
	 {
	    RuleNo	*Fire;		// rules matched at this node 
	    Condition	CondTest;	// new test 
	    int		Forks;		// number of branches 
	    RuleTree	*Branch;	// subtrees 
	 }
	 RuleTreeRec;


typedef struct _rulesetrec
	 {
	    RuleNo	SNRules;	// number of rules 
	    CRule	*SRule;		// rules 
	    ClassNo	SDefault;	// default class for this ruleset 
	    RuleTree	RT;		// rule tree for this ruleset 
	 }
	 RuleSetRec, *CRuleSet;


//***********************************************************************
//									 
//		Function prototypes					 
//									 
//***********************************************************************
/*
Boolean	    ReadName(FILE *f, String s, int n, char ColonOpt);
void	    GetNames(FILE *Nf);
void	    ExplicitAtt(FILE *Nf);
int	    Which(String Val, String *List, int First, int Last);
int	    InChar(FILE *f);

DataRec	    GetDataRec(FILE *Df, Boolean Train);
int	    StoreIVal(String S);
void	    CheckValue(DataRec DVec, Attribute Att);

void	    ImplicitAtt(FILE *Nf);
void	    ReadDefinition(FILE *f);
void	    Append(char c);
Boolean	    Expression();
Boolean	    Conjunct();
Boolean	    SExpression();
Boolean	    AExpression();
Boolean	    Term();
Boolean	    Factor();
Boolean	    Primary();
Boolean	    Atom();
Boolean	    Find(String S);
int	    FindOne(String *Alt);
Attribute   FindAttName();
void	    DefSyntaxError(String Msg);
void	    DefSemanticsError(int Fi, String Msg, int OpCode);
void	    Dump(char OpCode, ContValue F, String S, int Fi);
void	    DumpOp(char OpCode, int Fi);
Boolean	    UpdateTStack(char OpCode, ContValue F, String S, int Fi);
AttValue    EvaluateDef(Definition D, DataRec Case);

void	    ReadFilePrefix(String Extension);
void	    ReadHeader();
Tree	    GetTree(String Extension);
Tree	    InTree();
CRuleSet    GetRules(String Extension);
CRuleSet    InRules();
CRule	    InRule();
Condition   InCondition();
void	    ConstructRuleTree(CRuleSet RS);
void	    SetTestIndex(Condition C);
RuleTree    GrowRT(RuleNo *RR, int RRN, CRule *Rule);
int	    DesiredOutcome(CRule R, int TI);
int	    SelectTest(RuleNo *RR, int RRN, CRule *Rule);
int	    ReadProp(char *Delim);
String	    RemoveQuotes(String S);
Set	    MakeSubset(Attribute Att);
void	    BinRecoverDiscreteNames();
Tree	    BinInTree();
CRuleSet    BinInRules();
void	    StreamIn(String S, int n);
Tree	    Leaf(double *Freq, ClassNo NodeClass, CaseCount Cases,
		 CaseCount Errors);

void	    GetMCosts(FILE *f);

ClassNo	    TreeClassify(DataRec CaseDesc, Tree DecisionTree);
void	    FollowAllBranches(DataRec CaseDesc, Tree T, float Fraction);
void	    FindLeaf(DataRec CaseDesc, Tree T, Tree PT, float Wt);
ClassNo	    RuleClassify(DataRec CaseDesc, CRuleSet RS);
int	    FindOutcome(DataRec Case, Condition OneCond);
Boolean	    Satisfies(DataRec CaseDesc, Condition OneCond);
Boolean	    Matches(CRule R, DataRec Case);
void	    CheckActiveSpace(int N);
void	    MarkActive(RuleTree RT, DataRec Case);
ClassNo	    BoostClassify(DataRec CaseDesc, int MaxTrial);
ClassNo	    SelectClass(ClassNo Default, Boolean UseCosts);
ClassNo	    Classify(DataRec CaseDesc);
float	    Interpolate(Tree T, ContValue Val);

FILE *	    GetFile(String Extension, String RW);
void	    CheckFile(String Extension, Boolean Write);

char	    ProcessOption(int Argc, char *Argv[], char *Options);
void	    *Pmalloc(size_t Bytes);
void	    *Prealloc(void *Present, size_t Bytes);
void	    *Pcalloc(size_t Number, unsigned Size);
void	    Error(int ErrNo, String S1, String S2);
int	    Denominator(ContValue Val);
int	    GetInt(String S, int N);
int	    DateToDay(String DS);
int	    TimeToSecs(String TS);
void	    SetTSBase(int y);
int	    TStampToMins(String TS);

//void	    FreeGlobals();
void	    FreeCosts();
void	    FreeNames();
void	    FreeTree(Tree T);
void	    FreeRule(CRule R);
void	    FreeRuleTree(RuleTree RT);
void	    FreeRules(CRuleSet RS);
void	    FreeLastCase(DataRec DVec);
void	    FreeVector(void **V, int First, int Last);
*/

//***********************************************************************
//									 
//		Text strings						 
//									 
//***********************************************************************


#define	 TX_Line(l,f)		"\n*** line %d of `%s': ", l, f
#define	 E_NOFILE(f,e)		"cannot open file %s%s\n", f, e
#define	 E_BADATTNAME		"`:' or `:=' expected after attribute name"\
					" `%s'\n"
#define	 E_EOFINATT		"unexpected eof while reading attribute `%s'\n"
#define	 E_SINGLEATTVAL(a,v)	"attribute `%s' has only one value `%s'\n",\
					a, v
#define	 E_DUPATTNAME		"multiple attributes with name `%s'\n"
#define	 E_CWTATTERR		"case weight attribute must be continuous\n"
#define	 E_BADATTVAL(v,a)	"bad value of `%s' for attribute `%s'\n", v, a
#define	 E_BADNUMBER(a)		"value of `%s' changed to `?'\n", a
#define	 E_BADCLASS		"bad class value `%s'l\n"
#define	 E_BADCLASSTHRESH	"bad class threshold `%s'\n"
#define	 E_LEQCLASSTHRESH	"class threshold `%s' <= previous threshold\n"
#define	 E_BADCOSTCLASS		"bad class `%s'\n"
#define	 E_BADCOST		"bad cost value `%s'\n"
#define	 E_NOMEM		"unable to allocate sufficient memory\n"
#define	 E_TOOMANYVALS(a,n)	"too many values for attribute `%s'"\
					" (max %d)\n", a, n
#define	 E_BADDISCRETE		"bad number of discrete values for attribute"\
					" `%s'\n"
#define	 E_NOTARGET		"target attribute `%s' not found\n"
#define	 E_BADCTARGET		"target attribute `%s' must be"\
					" type `continuous'\n"
#define	 E_BADDTARGET		"target attribute `%s' must be specified by"\
					" a list of discrete values\n"
#define	 E_LONGNAME		"overlength name: check data file formats\n"
#define	 E_HITEOF		"unexpected end of file\n"
#define	 E_MISSNAME		"missing name or value before `%s'\n"
#define	 E_BADTSTMP(d,a)	"bad timestamp `%s' for attribute `%s'\n", d, a
#define	 E_BADDATE(d,a)		"bad date `%s' for attribute `%s'\n", d, a
#define	 E_BADTIME(d,a)		"bad time `%s' for attribute `%s'\n", d, a
#define	 E_UNKNOWNATT		"unknown attribute name `%s'\n"
#define	 E_BADDEF1(a,s,x)	"in definition of attribute `%s':\n"\
					"\tat `%.12s': expect %s\n", a, s, x
#define	 E_BADDEF2(a,s,x)	"in definition of attribute `%s':\n"\
					"\t`%s': %s\n", a, s, x
#define	 E_BADDEF3		"cannot define target attribute `%s'\n"
#define	 E_BADDEF4		"[warning] target attribute appears in"\
					" definition of attribute `%s'\n"
#define	 E_SAMEATT(a,b)		"[warning] attribute `%s' is identical to"\
					" attribute `%s'\n", a, b
#define	 EX_MODELFILE(f)	"file %s incompatible with .names file\n", f
#define	 E_MFATT		"undefined or excluded attribute"
#define	 E_MFATTVAL		"undefined attribute value"
#define	 E_MFCLASS		"undefined class"
#define	 E_MFEOF		"unexpected eof"
#define	 T_ErrorLimit		"Error limit exceeded\n"
