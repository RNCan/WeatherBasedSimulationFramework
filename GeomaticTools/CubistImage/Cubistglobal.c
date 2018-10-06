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

#include "Cubistdefns.h"
/*************************************************************************/
/*									 */
/*		General data for Cubist					 */
/*									 */
/*************************************************************************/


	Attribute	ClassAtt=0,	/* attribute to use as class */
			LabelAtt=0,	/* attribute containing case label */
			CWtAtt=0;	/* attribute containing case weight */

	int		MaxAtt,		/* max att number */
			MaxDiscrVal=3,	/* max discrete values for any att */
			Precision=2,	/* decimal places for target */
			MaxLabel=0,	/* max characters in case label */
			LineNo=0,	/* input line number */
			ErrMsgs=0,	/* errors found */
			AttExIn,	/* attribute exclusions/inclusions */
			TSBase=0,	/* base day for time stamps */
			Delimiter,	/* character at end of name */
			NCPU=1;		/* number of CPUs */

	double		ClassUnit;

	float		ErrReduction=1;	/* effect of committee model */

	CaseNo		MaxCase=-1;	/* max data item number */

	DataRec		*Case;		/* data items */

	DiscrValue	*MaxAttVal,	/* number of values for each att */
			*Modal;		/* most frequent value for discr att */

	char		*SpecialStatus;	/* special att treatment */

	Definition	*AttDef;	/* definitions of implicit atts */

	String		Target,		/* name of dependent att */
		  	*AttName,	/* att names */
		  	**AttValName;	/* att value names */

	char		*IgnoredVals=0;	/* values of labels and ignored atts */
	int		IValsSize=0,	/* size of above */
			IValsOffset=0;	/* index of first free char */

	String		FileStem="undefined";
	char		Fn[512];	/* file name */

	FILE		*Mf=0;		/* file for reading models */

	ContValue	*AttMean=Nil,	/* means of att values */
			*AttSD=Nil,	/* std dev ditto */
			*AttMaxVal=Nil,	/* max value in training data */
			*AttMinVal=Nil,	/* min value ditto */
			Ceiling=1E38,	/* max allowable global prediction */
			Floor=-1E38;	/* min allowable global prediction */

	int		*AttPrec=Nil;	/* Attribute precision  */

	DataRec		*Instance=Nil,	/* training cases */
			Ref[2];		/* reference point */
	Index		KDTree=Nil;	/* index for searching training cases */
	CaseNo		MaxInstance=-1;	/* highest instance */
	float		*RSPredVal=Nil, /* tabulated RS predictions */
			*RSErrLim=Nil;	/* tabulated RS error limits */
	NNEnvRec	GNNEnv;		/* global NN environment */

	unsigned char	*Tested;	/* used in BuildIndex */
	CaseCount	*ValFreq;	/* used in BuildIndex */

	RRuleSet	*CubistModel;	/* from .model file */

	Boolean		USEINSTANCES;
	float		EXTRAP=0.05f,	/* allowed extrapolation from models */
			SAMPLE=0.0,	/* sample training proportion  */
			MAXD,		/* max distance for close neighbors */
			GlobalMean=0,	/* global mean on training data */
			GlobalErrLim;	/* estimated global error limit */
	int		MEMBERS=1,	/* models in committee */
			NN=5,		/* nearest neighbors to use */
			KRInit;
