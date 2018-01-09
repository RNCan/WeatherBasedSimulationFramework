/*************************************************************************/
/*																		 */
/*	Source code for use with See5/C5.0 Release 2.07						 */
/*	-----------------------------------------------						 */
/*		      Copyright RuleQuest Research 2009							 */
/*																		 */
/*	This code is provided "as is" without warranty of any kind,			 */
/*	either express or implied.  All use is at your own risk.			 */
/*																		 */
/*************************************************************************/
#include "stdafx.h"
#include <float.h>
#include <assert.h>
#include <iostream>

#include "Basic/OpenMP.h"
#include "Geomatic/See5hooks.h"
#include "Geomatic/See5defns.h"


namespace WBSF
{

	CDecisionTreeBaseEx::CDecisionTreeBaseEx()
	{
		LBp = LineBuffer;
		LastExt = "";
		OptNo = 1;

		Test = Nil;
		NTest = 0;
		TestSpace = 0;
		TestOccur = Nil;
		RuleCondOK = Nil;
		TestUsed = Nil;

		BINARY = false;
		int	Entry = 0;

		memset(PropName, 0, 20 * sizeof(char));
		PropVal = Nil;
		Unquoted = Nil;
		PropValSize = 0;
	}

	/*************************************************************************/
	/*																		 */
	/*	Read a name from file f into string s, setting Delimiter.			 */
	/*																		 */
	/*	- Embedded periods are permitted, but periods followed by space		 */
	/*	  characters act as delimiters.										 */
	/*	- Embedded spaces are permitted, but multiple spaces are			 */
	/*	  replaced by a single space.										 */
	/*	- Any character can be escaped by '\'.								 */
	/*	- The remainder of a line following '|' is ignored.					 */
	/*																		 */
	/*************************************************************************/


	Boolean CDecisionTreeBaseEx::ReadName(FILE *f, String s, int n, char ColonOpt)
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
	/*	  ClassName	-	class names				 */
	/*	  AttName	-	attribute names				 */
	/*	  AttValName	-	attribute value names			 */
	/*	with:								 */
	/*	  MaxAttVal	-	number of values for each attribute	 */
	/*									 */
	/*	Other global variables set are:					 */
	/*	  MaxAtt	-	maximum attribute number		 */
	/*	  MaxClass	-	maximum class number			 */
	/*									 */
	/*	Note:  until the number of attributes is known, the name	 */
	/*	       information is assembled in local arrays			 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::GetNames(FILE *Nf)
		/*   --------  */
	{
		char	Buffer[1000] = "", *EndBuff;
		int		AttCeiling = 100, ClassCeiling = 100;
		Attribute	Att;
		ClassNo	c;

		ErrMsgs = AttExIn = 0;
		LineNo = 0;
		LBp = LineBuffer;
		*LBp = 0;

		MaxClass = ClassAtt = LabelAtt = CWtAtt = 0;

		/*  Get class names from names file.  This entry can be:
		- a list of discrete values separated by commas
		- the name of the discrete attribute to use as the class
		- the name of a continuous attribute followed by a colon and
		a comma-separated list of thresholds used to segment it  */

		ClassName = AllocZero((*this), ClassCeiling, String);
		do
		{
			ReadName(Nf, Buffer, 1000, ':');

			if (++MaxClass >= ClassCeiling)
			{
				ClassCeiling += 100;
				Realloc((*this), ClassName, ClassCeiling, String);
			}
			ClassName[MaxClass] = strdup(Buffer);
		} while (Delimiter == ',');

		if (Delimiter == ':')
		{
			/*  Thresholds for continuous class attribute  */

			ClassThresh = Alloc((*this), ClassCeiling, ContValue);
			MaxClass = 0;

			do
			{
				ReadName(Nf, Buffer, 1000, ':');

				if (++MaxClass >= ClassCeiling)
				{
					ClassCeiling += 100;
					Realloc((*this), ClassThresh, ClassCeiling, ContValue);
				}

				ClassThresh[MaxClass] = (ContValue)strtod(Buffer, &EndBuff);
				if (EndBuff == Buffer || *EndBuff != '\0')
				{
					Error(BADCLASSTHRESH, Buffer, Nil);
				}
				else
					if (MaxClass > 1 &&
						ClassThresh[MaxClass] <= ClassThresh[MaxClass - 1])
					{
						Error(LEQCLASSTHRESH, Buffer, Nil);
					}
			} while (Delimiter == ',');
		}

		/*  Get attribute and attribute value names from names file  */

		AttName = AllocZero((*this), AttCeiling, String);
		MaxAttVal = AllocZero((*this), AttCeiling, DiscrValue);
		AttValName = AllocZero((*this), AttCeiling, String *);
		SpecialStatus = AllocZero((*this), AttCeiling, char);
		AttDef = AllocZero((*this), AttCeiling, Definition);

		MaxAtt = 0;
		while (ReadName(Nf, Buffer, 1000, ':'))
		{
			if (Delimiter != ':' && Delimiter != '=')
			{
				Error(BADATTNAME, Buffer, "");
			}

			/*  Check for attributes included/excluded  */

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
				Realloc((*this), AttName, AttCeiling, String);
				Realloc((*this), MaxAttVal, AttCeiling, DiscrValue);
				Realloc((*this), AttValName, AttCeiling, String *);
				Realloc((*this), SpecialStatus, AttCeiling, char);
				Realloc((*this), AttDef, AttCeiling, Definition);
			}

			AttName[MaxAtt] = strdup(Buffer);
			SpecialStatus[MaxAtt] = Nil;
			AttDef[MaxAtt] = Nil;
			MaxAttVal[MaxAtt] = 0;

			if (Delimiter == '=')
			{
				if (MaxClass == 1 && !strcmp(ClassName[1], AttName[MaxAtt]))
				{
					Error(BADDEF3, Nil, Nil);
				}

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

				if (!Continuous((*this), CWtAtt))
				{
					Error(CWTATTERR, "", "");
				}
			}
		}

		/*  Check whether class is one of the attributes  */

		if (MaxClass == 1 || ClassThresh)
		{
			/*  Class attribute must be present and must be either
				a discrete attribute or a thresholded continuous attribute  */

			ClassAtt = Which(ClassName[1], AttName, 1, MaxAtt);

			if (ClassAtt <= 0 || Exclude((*this), ClassAtt))
			{
				Error(NOTARGET, ClassName[1], "");
			}
			else
				if (ClassThresh &&
					(!Continuous((*this), ClassAtt) ||
					StatBit((*this), ClassAtt, DATEVAL | STIMEVAL | TSTMPVAL)))
				{
					Error(BADCTARGET, ClassName[1], "");
				}
				else
					if (!ClassThresh &&
						(Continuous((*this), ClassAtt) || StatBit((*this), ClassAtt, DISCRETE)))
					{
						Error(BADDTARGET, ClassName[1], "");
					}

			Free(ClassName[1]);

			if (!ClassThresh)
			{
				Free(ClassName);
				MaxClass = MaxAttVal[ClassAtt];
				ClassName = AttValName[ClassAtt];
			}
			else
			{
				/*  Set up class names as segments of continuous target att  */

				MaxClass++;
				Realloc((*this), ClassName, MaxClass + 1, String);

				sprintf(Buffer, "%s <= %g", AttName[ClassAtt], ClassThresh[1]);
				ClassName[1] = strdup(Buffer);

				ForEach(c, 2, MaxClass - 1)
				{
					sprintf(Buffer, "%g < %s <= %g",
						ClassThresh[c - 1], AttName[ClassAtt], ClassThresh[c]);
					ClassName[c] = strdup(Buffer);
				}

				sprintf(Buffer, "%s > %g",
					AttName[ClassAtt], ClassThresh[MaxClass - 1]);
				ClassName[MaxClass] = strdup(Buffer);
			}
		}

		/*  Ignore case weight attribute if it is excluded; otherwise,
		it cannot be used in models  */

		if (CWtAtt)
		{
			if (Skip((*this), CWtAtt))
			{
				CWtAtt = 0;
			}
			else
			{
				SpecialStatus[CWtAtt] |= SKIP;
			}
		}

		ClassName[0] = "?";

		fclose(Nf);

		if (ErrMsgs > 0) Goodbye(1);
	}



	/*************************************************************************/
	/*									 */
	/*	Continuous or discrete attribute				 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::ExplicitAtt(FILE *Nf)
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

								AttValName[MaxAtt] = Alloc((*this), v + 3, String);
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

			AttValName[MaxAtt] = AllocZero((*this), ValCeiling, String);

			/*  Add "N/A" unless this attribute is the class  */

			if (MaxClass > 1 || strcmp(ClassName[1], AttName[MaxAtt]))
			{
				AttValName[MaxAtt][(MaxAttVal[MaxAtt] = 1)] = strdup("N/A");
			}
			else
			{
				MaxAttVal[MaxAtt] = 0;
			}

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
					Realloc((*this), AttValName[MaxAtt], ValCeiling, String);
				}

				AttValName[MaxAtt][MaxAttVal[MaxAtt]] = strdup(Buffer);
			} while (Delimiter == ',');

			/*  Cancel ordered status if <3 real values  */

			if (Ordered((*this), MaxAtt) && MaxAttVal[MaxAtt] <= 3)
			{
				SpecialStatus[MaxAtt] = 0;
			}
		}
	}



	/*************************************************************************/
	/*									 */
	/*	Locate value Val in List[First] to List[Last]			 */
	/*									 */
	/*************************************************************************/


	int CDecisionTreeBaseEx::Which(String Val, const String *List, int First, int Last)
		/*  -----  */
	{
		int	n = First;

		while (n <= Last && strcmp(Val, List[n])) n++;

		return (n <= Last ? n : First - 1);
	}



	/*************************************************************************/
	/*									 */
	/*	Read next char keeping track of line numbers			 */
	/*									 */
	/*************************************************************************/


	int CDecisionTreeBaseEx::InChar(FILE *f)
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



	/*************************************************************************/
	/*									 */
	/*  Read a raw case from file Df.					 */
	/*									 */
	/*  For each attribute, read the attribute value from the file.		 */
	/*  If it is a discrete valued attribute, find the associated no.	 */
	/*  of this attribute value (if the value is unknown this is 0).	 */
	/*									 */
	/*  Returns the array of attribute values.				 */
	/*									 */
	/*************************************************************************/


	//#define Error(a,b,c)	Error(a,b,c)


	DataRec CDecisionTreeBaseEx::GetDataRec(FILE *Df, Boolean Train)
		/*      ----------  */
	{
		Attribute	Att;
		char	Name[1000], *EndName;
		int		Dv;
		DataRec	Dummy, DVec;
		ContValue	Cv;
		Boolean	FirstValue = true;
		//#if defined WIN32  && ! defined _CONSOLE 
		//    extern int	XREF;
		//#endif


		if (ReadName(Df, Name, 1000, '\00'))
		{
			Dummy = AllocZero((*this), MaxAtt + 2, AttValue);
			DVec = &Dummy[1];
			ForEach(Att, 1, MaxAtt)
			{
				if (AttDef[Att])
				{
					DVec[Att] = EvaluateDef(AttDef[Att], DVec);

					if (Continuous((*this), Att))
					{
						CheckValue(DVec, Att);
					}

					if (SomeMiss)
					{
						SomeMiss[Att] |= Unknown(DVec, Att);
						SomeNA[Att] |= NotApplic(DVec, Att);
					}

					continue;
				}

				/*  Get the attribute value if don't already have it  */

				if (!FirstValue && !ReadName(Df, Name, 1000, '\00'))
				{
					Error(HITEOF, AttName[Att], "");
					FreeLastCase(DVec);
					return Nil;
				}
				FirstValue = false;

				if (Exclude((*this), Att))
				{
					//#if defined WIN32 && ! defined _CONSOLE
					//		if ( XREF || Att == LabelAtt )
					//#else
					if (Att == LabelAtt)
						//#endif
					{
						/*  Record the value as a string  */

						SVal(DVec, Att) = StoreIVal(Name);
					}
				}
				else
					if (!strcmp(Name, "?"))
					{
						/*  Set marker to indicate missing value  */

						DVal(DVec, Att) = DT_UNKNOWN;
						if (SomeMiss) SomeMiss[Att] = true;
					}
					else
						if (Att != ClassAtt && !strcmp(Name, "N/A"))
						{
							/*  Set marker to indicate not applicable  */

							DVal(DVec, Att) = DT_NA;
							if (SomeNA) SomeNA[Att] = true;
						}
						else
							if (Discrete((*this), Att))
							{
								/*  Discrete attribute  */

								Dv = Which(Name, AttValName[Att], 1, MaxAttVal[Att]);
								if (!Dv)
								{
									if (StatBit((*this), Att, DISCRETE))
									{
										if (Train)
										{
											/*  Add value to list  */

											if (MaxAttVal[Att] >= (long)AttValName[Att][0])
											{
												Error(TOOMANYVALS, AttName[Att],
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
										Error(BADATTVAL, AttName[Att], Name);
										Dv = DT_UNKNOWN;
									}
								}
								DVal(DVec, Att) = Dv;
							}
							else
							{
								/*  Continuous value  */

								if (TStampVal((*this), Att))
								{
									CVal(DVec, Att) = Cv = (ContValue)TStampToMins(Name);
									if (Cv >= 1E9)	/* long time in future */
									{
										Error(BADTSTMP, AttName[Att], Name);
										DVal(DVec, Att) = DT_UNKNOWN;
									}
								}
								else
									if (DateVal((*this), Att))
									{
										CVal(DVec, Att) = Cv = (ContValue)DateToDay(Name);
										if (Cv < 1)
										{
											Error(BADDATE, AttName[Att], Name);
											DVal(DVec, Att) = DT_UNKNOWN;
										}
									}
									else
										if (TimeVal((*this), Att))
										{
											CVal(DVec, Att) = Cv = (ContValue)TimeToSecs(Name);
											if (Cv < 0)
											{
												Error(BADTIME, AttName[Att], Name);
												DVal(DVec, Att) = DT_UNKNOWN;
											}
										}
										else
										{
											CVal(DVec, Att) = (ContValue)strtod(Name, &EndName);
											if (EndName == Name || *EndName != '\0')
											{
												Error(BADATTVAL, AttName[Att], Name);
												DVal(DVec, Att) = DT_UNKNOWN;
											}
										}

								CheckValue(DVec, Att);
							}
			}

			if (ClassAtt)
			{
				if (Discrete((*this), ClassAtt))
				{
					Class(DVec) = XDVal(DVec, ClassAtt);
				}
				else
					if (Unknown(DVec, ClassAtt) || NotApplic(DVec, ClassAtt))
					{
						Class(DVec) = 0;
					}
					else
					{
						/*  Find appropriate segment using class thresholds  */

						Cv = CVal(DVec, ClassAtt);

						for (Dv = 1; Dv < MaxClass && Cv > ClassThresh[Dv]; Dv++)
							;

						Class(DVec) = Dv;
					}
			}
			else
			{
				if (!ReadName(Df, Name, 1000, '\00'))
				{
					Error(HITEOF, Fn, "");
					FreeLastCase(DVec);
					return Nil;
				}

				Class(DVec) = Dv = Which(Name, ClassName, 1, MaxClass);
			}

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


	int CDecisionTreeBaseEx::StoreIVal(String S)
		/*  ---------  */
	{
		int		StartIx, Length;

		if ((Length = (int)strlen(S) + 1) + IValsOffset > IValsSize)
		{
			if (IgnoredVals)
			{
				Realloc((*this), IgnoredVals, IValsSize += 32768, char);
			}
			else
			{
				IValsSize = 32768;
				IValsOffset = 0;
				IgnoredVals = Alloc((*this), IValsSize, char);
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


	void CDecisionTreeBaseEx::CheckValue(DataRec DVec, Attribute Att)const
		/*   ----------  */
	{
		ContValue	Cv;

		Cv = CVal(DVec, Att);
		if (!_finite(Cv))
		{
			Error(BADNUMBER, AttName[Att], "");

			CVal(DVec, Att) = DT_UNKNOWN;
		}
	}



	/*************************************************************************/
	/*									 */
	/*	Routines to handle implicitly-defined attributes		 */
	/*									 */
	/*************************************************************************/


	//#define FailSyn(Msg)	 {DefSyntaxError(Msg); return false;}
	//#define FailSem(Msg)	 {DefSemanticsError(Fi, Msg, OpCode); return false;}

	//typedef  union  _xstack_elt
	//		 {
	//			DiscrValue  _discr_val;
	//			ContValue   _cont_val;
	//			String      _string_val;
	//		 }
	//	 XStackElt;

	//#define	cval		_cont_val
	//#define	sval		_string_val
	//#define	dval		_discr_val

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


	void CDecisionTreeBaseEx::ImplicitAtt(FILE *Nf)
		/*   -----------  */
	{
#ifdef CUBIST
		_UNK.cval = DT_UNKNOWN;
#else
		_UNK.dval = DT_UNKNOWN;
#endif
		_NA.dval = DT_NA;

		/*  Get definition as a string in Buff  */

		ReadDefinition(Nf);

		PreviousError = false;
		BN = 0;

		/*  Allocate initial stack and attribute definition  */

		TStack = Alloc((*this), TStackSize = 50, EltRec);
		TSN = 0;

		AttDef[MaxAtt] = Alloc((*this), DefSize = 100, DefElt);
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
				AttValName[MaxAtt] = AllocZero((*this), 4, String);
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


	void CDecisionTreeBaseEx::ReadDefinition(FILE *f)
		/*   --------------  */
	{
		Boolean	LastWasPeriod = false;
		char	c;

		Buff = Alloc((*this), BuffSize = 50, char);
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


	void CDecisionTreeBaseEx::Append(char c)
		/*   ------  */
	{
		if (c == ' ' && (!BN || Buff[BN - 1] == ' ')) return;

		if (BN >= BuffSize)
		{
			Realloc((*this), Buff, BuffSize += 50, char);
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


	Boolean CDecisionTreeBaseEx::Expression()
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



	Boolean CDecisionTreeBaseEx::Conjunct()
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



	static const String RelOps[] = { ">=", "<=", "!=", "<>", ">", "<", "=", (String)0 };

	Boolean CDecisionTreeBaseEx::SExpression()
		/*      -----------  */
	{
		int		o, Fi = BN;

		if (!AExpression()) FailSyn("expression");

		if ((o = FindOne(RelOps)) >= 0)
		{
			BN += (int)strlen(RelOps[o]);

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



	static const String AddOps[] = { "+", "-", (String)0 };

	Boolean CDecisionTreeBaseEx::AExpression()
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



	static const String MultOps[] = { "*", "/", "%", (String)0 };

	Boolean CDecisionTreeBaseEx::Term()
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



	Boolean CDecisionTreeBaseEx::Factor()
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



	Boolean CDecisionTreeBaseEx::Primary()
		/*      -------  */
	{
		if (Atom())
		{
			return true;
		}
		else
		{
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
	}



	static const String Funcs[] = { "sin", "cos", "tan", "log", "exp", "int", (String)0 };

	Boolean CDecisionTreeBaseEx::Atom()
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
				BN += (int)strlen(AttName[Att]);

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
						if ((F = (ContValue)DateToDay(Date)) == 0)
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
							if ((F = (ContValue)TimeToSecs(Time)) == 0)
							{
								Error(BADDEF1, Time, "time");
							}

							BN += 8;
						}
						else
						{
							F = (ContValue)strtod(Buff + BN, &EndPtr);

							/*  Check for period after integer  */

							if (EndPtr > Buff + BN + 1 && *(EndPtr - 1) == '.')
							{
								EndPtr--;
							}

							BN = int(EndPtr - Buff);
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


	Boolean CDecisionTreeBaseEx::Find(String S)
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


	int CDecisionTreeBaseEx::FindOne(const String *Alt)
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


	Attribute CDecisionTreeBaseEx::FindAttName()
		/*        -----------  */
	{
		Attribute	Att, LongestAtt = 0;

		ForEach(Att, 1, MaxAtt - 1)
		{
			if (!Exclude((*this), Att) && Find(AttName[Att]))
			{
				if (!LongestAtt ||
					strlen(AttName[Att]) > strlen(AttName[LongestAtt]))
				{
					LongestAtt = Att;
				}
			}
		}

		if (LongestAtt && (MaxClass == 1 || ClassThresh) &&
			!strcmp(ClassName[1], AttName[LongestAtt]))
		{
			Error(BADDEF4, Nil, Nil);
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


	void CDecisionTreeBaseEx::DefSyntaxError(String Msg)
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

				for ( ; (RestOfText[i] & 0x80) ; i++)
					;
#endif
				RestOfText[i] = RestOfText[i + 1] = '.';
			}

			Error(BADDEF1, RestOfText, Msg);
			PreviousError = true;
		}
	}



	void CDecisionTreeBaseEx::DefSemanticsError(int Fi, String Msg, int OpCode)
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



	void CDecisionTreeBaseEx::Dump(char OpCode, ContValue F, String S, int Fi)
		/*   ----  */
	{
		if (Buff[Fi] == ' ') Fi++;

		if (!UpdateTStack(OpCode, F, S, Fi)) return;

		/*  Make sure enough room for this element  */

		if (DN >= DefSize - 1)
		{
			Realloc((*this), AttDef[MaxAtt], DefSize += 100, DefElt);
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



	void CDecisionTreeBaseEx::DumpOp(char OpCode, int Fi)
		/*   ------  */
	{
		Dump(OpCode, 0, Nil, Fi);
	}



	Boolean CDecisionTreeBaseEx::UpdateTStack(char OpCode, ContValue F, String S, int Fi)
		/*      ------------  */
	{
		if (TSN >= TStackSize)
		{
			Realloc((*this), TStack, TStackSize += 50, EltRec);
		}

		switch (OpCode)
		{
		case OP_ATT:
			TStack[TSN].Type = (Continuous((*this), (long)S) ? 'N' : 'S');
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

#define	CUnknownVal(AV)		(AV.cval==_UNK.cval)
#define	DUnknownVal(AV)		(AV.dval==_UNK.dval)
#define DUNA(a)	(DUnknownVal(XStack[a]) || NotApplicVal(XStack[a]))
#define CUNA(a)	(CUnknownVal(XStack[a]) || NotApplicVal(XStack[a]))
#define	C1(x)	(CUNA(XSN-1) ? _UNK.cval : (x))
#define	C2(x)	(CUNA(XSN-1) || CUNA(XSN-2) ? _UNK.cval : (x))
#define	CD2(x)	(CUNA(XSN-1) || CUNA(XSN-2) ? _UNK.dval : (x))
#define	D2(x)	(DUNA(XSN-1) || DUNA(XSN-2) ? _UNK.dval : (x))


	AttValue CDecisionTreeBaseEx::EvaluateDef(Definition D, DataRec Case)const
		/*       -----------  */
	{
		XStackElt	XStack[100];			/* allows 100-level nesting  */
		int		XSN = 0, DN, bv1, bv2, Mult;
		double	cv1, cv2;
		String	sv1, sv2;
		Attribute	Att;
		DefElt	DElt;
		AttValue	ReturnVal;

		for (DN = 0;; DN++)
		{
			switch (DefOp((DElt = D[DN])))
			{
			case OP_ATT:
				Att = (long)DefSVal(DElt);

				//By RSA 13/02/2012
				//#if defined PREDICT || defined SEE5 && defined WIN32 && ! defined _CONSOLE
				//		    GetValue(Att);
				//#endif
				if (Continuous((*this), Att))
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
				XStack[XSN - 2].cval = (ContValue)C2(cv1 + cv2);
				XSN--;
				break;

			case OP_MINUS:
				cv1 = XStack[XSN - 2].cval;
				cv2 = XStack[XSN - 1].cval;
				XStack[XSN - 2].cval = (ContValue)C2(cv1 - cv2);
				XSN--;
				break;

			case OP_MULT:
				cv1 = XStack[XSN - 2].cval;
				cv2 = XStack[XSN - 1].cval;
				XStack[XSN - 2].cval = (ContValue)C2(cv1 * cv2);
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
					Mult = Denominator((ContValue)cv1);
					cv1 = cv1 / cv2;
					while (fabs(cv2) > 1)
					{
						Mult *= 10;
						cv2 /= 10;
					}
					XStack[XSN - 2].cval = (ContValue)(rint(cv1 * Mult) / Mult);
				}
				XSN--;
				break;

			case OP_MOD:
				cv1 = XStack[XSN - 2].cval;
				cv2 = XStack[XSN - 1].cval;
				XStack[XSN - 2].cval = (ContValue)C2(fmod(cv1, cv2));
				XSN--;
				break;

			case OP_POW:
				cv1 = XStack[XSN - 2].cval;
				cv2 = XStack[XSN - 1].cval;
				XStack[XSN - 2].cval =
					(CUNA(XSN - 1) || CUNA(XSN - 2) ||
					(cv1 < 0 && ceil(cv2) != cv2) ? _UNK.cval :
					(ContValue)pow(cv1, cv2));
				XSN--;
				break;

			case OP_UMINUS:
				cv1 = XStack[XSN - 1].cval;
				XStack[XSN - 1].cval = (ContValue)C1(-cv1);
				break;

			case OP_SIN:
				cv1 = XStack[XSN - 1].cval;
				XStack[XSN - 1].cval = (ContValue)C1(sin(cv1));
				break;

			case OP_COS:
				cv1 = XStack[XSN - 1].cval;
				XStack[XSN - 1].cval = (ContValue)C1(cos(cv1));
				break;

			case OP_TAN:
				cv1 = XStack[XSN - 1].cval;
				XStack[XSN - 1].cval = (ContValue)C1(tan(cv1));
				break;

			case OP_LOG:
				cv1 = XStack[XSN - 1].cval;
				XStack[XSN - 1].cval =
					(CUNA(XSN - 1) || cv1 <= 0 ? _UNK.cval : (ContValue)log(cv1));
				break;

			case OP_EXP:
				cv1 = XStack[XSN - 1].cval;
				XStack[XSN - 1].cval = (ContValue)C1(exp(cv1));
				break;

			case OP_INT:
				cv1 = XStack[XSN - 1].cval;
				XStack[XSN - 1].cval = (ContValue)C1(rint(cv1));
				break;

			case OP_END:
				ReturnVal.cval = XStack[0].cval;	/* cval >= dval bytes */
				return ReturnVal;
			}
		}
	}



	/*************************************************************************/
	/*									 */
	/*	Routines for reading model files				 */
	/*	--------------------------------				 */
	/*									 */
	/*************************************************************************/



	static const String	Prop[] = { "null",
		"att",
		"class",
		"cut",
		"conds",
		"elts",
		"entries",
		"forks",
		"freq",
		"id",
		"type",
		"low",
		"mid",
		"high",
		"result",
		"rules",
		"val",
		"lift",
		"cover",
		"ok",
		"default",
		"costs",
		"sample",
		"init"
	};


#define	PROPS 23

#define	ERRORP		0
#define ATTP		1
#define CLASSP		2
#define CUTP		3
#define	CONDSP		4
#define ELTSP		5
#define ENTRIESP	6
#define FORKSP		7
#define FREQP		8
#define IDP		9
#define TYPEP		10
#define LOWP		11
#define MIDP		12
#define HIGHP		13
#define RESULTP		14
#define RULESP		15
#define VALP		16
#define LIFTP		17
#define COVERP		18
#define OKP		19
#define DEFAULTP	20
#define COSTSP		21



	/*************************************************************************/
	/*									 */
	/*	Read header information and decide whether model files are	 */
	/*	in ASCII or binary format					 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::ReadFilePrefix(String Extension)
		/*   --------------  */
	{
#if defined WIN32 || defined _CONSOLE
		if (!(TRf = GetFile(Extension, "rb"))) Error(NOFILE, Fn, "");
#else
		if ( ! (TRf = GetFile(Extension, "r")) ) Error(NOFILE, Fn, "");
#endif

		StreamIn((char *)&TRIALS, sizeof(int));
		if (memcmp((char *)&TRIALS, "id=", 3) != 0)
		{
			BINARY = true;
			BinRecoverDiscreteNames();
		}
		else
		{
			BINARY = false;
			rewind(TRf);
			ReadHeader();
		}
	}



	/*************************************************************************/
	/*								  	 */
	/*	Read the header information (id, saved names, models)		 */
	/*								  	 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::ReadHeader()
		/*   ---------  */
	{
		Attribute	Att;
		DiscrValue	v;
		char	*p, Dummy;
		int		Year, Month, Day;
		FILE	*F;

		while (true)
		{
			switch (ReadProp(&Dummy))
			{
			case ERRORP:
				return;

			case IDP:
				/*  Recover year run and set base date for timestamps  */

				if (sscanf(PropVal + strlen(PropVal) - 11,
					"%d-%d-%d\"", &Year, &Month, &Day) == 3)
				{
					SetTSBase(Year);
				}
				break;

			case COSTSP:
				/*  Recover costs file used to generate model  */

				if ((F = GetFile(".costs", "r")))
				{
					GetMCosts(F);
				}
				break;

			case ATTP:
				Unquoted = RemoveQuotes(PropVal);
				Att = Which(Unquoted, AttName, 1, MaxAtt);
				if (!Att || Exclude((*this), Att))
				{
					Error(MODELFILE, E_MFATT, Unquoted);
				}
				break;

			case ELTSP:
				MaxAttVal[Att] = 1;
				AttValName[Att][1] = strdup("N/A");

				for (p = PropVal; *p;)
				{
					p = RemoveQuotes(p);
					v = ++MaxAttVal[Att];
					AttValName[Att][v] = strdup(p);

					for (p += strlen(p); *p != '"'; p++)
						;
					p++;
					if (*p == ',') p++;
				}
				AttValName[Att][MaxAttVal[Att] + 1] = "<other>";
				break;

			case ENTRIESP:
				sscanf(PropVal, "\"%d\"", &TRIALS);
				Entry = 0;
				return;
			}
		}
	}



	/*************************************************************************/
	/*									 */
	/*	Retrieve decision tree with extension Extension			 */
	/*									 */
	/*************************************************************************/


	Tree CDecisionTreeBaseEx::GetTree(String Extension)
		/*   -------  */
	{
		CheckFile(Extension, false);

		return (BINARY ? BinInTree() : InTree());
	}



	Tree CDecisionTreeBaseEx::InTree()
		/*   ------  */
	{
		Tree	T;
		DiscrValue	v, Subset = 0;
		char	Delim, *p;
		ClassNo	c;
		int		X;
		double	XD;

		T = (Tree)AllocZero((*this), 1, TreeRec);

		do
		{
			switch (ReadProp(&Delim))
			{
			case ERRORP:
				return Nil;

			case TYPEP:
				sscanf(PropVal, "\"%d\"", &X); T->NodeType = X;
				break;

			case CLASSP:
				Unquoted = RemoveQuotes(PropVal);
				T->Leaf = Which(Unquoted, ClassName, 1, MaxClass);
				if (!T->Leaf) Error(MODELFILE, E_MFCLASS, Unquoted);
				break;

			case ATTP:
				Unquoted = RemoveQuotes(PropVal);
				T->Tested = Which(Unquoted, AttName, 1, MaxAtt);
				if (!T->Tested || Exclude((*this), T->Tested))
				{
					Error(MODELFILE, E_MFATT, Unquoted);
				}
				break;

			case CUTP:
				sscanf(PropVal, "\"%lf\"", &XD);	T->Cut = (ContValue)XD;
				T->Lower = T->Mid = T->Upper = T->Cut;
				break;

			case LOWP:
				sscanf(PropVal, "\"%lf\"", &XD);	T->Lower = (ContValue)XD;
				break;

			case MIDP:
				sscanf(PropVal, "\"%lf\"", &XD);	T->Mid = (ContValue)XD;
				break;

			case HIGHP:
				sscanf(PropVal, "\"%lf\"", &XD);	T->Upper = (ContValue)XD;
				break;

			case FORKSP:
				sscanf(PropVal, "\"%d\"", &T->Forks);
				break;

			case FREQP:
				T->ClassDist = Alloc((*this), MaxClass + 1, CaseCount);
				p = PropVal + 1;

				ForEach(c, 1, MaxClass)
				{
					T->ClassDist[c] = (CaseCount)strtod(p, &p);
					T->Cases += T->ClassDist[c];
					p++;
				}
				break;

			case ELTSP:
				if (!Subset++)
				{
					T->Subset = AllocZero((*this), T->Forks + 1, Set);
				}

				T->Subset[Subset] = MakeSubset(T->Tested);
				break;
			}
		} while (Delim == ' ');

		if (T->ClassDist)
		{
			T->Errors = T->Cases - T->ClassDist[T->Leaf];
		}
		else
		{
			T->ClassDist = Alloc((*this), 1, CaseCount);
		}

		if (T->NodeType)
		{
			T->Branch = AllocZero((*this), T->Forks + 1, Tree);
			ForEach(v, 1, T->Forks)
			{
				T->Branch[v] = InTree();
			}
		}

		return T;
	}



	/*************************************************************************/
	/*									 */
	/*	Retrieve ruleset with extension Extension			 */
	/*	(Separate functions for ruleset, single rule, single condition)	 */
	/*									 */
	/*************************************************************************/


	CRuleSet CDecisionTreeBaseEx::GetRules(String Extension)
		/*	 --------  */
	{
		CheckFile(Extension, false);

		return (BINARY ? BinInRules() : InRules());
	}



	CRuleSet CDecisionTreeBaseEx::InRules()
		/*	 -------  */
	{
		CRuleSet	RS;
		RuleNo	r;
		char	Delim;

		RS = Alloc((*this), 1, RuleSetRec);

		do
		{
			switch (ReadProp(&Delim))
			{
			case ERRORP:
				return Nil;

			case RULESP:
				sscanf(PropVal, "\"%d\"", &RS->SNRules);
				CheckActiveSpace(RS->SNRules);
				break;

			case DEFAULTP:
				Unquoted = RemoveQuotes(PropVal);
				RS->SDefault = Which(Unquoted, ClassName, 1, MaxClass);
				if (!RS->SDefault) Error(MODELFILE, E_MFCLASS, Unquoted);
				break;
			}
		} while (Delim == ' ');

		/*  Read each rule  */

		RS->SRule = Alloc((*this), RS->SNRules + 1, CRule);
		ForEach(r, 1, RS->SNRules)
		{
			if ((RS->SRule[r] = InRule()))
			{
				RS->SRule[r]->RNo = r;
				RS->SRule[r]->TNo = Entry;
			}
		}
		ConstructRuleTree(RS);
		Entry++;
		return RS;
	}



	CRule CDecisionTreeBaseEx::InRule()
		/*    ------  */
	{
		CRule	R;
		int		d;
		char	Delim;
		float	Lift;

		R = Alloc((*this), 1, RuleRec);

		do
		{
			switch (ReadProp(&Delim))
			{
			case ERRORP:
				return Nil;

			case CONDSP:
				sscanf(PropVal, "\"%d\"", &R->Size);
				break;

			case COVERP:
				sscanf(PropVal, "\"%f\"", &R->Cover);
				break;

			case OKP:
				sscanf(PropVal, "\"%f\"", &R->Correct);
				break;

			case LIFTP:
				sscanf(PropVal, "\"%f\"", &Lift);
				R->Prior = (R->Correct + 1) / ((R->Cover + 2) * Lift);
				break;

			case CLASSP:
				Unquoted = RemoveQuotes(PropVal);
				R->Rhs = Which(Unquoted, ClassName, 1, MaxClass);
				if (!R->Rhs) Error(MODELFILE, E_MFCLASS, Unquoted);
				break;
			}
		} while (Delim == ' ');

		R->Lhs = Alloc((*this), R->Size + 1, Condition);
		ForEach(d, 1, R->Size)
		{
			R->Lhs[d] = InCondition();
		}

		R->Vote = int(1000 * (R->Correct + 1.0) / (R->Cover + 2.0) + 0.5);

		return R;
	}



	Condition CDecisionTreeBaseEx::InCondition()
		/*        -----------  */
	{
		Condition	C;
		char	Delim;
		int		X;
		double	XD;

		C = Alloc((*this), 1, CondRec);

		do
		{
			switch (ReadProp(&Delim))
			{
			case ERRORP:
				return Nil;

			case TYPEP:
				sscanf(PropVal, "\"%d\"", &X); C->NodeType = X;
				break;

			case ATTP:
				Unquoted = RemoveQuotes(PropVal);
				C->Tested = Which(Unquoted, AttName, 1, MaxAtt);
				if (!C->Tested || Exclude((*this), C->Tested))
				{
					Error(MODELFILE, E_MFATT, Unquoted);
				}
				break;

			case CUTP:
				sscanf(PropVal, "\"%lf\"", &XD);	C->Cut = (ContValue)XD;
				break;

			case RESULTP:
				C->TestValue = (PropVal[1] == '<' ? 2 : 3);
				break;

			case VALP:
				if (Continuous((*this), C->Tested))
				{
					C->TestValue = 1;
				}
				else
				{
					Unquoted = RemoveQuotes(PropVal);
					C->TestValue = Which(Unquoted,
						AttValName[C->Tested],
						1, MaxAttVal[C->Tested]);
					if (!C->TestValue) Error(MODELFILE, E_MFATTVAL, Unquoted);
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




	void CDecisionTreeBaseEx::ConstructRuleTree(CRuleSet RS)
		/*   -----------------  */
	{
		int		r, c;
		RuleNo	*All;

		Test = Alloc((*this), (TestSpace = 1000), Condition);
		NTest = 0;

		All = Alloc((*this), RS->SNRules, RuleNo);
		ForEach(r, 1, RS->SNRules)
		{
			All[r - 1] = r;

			ForEach(c, 1, RS->SRule[r]->Size)
			{
				SetTestIndex(RS->SRule[r]->Lhs[c]);
			}
		}

		TestOccur = Alloc((*this), NTest, int);
		TestUsed = AllocZero((*this), NTest, Boolean);

		RuleCondOK = AllocZero((*this), RS->SNRules + 1, int);

		RS->RT = GrowRT(All, RS->SNRules, RS->SRule);

		Free(All);
		Free(Test);
		Free(TestUsed);
		Free(TestOccur);
		Free(RuleCondOK);
	}



	void CDecisionTreeBaseEx::SetTestIndex(Condition C)
		/*   ------------  */
	{
		int		t;
		Condition	CC;
		Attribute	Att;

		Att = C->Tested;

		ForEach(t, 0, NTest - 1)
		{
			CC = Test[t];
			if (CC->Tested != Att || CC->NodeType != C->NodeType) continue;

			switch (C->NodeType)
			{
			case BrDiscr:
				C->TestI = t;
				return;

			case BrSubset:
				if (!memcmp(C->Subset, CC->Subset, (MaxAttVal[Att] >> 3) + 1))
				{
					C->TestI = t;
					return;
				}
				break;

			case BrThresh:
				if (C->TestValue == 1 && CC->TestValue == 1 ||
					(C->TestValue != 1 && CC->TestValue != 1 &&
					C->Cut == CC->Cut))
				{
					C->TestI = t;
					return;
				}
				break;
			}
		}

		/*  New test -- make sure have enough space  */

		if (NTest >= TestSpace)
		{
			Realloc((*this), Test, (TestSpace += 1000), Condition);
		}

		Test[NTest] = C;
		C->TestI = NTest++;
	}



	RuleTree CDecisionTreeBaseEx::GrowRT(RuleNo *RR, int RRN, CRule *Rule)
		/*       ------  */
	{
		RuleTree	Node;
		RuleNo	r, *LR;
		int		FP = 0, ri, TI, *Expect, LRN;
		DiscrValue	v;

		if (!RRN) return Nil;

		Node = AllocZero((*this), 1, RuleTreeRec);

		/*  Record and swap to front any rules that are satisfied  */

		ForEach(ri, 0, RRN - 1)
		{
			r = RR[ri];

			if (RuleCondOK[r] == Rule[r]->Size)
			{
				RR[ri] = RR[FP];
				RR[FP] = r;
				FP++;
			}
		}

		if (FP)
		{
			Node->Fire = Alloc((*this), FP + 1, RuleNo);
			memcpy(Node->Fire, RR, FP * sizeof(RuleNo));
			Node->Fire[FP] = 0;
			RR += FP;
			RRN -= FP;
		}
		else
		{
			Node->Fire = Nil;
		}

		if (!RRN) return Node;

		/*  Choose test for this node  */

		TI = SelectTest(RR, RRN, Rule);
		TestUsed[TI] = true;

		Node->CondTest = Test[TI];

		/*  Find the desired outcome for each rule  */

		Expect = Alloc((*this), RRN, int);
		ForEach(ri, 0, RRN - 1)
		{
			Expect[ri] = DesiredOutcome(Rule[RR[ri]], TI);
		}

		/*  Now construct individual branches.  Rules that do not reference
		the selected test go down branch 0; at classification time,
		any case with an unknown outcome for the selected test also
		goes to branch 0.  */

		Node->Forks =
			(Test[TI]->NodeType == BrDiscr ? MaxAttVal[Test[TI]->Tested] :
			Test[TI]->NodeType == BrSubset ? 1 : 3);

		Node->Branch = Alloc((*this), Node->Forks + 1, RuleTree);

		LR = Alloc((*this), RRN, RuleNo);
		ForEach(v, 0, Node->Forks)
		{
			/*  Extract rules with outcome v and increment conditions satisfied,
				if relevant  */

			LRN = 0;
			ForEach(ri, 0, RRN - 1)
			{
				if (abs(Expect[ri]) == v)
				{
					LR[LRN++] = RR[ri];

					if (Expect[ri] > 0) RuleCondOK[RR[ri]]++;
				}
			}

			/*  LR now contains rules with outcome v  */

			Node->Branch[v] = GrowRT(LR, LRN, Rule);

			if (v)
			{
				/*  Restore conditions satisfied  */

				ForEach(ri, 0, LRN - 1)
				{
					RuleCondOK[LR[ri]]--;
				}
			}
		}

		TestUsed[TI] = false;

		/*  Free local storage  */

		Free(LR);
		Free(Expect);

		return Node;
	}



	int CDecisionTreeBaseEx::DesiredOutcome(CRule R, int TI)
		/*  --------------  */
	{
		int		c;
		Boolean	ContinTest;

		ContinTest = Continuous((*this), Test[TI]->Tested);	/* test of continuous att */

		ForEach(c, 1, R->Size)
		{
			if (R->Lhs[c]->TestI == TI)
			{
				return R->Lhs[c]->TestValue;
			}

			/*  If this test references the same continuous attribute but
				with a different threshold, may be able to exploit outcome:
				-2 means "rule can only be matched down branch 2"
				-3 means "rule can only be matched down branch 3"  */

			if (ContinTest && Test[TI]->Tested == R->Lhs[c]->Tested)
			{
				switch (R->Lhs[c]->TestValue)
				{
				case 1:
					return 1;

				case 2:
					if (R->Lhs[c]->Cut < Test[TI]->Cut) return -2;
					break;

				case 3:
					if (R->Lhs[c]->Cut > Test[TI]->Cut) return -3;
				}
			}
		}

		return 0;
	}



	int CDecisionTreeBaseEx::SelectTest(RuleNo *RR, int RRN, CRule *Rule)
		/*  ----------  */
	{
		int		c, cc, ri;
		RuleNo	r;

		/*  Count test occurrences  */

		ForEach(c, 0, NTest - 1)
		{
			TestOccur[c] = 0;
		}

		ForEach(ri, 0, RRN - 1)
		{
			r = RR[ri];

			ForEach(c, 1, Rule[r]->Size)
			{
				TestOccur[Rule[r]->Lhs[c]->TestI]++;
			}
		}

		/*  Find most frequently-occurring test  */

		cc = -1;
		ForEach(c, 0, NTest - 1)
		{
			if (!TestUsed[c] && (cc < 0 || TestOccur[c] > TestOccur[cc]))
			{
				cc = c;
			}
		}

		return cc;
	}



	/*************************************************************************/
	/*									 */
	/*	ASCII reading utilities						 */
	/*									 */
	/*************************************************************************/


	int CDecisionTreeBaseEx::ReadProp(char *Delim)
		/*  --------  */
	{
		int		c, i;
		char	*p;
		Boolean	Quote = false;

		for (p = PropName; (c = fgetc(TRf)) != '=';)
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

		for (p = PropVal; ((c = fgetc(TRf)) != ' ' && c != '\n') || Quote;)
		{
			if (c == EOF)
			{
				Error(MODELFILE, E_MFEOF, "");
				PropName[0] = PropVal[0] = '\00';
				return 0;
			}

			if ((i = (int)(p - PropVal)) >= PropValSize)
			{
				Realloc((*this), PropVal, (PropValSize += 10000) + 3, char);
				p = PropVal + i;
			}

			*p++ = c;
			if (c == '\\')
			{
				*p++ = fgetc(TRf);
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


	String CDecisionTreeBaseEx::RemoveQuotes(String S)
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



	Set CDecisionTreeBaseEx::MakeSubset(Attribute Att)
		/*  ----------  */
	{
		int		Bytes, b;
		char	*p;
		Set		S;

		Bytes = (MaxAttVal[Att] >> 3) + 1;
		S = AllocZero((*this), Bytes, Byte);

		for (p = PropVal; *p;)
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



	/*************************************************************************/
	/*									 */
	/*	Recover attribute values read with "discrete N"			 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::BinRecoverDiscreteNames()
		/*   -----------------------  */
	{
		Attribute	Att;
		DiscrValue	v;
		int		Length;

		ForEach(Att, 1, MaxAtt)
		{
			if (!StatBit((*this), Att, DISCRETE)) continue;

			StreamIn((char *)&MaxAttVal[Att], sizeof(int));

			/*  Insert "N/A"  */

			AttValName[Att][1] = strdup("N/A");
			MaxAttVal[Att]++;

			ForEach(v, 2, MaxAttVal[Att])
			{
				StreamIn((char *)&Length, sizeof(int));

				AttValName[Att][v] = Alloc((*this), Length, char);
				StreamIn(AttValName[Att][v], Length);
			}

			/*  Invisible name for undefined values  */

			AttValName[Att][MaxAttVal[Att] + 1] = "<other>";
		}
	}



	/*************************************************************************/
	/*									 */
	/*	Retrieve tree from saved characters				 */
	/*									 */
	/*************************************************************************/


	Tree CDecisionTreeBaseEx::BinInTree()
		/*   ---------  */
	{
		Tree	T;
		DiscrValue	v, vv;
		int		Bytes;
		float	XFl;
		Set		S;

		T = (Tree)AllocZero((*this), 1, TreeRec);

		StreamIn((char *)&T->NodeType, sizeof(BranchType));
		StreamIn((char *)&T->Leaf, sizeof(ClassNo));
		StreamIn((char *)&T->Cases, sizeof(CaseCount));
		StreamIn((char *)&T->Errors, sizeof(CaseCount));

		T->ClassDist = AllocZero((*this), MaxClass + 1, CaseCount);
		StreamIn((char *)T->ClassDist, (MaxClass + 1) * sizeof(CaseCount));

		if (T->NodeType)
		{
			StreamIn((char *)&T->Tested, sizeof(Attribute));
			StreamIn((char *)&T->Forks, sizeof(int));
			T->Forks++;	/* for N/A */

			switch (T->NodeType)
			{
			case BrDiscr:
				break;

			case BrThresh:
				StreamIn((char *)&XFl, sizeof(float));	T->Cut = XFl;
				StreamIn((char *)&XFl, sizeof(float));	T->Lower = XFl;
				StreamIn((char *)&XFl, sizeof(float));	T->Upper = XFl;
				StreamIn((char *)&XFl, sizeof(float));	T->Mid = XFl;
				break;

			case BrSubset:
				T->Subset = (Set *)AllocZero((*this), T->Forks + 1, Set);

				Bytes = ((MaxAttVal[T->Tested] - 1) >> 3) + 1;
				S = AllocZero((*this), Bytes, Byte);
				T->Subset[1] = AllocZero((*this), Bytes, Byte);
				SetBit(1, T->Subset[1]);

				ForEach(v, 2, T->Forks)
				{
					T->Subset[v] = AllocZero((*this), Bytes, Byte);
					StreamIn((char *)S, Bytes);
					ForEach(vv, 1, MaxAttVal[T->Tested] - 1)
					{
						if (In(vv, S)) SetBit(vv + 1, T->Subset[v]);
					}
				}

				Free(S);
			}

			T->Branch = AllocZero((*this), T->Forks + 1, Tree);

			/*  Allow for N/A branch  */

			T->Branch[1] = Leaf(Nil, T->Leaf, 0.0, 0.0);

			ForEach(v, 2, T->Forks)
			{
				T->Branch[v] = BinInTree();
			}
		}

		return T;
	}



	/*************************************************************************/
	/*								  	 */
	/*	Recover a ruleset						 */
	/*								  	 */
	/*************************************************************************/


	CRuleSet CDecisionTreeBaseEx::BinInRules()
		/*       ----------  */
	{
		int		ri, d, Bytes, Dummy;
		CRuleSet	RS;
		CRule	R;
		Condition	C;
		float	XFl;
		Set		S;
		DiscrValue	vv;

		RS = Alloc((*this), 1, RuleSetRec);

		StreamIn((char *)&RS->SNRules, sizeof(RuleNo));
		StreamIn((char *)&RS->SDefault, sizeof(ClassNo));

		RS->SRule = Alloc((*this), RS->SNRules + 1, CRule);

		ForEach(ri, 1, RS->SNRules)
		{
			R = RS->SRule[ri] = Alloc((*this), 1, RuleRec);

			StreamIn((char *)&R->RNo, sizeof(int));
			StreamIn((char *)&R->TNo, sizeof(int));
			StreamIn((char *)&R->Size, sizeof(int));

			R->Lhs = Alloc((*this), R->Size + 1, Condition);
			ForEach(d, 1, R->Size)
			{
				C = R->Lhs[d] = Alloc((*this), 1, CondRec);

				StreamIn((char *)&C->NodeType, sizeof(BranchType));
				StreamIn((char *)&C->Tested, sizeof(Attribute));
				StreamIn((char *)&Dummy, sizeof(int));
				StreamIn((char *)&XFl, sizeof(float));	C->Cut = XFl;
				if (C->NodeType == BrSubset)
				{
					Bytes = ((MaxAttVal[C->Tested] - 1) >> 3) + 1;
					S = AllocZero((*this), Bytes, Byte);

					C->Subset = AllocZero((*this), Bytes, Byte);
					StreamIn((char *)S, Bytes);
					ForEach(vv, 1, MaxAttVal[C->Tested] - 1)
					{
						if (In(vv, S)) SetBit(vv + 1, C->Subset);
					}

					Free(S);
				}
				StreamIn((char *)&R->Lhs[d]->TestValue, sizeof(int));
				R->Lhs[d]->TestValue++;	/* to allow for N/A */
			}
			StreamIn((char *)&R->Rhs, sizeof(ClassNo));
			StreamIn((char *)&R->Cover, sizeof(CaseCount));
			StreamIn((char *)&R->Correct, sizeof(CaseCount));
			StreamIn((char *)&R->Prior, sizeof(float));

			if (R->Correct < 1)
			{
				/*  Prior to Release 1.11  */

				R->Correct = (R->Cover + 2) * (1 - R->Correct) - 1;
				memcpy(&R->Vote, &R->Prior, sizeof(int));
				R->Prior = (float)1E38;
			}
			else
			{
				R->Vote = int(1000 * (R->Correct + 1.0) / (R->Cover + 2.0) + 0.5);
			}
		}

		return RS;
	}



	/*************************************************************************/
	/*								  	 */
	/*	Character stream read for binary routines			 */
	/*								  	 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::StreamIn(String S, int n)
		/*   --------  */
	{
		while (n--) *S++ = getc(TRf);
	}



	/*************************************************************************/
	/*									 */
	/*	Construct a leaf in a given node				 */
	/*									 */
	/*************************************************************************/


	Tree CDecisionTreeBaseEx::Leaf(double *Freq, ClassNo NodeClass, CaseCount Cases, CaseCount Errors)
		/*   ----  */
	{
		Tree	Node;
		ClassNo	c;

		Node = AllocZero((*this), 1, TreeRec);

		Node->ClassDist = AllocZero((*this), MaxClass + 1, CaseCount);
		if (Freq)
		{
			ForEach(c, 1, MaxClass)
			{
				Node->ClassDist[c] = (CaseCount)Freq[c];
			}
		}

		Node->NodeType = 0;
		Node->Leaf = NodeClass;
		Node->Cases = Cases;
		Node->Errors = Errors;

		return Node;
	}


	/*************************************************************************/
	/*									 */
	/*	Read variable misclassification costs				 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::GetMCosts(FILE *Cf)
		/*   ---------  */
	{
		ClassNo	Pred, Real, p, r;
		char	Name[1000];
		float	Val;

		LineNo = 0;

		/*  Read entries from cost file  */

		while (ReadName(Cf, Name, 1000, ':'))
		{
			if (!(Pred = Which(Name, ClassName, 1, MaxClass)))
			{
				Error(BADCOSTCLASS, Name, "");
			}

			if (!ReadName(Cf, Name, 1000, ':') ||
				!(Real = Which(Name, ClassName, 1, MaxClass)))
			{
				Error(BADCOSTCLASS, Name, "");
			}

			if (!ReadName(Cf, Name, 1000, ':') ||
				sscanf(Name, "%f", &Val) != 1 || Val < 0)
			{
				Error(BADCOST, "", "");
				Val = 1;
			}

			if (Pred > 0 && Real > 0 && Pred != Real && Val != 1)
			{
				/*  Have a non-trivial cost entry  */

				if (!MCost)
				{
					/*  Set up cost matrices  */

					MCost = Alloc((*this), MaxClass + 1, float *);
					ForEach(p, 1, MaxClass)
					{
						MCost[p] = Alloc((*this), MaxClass + 1, float);
						ForEach(r, 1, MaxClass)
						{
							MCost[p][r] = (float)(p == r ? 0.0 : 1.0);
						}
					}
				}

				MCost[Pred][Real] = Val;
			}
		}
		fclose(Cf);
	}



	/*************************************************************************/
	/*                                                              	 */
	/*	Categorize a case using a decision tree				 */
	/*                                                              	 */
	/*************************************************************************/


	ClassNo CDecisionTreeBaseEx::TreeClassify(DataRec Case, Tree DecisionTree)
		/*      ------------  */
	{
		ClassNo	c;

		ForEach(c, 0, MaxClass)
		{
			ClassSum[c] = 0;
		}

		FindLeaf(Case, DecisionTree, Nil, 1.0);

		return SelectClass(1, (Boolean)(MCost != Nil));
	}



	/*************************************************************************/
	/*                                                              	 */
	/*	Follow all branches from a node, weighting them in proportion	 */
	/*	to the number of training cases they contain			 */
	/*                                                              	 */
	/*************************************************************************/



	void CDecisionTreeBaseEx::FollowAllBranches(DataRec Case, Tree T, float Fraction)
		/*   -----------------  */
	{
		DiscrValue	v;

		ForEach(v, 1, T->Forks)
		{
			if (T->Branch[v]->Cases > Epsilon)
			{
				FindLeaf(Case, T->Branch[v], T,
					(Fraction * T->Branch[v]->Cases) / T->Cases);
			}
		}
	}



	/*************************************************************************/
	/*                                                              	 */
	/*	Classify a case using the given subtree.			 */
	/*	Adjust the value ClassSum for each class			 */
	/*                                                              	 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::FindLeaf(DataRec Case, Tree T, Tree PT, float Fraction)
		/*   --------  */
	{
		DiscrValue	v, Dv;
		ClassNo	c;
		float	NewFrac, BrWt[4];


		switch (T->NodeType)
		{
		case 0:  /* leaf */

		LeafUpdate :

			/*  Use parent node if effectively no cases at this node  */

			if (T->Cases < Epsilon)
			{
				T = PT;
			}

				   /*  Update from all classes  */

				   ForEach(c, 1, MaxClass)
				   {
					   ClassSum[c] += Fraction * T->ClassDist[c] / T->Cases;
				   }

				   return;

		case BrDiscr:  /* test of discrete attribute */

			Dv = DVal(Case, T->Tested);	/* > MaxAttVal if unknown */

			if (Dv <= T->Forks)	/*  Make sure not new discrete value  */
			{
				FindLeaf(Case, T->Branch[Dv], T, Fraction);
			}
			else
			{
				FollowAllBranches(Case, T, Fraction);
			}

			return;

		case BrThresh:  /* test of continuous attribute */

			if (Unknown(Case, T->Tested))
			{
				FollowAllBranches(Case, T, Fraction);
			}
			else
				if (NotApplic(Case, T->Tested))
				{
					FindLeaf(Case, T->Branch[1], T, Fraction);
				}
				else
				{
					/*  Find weights for <= and > branches, interpolating if
						probabilistic thresholds are used  */

					BrWt[2] = Interpolate(T, CVal(Case, T->Tested));
					BrWt[3] = 1 - BrWt[2];

					ForEach(v, 2, 3)
					{
						if ((NewFrac = Fraction * BrWt[v]) >= 0.01)
						{
							FindLeaf(Case, T->Branch[v], T, NewFrac);
						}
					}
				}

			return;

		case BrSubset:  /* subset test on discrete attribute  */

			Dv = DVal(Case, T->Tested);	/* > MaxAttVal if unknown */

			if (Dv <= MaxAttVal[T->Tested])
			{
				ForEach(v, 1, T->Forks)
				{
					if (In(Dv, T->Subset[v]))
					{
						FindLeaf(Case, T->Branch[v], T, Fraction);

						return;
					}
				}

				/* Value not found in any subset -- treat as leaf  */

				goto LeafUpdate;
			}
			else
			{
				FollowAllBranches(Case, T, Fraction);
			}
		}
	}



	/*************************************************************************/
	/*                                                              	 */
	/*	Categorize a case using a ruleset				 */
	/*                                                              	 */
	/*************************************************************************/


	ClassNo CDecisionTreeBaseEx::RuleClassify(DataRec Case, CRuleSet RS)
		/*      ------------  */
	{
		ClassNo	c, Best;
		float	TotWeight = 0;
		int		a;
		CRule	R;
		RuleNo	r;

		ForEach(c, 0, MaxClass)
		{
			ClassSum[c] = 0;
			MostSpec[c] = Nil;
		}

		/*  Find active rules  */

		NActive = 0;

		if (RS->RT)
		{
			MarkActive(RS->RT, Case);
		}
		else
		{
			ForEach(r, 1, RS->SNRules)
			{
				R = RS->SRule[r];

				if (Matches(R, Case))
				{
					Active[NActive++] = r;
				}
			}
		}

		if (RULESUSED)
		{
			RulesUsed[NRulesUsed++] = NActive;
			ForEach(a, 0, NActive - 1)
			{
				RulesUsed[NRulesUsed++] = Active[a];
			}
		}

		/*  Vote active rules  */

		ForEach(a, 0, NActive - 1)
		{
			r = Active[a];
			R = RS->SRule[r];

			ClassSum[R->Rhs] += R->Vote;
			TotWeight += 1000.0;

			/*  Check whether this is the most specific rule for this class;
				resolve ties in favor of rule with higher vote  */

			if (!MostSpec[R->Rhs] ||
				R->Cover < MostSpec[R->Rhs]->Cover ||
				(R->Cover == MostSpec[R->Rhs]->Cover &&
				R->Vote > MostSpec[R->Rhs]->Vote))
			{
				MostSpec[R->Rhs] = R;
			}
		}

		/*  Check for default and normalise ClassSum  */

		if (!TotWeight)
		{
			Confidence = 0.5;
			return RS->SDefault;
		}

		ForEach(c, 1, MaxClass)
		{
			ClassSum[c] /= TotWeight;
		}

		Best = SelectClass(RS->SDefault, false);

		/*  Set Confidence to the vote for the most specific rule of class Best  */

		Confidence = (float)(MostSpec[Best]->Vote / 1000.0);

		return Best;
	}



	/*************************************************************************/
	/*                                                              	 */
	/*	Determine outcome of a test on a case.				 */
	/*	Return -1 if value of tested attribute is unknown		 */
	/*                                                              	 */
	/*************************************************************************/


	int CDecisionTreeBaseEx::FindOutcome(DataRec Case, Condition OneCond)
		/*  -----------  */
	{
		DiscrValue  v, Outcome;
		Attribute	Att;

		Att = OneCond->Tested;

		/*  Determine the outcome of this test on this case  */

		switch (OneCond->NodeType)
		{
		case BrDiscr:  /* test of discrete attribute */

			v = XDVal(Case, Att);
			Outcome = (v == 0 ? -1 : v);
			break;

		case BrThresh:  /* test of continuous attribute */

			Outcome = (Unknown(Case, Att) ? -1 :
				NotApplic(Case, Att) ? 1 :
				CVal(Case, Att) <= OneCond->Cut ? 2 : 3);
			break;

		case BrSubset:  /* subset test on discrete attribute  */

			v = XDVal(Case, Att);
			Outcome = (v <= MaxAttVal[Att] && In(v, OneCond->Subset) ?
				OneCond->TestValue : 0);
		}

		return Outcome;
	}



	/*************************************************************************/
	/*									 */
	/*	Determine whether a case satisfies a condition			 */
	/*									 */
	/*************************************************************************/


	Boolean CDecisionTreeBaseEx::Satisfies(DataRec Case, Condition OneCond)
		/*      ---------  */
	{
		return (FindOutcome(Case, OneCond) == OneCond->TestValue);
	}



	/*************************************************************************/
	/*									 */
	/*	Determine whether a case satisfies all conditions of a rule	 */
	/*									 */
	/*************************************************************************/


	Boolean CDecisionTreeBaseEx::Matches(CRule R, DataRec Case)
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



	/*************************************************************************/
	/*									 */
	/*	Make sure that Active[] has space for at least N rules		 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::CheckActiveSpace(int N)
		/*   ----------------  */
	{
		if (ActiveSpace <= N)
		{
			Realloc((*this), Active, (ActiveSpace = N + 1), RuleNo);
		}
	}



	/*************************************************************************/
	/*									 */
	/*	Use RT to enter active rules in Active[]			 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::MarkActive(RuleTree RT, DataRec Case)
		/*   ----------  */
	{
		DiscrValue	v;
		int		ri;
		RuleNo	r;

		if (!RT) return;

		/*  Enter any rules satisfied at this node  */

		if (RT->Fire)
		{
			for (ri = 0; (r = RT->Fire[ri]); ri++)
			{
				Active[NActive++] = r;
			}
		}

		if (!RT->Branch) return;

		/*  Explore subtree for rules that include condition at this node  */

		if ((v = FindOutcome(Case, RT->CondTest)) > 0 && v <= RT->Forks)
		{
			MarkActive(RT->Branch[v], Case);
		}

		/*  Explore default subtree for rules that do not include condition  */

		MarkActive(RT->Branch[0], Case);
	}



	/*************************************************************************/
	/*									 */
	/*	Classify a case using boosted tree or rule sequence		 */
	/*									 */
	/*************************************************************************/


	ClassNo CDecisionTreeBaseEx::BoostClassify(DataRec Case, int MaxTrial)
		/*	-------------  */
	{
		ClassNo	c, Best;
		int		t;
		float	Total = 0;

		ForEach(c, 1, MaxClass)
		{
			Vote[c] = 0;
		}

		ForEach(t, 0, MaxTrial)
		{
			Best = (RULES ? RuleClassify(Case, RuleSet[t]) :
				TreeClassify(Case, Pruned[t]));

			Vote[Best] += Confidence;
			Total += Confidence;

		}

		/*  Copy votes into ClassSum  */

		ForEach(c, 1, MaxClass)
		{
			ClassSum[c] = Vote[c] / Total;
		}

		return SelectClass(Default, false);
	}



	/*************************************************************************/
	/*									 */
	/*	Select the best class to return.  Take misclassification costs	 */
	/*	into account if they are defined.				 */
	/*									 */
	/*************************************************************************/


	ClassNo CDecisionTreeBaseEx::SelectClass(ClassNo Default, Boolean UseCosts)
		/*      -----------  */
	{
		ClassNo	c, cc, BestClass;
		float	ExpCost, BestCost = 1E38f, TotCost = 0;

		BestClass = Default;

		if (UseCosts)
		{
			ForEach(c, 1, MaxClass)
			{
				ExpCost = 0;
				ForEach(cc, 1, MaxClass)
				{
					if (cc == c) continue;
					ExpCost += ClassSum[cc] * MCost[c][cc];
				}

				TotCost += ExpCost;

				if (ExpCost < BestCost)
				{
					BestClass = c;
					BestCost = ExpCost;
				}
			}

			Confidence = 1 - BestCost / TotCost;
		}
		else
		{
			ForEach(c, 1, MaxClass)
			{
				if (ClassSum[c] > ClassSum[BestClass]) BestClass = c;
			}

			Confidence = ClassSum[BestClass];
		}

		return BestClass;
	}



	/*************************************************************************/
	/*								   	 */
	/*	General classification routine					 */
	/*								   	 */
	/*************************************************************************/


	ClassNo CDecisionTreeBaseEx::Classify(DataRec Case)
		/*      --------  */
	{
		NRulesUsed = 0;

		return (TRIALS > 1 ? BoostClassify(Case, TRIALS - 1) :
			RULES ? RuleClassify(Case, RuleSet[0]) :
			TreeClassify(Case, Pruned[0]));
	}



	/*************************************************************************/
	/*								   	 */
	/*	Interpolate a single value between Lower, Cut and Upper		 */
	/*								   	 */
	/*************************************************************************/


	float CDecisionTreeBaseEx::Interpolate(Tree T, ContValue Val)
		/*    -----------  */
	{
		return (float)(Val <= T->Lower ? 1.0 :
			Val >= T->Upper ? 0.0 :
			Val <= T->Mid ?
			1 - 0.5 * (Val - T->Lower) / (T->Mid - T->Lower + 1E-6) :
			0.5 - 0.5 * (Val - T->Mid) / (T->Upper - T->Mid + 1E-6));
	}



	/*************************************************************************/
	/*									 */
	/*	Open file with given extension for read/write			 */
	/*									 */
	/*************************************************************************/


	FILE * CDecisionTreeBaseEx::GetFile(String Extension, String RW)
		/*    --------  */
	{
		strcpy(Fn, FileStem);
		strcat(Fn, Extension);
		return fopen(Fn, RW);
	}



	/*************************************************************************/
	/*									 */
	/*	Check whether file is open.  If it is not, open it and		 */
	/*	read/write discrete names					 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::CheckFile(String Extension, Boolean Write)
		/*   ---------  */
	{


		if (!TRf || strcmp(LastExt, Extension))
		{
			LastExt = Extension;

			if (TRf)
			{
				fprintf(TRf, "\n");
				fclose(TRf);
			}

			ReadFilePrefix(Extension);
		}
	}



	/*************************************************************************/
	/*									 */
	/*	Specialised form of the getopt() utility			 */
	/*									 */
	/*************************************************************************/

	String	OptArg, Option;


	char CDecisionTreeBaseEx::ProcessOption(int Argc, char *Argv[], char *Options)
		/*   -------------  */
	{
		int		i;


		if (OptNo >= Argc) return '\00';

		if (*(Option = Argv[OptNo++]) != '-') return '?';

		for (i = 0; Options[i]; i++)
		{
			if (Options[i] == Option[1])
			{
				OptArg = (char *)(Options[i + 1] != '+' ? Nil :
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



	void *Pmalloc(CDecisionTreeBaseEx& DT, size_t Bytes)
		/*    -------  */
	{
		void *p = Nil;

		if (!Bytes || (p = (void *)malloc(Bytes)))
		{
			return p;
		}

		DT.Error(NOMEM, "", "");

#ifdef WIN32
		return Nil;
#endif
	}



	void *Prealloc(CDecisionTreeBaseEx& DT, void *Present, size_t Bytes)
		/*    --------  */
	{
		void *p = Nil;

		if (!Bytes) return Nil;

		if (!Present) return Pmalloc(DT, Bytes);

		if ((p = (void *)realloc(Present, Bytes)))
		{
			return p;
		}

		DT.Error(NOMEM, "", "");

#ifdef WIN32
		return Nil;
#endif
	}



	void * Pcalloc(CDecisionTreeBaseEx& DT, size_t Number, unsigned int Size)
		/*    -------  */
	{
		void *p = Nil;

		if (!Number || (p = (void *)calloc(Number, Size)))
		{
			return p;
		}

		DT.Error(NOMEM, "", "");

#ifdef WIN32
		return Nil;
#endif
	}



	/*************************************************************************/
	/*									 */
	/*	Error messages							 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::Error(int ErrNo, String S1, String S2)const
		/*   -----  */
	{
		Boolean	Quit = false, WarningOnly = false;
		char	Buffer[10000], *Msg = Buffer;
		CDecisionTreeBaseEx& me = const_cast<CDecisionTreeBaseEx&>(*this);

#ifdef WIN32
		if (ErrNo == NOMEM)
		{
			printf("Cannot allocate sufficient memory");
			//MessageBox(NULL, _T("Cannot allocate sufficient memory"), "Fatal Error",MB_ICONERROR | MB_OK);
			Goodbye(1);
		}
		else
			if (ErrNo == MODELFILE)
			{
				if (!ErrMsgs)
				{

					sprintf(Msg, "File %s is incompatible with .names file\n(%s `%s')", Fn, S1, S2);
					printf(Msg);
					//MessageBox(NULL, Msg, "Cannot Load Classifier",MB_ICONERROR | MB_OK);
				}


				me.ErrMsgs++;

				return;
			}
#endif

		if (Of) fprintf(Of, "\n");

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

		case BADCLASSTHRESH:
			sprintf(Msg, E_BADCLASSTHRESH, S1);
			break;

		case LEQCLASSTHRESH:
			sprintf(Msg, E_LEQCLASSTHRESH, S1);
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

		case BADCLASS:
			sprintf(Msg, E_BADCLASS, S2);
			break;

		case BADCOSTCLASS:
			sprintf(Msg, E_BADCOSTCLASS, S1);
			Quit = true;
			break;

		case BADCOST:
			sprintf(Msg, E_BADCOST, S1);
			Quit = true;
			break;

		case NOMEM:
			sprintf(Msg, E_NOMEM);
			Quit = true;
			break;

		case TOOMANYVALS:
			sprintf(Msg, E_TOOMANYVALS(S1, (int)(long)S2));
			break;

		case BADDISCRETE:
			sprintf(Msg, E_BADDISCRETE, S1);
			break;

		case NOTARGET:
			sprintf(Msg, E_NOTARGET, S1);
			Quit = true;
			break;

		case BADCTARGET:
			sprintf(Msg, E_BADCTARGET, S1);
			Quit = true;
			break;

		case BADDTARGET:
			sprintf(Msg, E_BADDTARGET, S1);
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
				printf(Buffer);
				//MessageBox(NULL, Buffer, (WarningOnly ? "Warning" : "Error"), MB_OK);
			}
#else
		fprintf(Of, Buffer);
#endif

		if (!WarningOnly) me.ErrMsgs++;

		if (ErrMsgs == 10)
		{
#if defined WIN32 && ! defined _CONSOLE
			printf(T_ErrorLimit);
			//MessageBox(NULL, T_ErrorLimit, "Too many errors!", MB_OK);
#else
			fprintf(Of,  T_ErrorLimit);
#endif
			Quit = true;
		}

		if (Quit && Of)
		{
			Goodbye(1);
		}
	}



	/*************************************************************************/
	/*									                                     */
	/*	Determine precision of floating value				                 */
	/*									                                     */
	/*************************************************************************/


	int Denominator(ContValue Val)
		/*  -----------  */
	{
		double	RoundErr, Accuracy;
		int		Mult;

		Accuracy = fabs(Val) * 1E-6;	/* approximate */
		Val = (ContValue)modf(Val, &RoundErr);

		for (Mult = 100000; Mult >= 1; Mult /= 10)
		{
			RoundErr = fabs(rint(Val * Mult) / Mult - Val);
			if (RoundErr > 2 * Accuracy)
			{
				return Mult * 10;
			}
		}

		return 1;
	}



	/*************************************************************************/
	/*									                                     */
	/*	Routines to process dates (algorithm due to Gauss),		             */
	/*	times, and timestamps						                         */
	/*									                                     */
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



	void CDecisionTreeBaseEx::SetTSBase(int y)
		/*   ---------  */
	{
		y -= 15;
		TSBase = y * 365 + y / 4 - y / 100 + y / 400 + (367 * 4) / 12 + 1 - 30;
	}



	int CDecisionTreeBaseEx::TStampToMins(String TS)
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
	/*	Free case space							 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::FreeLastCase(DataRec DVec)
		/*   ------------  */
	{
		free(&DVec[-1]);
		IValsOffset = 0;
	}



	/*************************************************************************/
	/*									 */
	/*	Deallocate the space used to perform classification		 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::FreeGlobals()
		/*   -----------  */
	{
		/*  Free memory allocated for classifier  */

		if (RULES)
		{
			ForEach(Trial, 0, TRIALS - 1)
			{
				FreeRules(RuleSet[Trial]);
			}
			Free(RuleSet);

			FreeUnlessNil(Active);
			FreeUnlessNil(RulesUsed);
			FreeUnlessNil(MostSpec);
		}

		if (Pruned)
		{
			ForEach(Trial, 0, TRIALS - 1)
			{
				FreeTree(Pruned[Trial]);
			}
			Free(Pruned);
		}


		FreeUnlessNil(PropVal);

		/*  Free memory allocated for cost matrix  */

		if (MCost)
		{
			FreeVector((void **)MCost, 1, MaxClass);
			MCost = Nil;
		}

		/*  Free memory for names etc  */

		FreeNames();
		FreeUnlessNil(IgnoredVals);

		Free(ClassSum);
		Free(Vote);
		TRIALS = 0;
	}



	/*************************************************************************/
	/*									 */
	/*	Free up all space allocated by GetNames()			 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::FreeNames()
		/*   ---------  */
	{
		Attribute a, t;

		if (!AttName) return;

		ForEach(a, 1, MaxAtt)
		{
			if (a != ClassAtt && Discrete((*this), a))
			{
				FreeVector((void **)AttValName[a], 1, MaxAttVal[a]);
			}
		}
		FreeUnlessNil(AttValName);				AttValName = Nil;
		FreeUnlessNil(MaxAttVal);				MaxAttVal = Nil;
		FreeUnlessNil(ClassThresh);				ClassThresh = Nil;
		FreeVector((void **)AttName, 1, MaxAtt);		AttName = Nil;
		FreeVector((void **)ClassName, 1, MaxClass);	ClassName = Nil;

		FreeUnlessNil(SpecialStatus);			SpecialStatus = Nil;

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
	}



	/*************************************************************************/
	/*									 */
	/*	Free up space taken up by tree Node				 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::FreeTree(Tree T)
		/*   --------  */
	{
		DiscrValue v;

		if (!T) return;

		if (T->NodeType)
		{
			ForEach(v, 1, T->Forks)
			{
				FreeTree(T->Branch[v]);
			}

			Free(T->Branch);

			if (T->NodeType == BrSubset)
			{
				FreeVector((void **)T->Subset, 1, T->Forks);
			}

		}

		Free(T->ClassDist);
		Free(T);
	}



	/*************************************************************************/
	/*									 */
	/*	Deallocate the space used to store rules			 */
	/*									 */
	/*************************************************************************/


	void CDecisionTreeBaseEx::FreeRule(CRule R)
		/*   --------  */
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
		FreeUnlessNil(R);
	}



	void CDecisionTreeBaseEx::FreeRuleTree(RuleTree RT)
		/*   ------------  */
	{
		int		b;

		if (!RT) return;

		if (RT->Branch)
		{
			ForEach(b, 0, RT->Forks)
			{
				FreeRuleTree(RT->Branch[b]);
			}
			Free(RT->Branch);
		}

		/*  Don't free RT->Cond since this is just a pointer to a condition
		in one of the rules  */

		FreeUnlessNil(RT->Fire);
		Free(RT);
	}



	void CDecisionTreeBaseEx::FreeRules(CRuleSet RS)
		/*   ---------  */
	{
		int	ri;

		ForEach(ri, 1, RS->SNRules)
		{
			FreeRule(RS->SRule[ri]);
		}
		Free(RS->SRule);
		FreeRuleTree(RS->RT);
		Free(RS);
	}



	void CDecisionTreeBaseEx::FreeVector(void **V, int First, int Last)
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


	void CDecisionTreeBaseEx::LoadModel(String filePath)
	{
		//  Read information on attribute names, values, and classes  
		//DT.FileStem = (String)filePath.c_str();
		FileStem = filePath;
		FILE* F = GetFile(".names", "r");

		if (!F)
			Error(NOFILE, Fn, "");

		GetNames(F);
		fclose(F);
		F = NULL;


		//  Read the appropriate classifier file.  Call CheckFile() to
		//determine the number of trials, then allocate space for
		//trees or rulesets  
		RULES = TRUE;
		CheckFile(".rules", false);
		RuleSet = AllocZero(*this, TRIALS + 1, CRuleSet);

		ForEach(Trial, 0, TRIALS - 1)
		{
			RuleSet[Trial] = GetRules(".rules");
		}


		MostSpec = Alloc(*this, MaxClass + 1, CRule);
		Default = RuleSet[0]->SDefault;

		//  Close the classifier file and reset the file variable  
		fclose(TRf);
		TRf = NULL;

		// load band reference
		//	m_attributeBandeRef.resize(MaxAtt);


		//FILE* F = GetFile(".bands", "r");



	}

	int CDecisionTreeBaseEx::GetNbVirtualBands()const
	{
		int nbVirtBand = 0;

		int Att = 1;
		ForEach(Att, 1, MaxAtt)
		{
			if (AttDef[Att])
				nbVirtBand++;
		}

		return nbVirtBand;
	}


	bool CDecisionTreeBaseEx::IsValidVirtualPixel(int Att, DataRec Case, ContValue noData)
	{
		bool bValid = true;

		Definition D = AttDef[Att];

		int i = 0;
		while (bValid && DefOp(AttDef[Att][i]) != OP_END)
		{
			if (DefOp(AttDef[Att][i]) == OP_ATT)
			{
				Attribute j = (long)DefSVal(AttDef[Att][i]);
				bValid = fabs(CVal(Case, j) - noData) > 0.001;
			}
			i++;
		}

		return bValid;
	}


	bool CDecisionTreeBaseEx::IsValidPixel(DataRec Case, ContValue noData)
	{
		bool bValid = true;

		for (int att = 2; att <= MaxAtt&&bValid; att++)
		{
			bValid = fabs(CVal(Case, att) - noData) > 0.001;
		}

		return bValid;
	}

	bool CDecisionTreeBaseEx::IsValidPixel(DataRec Case, std::vector<double>& noData)
	{
		assert(MaxAtt - 1 == noData.size());

		bool bValid = true;

		for (int att = 2; att <= MaxAtt&&bValid; att++)
		{
			bValid = fabs(CVal(Case, att) - noData[att - 2]) > 0.001;
		}

		return bValid;
	}




	//**********************************************************************************************************

	ERMsg CDecisionTree::Load(const std::string& filePath, size_t numThread, size_t numIOThread, bool bQuiet)
	{
		ERMsg msg;

		//because CDecisionTreeBaseEx is not thread safe, we have to create many object 
		//with the same initialisation

		resize(numThread);

		try
		{
			size_t x = 0;
			size_t xx = 0;
#pragma omp parallel for schedule(static, 1) shared(x,xx) num_threads( int(numIOThread) ) if(numIOThread>1)
			for (int i = 0; i < (int)size(); i++)
			{
				at(i).LoadModel((String)filePath.c_str());
				at(i).ClassSum = AllocZero(at(i), at(i).MaxClass + 1, float);  // used in classification 
				at(i).Vote = AllocZero(at(i), at(i).MaxClass + 1, float);  // used with boosting 

				if (!bQuiet)
				{
#pragma omp critical(Progress)
				{
#pragma omp flush(x)
#pragma omp flush(xx)

					x++;

					size_t nbX = (size_t)(80.0* ((double)x / numThread) + 0.5);

					while (xx < nbX)
					{
						std::cout << ".";
						xx++;
					}
#pragma omp flush(x)
#pragma omp flush(xx)
				}
				}
			}
		}
		catch (...)
		{
			msg.ajoute("Fail to load decision tree model " + filePath);
		}

		return msg;
	}

	void CDecisionTree::FreeMemory()
	{
		for (size_t i = 0; i < size(); i++)
			at(i).FreeGlobals();

	}

}