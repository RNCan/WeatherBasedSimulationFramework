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

#pragma once
//***********************************************************************
//									 
//		Definitions used by Cubist				 
//              --------------------------				 
//									 
//***********************************************************************

#define	 RELEASE	"2.07"
//#define USEDOUBLE

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#ifdef	 WIN64
#define	 WIN32
#endif

#define	 SMP
#define	 MAXCPU		8
#if defined WIN32 || defined _CONSOLE
//-----------------------------------
#include <windows.h>
#include <process.h>

#define	 Thread				HANDLE
#define	 ThreadFn			void *
#define	 BeginThread(t,f,a)		t = (Thread) _beginthreadex(NULL, 0,\
						(ThreadFn) (*f),\
						(PVOID) a, 0, NULL)
#define	 EndThread			_endthreadex(1)
#define	 ThreadReturn			return(0)
#define	 JoinThread(t)			{WaitForSingleObject(t, INFINITE);\
					 CloseHandle(t);}

#define	 Flag				HANDLE
#define	 MakeFlag(f,n)			f = CreateSemaphore(NULL, 0, n, NULL)
#define	 DestroyFlag(f)			CloseHandle(f)
#define	 RaiseFlag(f)			ReleaseSemaphore(f, 1, NULL)
#define	 WaitForFlag(f)			WaitForSingleObject(f, INFINITE)

#define	 Mutex				HANDLE
#define	 MakeMutex(m)			m = CreateMutex(NULL, FALSE, NULL)
#define	 DestroyMutex(m)		CloseHandle(m)
#define	 LockMutex(m)			WaitForSingleObject(m, INFINITE)
#define	 UnlockMutex(m)			ReleaseMutex(m)
//-----------------------------------
#else
//-----------------------------------
#include <pthread.h>
#include <semaphore.h>
#include <sys/unistd.h>

#ifndef _SC_NPROCESSORS_CONF
#define _SC_NPROCESSORS_CONF _SC_NPROC_CONF
#endif

#define	 Thread				pthread_t
#define	 ThreadFn			void *
#define	 BeginThread(t,f,a)		pthread_create(&t, NULL, f, (void *) a)
#define	 EndThread			pthread_exit(NULL)
#define	 ThreadReturn			return(0)
#define	 JoinThread(t)			pthread_join(t, NULL)

#define	 Flag				sem_t
#define	 MakeFlag(f,n)			sem_init(&f, 0, 0)
#define	 DestroyFlag(f)			sem_destroy(&f)
#define	 RaiseFlag(f)			sem_post(&f)
#define	 WaitForFlag(f)			sem_wait(&f)

#define	 Mutex				pthread_mutex_t
#define	 MakeMutex(m)			pthread_mutex_init(&m, NULL)
#define	 DestroyMutex(m)		pthread_mutex_destroy(&m)
#define	 LockMutex(m)			pthread_mutex_lock(&m)
#define	 UnlockMutex(m)			pthread_mutex_unlock(&m)
//-----------------------------------
#endif


//***********************************************************************
//									 
//		Constants, macros etc.					 
//									 
//***********************************************************************


#define	 CUBIST

#define  MAXN     20		// max number neighbors allowing for ties 

#define	 Nil	   0		// null pointer 
#define	 false	   0
#define	 true	   1
#define	 WIDTH_OUTPUT	   80		// approx max width of output 

#define  EXCLUDE   1		// special attribute status: do not use 
#define  SKIP	   2		// do not use in models 
#define  DISCRETE  4		// ditto: collect values as data read 
#define  ORDERED   8		// ditto: ordered discrete values 
#define  DATEVAL   16		// ditto: YYYY/MM/DD or YYYY-MM-DD 
#define  STIMEVAL  32		// ditto: HH:MM:SS 
#define	 TSTMPVAL  64		// date time 

#define	 UNKNOWN   1.5777218104420236e-30	// unlikely value! 
#define	 NA	   1

#define	 BrDiscr   1		// test on discrete attribute 
#define	 BrThresh  2		// threshold test on continuous attribute 
#define	 BrSubset  3		// subset test on discrete attribute 

#define  Plural(n)		((n) != 1 ? "s" : "")

#define  AllocZero(N,T)		(T *) Pcalloc(N, sizeof(T))
#define  Alloc(N,T)		AllocZero(N,T)  // for safety 
#define  Realloc(V,N,T)		V = (T *) Prealloc(V, (N)*sizeof(T))

#define	 Max(a,b)               ((a)>(b) ? a : b) 
#define	 Min(a,b)               ((a)<(b) ? a : b) 

#define	 Bit(b)			(1 << (b))
#define	 In(b,s)		((s[(b) >> 3]) & Bit((b) & 07))
#define	 ClearBits(n,s)		memset(s,0,n)
#define	 CopyBits(n,f,t)	memcpy(t,f,n)
#define	 SetBit(b,s)		(s[(b) >> 3] |= Bit((b) & 07))

#define	 ForEach(v,f,l)		for(v=f ; v<=l ; ++v) 
#define	 ForEach2(v,f,l)		for(int v=f ; v<=l ; ++v) 

#define	 StatBit(a,b)		(SpecialStatus[a]&(b))
#define	 Exclude(a)		StatBit(a,EXCLUDE)
#define	 Skip(a)		StatBit(a,EXCLUDE|SKIP)
#define  Discrete(a)		(MaxAttVal[a] || StatBit(a,DISCRETE))
#define  Continuous(a)		(! MaxAttVal[a] && ! StatBit(a,DISCRETE))
#define	 Ordered(a)		StatBit(a,ORDERED)
#define	 DateVal(a)		StatBit(a,DATEVAL)
#define	 TimeVal(a)		StatBit(a,STIMEVAL)
#define	 TStampVal(a)		StatBit(a,TSTMPVAL)

#define  NotApplic(c,a)		(DVal(c,a)==NA)
#define	 NotApplicVal(AV)	(AV._discr_val==NA)

#define  Space(s)		(s==' ' || s=='\n' || s=='\r' || s=='\t')
#define  SkipComment		while ( (c = InChar(f)) != '\n' && c != EOF )

#define	 Free(x)		{free(x); x=Nil;}
#define  FreeUnlessNil(p)	if((p)!=Nil) free(p)

#define  CheckClose(f)		if(f) {fclose(f); f=Nil;}

#define	 Before(n1,n2)		(n1->Tested < n2->Tested ||\
				 n1->Tested == n2->Tested && n1->Cut < n2->Cut)


#define	 NOFILE		 0
#define	 BADATTNAME	 1
#define	 EOFINATT	 2
#define	 SINGLEATTVAL	 3
#define	 BADATTVAL	 4
#define	 BADNUMBER	 5
#define	 DUPATTNAME	 6
#define	 NOMEM		 8
#define	 TOOMANYVALS	 9
#define	 BADDISCRETE	10
#define	 NOTARGET	11
#define	 BADTARGET	12
#define	 LONGNAME	13
#define	 HITEOF		14
#define	 MISSNAME	15
#define	 BADDATE	16
#define	 BADTIME	17
#define	 BADTSTMP	18
#define	 UNKNOWNATT	19
#define	 BADDEF1	20
#define	 BADDEF2	21
#define	 BADDEF3	22
#define	 SAMEATT	23
#define	 BADDEF4	24
#define	 MODELFILE	30
#define	 CWTATTERR	31

#define	 Of		stdout
#define	 Goodbye(x)	exit(x)
#define	 CharWidth(S)	((int) strlen(S))



//***********************************************************************
//									 
//		Type definitions					 
//									 
//***********************************************************************


typedef  unsigned char	Boolean, BranchType, *Set;
typedef	 char		*String;

typedef  int	CaseNo;		// data item number 
typedef  int	CaseCount;	// count of cases 

typedef  int	DiscrValue,	// discrete attribute value (0 = ?) 
Attribute;	// attribute number, 1..MaxAtt 

#ifdef USEDOUBLE
typedef	 double	ContValue;	// continuous attribute value 
#define	 PREC	14		// precision 
#else
typedef	 float	ContValue;	// continuous attribute value 
#define	 PREC	 7		// precision 
#endif

				// Attribute values are packed into a union:
					 //DVal = (int) discrete value
					 //CVal = (float) continuous value
					 //SVal = (int) offset in IgnoredVals

				   //Missing and non-applicable values are:

				   //  discrete:
				   //    not applicable:	DVal = NA
				   //    missing:		DVal = 0
				   //  continuous:
				   //    not applicable:	DVal = NA
				   //    missing:		CVal = UNKNOWN  

//typedef  union _attribute_value
typedef  union _attribute_value
{
	ContValue	_cont_val;
	DiscrValue	_discr_val;
}
AttValue, *DataRec;

#define  CVal(Case,Attribute)   Case[Attribute]._cont_val
#define  DVal(Case,Attribute)   Case[Attribute]._discr_val
#define  SVal(Case,Attribute)   Case[Attribute]._discr_val
#define  Class(Case)		Case[ClassAtt]._cont_val
#define	 PredVal(Case)		Case[MaxAtt+1]._cont_val
#define	 DRef1(Case)		Case[MaxAtt+1]._cont_val
#define	 DRef2(Case)		Case[MaxAtt+2]._cont_val


typedef  int	RuleNo;			// rule number 

typedef  struct _condrec
{
	BranchType	NodeType;	// test type 
	Attribute	Tested;		// attribute tested 
	ContValue	Cut;		// threshold (if relevant) 
	Set		Subset;		// subset (if relevant) 
	int		TestValue;	// specified outcome of test 
}
CondRec, *Condition;


typedef  struct _rulerec
{
	RuleNo	RNo;		// rule number 
	int		MNo,		// member number for committee models 
		Size;		// number of conditions 
	Condition	*Lhs;		// conditions themselves 
	double	*Rhs;		// model given by rule 
	CaseCount	Cover;		// number of cases covered 
	float	Mean,		// mean value of cases matching rule 
		LoVal,		// lowest value in data 
		HiVal,		// highest value in data 
		LoLim,		// lower bound on predictions 
		HiLim,		// upper bound on predictions 
		EstErr;		// estimated error 
}
RuleRec, *CRule;


typedef  struct _oldrulerec
{
	RuleNo	RNo;		// rule number 
	int		Size;		// number of conditions 
	Condition	*Lhs;		// conditions themselves 
	double	*Rhs;		// model given by rule 
	CaseCount	Cover;		// number of cases covered 
	float	Mean,		// mean value of cases matching rule 
		LoVal,		// lowest value in data 
		HiVal,		// highest value in data 
		LoLim,		// lower bound on predictions 
		HiLim,		// upper bound on predictions 
		EstErr;		// estimated error 
}
OldRuleRec;


typedef struct _rulesetrec
{
	RuleNo	SNRules;	// number of rules 
	CRule	*SRule;		// rules 
}
RuleSetRec, *RRuleSet;


typedef	 struct _indexrec	*Index;
typedef	 struct _indexrec
{
	Attribute	Tested;		// split attribute for KD-tree 
	ContValue	Cut,		// threshold for continuous atts 
		MinDRef[2],	// min reference distances 
		MaxDRef[2];	// max ditto 
	CaseNo	IFp, ILp;	// first and last item at leaf 
	Index	*SubIndex;	// subtrees 
}
IndexRec;


typedef  struct _nnrec
{
	int		BestI[MAXN];	// numbers of best instances 
	float	BestD[MAXN],	// distances to best instances 
		*WorstBest,	// points to worst BestD 
		*AttMinD;	// min attribute distance from case 
}
NNEnvRec, *NNEnv;


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


//***********************************************************************
//									 
//		Function prototypes					 
//									 
//***********************************************************************

void	    PrintResult(int t);
void	    PrepareForSMP(void);
void	    TerminateSMP(void);
//ThreadFn    ProcessCase(int t);

unsigned int __stdcall ProcessCase(void*);
Boolean	    ReadName(FILE *f, String s, int n, char ColonOpt);
void	    GetNames(FILE *Nf);
void	    ExplicitAtt(FILE *Nf);
int	    Which(String Val, String *List, int First, int Last);
void	    FreeNamesData();
int	    InChar(FILE *f);

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

void	    GetData(FILE *Df, Boolean Train, Boolean AllowUnknownTarget);
Boolean	    ReplaceUnknowns(DataRec Case, Boolean *AttMsg);
DataRec	    GetDataRec(FILE *Df, Boolean Train);
int	    StoreIVal(String S);
void	    CheckValue(DataRec Case, Attribute Att);
Boolean	    OutsideRange(DataRec Case);
void	    FreeCases(DataRec *Case, CaseNo MaxCase);
void	    FreeCase(DataRec DVec);

void	    CheckFile(String Extension, Boolean Write);
void	    ReadFilePrefix(String Extension);
void	    ReadHeader();
RRuleSet    *GetCommittee(String Extension);
RRuleSet    InRules();
CRule	    InRule();
Condition   InCondition();
void	    FreeCttee(RRuleSet *Cttee);
int	    ReadProp(char *Delim);
String	    RemoveQuotes(String S);
Set	    MakeSubset(Attribute Att);
void	    BinRecoverDiscreteNames();
RRuleSet    BinInRules();
void	    StreamIn(String S, int n);

void	    ReleaseRule(CRule R);
void	    PrintRules(RRuleSet RS, String Msg);
void	    PrintRule(CRule R);
void	    PrintCondition(Condition C);
Boolean	    Satisfies(DataRec CaseDesc, Condition OneCond);

float	    PredictValue(RRuleSet *Cttee, DataRec CaseDesc, float *ErrLim);
float	    RuleSetPrediction(RRuleSet RS, DataRec CaseDesc, float *ErrLim);
Boolean	    Matches(CRule R, DataRec Case);

void	    InitialiseInstances(RRuleSet *Cttee);
void	    CopyInstances(void);
float	    NNEstimate(RRuleSet *Cttee, DataRec Case, NNEnv E, float *ErrLim);
float	    Distance(DataRec Case1, DataRec Case2, float Thresh);
void	    CheckDistance(DataRec Case, CaseNo Saved, NNEnv E);
void	    FindNearestNeighbors(DataRec Case, NNEnv E);
float	    AverageNeighbors(RRuleSet *Cttee, DataRec Case, NNEnv E, float *ErrLim);
void	    ShowNearestNeighbors(int Offset, NNEnv E);
Index	    BuildIndex(CaseNo Fp, CaseNo Lp);
void	    ScanIndex(DataRec Case, Index Node, float MinD, NNEnv E);
void	    SwapInstance(CaseNo A, CaseNo B);
void	    FreeIndex(Index Node);
void	    FreeInstances(void);

char	    ProcessOption(int Argc, char *Argv[], char *Options);
void	    *Pmalloc(size_t Bytes);
void	    *Prealloc(void *Present, size_t Bytes);
void	    *Pcalloc(size_t Number, unsigned int Size);
void	    FreeVector(void **V, int First, int Last);

void	    Error(int ErrNo, String S1, String S2);
FILE	    *GetFile(String Extension, String RW);
int	    Denominator(ContValue Val);
int	    FracBase(Attribute Att);
int	    GetInt(String S, int N);
int	    DateToDay(String DS);
void	    DayToDate(int Day, String Date);
int	    TimeToSecs(String TS);
void	    SecsToTime(int Secs, String Time);
void	    SetTSBase(int y);
int	    TStampToMins(String TS);
void	    CValToStr(ContValue CV, Attribute Att, String DS);

#ifdef WIN32
double	    posrint(double);
//double	    rint(double);
#endif


//***********************************************************************
//									 
//		Text strings						 
//									 
//***********************************************************************


#define	 T_IgnoreNATarget	"*** Ignoring instances with N/A target value\n"
#define	 T_IgnoreBadTarget	"*** Ignoring instances with unknown or N/A"\
					" target value\n"
#define	 T_NoCases		"*** No instances with known target values\n"

#define	 T_ReplaceUnknowns	"\n    Replacing unknown attribute values:\n"
#define	 T_NoAppVals		"ignoring (no applicable values)"
#define	 T_By			"by"
#define	 T_Rule			"Rule"
#define	 TX_RInfo(c,p,m,l,h,e)	": [%d cases, mean %.*f, range %.7g to %.7g, "\
				"est err %.*f]\n\n",c,p,m,l,h,p,e
#define	 T_If			"if"
#define	 T_Then			"then"
#define	 T_ElementOf		"in"
#define	 T_InRange		"in"
#define	 T_IsUnknown		" is unknown\n"
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
#define	 E_NOMEM		"unable to allocate sufficient memory\n"
#define	 E_TOOMANYVALS(a,n)	"too many values for attribute `%s'"\
					" (max %d)\n", a, n
#define	 E_BADDISCRETE		"bad number of discrete values for attribute"\
					" `%s'\n"
#define	 E_NOTARGET		"target attribute `%s' not found\n"
#define	 E_BADTARGET		"target attribute `%s' is not numeric\n"
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
#define	 E_SAMEATT(a,b)		"attribute `%s' is identical to attribute"\
					" `%s'\n", a, b
#define	 E_BADDEF3		"cannot define target attribute `%s'\n"
#define	 E_BADDEF4		"target attribute appears in definition"\
					" of attribute `%s':\n"
#define	 EX_MODELFILE(f)	"file %s incompatible with .names file\n", f
#define	 E_MFATT		"undefined or excluded attribute"
#define	 E_MFATTVAL		"undefined attribute value"
#define	 E_MFEOF		"unexpected eof"
#define	 T_ErrorLimit		"Error limit exceeded\n"

#ifdef _CONSOLE
#define	 finite(x)	_finite(x)
#endif
