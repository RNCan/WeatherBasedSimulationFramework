/*************************************************************************/
/*									 */
/*	Source code for use with Cubist Release 2.09			 */
/*	--------------------------------------------			 */
/*		   Copyright RuleQuest Research 2016			 */
/*									 */
/*	This code is provided "as is" without warranty of any kind,	 */
/*	either express or implied.  All use is at your own risk.	 */
/*									 */
/*************************************************************************/

#include <float.h>
#include "Cubistdefns.h"
#include "Cubistglobal.c"

//#define	 false	   0
//#define	 true	   1

//extern int NCPU;
//extern int Delimiter;
//extern char		Fn[512];
//extern int LineNo;
//extern int ErrMsgs;
//extern int AttExIn;
//extern DiscrValue	*MaxAttVal;
//extern String		*AttName;

/*************************************************************************/
/*									 */
/*	Find number of CPUs to be used					 */
/*									 */
/*************************************************************************/


void FindNCPU(void)
/*   --------  */
{
#if defined WIN32 || defined _CONSOLE
	SYSTEM_INFO	SystemInfo;

	GetSystemInfo(&SystemInfo);
	NCPU = SystemInfo.dwNumberOfProcessors;
#else
	NCPU = sysconf(_SC_NPROCESSORS_CONF);
#endif
	if (NCPU > MAXCPU) NCPU = MAXCPU;
}



/*=======================================================================*/
/*									 */
/*	Get names of classes, attributes and attribute values		 */
/*									 */
/*=======================================================================*/



#define	MAXLINEBUFFER	10000
char	LineBuffer[MAXLINEBUFFER], *LBp = LineBuffer;


/*************************************************************************/
/*									 */
/*	Read a name from file f into string s, setting Delimiter.	 */
/*									 */
/*	- Embedded periods are permitted, but periods followed by space	 */
/*	  characters act as delimiters.					 */
/*	- Embedded spaces are permitted, but multiple spaces are	 */
/*	  replaced by a single space.					 */
/*	- Any character can be escaped by '\'.				 */
/*	- The remainder of a line following '|' is ignored.		 */
/*									 */
/*************************************************************************/


Boolean ReadName(FILE *f, String s, int n, char ColonOpt)
/*      --------  */
{
	register char *Sp = s;
	register int  c;
	char	  Msg[2];

	/*  Skip to first non-space character  */

	while ((c = InChar(f)) == '|' || Space(c))
	{
		if (c == '|') SkipComment;
	}

	/*  Return false if no names to read  */

	if (c == EOF)
	{
		Delimiter = EOF;
		return false;
	}

	/*  Read in characters up to the next delimiter  */

	while (c != ColonOpt && c != ',' && c != '\n' && c != '|' && c != EOF)
	{
		if (--n <= 0)
		{
			if (Of) Error(LONGNAME, "", "");
		}

		if (c == '.')
		{
			if ((c = InChar(f)) == '|' || Space(c) || c == EOF) break;
			*Sp++ = '.';
			continue;
		}

		if (c == '\\')
		{
			c = InChar(f);
		}

		if (Space(c))
		{
			*Sp++ = ' ';

			while ((c = InChar(f)) == ' ' || c == '\t')
				;
		}
		else
		{
			*Sp++ = c;
			c = InChar(f);
		}
	}

	if (c == '|') SkipComment;
	Delimiter = c;

	/*  Special case for ':='  */

	if (Delimiter == ':')
	{
		if (*LBp == '=')
		{
			Delimiter = '=';
			LBp++;
		}
	}

	/*  Strip trailing spaces  */

	while (Sp > s && Space(*(Sp - 1))) Sp--;

	if (Sp == s)
	{
		Msg[0] = (Space(c) ? '.' : c);
		Msg[1] = '\00';
		Error(MISSNAME, Fn, Msg);
	}

	*Sp++ = '\0';
	return true;
}



/*************************************************************************/
/*									 */
/*	Read names of classes, attributes and legal attribute values.	 */
/*	On completion, names are stored in:				 */
/*	  AttName	-	attribute names				 */
/*	  AttValName	-	attribute value names			 */
/*	with:								 */
/*	  MaxAttVal	-	number of values for each attribute	 */
/*									 */
/*	Other global variables set are:					 */
/*	  MaxAtt	-	maximum attribute number		 */
/*	  MaxDiscrVal	-	maximum discrete values for an attribute */
/*									 */
/*	Note:  until the number of attributes is known, the name	 */
/*	       information is assembled in local arrays			 */
/*									 */
/*************************************************************************/


void GetNames(FILE *Nf)
/*   --------  */
{
	char	Buffer[1000] = "", *Target;
	int		AttCeiling = 100;
	Attribute	Att;

	ErrMsgs = AttExIn = LineNo = 0;
	LBp = LineBuffer;
	*LBp = 0;

	/*  Get name of dependent att  */

	ReadName(Nf, Buffer, 1000, ':');
	Target = strdup(Buffer);

	/*  Get attribute and attribute value names from names file  */

	AttName = AllocZero(AttCeiling, String);
	MaxAttVal = AllocZero(AttCeiling, DiscrValue);
	AttValName = AllocZero(AttCeiling, String *);
	SpecialStatus = AllocZero(AttCeiling, char);
	AttDef = AllocZero(AttCeiling, Definition);

	MaxAtt = LabelAtt = CWtAtt = 0;
	while (ReadName(Nf, Buffer, 1000, ':'))
	{
		if (Delimiter != ':' && Delimiter != '=')
		{
			Error(BADATTNAME, Buffer, "");
		}

		/*  Check for include/exclude instruction  */

		if ((*Buffer == 'a' || *Buffer == 'A') &&
			!memcmp(Buffer + 1, "ttributes ", 10) &&
			!memcmp(Buffer + strlen(Buffer) - 6, "cluded", 6))
		{
			AttExIn = (!memcmp(Buffer + strlen(Buffer) - 8, "in", 2) ? 1 : -1);
			if (AttExIn == 1)
			{
				ForEach(Att, 1, MaxAtt)
				{
					SpecialStatus[Att] |= SKIP;
				}
			}

			while (ReadName(Nf, Buffer, 1000, ':'))
			{
				Att = Which(Buffer, AttName, 1, MaxAtt);
				if (!Att)
				{
					Error(UNKNOWNATT, Buffer, Nil);
				}
				else
					if (AttExIn == 1)
					{
						SpecialStatus[Att] -= SKIP;
					}
					else
					{
						SpecialStatus[Att] |= SKIP;
					}
			}

			break;
		}

		if (Which(Buffer, AttName, 1, MaxAtt) > 0)
		{
			Error(DUPATTNAME, Buffer, Nil);
		}

		if (++MaxAtt >= AttCeiling)
		{
			AttCeiling += 100;
			Realloc(AttName, AttCeiling, String);
			Realloc(MaxAttVal, AttCeiling, DiscrValue);
			Realloc(AttValName, AttCeiling, String *);
			Realloc(SpecialStatus, AttCeiling, char);
			Realloc(AttDef, AttCeiling, Definition);
		}

		AttName[MaxAtt] = strdup(Buffer);
		SpecialStatus[MaxAtt] = 0;
		AttDef[MaxAtt] = Nil;
		MaxAttVal[MaxAtt] = 0;

		if (Delimiter == '=')
		{
			ImplicitAtt(Nf);
		}
		else
		{
			ExplicitAtt(Nf);
		}

		/*  Check for case weight attribute, which must be type continuous  */

		if (!strcmp(AttName[MaxAtt], "case weight"))
		{
			CWtAtt = MaxAtt;

			if (!Continuous(CWtAtt))
			{
				Error(CWTATTERR, "", "");
			}
		}
	}

	ClassAtt = Which(Target, AttName, 1, MaxAtt);

	/*  Make sure not excluding class attribute  */

	if (Skip(ClassAtt)) SpecialStatus[ClassAtt] -= SKIP;

	/*  Class attribute must be defined and must be continuous  */

	if (ClassAtt <= 0)
	{
		Error(NOTARGET, Target, "");
	}
	else
		if (MaxAttVal[ClassAtt] > 0 ||
			StatBit(ClassAtt, DISCRETE | DATEVAL | STIMEVAL | EXCLUDE))
		{
			Error(BADTARGET, Target, "");
		}

	/*  Ignore case weight attribute if it is excluded; otherwise,
	it cannot be used in models  */

	if (CWtAtt)
	{
		if (Skip(CWtAtt))
		{
			CWtAtt = 0;
		}
		else
		{
			SpecialStatus[CWtAtt] |= SKIP;
		}
	}

	fclose(Nf);
	Free(Target);

	if (ErrMsgs > 0) Goodbye(1);
}



/*************************************************************************/
/*									 */
/*	Continuous or discrete attribute				 */
/*									 */
/*************************************************************************/


void ExplicitAtt(FILE *Nf)
/*   -----------  */
{
	char	Buffer[1000] = "", *p;
	DiscrValue	v;
	int		ValCeiling = 100, BaseYear;
	time_t	clock;

	/*  Read attribute type or first discrete value  */

	if (!(ReadName(Nf, Buffer, 1000, ':')))
	{
		Error(EOFINATT, AttName[MaxAtt], "");
	}

	MaxAttVal[MaxAtt] = 0;

	if (Delimiter != ',')
	{
		/*  Typed attribute  */

		if (!strcmp(Buffer, "continuous"))
		{
		}
		else
			if (!strcmp(Buffer, "timestamp"))
			{
				SpecialStatus[MaxAtt] = TSTMPVAL;

				/*  Set the base date if not done already  */

				if (!TSBase)
				{
					clock = time(0);
					BaseYear = gmtime(&clock)->tm_year + 1900;
					SetTSBase(BaseYear);
				}
			}
			else
				if (!strcmp(Buffer, "date"))
				{
					SpecialStatus[MaxAtt] = DATEVAL;
				}
				else
					if (!strcmp(Buffer, "time"))
					{
						SpecialStatus[MaxAtt] = STIMEVAL;
					}
					else
						if (!memcmp(Buffer, "discrete", 8))
						{
							SpecialStatus[MaxAtt] = DISCRETE;

							/*  Read max values and reserve space  */

							v = atoi(&Buffer[8]);
							if (v < 2)
							{
								Error(BADDISCRETE, AttName[MaxAtt], "");
							}

							AttValName[MaxAtt] = Alloc(v + 3, String);
							AttValName[MaxAtt][0] = (char *)(long)v + 1;
							AttValName[MaxAtt][(MaxAttVal[MaxAtt] = 1)] = strdup("N/A");
						}
						else
							if (!strcmp(Buffer, "ignore"))
							{
								SpecialStatus[MaxAtt] = EXCLUDE;
							}
							else
								if (!strcmp(Buffer, "label"))
								{
									LabelAtt = MaxAtt;
									SpecialStatus[MaxAtt] = EXCLUDE;
								}
								else
								{
									/*  Cannot have only one discrete value for an attribute  */

									Error(SINGLEATTVAL, AttName[MaxAtt], Buffer);
								}
	}
	else
	{
		/*  Discrete attribute with explicit values  */

		AttValName[MaxAtt] = AllocZero(ValCeiling, String);

		/*  Add "N/A"  */

		AttValName[MaxAtt][(MaxAttVal[MaxAtt] = 1)] = strdup("N/A");

		p = Buffer;

		/*  Special check for ordered attribute  */

		if (!memcmp(Buffer, "[ordered]", 9))
		{
			SpecialStatus[MaxAtt] = ORDERED;

			for (p = Buffer + 9; Space(*p); p++)
				;
		}

		/*  Record first real explicit value  */

		AttValName[MaxAtt][++MaxAttVal[MaxAtt]] = strdup(p);

		/*  Record remaining values  */

		do
		{
			if (!(ReadName(Nf, Buffer, 1000, ':')))
			{
				Error(EOFINATT, AttName[MaxAtt], "");
			}

			if (++MaxAttVal[MaxAtt] >= ValCeiling)
			{
				ValCeiling += 100;
				Realloc(AttValName[MaxAtt], ValCeiling, String);
			}

			AttValName[MaxAtt][MaxAttVal[MaxAtt]] = strdup(Buffer);
		} while (Delimiter == ',');

		/*  Cancel ordered status if <3 real values  */

		if (Ordered(MaxAtt) && MaxAttVal[MaxAtt] <= 3)
		{
			SpecialStatus[MaxAtt] = 0;
		}
		if (MaxAttVal[MaxAtt] > MaxDiscrVal) MaxDiscrVal = MaxAttVal[MaxAtt];
	}
}



/*=======================================================================*/
/*									 */
/*	Routines to handle implicitly-defined attributes		 */
/*									 */
/*=======================================================================*/


char	*Buff;			/* buffer for input characters */
int	BuffSize, BN;		/* size and index of next character */

EltRec	*TStack;		/* expression stack model */
int	TStackSize, TSN;	/* size of stack and index of next entry */

int	DefSize, DN;		/* size of definition and next element */

Boolean PreviousError;		/* to avoid parasytic errors */

AttValue _UNK,			/* quasi-constant for unknown value */
_NA;			/* ditto for not applicable */


#define FailSyn(Msg)	 {DefSyntaxError(Msg); return false;}
#define FailSem(Msg)	 {DefSemanticsError(Fi, Msg, OpCode); return false;}

typedef  union  _xstack_elt
{
	DiscrValue  _discr_val;
	ContValue   _cont_val;
	String      _string_val;
}
XStackElt;

#define	cval		_cont_val
#define	sval		_string_val
#define	dval		_discr_val
#define	XDVal(c,a)	DVal(c,a)



/*************************************************************************/
/*									 */
/*	A definition is handled in two stages:				 */
/*	  - The definition is read (up to a line ending with a period)	 */
/*	    replacing multiple whitespace characters with one space	 */
/*	  - The definition is then read (using a recursive descent	 */
/*	    parser), building up a reverse polish expression		 */
/*	Syntax and semantics errors are flagged				 */
/*									 */
/*************************************************************************/


void ImplicitAtt(FILE *Nf)
/*   -----------  */
{
#ifdef CUBIST
	_UNK.cval = UNKNOWN;
#else
	_UNK.dval = UNKNOWN;
#endif
	_NA.dval = NA;

	/*  Get definition as a string in Buff  */

	ReadDefinition(Nf);

	PreviousError = false;
	BN = 0;

	/*  Allocate initial stack and attribute definition  */

	TStack = Alloc(TStackSize = 50, EltRec);
	TSN = 0;

	AttDef[MaxAtt] = Alloc(DefSize = 100, DefElt);
	DN = 0;

	/*  Parse Buff as an expression terminated by a period  */

	Expression();
	if (!Find(".")) DefSyntaxError("'.' ending definition");

	/*  Final check -- defined attribute must not be of type String  */

	if (!PreviousError)
	{
		if (DN == 1 && DefOp(AttDef[MaxAtt][0]) == OP_ATT &&
			strcmp(AttName[MaxAtt], "case weight"))
		{
			Error(SAMEATT, AttName[(long)DefSVal(AttDef[MaxAtt][0])], Nil);
		}

		if (TStack[0].Type == 'B')
		{
			/*  Defined attributes should never have a value N/A  */

			MaxAttVal[MaxAtt] = 3;
			AttValName[MaxAtt] = AllocZero(4, String);
			AttValName[MaxAtt][1] = strdup("??");
			AttValName[MaxAtt][2] = strdup("t");
			AttValName[MaxAtt][3] = strdup("f");
		}
		else
		{
			MaxAttVal[MaxAtt] = 0;
		}
	}

	if (PreviousError)
	{
		DN = 0;
		SpecialStatus[MaxAtt] = EXCLUDE;
	}

	/*  Write a terminating marker  */

	DefOp(AttDef[MaxAtt][DN]) = OP_END;

	Free(Buff);
	Free(TStack);
}



/*************************************************************************/
/*									 */
/*	Read the text of a definition.  Skip comments, collapse		 */
/*	multiple whitespace characters.					 */
/*									 */
/*************************************************************************/


void ReadDefinition(FILE *f)
/*   --------------  */
{
	Boolean	LastWasPeriod = false;
	char	c;

	Buff = Alloc(BuffSize = 50, char);
	BN = 0;

	while (true)
	{
		c = InChar(f);

		if (c == '|') SkipComment;

		if (c == EOF || c == '\n' && LastWasPeriod)
		{
			/*  The definition is complete.  Add a period if it's
			not there already and terminate the string  */

			if (!LastWasPeriod) Append('.');
			Append(0);

			return;
		}

		if (Space(c))
		{
			Append(' ');
		}
		else
			if (c == '\\')
			{
				/*  Escaped character -- bypass any special meaning  */

				Append(InChar(f));
			}
			else
			{
				LastWasPeriod = (c == '.');
				Append(c);
			}
	}
}



/*************************************************************************/
/*									 */
/*	Append a character to Buff, resizing it if necessary		 */
/*									 */
/*************************************************************************/


void Append(char c)
/*   ------  */
{
	if (c == ' ' && (!BN || Buff[BN - 1] == ' ')) return;

	if (BN >= BuffSize)
	{
		Realloc(Buff, BuffSize += 50, char);
	}

	Buff[BN++] = c;
}



/*************************************************************************/
/*									 */
/*	Recursive descent parser with syntax error checking.		 */
/*	The reverse polish is built up by calls to Dump() and DumpOp(),	 */
/*	which also check for semantic validity.				 */
/*									 */
/*	For possible error messages, each routine also keeps track of	 */
/*	the beginning of the construct that it recognises (in Fi).	 */
/*									 */
/*************************************************************************/


Boolean Expression(void)
/*      ----------  */
{
	int		Fi = BN;

	if (Buff[BN] == ' ') BN++;

	if (!Conjunct()) FailSyn("expression");

	while (Find("or"))
	{
		BN += 2;

		if (!Conjunct()) FailSyn("expression");

		DumpOp(OP_OR, Fi);
	}

	return true;
}



Boolean Conjunct(void)
/*      --------  */
{
	int		Fi = BN;

	if (!SExpression()) FailSyn("expression");

	while (Find("and"))
	{
		BN += 3;

		if (!SExpression()) FailSyn("expression");

		DumpOp(OP_AND, Fi);
	}

	return true;
}



String RelOps[] = { ">=", "<=", "!=", "<>", ">", "<", "=", (String)0 };

Boolean SExpression(void)
/*      -----------  */
{
	int		o, Fi = BN;

	if (!AExpression()) FailSyn("expression");

	if ((o = FindOne(RelOps)) >= 0)
	{
		BN += strlen(RelOps[o]);

		if (!AExpression()) FailSyn("expression");

		DumpOp((o == 0 ? OP_GE :
			o == 1 ? OP_LE :
			o == 4 ? OP_GT :
			o == 5 ? OP_LT :
			o == 2 || o == 3 ?
			(TStack[TSN - 1].Type == 'S' ? OP_SNE : OP_NE) :
			(TStack[TSN - 1].Type == 'S' ? OP_SEQ : OP_EQ)), Fi);
	}

	return true;
}



String AddOps[] = { "+", "-", (String)0 };

Boolean AExpression(void)
/*      -----------  */
{
	int		o, Fi = BN;

	if (Buff[BN] == ' ') BN++;

	if ((o = FindOne(AddOps)) >= 0)
	{
		BN += 1;
	}

	if (!Term()) FailSyn("expression");

	if (o == 1) DumpOp(OP_UMINUS, Fi);

	while ((o = FindOne(AddOps)) >= 0)
	{
		BN += 1;

		if (!Term()) FailSyn("arithmetic expression");

		DumpOp((char)(OP_PLUS + o), Fi);
	}

	return true;
}



String MultOps[] = { "*", "/", "%", (String)0 };

Boolean Term(void)
/*      ----  */
{
	int		o, Fi = BN;

	if (!Factor()) FailSyn("expression");

	while ((o = FindOne(MultOps)) >= 0)
	{
		BN += 1;

		if (!Factor()) FailSyn("arithmetic expression");

		DumpOp((char)(OP_MULT + o), Fi);
	}

	return true;
}



Boolean Factor(void)
/*      ----  */
{
	int		Fi = BN;

	if (!Primary()) FailSyn("value");

	while (Find("^"))
	{
		BN += 1;

		if (!Primary()) FailSyn("exponent");

		DumpOp(OP_POW, Fi);
	}

	return true;
}



Boolean Primary(void)
/*      -------  */
{
	if (Atom())
	{
		return true;
	}
	else
		if (Find("("))
		{
			BN++;
			if (!Expression()) FailSyn("expression in parentheses");
			if (!Find(")")) FailSyn("')'");
			BN++;
			return true;
		}
		else
		{
			FailSyn("attribute, value, or '('");
		}
}



String Funcs[] = { "sin", "cos", "tan", "log", "exp", "int", (String)0 };

Boolean Atom(void)
/*      ----  */
{
	char	*EndPtr, *Str, Date[11], Time[9];
	int		o, FirstBN, Fi = BN;
	ContValue	F;
	Attribute	Att;

	if (Buff[BN] == ' ') BN++;

	if (Buff[BN] == '"')
	{
		FirstBN = ++BN;
		while (Buff[BN] != '"')
		{
			if (!Buff[BN]) FailSyn("closing '\"'");
			BN++;
		}

		/*  Make a copy of the string without double quotes  */

		Buff[BN] = '\00';
		Str = strdup(Buff + FirstBN);

		Buff[BN++] = '"';
		Dump(OP_STR, 0, Str, Fi);
	}
	else
		if ((Att = FindAttName()))
		{
			BN += strlen(AttName[Att]);

			Dump(OP_ATT, 0, (String)(long)Att, Fi);
		}
		else
			if (isdigit(Buff[BN]))
			{
				/*  Check for date or time first  */

				if ((Buff[BN + 4] == '/' && Buff[BN + 7] == '/' ||
					Buff[BN + 4] == '-' && Buff[BN + 7] == '-') &&
					isdigit(Buff[BN + 1]) && isdigit(Buff[BN + 2]) &&
					isdigit(Buff[BN + 3]) &&
					isdigit(Buff[BN + 5]) && isdigit(Buff[BN + 6]) &&
					isdigit(Buff[BN + 8]) && isdigit(Buff[BN + 9]))
				{
					memcpy(Date, Buff + BN, 10);
					Date[10] = '\00';
					if ((F = DateToDay(Date)) == 0)
					{
						Error(BADDEF1, Date, "date");
					}

					BN += 10;
				}
				else
					if (Buff[BN + 2] == ':' && Buff[BN + 5] == ':' &&
						isdigit(Buff[BN + 1]) &&
						isdigit(Buff[BN + 3]) && isdigit(Buff[BN + 4]) &&
						isdigit(Buff[BN + 6]) && isdigit(Buff[BN + 7]))
					{
						memcpy(Time, Buff + BN, 8);
						Time[8] = '\00';
						if ((F = TimeToSecs(Time)) == 0)
						{
							Error(BADDEF1, Time, "time");
						}

						BN += 8;
					}
					else
					{
						F = strtod(Buff + BN, &EndPtr);

						/*  Check for period after integer  */

						if (EndPtr > Buff + BN + 1 && *(EndPtr - 1) == '.')
						{
							EndPtr--;
						}

						BN = EndPtr - Buff;
					}

				Dump(OP_NUM, F, Nil, Fi);
			}
			else
				if ((o = FindOne(Funcs)) >= 0)
				{
					BN += 3;

					if (!Find("(")) FailSyn("'(' after function name");
					BN++;

					if (!Expression()) FailSyn("expression");

					if (!Find(")")) FailSyn("')' after function argument");
					BN++;

					DumpOp((char)(OP_SIN + o), Fi);
				}
				else
					if (Buff[BN] == '?')
					{
						BN++;
						if (TStack[TSN - 1].Type == 'N')
						{
							Dump(OP_NUM, _UNK.cval, Nil, Fi);
						}
						else
						{
							Dump(OP_STR, 0, Nil, Fi);
						}
					}
					else
						if (!memcmp(Buff + BN, "N/A", 3))
						{
							BN += 3;
							if (TStack[TSN - 1].Type == 'N')
							{
								Dump(OP_NUM, _NA.cval, Nil, Fi);
							}
							else
							{
								Dump(OP_STR, 0, strdup("N/A"), Fi);
							}
						}
						else
						{
							return false;
						}

	return true;
}



/*************************************************************************/
/*									 */
/*	Skip spaces and check for specific string			 */
/*									 */
/*************************************************************************/


Boolean Find(String S)
/*      ----  */
{
	if (Buff[BN] == ' ') BN++;

	return (!Buff[BN] ? false : !memcmp(Buff + BN, S, strlen(S)));
}



/*************************************************************************/
/*									 */
/*	Find one of a zero-terminated list of alternatives		 */
/*									 */
/*************************************************************************/


int FindOne(String *Alt)
/*  -------  */
{
	int	a;

	for (a = 0; Alt[a]; a++)
	{
		if (Find(Alt[a])) return a;
	}

	return -1;
}



/*************************************************************************/
/*									 */
/*	Find an attribute name						 */
/*									 */
/*************************************************************************/


Attribute FindAttName(void)
/*        -----------  */
{
	Attribute	Att, LongestAtt = 0;

	ForEach(Att, 1, MaxAtt - 1)
	{
		if (!Exclude(Att) && Find(AttName[Att]))
		{
			if (!LongestAtt ||
				strlen(AttName[Att]) > strlen(AttName[LongestAtt]))
			{
				LongestAtt = Att;
			}
		}
	}

	return LongestAtt;
}



/*************************************************************************/
/*									 */
/*	Error message routines.  Syntax errors come from the		 */
/*	recursive descent parser, semantics errors from the routines	 */
/*	that build up the equivalent polish				 */
/*									 */
/*************************************************************************/


void DefSyntaxError(String Msg)
/*   --------------  */
{
	String	RestOfText;
	int		i = 10;

	if (!PreviousError)
	{
		RestOfText = Buff + BN;

		/*  Abbreviate text if longer than 12 characters  */

		if (CharWidth(RestOfText) > 12)
		{
#ifdef UTF8
			/*  Find beginning of UTF-8 character  */

			for (; (RestOfText[i] & 0x80); i++)
				;
#endif
			RestOfText[i] = RestOfText[i + 1] = '.';
		}

		Error(BADDEF1, RestOfText, Msg);
		PreviousError = true;
	}
}



void DefSemanticsError(int Fi, String Msg, int OpCode)
/*   -----------------  */
{
	char	Exp[1000], XMsg[1000], Op[1000];

	if (!PreviousError)
	{
		/*  Abbreviate the input if necessary  */

		if (BN - Fi > 23)
		{
			sprintf(Exp, "%.10s...%.10s", Buff + Fi, Buff + BN - 10);
		}
		else
		{
			sprintf(Exp, "%.*s", BN - Fi, Buff + Fi);
		}

		switch (OpCode)
		{
		case OP_AND:	sprintf(Op, "%s", "and"); break;
		case OP_OR:		sprintf(Op, "%s", "or"); break;
		case OP_SEQ:
		case OP_EQ:		sprintf(Op, "%s", "="); break;
		case OP_SNE:
		case OP_NE:		sprintf(Op, "%s", "<>"); break;
		case OP_GT:		sprintf(Op, "%s", ">"); break;
		case OP_GE:		sprintf(Op, "%s", ">="); break;
		case OP_LT:		sprintf(Op, "%s", "<"); break;
		case OP_LE:		sprintf(Op, "%s", "<="); break;
		case OP_PLUS:	sprintf(Op, "%s", "+"); break;
		case OP_MINUS:	sprintf(Op, "%s", "-"); break;
		case OP_UMINUS:	sprintf(Op, "%s", "unary -"); break;
		case OP_MULT:	sprintf(Op, "%s", "*"); break;
		case OP_DIV:	sprintf(Op, "%s", "/"); break;
		case OP_MOD:	sprintf(Op, "%s", "%"); break;
		case OP_POW:	sprintf(Op, "%s", "^"); break;
		case OP_SIN:	sprintf(Op, "%s", "sin"); break;
		case OP_COS:	sprintf(Op, "%s", "cos"); break;
		case OP_TAN:	sprintf(Op, "%s", "tan"); break;
		case OP_LOG:	sprintf(Op, "%s", "log"); break;
		case OP_EXP:	sprintf(Op, "%s", "exp"); break;
		case OP_INT:	sprintf(Op, "%s", "int");
		}

		sprintf(XMsg, "%s with '%s'", Msg, Op);
		Error(BADDEF2, Exp, XMsg);
		PreviousError = true;
	}
}



/*************************************************************************/
/*									 */
/*	Reverse polish routines.  These use a model of the stack	 */
/*	during expression evaluation to detect type conflicts etc	 */
/*									 */
/*************************************************************************/



void Dump(char OpCode, ContValue F, String S, int Fi)
/*   ----  */
{
	if (Buff[Fi] == ' ') Fi++;

	if (!UpdateTStack(OpCode, F, S, Fi)) return;

	/*  Make sure enough room for this element  */

	if (DN >= DefSize - 1)
	{
		Realloc(AttDef[MaxAtt], DefSize += 100, DefElt);
	}

	DefOp(AttDef[MaxAtt][DN]) = OpCode;
	if (OpCode == OP_ATT || OpCode == OP_STR)
	{
		DefSVal(AttDef[MaxAtt][DN]) = S;
	}
	else
	{
		DefNVal(AttDef[MaxAtt][DN]) = F;
	}

	DN++;
}



void DumpOp(char OpCode, int Fi)
/*   ------  */
{
	Dump(OpCode, 0, Nil, Fi);
}



Boolean UpdateTStack(char OpCode, ContValue F, String S, int Fi)
/*      ------------  */
{
	if (TSN >= TStackSize)
	{
		Realloc(TStack, TStackSize += 50, EltRec);
	}

	switch (OpCode)
	{
	case OP_ATT:
		TStack[TSN].Type = (Continuous((long)S) ? 'N' : 'S');
		break;

	case OP_NUM:
		TStack[TSN].Type = 'N';
		break;

	case OP_STR:
		TStack[TSN].Type = 'S';
		break;

	case OP_AND:
	case OP_OR:
		if (TStack[TSN - 2].Type != 'B' || TStack[TSN - 1].Type != 'B')
		{
			FailSem("non-logical value");
		}
		TSN -= 2;
		break;

	case OP_EQ:
	case OP_NE:
		if (TStack[TSN - 2].Type != TStack[TSN - 1].Type)
		{
			FailSem("incompatible values");
		}
		TSN -= 2;
		TStack[TSN].Type = 'B';
		break;

	case OP_GT:
	case OP_GE:
	case OP_LT:
	case OP_LE:
		if (TStack[TSN - 2].Type != 'N' || TStack[TSN - 1].Type != 'N')
		{
			FailSem("non-arithmetic value");
		}
		TSN -= 2;
		TStack[TSN].Type = 'B';
		break;

	case OP_SEQ:
	case OP_SNE:
		if (TStack[TSN - 2].Type != 'S' || TStack[TSN - 1].Type != 'S')
		{
			FailSem("incompatible values");
		}
		TSN -= 2;
		TStack[TSN].Type = 'B';
		break;

	case OP_PLUS:
	case OP_MINUS:
	case OP_MULT:
	case OP_DIV:
	case OP_MOD:
	case OP_POW:
		if (TStack[TSN - 2].Type != 'N' || TStack[TSN - 1].Type != 'N')
		{
			FailSem("non-arithmetic value");
		}
		TSN -= 2;
		break;

	case OP_UMINUS:
		if (TStack[TSN - 1].Type != 'N')
		{
			FailSem("non-arithmetic value");
		}
		TSN--;
		break;

	case OP_SIN:
	case OP_COS:
	case OP_TAN:
	case OP_LOG:
	case OP_EXP:
	case OP_INT:
		if (TStack[TSN - 1].Type != 'N')
		{
			FailSem("non-arithmetic argument");
		}
		TSN--;
	}

	TStack[TSN].Fi = Fi;
	TStack[TSN].Li = BN - 1;
	TSN++;

	return true;
}



/*************************************************************************/
/*									 */
/*	Evaluate an implicit attribute for a case			 */
/*									 */
/*************************************************************************/

#define	Unknown(c,a)		(! DVal(c,a))
#define	CUnknownVal(AV)		(AV.cval==UNKNOWN)
#define	DUnknownVal(AV)		(! AV.dval)
#define DUNA(a)	(DUnknownVal(XStack[a]) || NotApplicVal(XStack[a]))
#define CUNA(a)	(CUnknownVal(XStack[a]) || NotApplicVal(XStack[a]))
#define	C1(x)	(CUNA(XSN-1) ? _UNK.cval : (x))
#define	C2(x)	(CUNA(XSN-1) || CUNA(XSN-2) ? _UNK.cval : (x))
#define	CD2(x)	(CUNA(XSN-1) || CUNA(XSN-2) ? 0 : (x))
#define	D2(x)	(DUNA(XSN-1) || DUNA(XSN-2) ? 0 : (x))


AttValue EvaluateDef(Definition D, DataRec Case)
/*       -----------  */
{
	XStackElt	XStack[100];			/* allows 100-level nesting  */
	int		XSN = 0, DN, bv1, bv2, Mult;
	double	cv1, cv2;
	String	sv1, sv2;
	Attribute	Att;
	DefElt	DElt;
	AttValue	ReturnVal;

	for (DN = 0; ; DN++)
	{
		switch (DefOp((DElt = D[DN])))
		{
		case OP_ATT:
			Att = (long)DefSVal(DElt);

#if defined PREDICT || defined SEE5 && defined WIN32 && ! defined _CONSOLE
			GetValue(Att);
#endif
			if (Continuous(Att))
			{
				XStack[XSN++].cval = CVal(Case, Att);
			}
			else
			{
				XStack[XSN++].sval =
					(Unknown(Case, Att) && !NotApplic(Case, Att) ? 0 :
						AttValName[Att][XDVal(Case, Att)]);
			}
			break;

		case OP_NUM:
			XStack[XSN++].cval = DefNVal(DElt);
			break;

		case OP_STR:
			XStack[XSN++].sval = DefSVal(DElt);
			break;

		case OP_AND:
			bv1 = XStack[XSN - 2].dval;
			bv2 = XStack[XSN - 1].dval;
			XStack[XSN - 2].dval = (bv1 == 3 || bv2 == 3 ? 3 :
				D2(bv1 == 2 && bv2 == 2 ? 2 : 3));
			XSN--;
			break;

		case OP_OR:
			bv1 = XStack[XSN - 2].dval;
			bv2 = XStack[XSN - 1].dval;
			XStack[XSN - 2].dval = (bv1 == 2 || bv2 == 2 ? 2 :
				D2(bv1 == 2 || bv2 == 2 ? 2 : 3));
			XSN--;
			break;

		case OP_EQ:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].dval = (cv1 == cv2 ? 2 : 3);
			XSN--;
			break;

		case OP_NE:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].dval = (cv1 != cv2 ? 2 : 3);
			XSN--;
			break;

		case OP_GT:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].dval = CD2(cv1 > cv2 ? 2 : 3);
			XSN--;
			break;

		case OP_GE:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].dval = CD2(cv1 >= cv2 ? 2 : 3);
			XSN--;
			break;

		case OP_LT:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].dval = CD2(cv1 < cv2 ? 2 : 3);
			XSN--;
			break;

		case OP_LE:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].dval = CD2(cv1 <= cv2 ? 2 : 3);
			XSN--;
			break;

		case OP_SEQ:
			sv1 = XStack[XSN - 2].sval;
			sv2 = XStack[XSN - 1].sval;
			XStack[XSN - 2].dval =
				(!sv1 && !sv2 ? 2 :
					!sv1 || !sv2 ? 3 :
					!strcmp(sv1, sv2) ? 2 : 3);
			XSN--;
			break;

		case OP_SNE:
			sv1 = XStack[XSN - 2].sval;
			sv2 = XStack[XSN - 1].sval;
			XStack[XSN - 2].dval =
				(!sv1 && !sv2 ? 3 :
					!sv1 || !sv2 ? 2 :
					strcmp(sv1, sv2) ? 2 : 3);
			XSN--;
			break;

		case OP_PLUS:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].cval = C2(cv1 + cv2);
			XSN--;
			break;

		case OP_MINUS:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].cval = C2(cv1 - cv2);
			XSN--;
			break;

		case OP_MULT:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].cval = C2(cv1 * cv2);
			XSN--;
			break;

		case OP_DIV:
			/*  Note: have to set precision of result  */

			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			if (!cv2 ||
				CUnknownVal(XStack[XSN - 2]) ||
				CUnknownVal(XStack[XSN - 1]) ||
				NotApplicVal(XStack[XSN - 2]) ||
				NotApplicVal(XStack[XSN - 1]))
			{
				XStack[XSN - 2].cval = _UNK.cval;
			}
			else
			{
				Mult = Denominator(cv1);
				cv1 = cv1 / cv2;
				while (fabs(cv2) > 1)
				{
					Mult *= 10;
					cv2 /= 10;
				}
				XStack[XSN - 2].cval = rint(cv1 * Mult) / Mult;
			}
			XSN--;
			break;

		case OP_MOD:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].cval = C2(fmod(cv1, cv2));
			XSN--;
			break;

		case OP_POW:
			cv1 = XStack[XSN - 2].cval;
			cv2 = XStack[XSN - 1].cval;
			XStack[XSN - 2].cval =
				(CUNA(XSN - 1) || CUNA(XSN - 2) ||
				(cv1 < 0 && ceil(cv2) != cv2) ? _UNK.cval :
					pow(cv1, cv2));
			XSN--;
			break;

		case OP_UMINUS:
			cv1 = XStack[XSN - 1].cval;
			XStack[XSN - 1].cval = C1(-cv1);
			break;

		case OP_SIN:
			cv1 = XStack[XSN - 1].cval;
			XStack[XSN - 1].cval = C1(sin(cv1));
			break;

		case OP_COS:
			cv1 = XStack[XSN - 1].cval;
			XStack[XSN - 1].cval = C1(cos(cv1));
			break;

		case OP_TAN:
			cv1 = XStack[XSN - 1].cval;
			XStack[XSN - 1].cval = C1(tan(cv1));
			break;

		case OP_LOG:
			cv1 = XStack[XSN - 1].cval;
			XStack[XSN - 1].cval =
				(CUNA(XSN - 1) || cv1 <= 0 ? _UNK.cval : log(cv1));
			break;

		case OP_EXP:
			cv1 = XStack[XSN - 1].cval;
			XStack[XSN - 1].cval = C1(exp(cv1));
			break;

		case OP_INT:
			cv1 = XStack[XSN - 1].cval;
			XStack[XSN - 1].cval = C1(rint(cv1));
			break;

		case OP_END:
			ReturnVal.cval = XStack[0].cval;	/* cval >= dval bytes */
			return ReturnVal;
		}
	}
}



/*************************************************************************/
/*									 */
/*	Locate value Val in List[First] to List[Last]			 */
/*									 */
/*************************************************************************/


int Which(String Val, String *List, int First, int Last)
/*  -----  */
{
	int	n = First;

	while (n <= Last && strcmp(Val, List[n])) n++;

	return (n <= Last ? n : First - 1);
}



/*************************************************************************/
/*									 */
/*	Free global names data						 */
/*									 */
/*************************************************************************/


void FreeNamesData(void)
/*   -------------  */
{
	Attribute a, t;

	FreeVector((void **)AttName, 1, MaxAtt);		AttName = Nil;

	ForEach(a, 1, MaxAtt)
	{
		if (a != ClassAtt && Discrete(a))
		{
			FreeVector((void **)AttValName[a], 1, MaxAttVal[a]);
		}
	}
	FreeUnlessNil(AttValName);				AttValName = Nil;

	/*  Definitions (if any)  */

	if (AttDef)
	{
		ForEach(a, 1, MaxAtt)
		{
			if (AttDef[a])
			{
				for (t = 0; DefOp(AttDef[a][t]) != OP_END; t++)
				{
					if (DefOp(AttDef[a][t]) == OP_STR)
					{
						Free(DefSVal(AttDef[a][t]));
					}
				}

				Free(AttDef[a]);
			}
		}
		Free(AttDef);					AttDef = Nil;
	}

	FreeUnlessNil(MaxAttVal);				MaxAttVal = Nil;
	FreeUnlessNil(SpecialStatus);			SpecialStatus = Nil;

	FreeUnlessNil(AttMean);				AttMean = Nil;
	FreeUnlessNil(AttSD);				AttSD = Nil;
	FreeUnlessNil(AttMaxVal);				AttMaxVal = Nil;
	FreeUnlessNil(AttMinVal);				AttMinVal = Nil;
	FreeUnlessNil(AttPrec);				AttPrec = Nil;
	FreeUnlessNil(Modal);				Modal = Nil;
}



/*************************************************************************/
/*									 */
/*	Read next char keeping track of line numbers			 */
/*									 */
/*************************************************************************/


int InChar(FILE *f)
/*  ------  */
{
	if (!*LBp)
	{
		LBp = LineBuffer;

		if (!fgets(LineBuffer, MAXLINEBUFFER, f))
		{
			LineBuffer[0] = '\00';
			return EOF;
		}

		LineNo++;
	}

	return (int)*LBp++;
}



/*=======================================================================*/
/*									 */
/*	Get case descriptions from data file				 */
/*									 */
/*=======================================================================*/


#define Inc 2048

CaseCount	*Freq;			/* discrete value frequencies */


/*************************************************************************/
/*									 */
/*	Read raw cases from file with given extension.			 */
/*									 */
/*	On completion, cases are stored in array Case in the form	 */
/*	of vectors of attribute values, and MaxCase is set to the	 */
/*	number of data cases.						 */
/*									 */
/*************************************************************************/


#if defined WIN32 || defined _CONSOLE
#define AltRandom ((rand() & 32767) / 32768.0)
#else
#define AltRandom drand48(void)
double drand48();
#endif

#define XError(a,b,c)	Error(a,b,c)


void GetData(FILE *Df, Boolean Train, Boolean AllowUnknownTarget)
/*   -------  */
{
	CaseNo	CaseSpace, i;
	DataRec	DVec;
	Boolean	AnyUnknown = false, FirstIgnore = true, *AttMsg;
	Attribute	Att;
	ContValue	Val, Range;
	char	CVS[20];

	LineNo = 0;

	if (Train || !Case)
	{
		MaxCase = MaxLabel = CaseSpace = 0;
		Case = Alloc(1, DataRec);	/* for error reporting */
	}
	else
	{
		CaseSpace = MaxCase + 1;
		MaxCase++;
	}


	while ((DVec = GetDataRec(Df, Train)))
	{

		/*  Make sure there is room for another item  */

		if (MaxCase >= CaseSpace)
		{
			CaseSpace += Inc;
			Realloc(Case, CaseSpace + 1, DataRec);
		}

		/*  Ignore cases with N/A target value (!) but possibly allow
			cases with unknown target value  */

		if (!NotApplic(DVec, ClassAtt) &&
			(AllowUnknownTarget || CVal(DVec, ClassAtt) != UNKNOWN))
		{
			Case[MaxCase] = DVec;

#ifndef _CONSOLE
#ifdef WIN32
			if (TerminateSignal())
			{
				fclose(Df);
				NotifyNumber("", -1);
				Goodbye(0);
			}

			if (MaxCase > 0 && MaxCase % Incr == 0)
			{
				NotifyNumber("Case", MaxCase);
				if (Incr < 100000 && ++Times == 10)
				{
					Incr *= 10;
					Times = 1;
				}
			}
#endif
#endif
			MaxCase++;
		}
		else
		{
			if (FirstIgnore && Of)
			{
				fprintf(Of, (AllowUnknownTarget ? T_IgnoreNATarget :
					T_IgnoreBadTarget));
				FirstIgnore = false;
			}

			FreeCase(DVec);
		}
	}

	fclose(Df);
	MaxCase--;

	if (Of && MaxCase < 0)
	{
		fprintf(Of, T_NoCases);
		Goodbye(1);
	}

#ifndef _CONSOLE
#ifdef WIN32
	NotifyNumber("", -1);
#endif
#endif

	/*  Replace unknown values by means or modals  */


	AttMsg = AllocZero(MaxAtt + 1, Boolean);

	ForEach(i, 0, MaxCase)
	{
		AnyUnknown |= ReplaceUnknowns(Case[i], AttMsg);
	}

	if (Train)
	{
		/*  Find global range and set lowest and highest predicted value  */

		Range = AttMaxVal[ClassAtt] - AttMinVal[ClassAtt];

		Ceiling = AttMaxVal[ClassAtt] + EXTRAP * Range;
		if (AttMaxVal[ClassAtt] <= 0 && Ceiling > 0) Ceiling = 0;

		Floor = AttMinVal[ClassAtt] - EXTRAP * Range;
		if (AttMinVal[ClassAtt] >= 0 && Floor < 0) Floor = 0;
	}

	Free(AttMsg);					AttMsg = Nil;
}



/*************************************************************************/
/*									 */
/*	Replace any unknown values in a case				 */
/*									 */
/*************************************************************************/


Boolean ReplaceUnknowns(DataRec Case, Boolean *AttMsg)
/*      ---------------  */
{
	Attribute	Att;
	Boolean	Replaced = false;

	ForEach(Att, 1, MaxAtt)
	{
		if (Skip(Att) || Att == ClassAtt) continue;

		if (Discrete(Att) && !DVal(Case, Att))
		{
			DVal(Case, Att) = Modal[Att];
			if (AttMsg) AttMsg[Att] = Replaced = true;
		}
		else
			if (Continuous(Att) && CVal(Case, Att) == UNKNOWN)
			{
				CVal(Case, Att) = AttMean[Att];
				if (AttMsg) AttMsg[Att] = Replaced = true;
			}
	}

	Class(Case) = CVal(Case, ClassAtt);

	return Replaced;
}



/*************************************************************************/
/*									 */
/*	Read a raw case from file Df.					 */
/*									 */
/*	For each attribute, read the attribute value from the file.	 */
/*	If it is a discrete valued attribute, find the associated no.	 */
/*	of this attribute value (if the value is unknown this is 0).	 */
/*									 */
/*	Returns the DataRec of the case (i.e. the array of attribute	 */
/*	values).							 */
/*									 */
/*************************************************************************/


DataRec GetDataRec(FILE *Df, Boolean Train)
/*      ----------  */
{
	Attribute	Att;
	char	Name[1000], *EndVal;
	int		Dv;
	ContValue	Cv;
	DataRec	DVec;
	Boolean	FirstValue = true;
#if defined WIN32  && ! defined _CONSOLE 
	extern int	XREF;
#endif


	if (ReadName(Df, Name, 1000, '\00'))
	{
		DVec = AllocZero(MaxAtt + 3, AttValue);
		ForEach(Att, 1, MaxAtt)
		{
			if (AttDef[Att])
			{
				DVec[Att] = EvaluateDef(AttDef[Att], DVec);
				if (Continuous(Att))
				{
					CheckValue(DVec, Att);
				}
				continue;
			}

			/*  Get the attribute value if don't already have it  */

			if (!FirstValue && !ReadName(Df, Name, 1000, '\00'))
			{
				XError(EOFINATT, AttName[Att], "");
				FreeCase(DVec);
				return Nil;
			}
			FirstValue = false;

			if (Exclude(Att))
			{
#if defined WIN32 && ! defined _CONSOLE
				if (XREF || Att == LabelAtt)
#else
				if (Att == LabelAtt)
#endif
				{
					/*  Record the value as a string  */

					SVal(DVec, Att) = StoreIVal(Name);
				}
			}
			else
				if (!strcmp(Name, "?"))
				{
					/*  Unknown value  */

					if (Continuous(Att))
					{
						CVal(DVec, Att) = UNKNOWN;
					}
					else
					{
						DVal(DVec, Att) = 0;
					}
				}
				else
					if (!strcmp(Name, "N/A"))
					{
						/*  Non-applicable value  */

						DVal(DVec, Att) = NA;
					}
					else
						if (Discrete(Att))
						{
							Dv = Which(Name, AttValName[Att], 1, MaxAttVal[Att]);
							if (!Dv)
							{
								if (StatBit(Att, DISCRETE))
								{
									if (Train)
									{
										/*  Add value to list  */

										if (MaxAttVal[Att] >= (long)AttValName[Att][0])
										{
											XError(TOOMANYVALS, AttName[Att],
												(char *)AttValName[Att][0] - 1);
											Dv = MaxAttVal[Att];
										}
										else
										{
											Dv = ++MaxAttVal[Att];
											AttValName[Att][Dv] = strdup(Name);
											AttValName[Att][Dv + 1] = "<other>"; /* no free */
										}
									}
									else
									{
										/*  Set value to "<other>"  */

										Dv = MaxAttVal[Att] + 1;
									}
								}
								else
								{
									XError(BADATTVAL, AttName[Att], Name);
								}
							}
							DVal(DVec, Att) = Dv;
						}
						else
						{
							/*  Continuous value  */

							if (TStampVal(Att))
							{
								Cv = TStampToMins(Name);
								if (Cv >= 1E9)	/* long time in future */
								{
									XError(BADTSTMP, AttName[Att], Name);
									Cv = UNKNOWN;
								}
							}
							else
								if (DateVal(Att))
								{
									Cv = DateToDay(Name);
									if (Cv < 1)
									{
										XError(BADDATE, AttName[Att], Name);
										Cv = UNKNOWN;
									}
								}
								else
									if (TimeVal(Att))
									{
										Cv = TimeToSecs(Name);
										if (Cv < 0)
										{
											XError(BADTIME, AttName[Att], Name);
											Cv = UNKNOWN;
										}
									}
									else
									{
										Cv = strtod(Name, &EndVal);
										if (EndVal == Name || *EndVal != '\0')
										{
											XError(BADATTVAL, AttName[Att], Name);
											Cv = UNKNOWN;
										}
									}

							CVal(DVec, Att) = Cv;

							CheckValue(DVec, Att);
						}
		}

		/*  Preserve original case number  */

		DVal(DVec, 0) = MaxCase + 1;

		return DVec;
	}
	else
	{
		return Nil;
	}
}



/*************************************************************************/
/*									 */
/*	Store a label or ignored value in IValStore			 */
/*									 */
/*************************************************************************/


int StoreIVal(String S)
/*  ---------  */
{
	int		StartIx, Length;

	if ((Length = strlen(S) + 1) + IValsOffset > IValsSize)
	{
		if (IgnoredVals)
		{
			Realloc(IgnoredVals, IValsSize += 32768, char);
		}
		else
		{
			IValsSize = 32768;
			IValsOffset = 0;
			IgnoredVals = Alloc(IValsSize, char);
		}
	}

	StartIx = IValsOffset;
	strcpy(IgnoredVals + StartIx, S);
	IValsOffset += Length;

	return StartIx;
}



/*************************************************************************/
/*									 */
/*	Check for bad continuous value					 */
/*									 */
/*************************************************************************/


void CheckValue(DataRec DVec, Attribute Att)
/*   ----------  */
{
	ContValue	Cv;

	Cv = CVal(DVec, Att);
	if (!_finite(Cv))
	{
		Error(BADNUMBER, AttName[Att], "");

		CVal(DVec, Att) = UNKNOWN;
		DVal(DVec, Att) = 0;
	}
}



/*************************************************************************/
/*									 */
/*	Check whether relevant continuous attribute values lie outside	 */
/*	the range observed in the training data				 */
/*									 */
/*************************************************************************/


Boolean OutsideRange(DataRec Case)
/*	------------  */
{
	Attribute	Att;
	static Boolean *Relevant = Nil;

	/*  On first call, find relevant attributes  */

	if (!Relevant)
	{
		RRuleSet RS;
		CRule    R;
		int      m, d;
		RuleNo   r;

		Relevant = AllocZero(MaxAtt + 1, Boolean);

		if (USEINSTANCES)
		{
			ForEach(Att, 1, MaxAtt)
			{
				Relevant[Att] =
					(Continuous(Att) && !Skip(Att) && Att != ClassAtt);
			}
		}
		else
		{
			/*  Check each committee member  */

			ForEach(m, 0, MEMBERS - 1)
			{
				RS = CubistModel[m];

				/*  Check each rule  */

				ForEach(r, 1, RS->SNRules)
				{
					R = RS->SRule[r];

					/*  Check rule conditions ...  */

					ForEach(d, 1, R->Size)
					{
						if (R->Lhs[d]->NodeType == BrThresh)
						{
							Relevant[R->Lhs[d]->Tested] = true;
						}
					}

					/*  ... and associated linear model  */

					ForEach(Att, 1, MaxAtt)
					{
						if (R->Rhs[Att]) Relevant[Att] = true;
					}
				}
			}
		}
	}

	/*  Check each relevant attribute  */

	ForEach(Att, 1, MaxAtt)
	{
		if (Relevant[Att] &&
			(CVal(Case, Att) < AttMinVal[Att] - 1E-3 ||
				CVal(Case, Att) > AttMaxVal[Att] + 1E-3))
		{
			return true;
		}
	}

	return false;
}



/*************************************************************************/
/*									 */
/*	Free case description space					 */
/*									 */
/*************************************************************************/


void FreeCases(DataRec *Case, CaseNo MaxCase)
/*   --------  */
{
	CaseNo      i;

	/*  Release any strings holding ignored attribute values  */

	ForEach(i, 0, MaxCase)
	{
		FreeCase(Case[i]);
	}

	free(Case);
}



void FreeCase(DataRec DVec)
/*   --------  */
{
	free(DVec);
}



/*************************************************************************/
/*									 */
/*	Routines for reading model files				 */
/*	--------------------------------				 */
/*									 */
/*************************************************************************/


int	Entry;

char*	Prop[] = { "null",
		"id",
		"att",
		"elts",
		"prec",
		"globalmean",
		"floor",
		"ceiling",
		"sample",
		"init",
		"mean",
		"sd",
		"mode",
		"entries",
		"rules",
		"cover",
		"loval",
		"hival",
		"extrap",
		"insts",
		"nn",
		"maxd",
		"esterr",
		"conds",
		"type",
		"cut",
		"result",
		"val",
		"coeff",
		"max",
		"min",
		"redn"
};

char	PropName[20],
*PropVal = Nil,
*Unquoted;
int	PropValSize = 0;

#define	PROPS 31

#define IDP		1
#define ATTP		2
#define ELTSP		3
#define PRECP		4
#define GLOBALMEANP	5
#define FLOORP		6
#define CEILINGP	7
#define SAMPLEP		8
#define INITP		9
#define MEANP		10
#define SDP		11
#define MODEP		12
#define ENTRIESP	13
#define RULESP		14
#define COVERP		15
#define LOVALP		16
#define HIVALP		17
#define EXTRAPP		18
#define INSTSP		19
#define NNP		20
#define MAXDP		21
#define ESTERRP		22
#define CONDSP		23
#define TYPEP		24
#define CUTP		25
#define RESULTP		26
#define VALP		27
#define COEFFP		28
#define	MAXP		29
#define	MINP		30
#define REDNP		31


/*************************************************************************/
/*									 */
/*	Check whether file is open.  If it is not, open it and		 */
/*	read/write sampling information and discrete names		 */
/*									 */
/*************************************************************************/


void CheckFile(String Extension, Boolean Write)
/*   ---------  */
{
	static char	*LastExt = "";

	if (!Mf || strcmp(LastExt, Extension))
	{
		LastExt = Extension;

		if (Mf)
		{
			fclose(Mf);					Mf = Nil;
		}

		ReadFilePrefix(Extension);
	}
}



/*************************************************************************/
/*									 */
/*	Read header information and decide whether model files are	 */
/*	in ASCII or binary format					 */
/*									 */
/*************************************************************************/


void ReadFilePrefix(String Extension)
/*   --------------  */
{
#if defined WIN32 || defined _CONSOLE
	if (!(Mf = GetFile(Extension, "rb"))) Error(NOFILE, Fn, "");
#else
	if (!(Mf = GetFile(Extension, "r"))) Error(NOFILE, Fn, "");
#endif

	ReadHeader();
}



/*************************************************************************/
/*								  	 */
/*	Read the header information (id, saved names, models)		 */
/*								  	 */
/*************************************************************************/


void ReadHeader(void)
/*   ---------  */
{
	Attribute	Att;
	DiscrValue	v;
	char	*p, Dummy;
	double	Xd;
	int		Year, Month, Day;

	/*  First allocate storage for various globals  */

	AttMean = Alloc(MaxAtt + 1, ContValue);
	AttSD = Alloc(MaxAtt + 1, ContValue);
	AttMaxVal = Alloc(MaxAtt + 1, ContValue);
	AttMinVal = Alloc(MaxAtt + 1, ContValue);
	Modal = Alloc(MaxAtt + 1, DiscrValue);

	while (true)
	{
		switch (ReadProp(&Dummy))
		{
		case IDP:
			if (memcmp(PropVal + 8, RELEASE, 4))
			{
				printf("\nError: model file was generated by an"
					" unsupported release\n");
				exit(1);
			}

			/*  Recover year run and set base date for timestamps  */

			if (sscanf(PropVal + strlen(PropVal) - 11,
				"%d-%d-%d\"", &Year, &Month, &Day) == 3)
			{
				SetTSBase(Year);
			}
			break;

		case ATTP:
			Unquoted = RemoveQuotes(PropVal);
			Att = Which(Unquoted, AttName, 1, MaxAtt);
			if (!Att || Exclude(Att))
			{
				Error(MODELFILE, E_MFATT, Unquoted);
			}
			break;

		case ELTSP:
			/*  First element ("N/A") already added by GetNames()  */

			MaxAttVal[Att] = 1;

			for (p = PropVal; *p; )
			{
				p = RemoveQuotes(p);
				v = ++MaxAttVal[Att];
				AttValName[Att][v] = Alloc(strlen(p) + 1, char);
				strcpy(AttValName[Att][v], p);

				for (p += strlen(p); *p != '"'; p++)
					;
				p++;
				if (*p == ',') p++;
			}
			AttValName[Att][MaxAttVal[Att] + 1] = "<other>";
			MaxDiscrVal = Max(MaxDiscrVal, MaxAttVal[Att] + 1);
			break;

		case PRECP:
			sscanf(PropVal, "\"%d\"", &Precision);
			ClassUnit = 1.0 / pow(10.0, Precision + 1);
			break;

		case GLOBALMEANP:
			sscanf(PropVal, "\"%f\"", &GlobalMean);
			break;

		case EXTRAPP:
			sscanf(PropVal, "\"%f\"", &EXTRAP);
			break;

		case INSTSP:
			USEINSTANCES = PropVal[1] - '0';
			if (USEINSTANCES)
			{
				/*  Set legacy values  */

				NN = 5;
				MAXD = 50;
			}
			break;

		case NNP:
			sscanf(PropVal, "\"%d\"", &NN);
			break;

		case MAXDP:
			sscanf(PropVal, "\"%f\"", &MAXD);
			break;

		case CEILINGP:
			sscanf(PropVal, "\"%lf\"", &Xd);	Ceiling = Xd;
			break;

		case FLOORP:
			sscanf(PropVal, "\"%lf\"", &Xd);	Floor = Xd;
			break;

		case MEANP:
			sscanf(PropVal, "\"%lf\"", &Xd);	AttMean[Att] = Xd;
			break;

		case SDP:
			sscanf(PropVal, "\"%lf\"", &Xd);	AttSD[Att] = Xd;
			break;

		case MAXP:
			sscanf(PropVal, "\"%lf\"", &Xd);	AttMaxVal[Att] = Xd;
			break;

		case MINP:
			sscanf(PropVal, "\"%lf\"", &Xd);	AttMinVal[Att] = Xd;
			break;

		case MODEP:
			Unquoted = RemoveQuotes(PropVal);
			Modal[Att] = Which(Unquoted,
				AttValName[Att], 1, MaxAttVal[Att]);

			if (!Modal[Att])
			{
				/*  An unknown modal value is an error!  */

				Error(MODELFILE, E_MFATTVAL, Unquoted);
			}
			else
				if (Modal[Att] == 1)
				{
					/*  This means that all training cases had value N/A.
					For consistency with instance distances, this
					attribute is ignored  */

					SpecialStatus[Att] |= SKIP;
				}
			break;

		case SAMPLEP:
			sscanf(PropVal, "\"%f\"", &SAMPLE);
			break;

		case INITP:
			sscanf(PropVal, "\"%d\"", &KRInit);
			break;

		case REDNP:
			sscanf(PropVal, "\"%f\"", &ErrReduction);
			break;

		case ENTRIESP:
			sscanf(PropVal, "\"%d\"", &MEMBERS);
			Entry = 0;
			return;
		}
	}
}



/*************************************************************************/
/*									 */
/*	Retrieve ruleset with extension Extension			 */
/*	(Separate functions for ruleset, single rule, single condition)	 */
/*									 */
/*************************************************************************/


RRuleSet *GetCommittee(String Extension)
/*	 -------------  */
{
	RRuleSet	*Cttee;
	int		m;

	ErrMsgs = 0;

	CheckFile(Extension, false);
	if (ErrMsgs)
	{
		if (Mf)
		{
			fclose(Mf);					Mf = Nil;
		}
		return Nil;
	}

	Cttee = Alloc(MEMBERS, RRuleSet);

	ForEach(m, 0, MEMBERS - 1)
	{
		Cttee[m] = InRules();
	}

	fclose(Mf);						Mf = Nil;

	return (ErrMsgs ? Nil : Cttee);
}



RRuleSet InRules(void)
/*	 -------  */
{
	RRuleSet	RS;
	RuleNo	r;
	char	Delim;

	RS = Alloc(1, RuleSetRec);

	do
	{
		switch (ReadProp(&Delim))
		{
		case RULESP:
			sscanf(PropVal, "\"%d\"", &RS->SNRules);
			break;
		}
	} while (Delim == ' ');

	/*  Read each rule  */

	RS->SRule = Alloc(RS->SNRules + 1, CRule);
	ForEach(r, 1, RS->SNRules)
	{
		RS->SRule[r] = InRule();
		RS->SRule[r]->RNo = r;
		RS->SRule[r]->MNo = Entry;
	}

	Entry++;

	return RS;
}



CRule InRule(void)
/*    ------  */
{
	CRule	R;
	int		d;
	char	Delim;
	Attribute	Att = 0;
	float	V, Range;

	R = Alloc(1, RuleRec);

	/*  General rule information  */

	do
	{
		switch (ReadProp(&Delim))
		{
		case CONDSP:
			sscanf(PropVal, "\"%d\"", &R->Size);
			break;

		case COVERP:
			sscanf(PropVal, "\"%d\"", &R->Cover);
			break;

		case MEANP:
			sscanf(PropVal, "\"%f\"", &R->Mean);
			break;

		case LOVALP:
			sscanf(PropVal, "\"%f\"", &R->LoVal);
			break;

		case HIVALP:
			sscanf(PropVal, "\"%f\"", &R->HiVal);
			break;

		case ESTERRP:
			sscanf(PropVal, "\"%f\"", &R->EstErr);
			break;
		}
	} while (Delim == ' ');

	Range = R->HiVal - R->LoVal;
	R->LoLim = ((V = R->LoVal - EXTRAP * Range) < 0 && R->LoVal >= 0 ? 0 : V);
	R->HiLim = ((V = R->HiVal + EXTRAP * Range) > 0 && R->HiVal <= 0 ? 0 : V);

	/*  Conditions making up rule's left-hand side  */

	R->Lhs = Alloc(R->Size + 1, Condition);
	ForEach(d, 1, R->Size)
	{
		R->Lhs[d] = InCondition();
	}

	/*  Linear model associated with rule  */

	R->Rhs = AllocZero(MaxAtt + 1, double);
	do
	{
		switch (ReadProp(&Delim))
		{
		case ATTP:
			Unquoted = RemoveQuotes(PropVal);
			Att = Which(Unquoted, AttName, 1, MaxAtt);
			if (!Att || Exclude(Att))
			{
				Error(MODELFILE, E_MFATT, Unquoted);
			}
			break;

		case COEFFP:
			sscanf(PropVal, "\"%lf\"", &R->Rhs[Att]);
			break;
		}
	} while (Delim == ' ');

	return R;
}



Condition InCondition(void)
/*        -----------  */
{
	Condition	C;
	char	Delim;
	int		X;
	double	XD;

	C = Alloc(1, CondRec);

	do
	{
		switch (ReadProp(&Delim))
		{
		case TYPEP:
			sscanf(PropVal, "\"%d\"", &X); C->NodeType = X;
			break;

		case ATTP:
			Unquoted = RemoveQuotes(PropVal);
			C->Tested = Which(Unquoted, AttName, 1, MaxAtt);
			if (!C->Tested || Exclude(C->Tested))
			{
				Error(MODELFILE, E_MFATT, Unquoted);
			}
			break;

		case CUTP:
			sscanf(PropVal, "\"%lf\"", &XD);	C->Cut = XD;
			break;

		case RESULTP:
			C->TestValue = (!strcmp(PropVal, "\"<=\"") ? 2 : 3);
			break;

		case VALP:
			if (Continuous(C->Tested))
			{
				C->TestValue = 1;
			}
			else
			{
				Unquoted = RemoveQuotes(PropVal);
				C->TestValue = Which(Unquoted,
					AttValName[C->Tested],
					1, MaxAttVal[C->Tested]);
				if (!C->TestValue)
				{
					Error(MODELFILE, E_MFATTVAL, Unquoted);
				}
			}
			break;

		case ELTSP:
			C->Subset = MakeSubset(C->Tested);
			C->TestValue = 1;
			break;
		}
	} while (Delim == ' ');

	return C;
}



void FreeCttee(RRuleSet *Cttee)
/*   ---------  */
{
	int		m, r;
	RRuleSet	RS;

	ForEach(m, 0, MEMBERS - 1)
	{
		if (!(RS = Cttee[m])) continue;

		ForEach(r, 1, RS->SNRules)
		{
			ReleaseRule(RS->SRule[r]);
		}
		Free(RS->SRule);
		Free(RS);
	}

	Free(Cttee);
}



/*************************************************************************/
/*									 */
/*	ASCII reading utilities						 */
/*									 */
/*************************************************************************/


int ReadProp(char *Delim)
/*  --------  */
{
	int		c, i;
	char	*p;
	Boolean	Quote = false;

	for (p = PropName; (c = fgetc(Mf)) != '='; )
	{
		if (p - PropName >= 19 || c == EOF)
		{
			Error(MODELFILE, E_MFEOF, "");
			PropName[0] = PropVal[0] = *Delim = '\00';
			return 0;
		}
		*p++ = c;
	}
	*p = '\00';

	for (p = PropVal; ((c = fgetc(Mf)) != ' ' && c != '\n') || Quote; )
	{
		if (c == EOF)
		{
			Error(MODELFILE, E_MFEOF, "");
			PropName[0] = PropVal[0] = '\00';
			return 0;
		}

		if ((i = p - PropVal) >= PropValSize)
		{
			Realloc(PropVal, (PropValSize += 10000) + 3, char);
			p = PropVal + i;
		}
		*p++ = c;
		if (c == '\\')
		{
			*p++ = fgetc(Mf);
		}
		else
			if (c == '"')
			{
				Quote = !Quote;
			}
	}
	*p = '\00';
	*Delim = c;

	return Which(PropName, Prop, 1, PROPS);
}


String RemoveQuotes(String S)
/*     ------------  */
{
	char	*p, *Start;

	p = Start = S;

	for (S++; *S != '"'; S++)
	{
		if (*S == '\\') S++;
		*p++ = *S;
		*S = '-';
	}
	*p = '\00';

	return Start;
}



Set MakeSubset(Attribute Att)
/*  ----------  */
{
	int		Bytes, b;
	char	*p;
	Set		S;

	Bytes = (MaxAttVal[Att] >> 3) + 1;
	S = AllocZero(Bytes, unsigned char);

	for (p = PropVal; *p; )
	{
		p = RemoveQuotes(p);
		b = Which(p, AttValName[Att], 1, MaxAttVal[Att]);
		if (!b) Error(MODELFILE, E_MFATTVAL, p);
		SetBit(b, S);

		for (p += strlen(p); *p != '"'; p++)
			;
		p++;
		if (*p == ',') p++;
	}

	return S;
}



/*=======================================================================*/
/*								  	 */
/*	Miscellaneous routines for rule handling		  	 */
/*								  	 */
/*=======================================================================*/


/*************************************************************************/
/*								  	 */
/*	Free space occupied by a rule					 */
/*								  	 */
/*************************************************************************/


void ReleaseRule(CRule R)
/*   -----------  */
{
	int	d;

	ForEach(d, 1, R->Size)
	{
		if (R->Lhs[d]->NodeType == BrSubset)
		{
			FreeUnlessNil(R->Lhs[d]->Subset);
		}
		FreeUnlessNil(R->Lhs[d]);
	}
	FreeUnlessNil(R->Lhs);
	FreeUnlessNil(R->Rhs);
	FreeUnlessNil(R);
}



/*************************************************************************/
/*								  	 */
/*	Print a ruleset							 */
/*								  	 */
/*************************************************************************/


void PrintRules(RRuleSet RS, String Msg)
/*   ----------  */
{
	int	r;

	fprintf(Of, "\n%s\n", Msg);

	ForEach(r, 1, RS->SNRules)
	{
		PrintRule(RS->SRule[r]);
	}
}



/*************************************************************************/
/*								  	 */
/*	Print the rule R					  	 */
/*								  	 */
/*************************************************************************/


void PrintRule(CRule R)
/*   ---------  */
{
	int		c, d, dd, id, LineLen, EntryLen, Indent, NCoeff = 0;
	Attribute	Att;
	char	Entry[1000];
	double	*Model;
	float	*Importance;

	if (MEMBERS > 1)
	{
		fprintf(Of, "\n  " T_Rule " %d/%d", R->MNo + 1, R->RNo);
	}
	else
	{
		fprintf(Of, "\n  " T_Rule " %d", R->RNo);
	}
	fprintf(Of, TX_RInfo(R->Cover, Precision + 1, R->Mean,
		R->LoVal, R->HiVal, R->EstErr));

	if (R->Size)
	{
		fprintf(Of, "    " T_If "\n");

		/*  Mark all conditions as unprinted or-ing flag to NodeType  */

		ForEach(d, 1, R->Size)
		{
			R->Lhs[d]->NodeType |= 8;
		}

		ForEach(d, 1, R->Size)
		{
			dd = 0;
			ForEach(id, 1, R->Size)
			{
				if ((R->Lhs[id]->NodeType & 8) &&
					(!dd || Before(R->Lhs[id], R->Lhs[dd])))
				{
					dd = id;
				}
			}

			R->Lhs[dd]->NodeType &= 7;
			PrintCondition(R->Lhs[dd]);
		}

		fprintf(Of, "    " T_Then "\n");
	}

	/*  Print the model.  First estimate the importance of the coefficients  */

	Model = R->Rhs;

	Importance = AllocZero(MaxAtt + 1, float);
	ForEach(Att, 1, MaxAtt)
	{
		if (Att != ClassAtt && Model[Att])
		{
			Importance[Att] = fabs(Model[Att]) * AttSD[Att];
			NCoeff++;
		}
	}

	sprintf(Entry, "%s =", AttName[ClassAtt]);
	Indent = CharWidth(Entry);

	sprintf(Entry + Indent, " %.14g", Model[0]);
	fprintf(Of, "\t%s", Entry);
	LineLen = CharWidth(Entry);

	ForEach(c, 1, NCoeff)
	{
		/*  Select the next attribute to print  */

		Att = 1;
		ForEach(d, 2, MaxAtt)
		{
			if (Importance[d] > Importance[Att]) Att = d;
		}
		Importance[Att] = 0;

		/*  Print, breaking lines when necessary  */

		sprintf(Entry, " %c %.14g %s",
			(Model[Att] > 0 ? '+' : '-'),
			fabs(Model[Att]),
			AttName[Att]);
		EntryLen = CharWidth(Entry);

		if (LineLen + EntryLen > 72)
		{
			fprintf(Of, "\n\t%*s", Indent, " ");
			LineLen = Indent;
		}
		fprintf(Of, "%s", Entry);
		LineLen += EntryLen;
	}
	fprintf(Of, "\n");
	Free(Importance);
}



/*************************************************************************/
/*								  	 */
/*	Print a condition C of a rule				  	 */
/*								  	 */
/*************************************************************************/


void PrintCondition(Condition C)
/*  --------------  */
{
	DiscrValue	v, pv, Last, Values = 0;
	Boolean	First = true;
	Attribute	Att;
	int		Col, Base, Entry;
	char	CVS[20];

	v = C->TestValue;
	Att = C->Tested;

	fprintf(Of, "\t%s", AttName[Att]);

	if (v < 0)
	{
		fprintf(Of, T_IsUnknown);
		return;
	}

	switch (C->NodeType)
	{
	case BrDiscr:
		fprintf(Of, " = %s\n", AttValName[Att][v]);
		break;

	case BrThresh:
		if (v == 1)
		{
			fprintf(Of, " = N/A\n");
		}
		else
		{
			CValToStr(C->Cut, Att, CVS);
			fprintf(Of, " %s %s\n", (v == 2 ? "<=" : ">"), CVS);
		}
		break;

	case BrSubset:
		/*  Count values at this branch  */

		ForEach(pv, 1, MaxAttVal[Att])
		{
			if (In(pv, C->Subset))
			{
				Last = pv;
				Values++;
			}
		}

		if (Values == 1)
		{
			fprintf(Of, " = %s\n", AttValName[Att][Last]);
			break;
		}

		if (Ordered(Att))
		{
			/*  Find first value  */

			for (pv = 1; !In(pv, C->Subset); pv++)
				;

			fprintf(Of, " " T_InRange " [%s-%s]\n",
				AttValName[Att][pv], AttValName[Att][Last]);
			break;
		}

		/*  Must keep track of position to break long lines  */

		fprintf(Of, " %s {", T_ElementOf);
		Col = Base = CharWidth(AttName[Att]) + CharWidth(T_ElementOf) + 11;

		ForEach(pv, 1, MaxAttVal[Att])
		{
			if (In(pv, C->Subset))
			{
				Entry = CharWidth(AttValName[Att][pv]);

				if (First)
				{
					First = false;
				}
				else
					if (Col + Entry + 2 >= WIDTH)
					{
						Col = Base;
						fprintf(Of, ",\n%*s", Col, "");
					}
					else
					{
						fprintf(Of, ", ");
						Col += 2;
					}

				fprintf(Of, "%s", AttValName[Att][pv]);
				Col += Entry;
			}
		}
		fprintf(Of, "}\n");
	}
}



/*************************************************************************/
/*								  	 */
/*	Check whether a case satisfies a condition			 */
/*								  	 */
/*************************************************************************/


Boolean Satisfies(DataRec CaseDesc, Condition OneCond)
/*      ---------  */
{
	DiscrValue	v;
	ContValue	cv;
	DiscrValue	Outcome;
	Attribute	Att;

	Att = OneCond->Tested;

	/*  Determine the outcome of this test on this item  */

	switch (OneCond->NodeType)
	{
	case BrDiscr:  /* test of discrete attribute */

		v = DVal(CaseDesc, Att);
		Outcome = (v == 0 ? -1 : v);
		break;

	case BrThresh:  /* test of continuous attribute */

		cv = CVal(CaseDesc, Att);
		Outcome = (NotApplic(CaseDesc, Att) ? 1 :
			cv <= OneCond->Cut ? 2 : 3);
		break;

	case BrSubset:  /* subset test on discrete attribute  */

		v = DVal(CaseDesc, Att);
		Outcome = (v <= MaxAttVal[Att] && In(v, OneCond->Subset) ?
			OneCond->TestValue : 0);
	}

	return (Outcome == OneCond->TestValue);
}



/*=======================================================================*/
/*                                                              	 */
/*	Predict the value of a case from a ruleset			 */
/*                                                              	 */
/*=======================================================================*/



/*************************************************************************/
/*                                                              	 */
/*	Find rules that apply to case and form average			 */
/*                                                              	 */
/*************************************************************************/


float PredictValue(RRuleSet *Cttee, DataRec CaseDesc, float *ErrLim)
/*    ------------  */
{
	double	PredSum = 0;
	int		m;
	float	IntErrLim;
	double	SumErrLim = 0;

	ForEach(m, 0, MEMBERS - 1)
	{
		PredSum += RuleSetPrediction(Cttee[m], CaseDesc, &IntErrLim);
		SumErrLim += IntErrLim;
	}

	*ErrLim = (SumErrLim / MEMBERS) * ErrReduction;

	return PredSum / MEMBERS;
}



float RuleSetPrediction(RRuleSet RS, DataRec CaseDesc, float *ErrLim)
/*    -----------------  */
{
	double	Sum = 0, Weight = 0, Val;
	int		r;
	CRule	R;
	Attribute	Att;
	double	SumErrLim = 0;

	ForEach(r, 1, RS->SNRules)
	{
		R = RS->SRule[r];

		if (Matches(R, CaseDesc))
		{
			/*  Evaluate RHS.  Cannot use RawLinModel() because
			have not run FindModelAtts()  */

			Val = R->Rhs[0];
			ForEach(Att, 1, MaxAtt)
			{
				Val += CVal(CaseDesc, Att) * R->Rhs[Att];
			}

			Sum += Bound(Val, R->LoLim, R->HiLim);
			SumErrLim += 2.5 * R->EstErr;
			Weight += 1.0;
		}
	}

	if (Weight)
	{
		*ErrLim = SumErrLim / Weight;
		return Sum / Weight;
	}
	else
	{
		*ErrLim = GlobalErrLim;
		return GlobalMean;
	}
}



/*************************************************************************/
/*								         */
/*	Determine whether a case satisfies all conditions of a rule	 */
/*								         */
/*************************************************************************/


Boolean Matches(CRule R, DataRec Case)
/*      -------  */
{
	int d;

	ForEach(d, 1, R->Size)
	{
		if (!Satisfies(Case, R->Lhs[d]))
		{
			return false;
		}
	}

	return true;
}



/*=======================================================================*/
/*								         */
/*	Routines for instance-based prediction				 */
/*								         */
/*=======================================================================*/



#define	 DPREC	       16	/* distance precision */

#define  AdjustedValue(i,b)	(Cttee?(Class(Instance[i])+b-RSPredVal[i]):\
				 Class(Instance[i]))

#define	 CVDiff(c,cv,a)		(fabs(CVal(c,a)-(cv))/(5*AttSD[a]))

int	 MinN;			/* minimum close neighbors */

AttValue *KDBlock;		/* copy of instances in KDTree */



/*************************************************************************/
/*									 */
/*	Prepare for instance-based prediction				 */
/*									 */
/*************************************************************************/


void InitialiseInstances(RRuleSet *Cttee)
/*   -------------------  */
{
	CaseNo	i, FarInstance;
	Attribute	Att;
	double	Dist;

	/*  Use the data items as instances.  The vector Instance[] must
	be allocated separately since the indexing re-orders the items  */

	Instance = Alloc(MaxCase + 1, DataRec);
	MaxInstance = MaxCase;
	ForEach(i, 0, MaxCase)
	{
		Instance[i] = Case[i];
	}

	Tested = AllocZero(MaxAtt + 1, unsigned char);
	ValFreq = Alloc(MaxDiscrVal + 1, CaseCount);

	GNNEnv.AttMinD = Alloc(MaxAtt + 1, float);

	/*  Construct the first reference point  */

	Ref[0] = Alloc(MaxAtt + 1, AttValue);
	ForEach(Att, 1, MaxAtt)
	{
		if (Continuous(Att))
		{
			CVal(Ref[0], Att) = AttMean[Att] - 2.5 * AttSD[Att];
		}
		else
		{
			DVal(Ref[0], Att) = 2;
		}
	}

	/*  Compute distances and find an instance as far as possible from
	the first reference point  */

	FarInstance = 0;

	ForEach(i, 0, MaxInstance)
	{
		DRef1(Instance[i]) = Dist = Distance(Instance[i], Ref[0], 1E38);

		if (Dist > DRef1(Instance[FarInstance])) FarInstance = i;
	}

	Ref[1] = Alloc(MaxAtt + 1, AttValue);
	memcpy(Ref[1], Instance[FarInstance], (MaxAtt + 1) * sizeof(AttValue));

	/*  Now compute distances to the second reference point  */

	ForEach(i, 0, MaxInstance)
	{
		DRef2(Instance[i]) = Dist = Distance(Instance[i], Ref[1], 1E38);
	}

	KDTree = BuildIndex(0, MaxCase);

	Free(Tested);				Tested = Nil;
	Free(ValFreq);				ValFreq = Nil;

	/*  Tabulate values predicted by ruleset.  Cannot use
	FindPredictedValues() because that would use instances!  */

	RSPredVal = Alloc(MaxCase + 1, float);
	RSErrLim = Alloc(MaxCase + 1, float);

	ForEach(i, 0, MaxCase)
	{
		RSPredVal[i] = PredictValue(Cttee, Instance[i], &RSErrLim[i]);
	}


	MinN = (NN + 1) / 2;
	GNNEnv.WorstBest = GNNEnv.BestD + NN - 1;
}



/*************************************************************************/
/*									 */
/*	Clean up storage associated with instances			 */
/*									 */
/*************************************************************************/


void FreeInstances(void)
/*   -------------  */
{
	if (Instance)
	{
		Free(Instance);					Instance = Nil;
		Free(KDBlock);					KDBlock = Nil;
	}

	FreeUnlessNil(GNNEnv.AttMinD);			GNNEnv.AttMinD = Nil;
	FreeUnlessNil(RSPredVal);				RSPredVal = Nil;

	if (KDTree)
	{
		FreeUnlessNil(Ref[0]);				Ref[0] = Nil;
		FreeUnlessNil(Ref[1]);				Ref[1] = Nil;
		FreeIndex(KDTree);				KDTree = Nil;
	}
}



/*************************************************************************/
/*									 */
/*	Use nearest neighbors to estimate target for Case.		 */
/*	If RS is not nil, values of neighbors are adjusted by		 */
/*	ruleset RS before being combined.			 	 */
/*									 */
/*************************************************************************/


float NNEstimate(RRuleSet *Cttee, DataRec Case, NNEnv E, float *ErrLim)
/*    ----------  */
{
	FindNearestNeighbors(Case, E);

	return AverageNeighbors(Cttee, Case, E, ErrLim);
}



/*************************************************************************/
/*									 */
/*	Determine the distance between a case and a saved instance.	 */
/*	Stop if the distance becomes too great to be relevant.		 */
/*									 */
/*	Cubist uses Manhattan distance between instances (i.e. sum	 */
/*	of the attribute differences).  These are defined as:		 */
/*	* If the values are the same: 0.0				 */
/*	* If one value is N/A and the other is not: 1.0			 */
/*	* For ordered attribute:  difference / number of values		 */
/*	* discrete att: 2 / number of discrete values			 */
/*	* continuous att: difference / 5 SD				 */
/*									 */
/*************************************************************************/


float Distance(DataRec Case1, DataRec Case2, float Thresh)
/*    --------  */
{
	Attribute	Att;
	double	DTot, Diff;

	for (Att = 1, DTot = 0; DTot < Thresh && Att <= MaxAtt; Att++)
	{
		if (Skip(Att) || Att == ClassAtt) continue;

		if (NotApplic(Case2, Att) != NotApplic(Case1, Att))
		{
			DTot += 1.0;
		}
		else
			if (Continuous(Att))
			{
				Diff = CVDiff(Case2, CVal(Case1, Att), Att);
				DTot += Min(1.0, Diff);
			}
			else
				if (Ordered(Att))
				{
					DTot += fabs(DVal(Case2, Att) - DVal(Case1, Att)) /
						(MaxAttVal[Att] - 1);
				}
				else
					if (DVal(Case2, Att) != DVal(Case1, Att))
					{
						DTot += 2.0 / (MaxAttVal[Att] - 1);
					}
	}

	return DTot;
}



/*************************************************************************/
/*									 */
/*	Check whether a saved instance should be one of the neighbors.	 */
/*	Distances are rounded to precision 1/DPREC			 */
/*									 */
/*************************************************************************/


void CheckDistance(DataRec Case, CaseNo Saved, NNEnv E)
/*   -------------  */
{
	int		d, dd;
	float	Dist;

	if (Instance[Saved] == Case) return;

	Dist = rint(DPREC *
		Distance(Case, Instance[Saved], *E->WorstBest + 0.55 / DPREC))
		/ DPREC;

	if (Dist <= *E->WorstBest)
	{
		for (d = 0; d < MAXN && E->BestD[d] < Dist; d++)
			;

		if (d < MAXN)
		{
			for (dd = MAXN - 1; dd > d; dd--)
			{
				E->BestD[dd] = E->BestD[dd - 1];
				E->BestI[dd] = E->BestI[dd - 1];
			}

			E->BestD[d] = Dist;
			E->BestI[d] = Saved;
		}
	}
}



/*************************************************************************/
/*									 */
/*	Find up to MAXN nearest neighbors (allowing for ties)		 */
/*	Stop checking an instance if it is already too far from the	 */
/*	current case.							 */
/*									 */
/*************************************************************************/


void FindNearestNeighbors(DataRec Case, NNEnv E)
/*   --------------------  */
{
	int		d;
	Attribute	Att;

	/*  Clear best distances and attribute minimum distances  */

	ForEach(d, 0, MAXN - 1)
	{
		E->BestD[d] = MAXD;
		E->BestI[d] = -1;
	}

	ForEach(Att, 1, MaxAtt)
	{
		E->AttMinD[Att] = 0;
	}

	DRef1(Case) = Distance(Case, Ref[0], 1E38);
	DRef2(Case) = Distance(Case, Ref[1], 1E38);

	ScanIndex(Case, KDTree, 0.0, E);
}



/*************************************************************************/
/*									 */
/*	Find weighted average value of selected neighbors.		 */
/*	[Weight = 1 / (ManHattan distance + 0.5).]			 */
/*	Have to be careful with ties.					 */
/*									 */
/*************************************************************************/


float AverageNeighbors(RRuleSet *Cttee, DataRec Case, NNEnv E, float *ErrLim)
/*    ----------------  */
{
	int		d = 0, Count = 0, Same, Last;
	double	Est, BaseEst, Wt, SameSum, SameWt, TotSum = 0, TotWt = 0;
	float	BaseErrLim, SumErrLim = 0, SumELWt = 0;

	if (Cttee)
	{
		BaseEst = PredictValue(Cttee, Case, &BaseErrLim);
	}
	else
	{
		BaseEst = GlobalMean;
		BaseErrLim = GlobalErrLim;
	}

	/*  Check the number of neighbors actually found  */

	for (Last = MAXN - 1; Last >= 0 && E->BestI[Last] < 0; Last--)
		;

	if (Last + 1 < MinN)
	{
		*ErrLim = BaseErrLim;
		return BaseEst;
	}

	/*  Extract groups of neighbors with the same values of BestD  */

	while (d <= Last && Count < NN && Count < MaxInstance)
	{
		SameSum = SameWt = Same = 0;
		Wt = 1 / (E->BestD[d] + 0.5);
		do
		{
			Est = AdjustedValue(E->BestI[d], BaseEst);
			Est = Bound(Est, Floor, Ceiling);

			SameSum += Wt * Est;
			SameWt += Wt;
			SumErrLim += Wt * Max(BaseErrLim, RSErrLim[E->BestI[d]]);
			SumELWt += Wt;

			Same++;
			d++;
		} while (d <= Last && E->BestD[d] == E->BestD[d - 1]);

		if (Count + Same > NN)
		{
			Wt = (NN - Count) / (float)Same;
			TotSum += Wt * SameSum;
			TotWt += Wt * SameWt;
			Count = NN;
		}
		else
		{
			TotSum += SameSum;
			TotWt += SameWt;
			Count += Same;
		}
	}

	*ErrLim = 0.8 * SumErrLim / SumELWt;
	Est = TotSum / TotWt;
	return Bound(Est, Floor, Ceiling);
}



/*************************************************************************/
/*									 */
/*	Show neighbors for the last case processed.			 */
/*									 */
/*************************************************************************/


void ShowNearestNeighbors(int Offset, NNEnv E)
/*   --------------------  */
{
	int		NoN, d;
	CaseNo	i;
	float	MD;

	/*  Find number of NN */

	MD = E->BestD[NN - 1];
	for (NoN = 0;
		NoN < MAXN && E->BestI[NoN] >= 0 && E->BestD[NoN] <= MD;
		NoN++)
		;

	if (NoN < MinN) return;

	ForEach(d, 0, NoN - 1)
	{
		if (d > 0)
		{
			printf("\n%*s", Offset, " ");
		}
		else
		{
			printf("  ");
		}

		i = E->BestI[d];

		printf("%6.1f", E->BestD[d]);

		if (LabelAtt)
		{
			printf("  %-15.15s",
				(String)(IgnoredVals + SVal(Instance[i], LabelAtt)));
		}
		else
		{
			printf("  %6d", DVal(Instance[i], 0));
		}
	}
}



/*************************************************************************/
/*									 */
/*	The following routines are concerned with indexing the		 */
/*	instances in a KD-tree, and using the tree to locate nearest	 */
/*	neighbors without having to examine each saved instance.	 */
/*	Note that BuildIndex() can create branches that have no cases	 */
/*	associated with them -- this is ok since it speeds up		 */
/*	ScanIndex() by increasing MinD.					 */
/*									 */
/*************************************************************************/


Index BuildIndex(CaseNo Fp, CaseNo Lp)
/*    ----------  */
{
	Index	Node;
	DiscrValue	v, vv;
	CaseNo	i, Xp, Kp;
	CaseCount	Cases;
	double	Mean, BestMean, ExpDist, BestExpDist = 0, ProbNA;
	float	Dist, MinDRef[2], MaxDRef[2];
	Attribute	Att, BestAtt = 0;

	if (Lp < Fp) return Nil;

	Node = AllocZero(1, IndexRec);

	if (Lp > Fp)
	{
		MinDRef[0] = MaxDRef[0] = DRef1(Instance[Fp]);
		MinDRef[1] = MaxDRef[1] = DRef2(Instance[Fp]);

		ForEach(i, Fp + 1, Lp)
		{
			if ((Dist = DRef1(Instance[i])) < MinDRef[0])
			{
				MinDRef[0] = Dist;
			}
			else
				if (Dist > MaxDRef[0])
				{
					MaxDRef[0] = Dist;
				}

			if ((Dist = DRef2(Instance[i])) < MinDRef[1])
			{
				MinDRef[1] = Dist;
			}
			else
				if (Dist > MaxDRef[1])
				{
					MaxDRef[1] = Dist;
				}
		}

		Node->MinDRef[0] = MinDRef[0];
		Node->MaxDRef[0] = MaxDRef[0];
		Node->MinDRef[1] = MinDRef[1];
		Node->MaxDRef[1] = MaxDRef[1];

		/*  Find the attribute with the greatest expected difference.
			Distances to the reference points are candidates  */

		ForEach(Att, 1, MaxAtt)
		{
			if (Skip(Att) || Att == ClassAtt || Tested[Att] && Discrete(Att))
			{
				continue;
			}

			/*  Separate all N/A values (real attributes only)  */

			Xp = Fp;
			ForEach(i, Fp, Lp)
			{
				if (NotApplic(Instance[i], Att))
				{
					SwapInstance(i, Xp++);
				}
			}

			ProbNA = (Xp - Fp) / (Lp - Fp + 1.0);
			if (!(Cases = Lp - Xp + 1)) continue;

			ExpDist = Mean = 0;

			if (Continuous(Att))
			{
				/*  Expected distance is average difference from mean  */

				ForEach(i, Xp, Lp)
				{
					Mean += CVal(Instance[i], Att);
				}
				Mean /= Cases;

				ForEach(i, Xp, Lp)
				{
					ExpDist += CVDiff(Instance[i], Mean, Att);
				}
				ExpDist /= Cases;
			}
			else
			{
				/*  Expected distance is computed from pairwise differences
					of values  */

				ForEach(v, 2, MaxAttVal[Att])
				{
					ValFreq[v] = 0;
				}

				ForEach(i, Xp, Lp)
				{
					ValFreq[DVal(Instance[i], Att)] ++;
				}

				if (Ordered(Att))
				{
					ForEach(v, 2, MaxAttVal[Att])
					{
						ForEach(vv, 2, MaxAttVal[Att])
						{
							ExpDist += ValFreq[v] * ValFreq[vv] * fabs(vv - v);
						}
					}
				}
				else
				{
					ForEach(v, 2, MaxAttVal[Att])
					{
						ExpDist += ValFreq[v] * (Cases - ValFreq[v]) * 2.0;
					}
				}

				ExpDist /= (MaxAttVal[Att] - 1) * Cases * Cases;
			}

			/*  Final expected distance =
				(prob one or other N/A) * 1 +
				(prob both known) * (expected distance if known)  */

			ExpDist = 2 * ProbNA * (1 - ProbNA) +
				(1 - ProbNA) * (1 - ProbNA) * ExpDist;

			if (ExpDist > BestExpDist)
			{
				BestExpDist = ExpDist;
				BestAtt = Att;
				BestMean = Mean;
			}
		}
	}

	/*  Check whether leaf or sub-index  */

	if (!BestAtt)
	{
		Node->Tested = 0;
		Node->IFp = Fp;
		Node->ILp = Lp;
	}
	else
		if (Discrete(BestAtt))
		{
			Node->Tested = BestAtt;
			Node->SubIndex = Alloc(MaxAttVal[BestAtt] + 1, Index);

			Tested[BestAtt] = true;

			/*  Sort instances by attribute value  */

			Kp = Fp;
			ForEach(v, 1, MaxAttVal[BestAtt])
			{
				ForEach(Xp, Kp, Lp)
				{
					if (DVal(Instance[Xp], BestAtt) == v)
					{
						SwapInstance(Xp, Kp++);
					}
				}

				Node->SubIndex[v] = BuildIndex(Fp, Kp - 1);
				Fp = Kp;
			}

			Tested[BestAtt] = false;
		}
		else
		{
			Node->Tested = BestAtt;
			Node->Cut = BestMean;

			/*  There are three branches for continuous attributes:
				N/A / value <= mean / value > mean  */

			Xp = Fp;
			ForEach(i, Fp, Lp)
			{
				if (NotApplic(Instance[i], BestAtt))
				{
					SwapInstance(i, Xp++);
				}
			}

			Kp = Xp;
			ForEach(i, Xp, Lp)
			{
				if (CVal(Instance[i], BestAtt) <= BestMean)
				{
					SwapInstance(i, Kp++);
				}
			}

			/*  Safety check:  return leaf if not a sensible split  */

			if (Xp == Lp + 1 ||
				Xp == Fp && Kp == Lp + 1 ||
				Kp == Fp)
			{
				Node->Tested = 0;
				Node->IFp = Fp;
				Node->ILp = Lp;
			}
			else
			{
				Node->SubIndex = Alloc(4, Index);
				Node->SubIndex[1] = BuildIndex(Fp, Xp - 1);
				Node->SubIndex[2] = BuildIndex(Xp, Kp - 1);
				Node->SubIndex[3] = BuildIndex(Kp, Lp);
			}
		}

	return Node;
}



void SwapInstance(CaseNo A, CaseNo B)
/*   ------------  */
{
	DataRec	Hold;

	Hold = Instance[A];
	Instance[A] = Instance[B];
	Instance[B] = Hold;
}



void CopyInstances(void)
/*   -------------  */
{
	AttValue	*KDBlockP;
	CaseNo	i;

	KDBlockP = KDBlock = Alloc((MaxInstance + 1) * (MaxAtt + 3), AttValue);
	ForEach(i, 0, MaxInstance)
	{
		memcpy(KDBlockP, Instance[i], (MaxAtt + 3) * sizeof(AttValue));
		Instance[i] = KDBlockP;
		KDBlockP += MaxAtt + 3;
	}
}



void ScanIndex(DataRec Case, Index Node, float MinD, NNEnv E)
/*   ---------  */
{
	CaseNo	Xp;
	DiscrValue	Forks, First, v;
	float	NewMinD, SaveAttMinD;
	Attribute	Att;

	if (Node == Nil) return;

	if (!(Att = Node->Tested))
	{
		ForEach(Xp, Node->IFp, Node->ILp)
		{
			CheckDistance(Case, Xp, E);
		}
	}
	else
		if (Max(Node->MinDRef[0] - DRef1(Case), DRef1(Case) - Node->MaxDRef[0]) <=
			*E->WorstBest + 0.5 / DPREC &&
			Max(Node->MinDRef[1] - DRef2(Case), DRef2(Case) - Node->MaxDRef[1]) <=
			*E->WorstBest + 0.5 / DPREC)
		{
			if (Discrete(Att))
			{
				First = DVal(Case, Att);
				Forks = MaxAttVal[Att];
			}
			else
			{
				First = (NotApplic(Case, Att) ? 1 :
					CVal(Case, Att) <= Node->Cut ? 2 : 3);
				Forks = 3;
			}

			/*  Try best sub-index first, then other sub-indices so long
				as can improve on current best neighbors  */

			if (First <= Forks)
			{
				ScanIndex(Case, Node->SubIndex[First], MinD, E);
			}

			SaveAttMinD = E->AttMinD[Att];

			ForEach(v, 1, Forks)
			{
				if (v == First || !Node->SubIndex[v]) continue;

				E->AttMinD[Att] =
					(v == 1 || First == 1 ? 1.0 :
						Continuous(Att) ? CVDiff(Case, Node->Cut, Att) :
						Ordered(Att) ?
						fabs(v - First) / (MaxAttVal[Att] - 1) :
						2.0 / (MaxAttVal[Att] - 1));
				NewMinD = MinD + E->AttMinD[Att] - SaveAttMinD;

				if (NewMinD <= *E->WorstBest + 0.5 / DPREC)
				{
					ScanIndex(Case, Node->SubIndex[v], NewMinD, E);
				}
			}

			E->AttMinD[Att] = SaveAttMinD;
		}
}



void FreeIndex(Index Node)
/*   ---------  */
{
	DiscrValue	v, Forks;
	Attribute	Att;

	if (Node == Nil) return;

	if ((Att = Node->Tested))
	{
		Forks = (Discrete(Att) ? MaxAttVal[Att] : 3);
		ForEach(v, 1, Forks)
		{
			FreeIndex(Node->SubIndex[v]);
		}
		Free(Node->SubIndex);
	}

	Free(Node);
}



/*=======================================================================*/
/*									 */
/*	Various utility routines					 */
/*									 */
/*=======================================================================*/


/*************************************************************************/
/*									 */
/*	This is a specialised form of the getopt utility.		 */
/*									 */
/*************************************************************************/


String	OptArg, Option;


char ProcessOption(int Argc, char *Argv[], char *Options)
/*   -------------  */
{
	int		i;
	static int	OptNo = 1;

	if (OptNo >= Argc) return '\00';

	if (*(Option = Argv[OptNo++]) != '-') return '?';

	for (i = 0; Options[i]; i++)
	{
		if (Options[i] == Option[1])
		{
			OptArg = (Options[i + 1] != '+' ? 0 :
				Option[2] ? Option + 2 :
				OptNo < Argc ? Argv[OptNo++] : "0");
			return Option[1];
		}
	}

	return '?';
}



/*************************************************************************/
/*									 */
/*	Protected memory allocation routines				 */
/*									 */
/*************************************************************************/



void *Pmalloc(size_t Bytes)
/*    -------  */
{
	void *p = Nil;

	if (!Bytes || (p = (void *)malloc(Bytes)))
	{
		return p;
	}

	Error(NOMEM, "", "");

	return Nil;	/* to keep compilers happy */
}



void *Prealloc(void *Present, size_t Bytes)
/*    --------  */
{
	void *p = Nil;

	if (!Bytes) return Nil;

	if (!Present) return Pmalloc(Bytes);

	if ((p = (void *)realloc(Present, Bytes)))
	{
		return p;
	}

	Error(NOMEM, "", "");

	return Nil;	/* to keep compilers happy */
}



void *Pcalloc(size_t Number, unsigned int Size)
/*    -------  */
{
	void *p = Nil;

	if (!Number || (p = (void *)calloc(Number, Size)))
	{
		return p;
	}

	Error(NOMEM, "", "");

	return Nil;	/* to keep compilers happy */
}



void FreeVector(void **V, int First, int Last)
/*   ----------  */
{
	if (V)
	{
		while (First <= Last)
		{
			FreeUnlessNil(V[First]);
			First++;
		}

		Free(V);
	}
}



/*************************************************************************/
/*									 */
/*	Error messages							 */
/*									 */
/*************************************************************************/


void Error(int ErrNo, String S1, String S2)
/*   -----  */
{
	Boolean	Quit = false, WarningOnly = false;
	char	Buffer[10000], *Msg = Buffer;

#ifdef WIN32
	if (ErrNo == NOMEM)
	{
		MessageBox(NULL, "Cannot allocate sufficient memory", "Fatal Error",
			MB_ICONERROR | MB_OK);
		Goodbye(1);
	}
	else
		if (ErrNo == MODELFILE)
		{
			if (!ErrMsgs)
			{
				sprintf(Msg, "File %s is incompatible with .names file\n(%s `%s')",
					Fn, S1, S2);
				MessageBox(NULL, Msg, "Cannot Load Model", MB_ICONERROR | MB_OK);
			}
			ErrMsgs++;
			return;
		}
#endif

#ifndef WIN32
	if (!ErrMsgs) fprintf(Of, "\n");
#endif

	if (ErrNo == NOFILE || ErrNo == NOMEM || ErrNo == MODELFILE)
	{
		sprintf(Msg, "*** ");
	}
	else
	{
		sprintf(Msg, TX_Line(LineNo, Fn));
	}
	Msg += strlen(Buffer);

	switch (ErrNo)
	{
	case NOFILE:
		sprintf(Msg, E_NOFILE(Fn, S2));
		Quit = true;
		break;

	case BADATTNAME:
		sprintf(Msg, E_BADATTNAME, S1);
		break;

	case EOFINATT:
		sprintf(Msg, E_EOFINATT, S1);
		break;

	case SINGLEATTVAL:
		sprintf(Msg, E_SINGLEATTVAL(S1, S2));
		break;

	case DUPATTNAME:
		sprintf(Msg, E_DUPATTNAME, S1);
		break;

	case CWTATTERR:
		sprintf(Msg, E_CWTATTERR);
		break;

	case BADATTVAL:
		sprintf(Msg, E_BADATTVAL(S2, S1));
		break;

	case BADNUMBER:
		sprintf(Msg, E_BADNUMBER(S1));
		break;

	case NOMEM:
		sprintf(Msg, E_NOMEM);
		Quit = true;
		break;

	case TOOMANYVALS:
		sprintf(Msg, E_TOOMANYVALS(S1, (int)(long)S2));
		Quit = true;
		break;

	case BADDISCRETE:
		sprintf(Msg, E_BADDISCRETE, S1);
		Quit = true;
		break;

	case NOTARGET:
		sprintf(Msg, E_NOTARGET, S1);
		Quit = true;
		break;

	case BADTARGET:
		sprintf(Msg, E_BADTARGET, S1);
		Quit = true;
		break;

	case LONGNAME:
		sprintf(Msg, E_LONGNAME);
		Quit = true;
		break;

	case HITEOF:
		sprintf(Msg, E_HITEOF);
		break;

	case MISSNAME:
		sprintf(Msg, E_MISSNAME, S2);
		break;

	case BADTSTMP:
		sprintf(Msg, E_BADTSTMP(S2, S1));
		break;

	case BADDATE:
		sprintf(Msg, E_BADDATE(S2, S1));
		break;

	case BADTIME:
		sprintf(Msg, E_BADTIME(S2, S1));
		break;

	case UNKNOWNATT:
		sprintf(Msg, E_UNKNOWNATT, S1);
		break;

	case BADDEF1:
		sprintf(Msg, E_BADDEF1(AttName[MaxAtt], S1, S2));
		break;

	case BADDEF2:
		sprintf(Msg, E_BADDEF2(AttName[MaxAtt], S1, S2));
		break;

	case SAMEATT:
		sprintf(Msg, E_SAMEATT(AttName[MaxAtt], S1));
		WarningOnly = true;
		break;

	case BADDEF3:
		sprintf(Msg, E_BADDEF3, AttName[MaxAtt]);
		break;

	case BADDEF4:
		sprintf(Msg, E_BADDEF4, AttName[MaxAtt]);
		WarningOnly = true;
		break;

	case MODELFILE:
		sprintf(Msg, EX_MODELFILE(Fn));
		sprintf(Msg, "    (%s `%s')\n", S1, S2);
		Quit = true;
		break;
	}

#ifdef WIN32
	if (Of)
	{
		fprintf(Of, Buffer);
	}
	else
		if (ErrMsgs <= 10)
		{
			MessageBox(NULL, Buffer, (WarningOnly ? "Warning" : "Error"), MB_OK);
		}
#else
	fprintf(Of, Buffer);
#endif

	if (!WarningOnly) ErrMsgs++;

	if (ErrMsgs == 10)
	{
#if defined WIN32 && ! defined _CONSOLE
		MessageBox(NULL, T_ErrorLimit, "Too many errors!", MB_OK);
#else
		fprintf(Of, T_ErrorLimit);
#endif
		Quit = true;
	}

	if (Quit && Of)
	{
		Goodbye(1);
	}
}



/*************************************************************************/
/*									 */
/*	Open file with given extension for read/write			 */
/*									 */
/*************************************************************************/


FILE *GetFile(String Extension, String RW)
/*    --------  */
{
	strcpy(Fn, FileStem);
	strcat(Fn, Extension);
	return fopen(Fn, RW);
}



/*************************************************************************/
/*									 */
/*	Determine precision of floating value				 */
/*									 */
/*************************************************************************/


int Denominator(ContValue Val)
/*  -----------  */
{
	unsigned int	N, D = 1;
	float		AltVal;

	Val = fabs(Val);

	while (D < MAXDENOM)
	{
		N = Val * D + 0.5;

		AltVal = N / (double)D;

		if (abs((*(int*)&AltVal) - (*(int*)&Val)) < 2) break;

		D *= 10;
	}

	return D;
}


int FracBase(Attribute Att)
/*  --------  */
{
	CaseNo	i;
	long double	Denom = 1, Num;
	int		Prec = 0;
	float	Val, AltVal;

	ForEach(i, 0, MaxCase)
	{
		if ((Val = CVal(Case[i], Att)) != UNKNOWN &&
			!NotApplic(Case[i], Att))
		{
			Val = fabs(Val);

			while (true)
			{
				Num = rint(Val * Denom);

				AltVal = Num / Denom;

				if (abs((*(int*)&AltVal) - (*(int*)&Val)) < 2) break;

				Denom *= 10;
				if (++Prec > 15) return Prec;
			}
		}
	}

	return Prec;
}



/*************************************************************************/
/*									 */
/*	Routines to process date (Algorithm due to Gauss?)		 */
/*									 */
/*************************************************************************/


int GetInt(String S, int N)
/*  ------  */
{
	int	Result = 0;

	while (N--)
	{
		if (!isdigit(*S)) return 0;

		Result = Result * 10 + (*S++ - '0');
	}

	return Result;
}


int DateToDay(String DS)	/*  Day 1 is 0000/03/01  */
/*  ---------  */
{
	int Year, Month, Day;

	if (strlen(DS) != 10) return 0;

	Year = GetInt(DS, 4);
	Month = GetInt(DS + 5, 2);
	Day = GetInt(DS + 8, 2);

	if (!(DS[4] == '/' && DS[7] == '/' || DS[4] == '-' && DS[7] == '-') ||
		Year < 0 || Month < 1 || Day < 1 ||
		Month > 12 ||
		Day > 31 ||
		Day > 30 &&
		(Month == 4 || Month == 6 || Month == 9 || Month == 11) ||
		Month == 2 &&
		(Day > 29 ||
			Day > 28 && (Year % 4 != 0 ||
				Year % 100 == 0 && Year % 400 != 0)))
	{
		return 0;
	}

	if ((Month -= 2) <= 0)
	{
		Month += 12;
		Year -= 1;
	}

	return Year * 365 + Year / 4 - Year / 100 + Year / 400
		+ 367 * Month / 12
		+ Day - 30;
}


void DayToDate(int Day, String Date)
/*   ---------  */
{
	int Year, Month, OrigDay = Day;

	if (Day <= 0)
	{
		strcpy(Date, "?");
		return;
	}

	Year = (Day - 1) / 365.2425L;  /*  Year = completed years  */
	Day -= Year * 365 + Year / 4 - Year / 100 + Year / 400;

	if (Day < 1)
	{
		Year--;
		Day = OrigDay - (Year * 365 + Year / 4 - Year / 100 + Year / 400);
	}
	else
		if (Day > 366 ||
			Day == 366 &&
			((Year + 1) % 4 != 0 || (Year + 1) % 100 == 0 && (Year + 1) % 400 != 0))
		{
			Year++;
			Day = OrigDay - (Year * 365 + Year / 4 - Year / 100 + Year / 400);
		}

	Month = (Day + 30) * 12 / 367;
	Day -= 367 * Month / 12 - 30;
	if (Day < 1)
	{
		Month = 11;
		Day = 31;
	}

	Month += 2;
	if (Month > 12)
	{
		Month -= 12;
		Year++;
	}

	sprintf(Date, "%d/%d%d/%d%d", Year, Month / 10, Month % 10, Day / 10, Day % 10);
}



/*************************************************************************/
/*									 */
/*	Routines to process clock time					 */
/*									 */
/*************************************************************************/


int TimeToSecs(String TS)
/*  ----------  */
{
	int Hour, Mins, Secs;

	if (strlen(TS) != 8) return -1;

	Hour = GetInt(TS, 2);
	Mins = GetInt(TS + 3, 2);
	Secs = GetInt(TS + 6, 2);

	if (TS[2] != ':' || TS[5] != ':' ||
		Hour >= 24 || Mins >= 60 || Secs >= 60)
	{
		return -1;
	}

	return Hour * 3600 + Mins * 60 + Secs;
}


void SecsToTime(int Secs, String Time)
/*   ----------  */
{
	int Hour, Mins;

	Hour = Secs / 3600;
	Mins = (Secs % 3600) / 60;
	Secs = Secs % 60;

	sprintf(Time, "%d%d:%d%d:%d%d",
		Hour / 10, Hour % 10,
		Mins / 10, Mins % 10,
		Secs / 10, Secs % 10);
}


void SetTSBase(int y)
/*   ---------  */
{
	y -= 15;
	TSBase = y * 365 + y / 4 - y / 100 + y / 400 + (367 * 4) / 12 + 1 - 30;
}


int TStampToMins(String TS)
/*  ------------  */
{
	int		Day, Sec, i;

	/*  Check for reasonable length and space between date and time  */

	if (strlen(TS) < 19 || !Space(TS[10])) return (1 << 30);

	/*  Read date part  */

	TS[10] = '\00';
	Day = DateToDay(TS);
	TS[10] = ' ';

	/*  Skip one or more spaces  */

	for (i = 11; TS[i] && Space(TS[i]); i++)
		;

	/*  Read time part  */

	Sec = TimeToSecs(TS + i);

	/*  Return a long time in the future if there is an error  */

	return (Day < 1 || Sec < 0 ? (1 << 30) :
		(Day - TSBase) * 1440 + (Sec + 30) / 60);
}



/*************************************************************************/
/*									 */
/*	Convert a continuous value to a string.		DS must be	 */
/*	large enough to hold any value (e.g. a date, time, ...)		 */
/*									 */
/*************************************************************************/


void CValToStr(ContValue CV, Attribute Att, String DS)
/*   ---------  */
{
	int	Mins;

	if (TStampVal(Att))
	{
		DayToDate(floor(CV / 1440) + TSBase, DS);
		DS[10] = ' ';
		Mins = rint(CV) - floor(CV / 1440) * 1440;
		SecsToTime(Mins * 60, DS + 11);
	}
	else
		if (DateVal(Att))
		{
			DayToDate(CV, DS);
		}
		else
			if (TimeVal(Att))
			{
				SecsToTime(CV, DS);
			}
			else
			{
				sprintf(DS, "%.*g", PREC, CV);
			}
}



#ifdef WIN32
double posrint(double v)
/*     -------  */
{
	double	IntPart, Frac;

	Frac = modf(v, &IntPart);

	if (Frac < 0.5) return IntPart;
	else
		if (Frac > 0.5) return IntPart + 1;
		else
			return (fmod(IntPart, 2.0) >= 1 ? (IntPart + 1) : IntPart);
}

double rint(double v)
/*     ----  */
{
	return (v >= 0 ? posrint(v) : -posrint(-v));
}
#endif
