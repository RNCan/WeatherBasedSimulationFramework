// MiniTab2CSVDlg.cpp 
//
//LOG
//23/11/2005 : modify VX file to take CRLF or LF file
//17/03/2005 : creation

#include "stdafx.h"
#include <math.h>
#include <stdio.h>
#include <algorithm>
#include <array>

#include "MiniTab2CSV.h"
#include "MiniTab2CSVDlg.h"
//#include "CSV3.h"
//#include "Basic/ERMsg.h"
//#include "UI/Common/SYShowMessage.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/UtilWin.h"
//#include "UI/Common/SelectDirectory.h"
//#include "UI/Common/CustomDDX.h"

using namespace std;
using namespace UtilWin;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//using namespace CFL;
/* Tools for storage/retrieval of arbitrary size bytes from 32-bit words
	(note - this version is not currently (6/30/88) described in the
	gbytes document)

	gbytes(p,u,q,b,s,n)
	gbyte (p,u,q,b)
	sbytes(p,u,q,b,s,n)
	sbyte (p,u,q,b)

			 q >= 0     number of bits to be skipped preceeding first byte in p
	  0 <    b < sword  byte size
			 s >= 0     number of bits to be skipped between bytes
			 n >= 0     number of bytes to be packed/unpacked

	gbytes unpacks n b bit bytes from p into u, starting by skipping
		 q bits in p, then skipping s bits between bytes.
	gbyte unpacks one such byte.
	sbytes   packs n b bit bytes from u into p, starting by skipping
		 q bits in p, then skipping s bits between bytes.
	sbyte  packs one such byte. */
# define SWORD 32                              /* Word size in bits */
# define MASK 0xffffffff                       /* Mask of sword bits */
# define G1BYTE(p,q,b) ((b==32 ? MASK : ~(MASK<<b)) & (p>>(SWORD-(q+b))))
	/* Get 1 word contained byte */
# define MASK1(q,b) (b==32 ? MASK : (~(MASK<<b)<<(SWORD-(q+b))))
											   /* Mask of sword bits */
/* Common code for gbytes, sbytes */
void gsbytes(long p[], long u[], long q, long b, long s, long n,
	void(*gsbyte)(long p[], long *u, long q, long b))
{
	long jp, ju;
	jp = 0;
	for (ju = 0; ju < n; ++ju) {
		(*gsbyte)(&p[jp], &u[ju], q, b);
		q += b + s;
		jp += q / SWORD;
		q %= SWORD;
	}
}
void gbyte(long p[], long *u, long q, long b)
{
	long qb, j, lb;

	if (q >= SWORD) {
		j = q / SWORD; /* number of words offset */
		q %= SWORD;  /* odd bits of offset     */
	}
	else {
		j = 0;
	}
	qb = q + b;
	if (qb > SWORD) {
		qb = SWORD - q;
		b -= qb;
		lb = (G1BYTE(p[j], q, qb)) << b;
		q = 0;
		j++;  /* increment to next word */
	}
	else lb = 0;
	*u = lb + (G1BYTE(p[j], q, b));
}
void gbytes(long p[], long u[], long q, long b, long s, long n)
{
	gsbytes(p, u, q, b, s, n, gbyte);
}
void sbyte(long p[], long *u, long q, long b)
{
	long qb, j, rb;

	if (q >= SWORD) {
		j = q / SWORD;    /* number of words offset */
		q %= SWORD;       /* odd bit offset         */
	}
	else {
		j = 0;
	}
	qb = q + b;
	if (qb > SWORD) {
		qb = SWORD - q;
		q = SWORD - b;
		b -= qb;
		p[j] = ((p[j] >> qb) << qb) + (G1BYTE(*u, q, qb));
		q = 0;
		j++;  /* point to next word */
	}
	rb = G1BYTE(*u, SWORD - b, b);
	p[j] = (p[j] & ~MASK1(q, b)) + (rb << (SWORD - (b + q)));
}
void sbytes(long p[], long u[], long q, long b, long s, long n)
{
	gsbytes(p, u, q, b, s, n, sbyte);
}


/* VAX float representation:
**
** bit:  0                             31
**       +--------------+-+------+------+
**       |   FRACTION   |S|  EXP | FRAC |
**       +--------------+-+------+------+
**
** The exponent is stored in binary 128 excess notation.
** The fraction is a 24-bit normalized fraction with the most significant bit
**   not stored.  If the exponent is not 0, this bit is assumed to be 1.  A
**   number with sign 0 and exponent 0 is assumed to be 0. in value.
*/

void vaxfloat(long *val, float *rval, int num)
{
	/* val  is the first address of a block of integers containing the
	**      representations of the VAX floats
	** rval is the first address of a block of floats in native format created from
	**      the VAX representations
	** num  is the number of VAX representations to convert to native format
	*/

	long  n, sign, exp, fr[2];

	for (n = 0; n < num; n++) {
		/* unpack sign, exponent, and fraction */
		gbyte(&val[n], &sign, 16, 1);
		gbyte(&val[n], &exp, 17, 8);
		if (exp == 0)
			rval[n] = 0.;
		else {
			exp -= 128;
			gbyte(&val[n], &fr[0], 0, 16);
			gbyte(&val[n], &fr[1], 25, 7);

			/* reconstruct fraction */
			fr[0] += ((fr[1] << 16) | 0x800000);
			rval[n] = fr[0] * pow(2.0f, (float)(exp - 24));
			if (sign == 1) rval[n] = -rval[n];
		}
	}
}



// CMiniTab2CSVDlg 

CMiniTab2CSVDlg::CMiniTab2CSVDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMiniTab2CSVDlg::IDD, pParent)
{
	CAppOption option;
	m_inputPath = option.GetProfileString(_T("input"), _T(""));
	m_outputPath = option.GetProfileString(_T("output"), _T(""));
	m_nFilterIndex = option.GetProfileInt(_T("index"), 0);

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMiniTab2CSVDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_INPUT, m_inputPathCtrl);
	DDX_Text(pDX, IDC_INPUT, m_inputPath);
	DDX_Text(pDX, IDC_OUTPUT, m_outputPath);
}

BEGIN_MESSAGE_MAP(CMiniTab2CSVDlg, CDialogEx)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_TRANSFORM, OnBnClickedTransform)
	ON_BN_CLICKED(ID_CANCEL, OnButtonCancel)
	ON_BN_CLICKED(IDC_INPUT_BROWSE, OnBnClickedInputBrowse)
	ON_BN_CLICKED(IDC_OUTPUT_BROWSE, OnBnClickedOutputBrowse)
END_MESSAGE_MAP()


// CMiniTab2CSVDlg 

BOOL CMiniTab2CSVDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();


	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);



	return TRUE;
}


void CMiniTab2CSVDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);


		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;


		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}


HCURSOR CMiniTab2CSVDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMiniTab2CSVDlg::OnButtonCancel()
{
	OnCancel();
}

void CMiniTab2CSVDlg::OnBnClickedTransform()
{
	//MTW2CSV("D:\\travail\\Programm old minitab\\1std3run.min", "D:\\travail\\Programm old minitab\\output.CSV");


	if (!UpdateData(true))
		return;

	if (!m_inputPath.IsEmpty() &&
		(GetFileExtension(m_inputPath).CompareNoCase(_T(".mtw")) >= 0 ||
			GetFileExtension(m_inputPath).CompareNoCase(_T(".min")) >= 0))
	{
		CAppOption option;
		option.WriteProfileString(_T("input"), m_inputPath);
		//option.WriteProfileString("output", m_outputPath);

		CString filePathIn(m_inputPath);
		CString filePathOut(m_inputPath);
		SetFileExtension(filePathOut, _T(".csv"));
		MTW2CSV(filePathIn, filePathOut);

		//CStringArray fileList;
		//UtilWin::GetFileList(fileList, (CString)m_inputPath + "*.mtw", true);

		//for(int i=0; i<fileList.GetSize(); i++)
		//{
		//	CPath filePathIn(fileList[i]);
		//	CPath filePathOut(fileList[i]);
		//	
		//	filePathOut.SetLocation(m_outputPath);
		//	filePathOut.SetExtension(".CSV");
		//	MTW2CSV(filePathIn, filePathOut);
		//}

	}

}

void CMiniTab2CSVDlg::MTW2CSV(const CString& filePathIn, const CString& filePathOut)
{
	CFile mtw;
	//CCSV3 CSV;
	CStdioFile csv;


	CFileException e;
	if (mtw.Open(filePathIn, CFile::modeRead|CFile::typeBinary| CFile::shareDenyNone, &e) && csv.Open(filePathOut, CFile::modeWrite | CFile::modeCreate, &e))
	{
		CMTWColomnArray colArray;

		CStringA head;
		VERIFY(mtw.Read(head.GetBufferSetLength(80), 80) == 80);
		head.ReleaseBuffer();
		if (head[0] == 3)
		{

			mtw.Seek(2, CFile::begin);
			VERIFY(mtw.Read(head.GetBufferSetLength(80), 80) == 80);

		}

		head.MakeLower();



		if (head.Find("mtbv80.1") >= 0)
		{
			ReadVersion80(mtw, colArray);
		}
		else if (head.Find("vx") >= 0)//head.Find( "worksheet stored by minitab version  5.1" ) >= 0 ||
			//head.Find( "worksheet stored by minitab version 5.1" ) >= 0 )
		{
			ReadVersionVAX(mtw, colArray);
		}
		else //if( head.Find( "worksheet stored by minitab release  6.1" ) >= 0 ||
			//	 head.Find( "worksheet stored by minitab release 6.1" ) >= 0 )
		{
			ReadVersionIBM(mtw, colArray);
		}


		INT_PTR nbRow = 0;
		for (int i = 0; i < colArray.GetSize(); i++)
		{
			if (i > 0)
				csv.WriteString(_T(","));

			csv.WriteString( CString(colArray[i].m_name));
			nbRow =nbRow>colArray[i].m_data.GetSize()? nbRow: colArray[i].m_data.GetSize();
		}
		
		csv.WriteString(_T("\n"));
		//CSV.SetNbRecord(maxSize);

		for (int j = 0; j < nbRow; j++)
		{
			for (int i = 0; i < colArray.GetSize(); i++)
			{
				if (j < colArray[i].m_data.GetSize())
				{
					float value = colArray[i].m_data[j];
					if (value > -1.234e30f &&
						value < 1.23456e30f)
					{
						if (i > 0)
							csv.WriteString(_T(","));
						
						csv.WriteString(ToCString(colArray[i].m_data[j]));
					}
				}
			}
			
			csv.WriteString(_T("\n"));
		}


		//ERMsg message = CSV.Write(filePathOut);
		//if (!message)
		//{
			//SYShowMessage(message, NULL);
		//}
		mtw.Close();
		csv.Close();
	}
	else
	{
		e.ReportError();
	}

}



bool CMiniTab2CSVDlg::ReadVersionIBM(CFile& mtw, CMTWColomnArray& colArray)
{
	CStringA head;

	mtw.SeekToBegin();

	VERIFY(mtw.Read(head.GetBufferSetLength(80), 80) == 80);
	head.MakeLower();

	while (mtw.GetPosition() < mtw.GetLength() - 1)
	{
		long firstRec[8] = { 0 };
		int rel = 72;
		short size = 6;
		if (head.Find("release 7.2") >= 0 ||
			(head.Find("release 8.2") >= 0 && head.Find("version 5.1") >= 0 && head.Find("1994")>=0))
		{
			rel = 72;
			size = 8;

			for (int i = 0; i < 8; i++)
			{
				short tmp;
				VERIFY(mtw.Read(&tmp, 2) == 2);
				firstRec[i] = tmp;
			}
		}
		else if (head.Find("release 8.2") >= 0)
		{
			rel = 82;
			size = 6;

			for (int i = 0; i < 6; i++)
				VERIFY(mtw.Read(&(firstRec[i]), 4) == 4);
		}
		else
		{
			AfxMessageBox(_T("release not supported"));
		}

		if (firstRec[0] == 2 || firstRec[0] == 3 || firstRec[0] == 4 || firstRec[0] == 127)
		{
			INT_PTR pos = colArray.GetSize();
			colArray.SetSize(pos + 1);
			CMTWColomn& col = colArray[pos];

			col.m_no = firstRec[1];
			col.m_data.SetSize(firstRec[2]);
			col.m_nbNoData = firstRec[3];
			if (firstRec[4] != 0)
			{
				for (int j = 4; j < size; j++)
				{
					CStringA number;
					number.Format("%d", firstRec[j]);
					for (int k = 0; k < ceil(number.GetLength() / 2.0); k++)
					{
						char c = atoi(number.Mid(k * 2, 2)) + ((rel == 72) ? 0 : 32);
						if (::iswprint(c))
							col.m_name += c;
					}
				}
			}

			if (col.m_name.IsEmpty())
			{
				switch (firstRec[0])
				{
				case 2: col.m_name.Format("Constant %d", col.m_no); break;
				case 3: col.m_name.Format("Column %d", col.m_no); break;
				case 4: col.m_name.Format("Matrix %d", col.m_no); break;
				case 127:col.m_name.Format("Unknown %d", col.m_no); break;
				default: ASSERT(false);
				}
			}

			for (int i = 0; i < firstRec[2]; i++)
			{
				if (firstRec[0] == 2 || firstRec[0] == 3)
				{
					VERIFY(mtw.Read(&(col.m_data.GetData()[i]), 4) == 4);
				}
				else if (firstRec[0] == 127)
				{
					long tmp;
					VERIFY(mtw.Read(&tmp, 4) == 4);
					col.m_data[i] = (float)tmp;
				}
			}
		}
		else
		{
			ASSERT(false);
			AfxMessageBox(_T("type de données non supporter"));
		}
	}
	//	}

	return true;
}

/*ReadShort( short*, nbShort)
{
	short firstRec[8] = {0};
			for(int i=0; i<8; i++)
				VERIFY( mtw.Read( &(firstRec[i]), 2) == 2);
}

ReadLong( long*, nbLong)
{
}
*/

array<unsigned char,4> IntToByteArray(long value)
{
	array<unsigned char, 4> bytes;
	for (int i = 0; i < 4; i++)
	{
		bytes[i] = ((value >> (8 * i)) & 0XFF);
	}

	return bytes;
}



bool CMiniTab2CSVDlg::ReadVersionVAX(CFile& mtw, CMTWColomnArray& colArray)
{
	CStringA head;

	mtw.SeekToBegin();

	short h1;
	VERIFY(mtw.Read(&h1, 2) == 2);
	VERIFY(mtw.Read(head.GetBufferSetLength(80), 80) == 80);
	head.MakeLower();

	short maxSize = 514;
	if (head.Find("release  5.1") >= 0 || head.Find("release 82.1") >= 0)
	{
		maxSize = 161;
	}


	//read cariage return
	char CRLF[2] = { 0 };
	VERIFY(mtw.Read(&CRLF, 2));

	//determine if there are CRLF or LF
	short CRLFSize = CRLF[0] == 10 ? 1 : 2;
	mtw.Seek(-2, CFile::current);//rewind

	//reread and verify
	VERIFY(mtw.Read(&CRLF, CRLFSize) == CRLFSize);
	(CRLFSize == 1) ? ASSERT(CRLF[0] == 10) : ASSERT(CRLF[0] == 13 && CRLF[1] == 10);


	if (head.Find("release 6.1", 52) >= 0 ||
		head.Find("release 7.1", 52) >= 0 ||
		head.Find("release  5.1") >= 0 ||
		head.Find("release 82.1") >= 0)
	{

		while (mtw.GetPosition() < mtw.GetLength() - 1)
		{
			//read a short???
			short h2 = 0;
			VERIFY(mtw.Read(&h2, 2) == 2);

			//read 6 long
			long firstRec[6] = { 0 };
			for (int i = 0; i < 6; i++)
				VERIFY(mtw.Read(&(firstRec[i]), 4) == 4);

			//read cariage return
			VERIFY(mtw.Read(&CRLF, CRLFSize) == CRLFSize);
			(CRLFSize == 1) ? ASSERT(CRLF[0] == 10) : ASSERT(CRLF[0] == 13 && CRLF[1] == 10);

			VERIFY(mtw.Read(&h2, 2) == 2);

			if (firstRec[0] == 2 || firstRec[0] == 3 || firstRec[0] == 4 || firstRec[0] == 127)
			{
				INT_PTR pos = colArray.GetSize();
				colArray.SetSize(pos + 1);
				CMTWColomn& col = colArray[pos];

				col.m_no = firstRec[1];
				col.m_data.SetSize(firstRec[2]);
				col.m_nbNoData = firstRec[3];
				if (firstRec[4] != 0)
				{
					for (int j = 0; j < 2; j++)
					{
						CStringA number;
						number.Format("%d", firstRec[4 + j]);
						for (int k = 0; k < 4; k++)
						{
							char c = atoi(number.Mid(k * 2, 2)) + 32;
							if (::iswprint(c))
								col.m_name += c;
						}
					}
				}

				if (col.m_name.IsEmpty())
				{
					switch (firstRec[0])
					{
					case 2: col.m_name.Format("Constant %d", col.m_no); break;
					case 3: col.m_name.Format("Column %d", col.m_no); break;
					case 4: col.m_name.Format("Matrix %d", col.m_no); break;
					case 127:col.m_name.Format("Unknown %d", col.m_no); break;
					default: ASSERT(false);
					}
				}

				int nbRead = 0;
				for (int i = 0; i < firstRec[2]; i++, nbRead++)
				{
					char test1;
					VERIFY(mtw.Read(&test1, 1));
					
					if (test1 == -1)
					{
						int i;
						i = 0;
						mtw.Seek(-1, CFile::current);//have to determine when rewind and not???
					}
					else
					{
						char test2;
						VERIFY(mtw.Read(&test2, 1));
						if (test1 == '\r' && test2 == '\n')
						{
							//probably a /r added in conversion between vax and windows...
							mtw.Seek(-1, CFile::current);//rewind
						}
						else
						{
							mtw.Seek(-2, CFile::current);//rewind
						}
					}


					if (nbRead == maxSize)
					{
						VERIFY(mtw.Read(&CRLF, CRLFSize) == CRLFSize);
						(CRLFSize == 1) ? ASSERT(CRLF[0] == 10) : ASSERT(CRLF[0] == 13 && CRLF[1] == 10);

						VERIFY(mtw.Read(&h2, 2) == 2);
						ASSERT(h2 == 0 || h2 == 2);
						nbRead = 0;
					}

					long tmp;
					VERIFY(mtw.Read(&tmp, 4) == 4);
					ULONGLONG test_pos = mtw.GetPosition();

					array<unsigned char,4> test_tmp = IntToByteArray(tmp);
					
					


					float f1 = 0;
					vaxfloat(&tmp, &f1, 1);
					ASSERT((f1>-10000 && f1<-0.000001) || f1==0 || (f1 > 0.0001 && f1 < 100000) );
					col.m_data[i] = f1;


				}
			}
			else
			{
				ASSERT(false);
				AfxMessageBox(_T("type de données non supporter"));
			}

			ULONGLONG test_pos = mtw.GetPosition();

			//read cariage return
			VERIFY(mtw.Read(&CRLF, CRLFSize) == CRLFSize);
			(CRLFSize == 1) ? ASSERT(CRLF[0] == 10) : ASSERT(CRLF[0] == 13 && CRLF[1] == 10);
		}
	}
	else
	{
		ASSERT(false);
		AfxMessageBox(_T("release non supporter"));
	}

	return true;
}

void CMiniTab2CSVDlg::ReadVersion80(CFile& mtw, CMTWColomnArray& colArray)
{
	CStringA head;

	//bool bHaveLF = true;
	short sizeLF = 1;
	char c = 0;
	mtw.Seek(0x57, CFile::begin);
	VERIFY(mtw.Read(&c, 1) == 1);
	if (c != 10)
		sizeLF = 0;

	mtw.SeekToBegin();

	short h1;
	VERIFY(mtw.Read(&h1, 2) == 2);
	VERIFY(mtw.Read(head.GetBufferSetLength(10), 10) == 10);
	head.MakeLower();



	//read cariage return
	char LF = 10;
	int nbValue = 0;
	short h2 = 0;

	VERIFY(mtw.Read(&LF, sizeLF) == sizeLF);

	CStringA comment;
	VERIFY(mtw.Read(comment.GetBufferSetLength(74), 74) == 74);
	comment.ReleaseBuffer();

	VERIFY(mtw.Read(&LF, sizeLF) == sizeLF);
	VERIFY(mtw.Read(&h2, 2) == 2);
	ASSERT(LF == 10);

	short unknow1 = 0;
	short nbCol = 50;
	VERIFY(mtw.Read(&unknow1, 2) == 2); nbValue++;
	VERIFY(mtw.Read(&nbCol, 2) == 2); nbValue++;


	short nbRow[50] = { 0 };
	//CMTWColomn& col = colArray[0];

	for (int i = 0; i < nbCol; i++)
	{
		if (nbValue == 62)
		{
			VERIFY(mtw.Read(&LF, sizeLF) == sizeLF);
			ASSERT(LF == 10);
			VERIFY(mtw.Read(&h2, 2) == 2);
			//ASSERT( h2 == 0);
			nbValue = 0;
		}

		VERIFY(mtw.Read(&(nbRow[i]), 2) == 2);
		nbValue++;
	}

	short colPos[80] = { 0 };
	for (int i = 0; i < 80; i++)
	{
		if (nbValue == 62)
		{
			VERIFY(mtw.Read(&LF, sizeLF) == sizeLF);
			ASSERT(LF == 10);
			VERIFY(mtw.Read(&h2, 2) == 2);
			//ASSERT( h2 == 0);
			nbValue = 0;
		}

		VERIFY(mtw.Read(&(colPos[i]), 2) == 2);
		nbValue++;
	}

	short unknow2 = 0;
	short unknow3 = 0;
	short nbData = 0;
	VERIFY(mtw.Read(&unknow2, 2) == 2); nbValue++;
	VERIFY(mtw.Read(&unknow3, 2) == 2); nbValue++;
	VERIFY(mtw.Read(&nbData, 2) == 2); nbValue++;

	UtilWin::CFloatArray data;
	data.SetSize(nbData);

	nbValue = 0;
	VERIFY(mtw.Read(&LF, sizeLF) == sizeLF);
	VERIFY(mtw.Read(&h2, 2) == 2);
	ASSERT(LF == 10);

	for (int i = 0; i < nbData; i++)
	{
		if (nbValue == 31)
		{
			VERIFY(mtw.Read(&LF, sizeLF) == sizeLF);
			ASSERT(LF == 10);
			VERIFY(mtw.Read(&h2, 2) == 2);
			//ASSERT( h2 == 0);
			nbValue = 0;
		}

		long tmp;
		VERIFY(mtw.Read(&tmp, 4) == 4);
		nbValue++;

		float f1 = 0;
		vaxfloat(&tmp, &f1, 1);
		data[i] = f1;
	}
	/*
		for(int i=0; i<24; i++)
		{
			int pos = colArray.GetSize();
			colArray.SetSize( pos + 1);
			CMTWColomn& col = colArray[pos];

			col.m_no = i+1;
			col.m_name.Format("col : %d",  i+1);
			col.m_data.SetSize(24);
			col.m_nbNoData = 0;

			for(int j=0; j<24; j++)
			{
				if( nbValue == 31)
				{
					VERIFY( mtw.Read( &LF, 1) == 1);
					ASSERT( LF == 10);
					VERIFY( mtw.Read( &h2, 2) == 2);
					//ASSERT( h2 == 0);
					nbValue = 0;
				}

				long tmp;
				VERIFY( mtw.Read(&tmp, 4)==4);
				nbValue++;
				aaa++;

				float f1=0;
				vaxfloat(&tmp,&f1,1);
				col.m_data[j] = f1;

			}
		}
	*/

	VERIFY(mtw.Read(&LF, sizeLF) == sizeLF);
	ASSERT(LF == 10);

	//read col header
//	ULONGLONG pos = mtw.GetPosition();

	short valName[250] = { 0 };
	const short nbValName[5] = { 62,38,50,62,38 };

	nbValue = 0;
	for (int i = 0; i < 5; i++)
	{
		VERIFY(mtw.Read(&h2, 2) == 2);
		for (int j = 0; j < nbValName[i]; j++)
		{
			VERIFY(mtw.Read(&valName[nbValue++], 2) == 2);
		}

		VERIFY(mtw.Read(&LF, sizeLF) == sizeLF);
		ASSERT(LF == 10);
	}


	colArray.SetSize(nbCol);
	//assign data
	for (int i = 0; i < nbCol; i++)
	{
		CMTWColomn& col = colArray[i];
		col.m_no = i + 1;
		//col.m_name.Format( "col %d", i+1;
		col.m_nbNoData = 0;
		if (nbRow[i] > 0)
		{
			col.m_data.SetSize(nbRow[i]);
			for (int j = 0; j < nbRow[i]; j++)
			{
				col.m_data[j] = data[colPos[i] + j];
			}

			//assign title
			for (int j = 0; j < 5; j++)
			{
				int pos = i + 50 * j;
				short val = valName[pos];
				if (val > 0)
				{
					CStringA number;
					number.Format("%d", val);
					if (number.GetLength() == 3)
						number = '0' + number;
					for (int k = 0; k < 2; k++)
					{
						char c = atoi(number.Mid(k * 2, 2));
						if (c >= 1 && c <= 10)
							c += 47;
						else if (c >= 11 && c <= 36)c += 54;
						else if (c == 45) c = ' ';
						else if (c == 38) c = '.';
						else if (c == 57) c = '#';
						else if (c == 58) c = '%';
						else if (c == 39) c = '_';
						else if (c == 40) c = '-';
						else if (c == 37) c = '/';
						else
						{
							ASSERT(c == 0);
						}

						if (::iswprint(c))
							col.m_name += c;
						else
						{
							ASSERT(c == 0);
						}
					}
				}
			}
		}
	}
	//eliminate empty column
	for (int i = 0; i < colArray.GetSize(); i++)
	{
		if (colArray[i].m_data.GetSize() == 0)
		{
			colArray.RemoveAt(i);
			i--;
		}
		else
		{
			if (colArray[i].m_name.IsEmpty())
			{
				colArray[i].m_name.Format("col %d", i + 1);
			}
		}
	}

	//add constant 0-99
	for (int i = 0; i < 2; i++)
	{
		INT_PTR pos = colArray.GetSize();
		colArray.SetSize(pos + 1);
		//assign data
		CMTWColomn& col = colArray[pos];
		col.m_no = long(pos + 1);
		col.m_name.Format("constant%d", i + 1);
		col.m_nbNoData = 0;
		col.m_data.SetSize(50);
		for (int j = 0; j < 50; j++)
		{
			col.m_data[j] = data[i * 50 + j];
		}
	}
}

void CMiniTab2CSVDlg::OnBnClickedInputBrowse()
{
	CString lastDir;
	GetDlgItem(IDC_INPUT)->GetWindowText(lastDir);

	CFileDialog openDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Minitab workspace(*.min,*.mtw,*.mtb)|*.min;*.mtw;*.mtb||"), this);
	//openDlg.m_ofn.lpstrInitialDir = CString::GetLocation(lastDir);
	openDlg.m_ofn.nFilterIndex = m_nFilterIndex;

	if (openDlg.DoModal() == IDOK)
	{
		CAppOption option;
		m_nFilterIndex = openDlg.m_ofn.nFilterIndex;
		option.WriteProfileInt(_T("index"), m_nFilterIndex);
		GetDlgItem(IDC_INPUT)->SetWindowText(openDlg.GetPathName());
	}

	/*CSelectDirectory dlg(this, "*.mtw");
	GetDlgItem(IDC_INPUT)->GetWindowText(dlg.m_strPath);

	if( dlg.DoModal() == IDOK)
	{
		GetDlgItem(IDC_INPUT)->SetWindowText(dlg.m_strPath);
	}
	*/
}

void CMiniTab2CSVDlg::OnBnClickedOutputBrowse()
{
	/*CSelectDirectory dlg(this, "*.CSV");
	GetDlgItem(IDC_OUTPUT)->GetWindowText(dlg.m_strPath);

	if( dlg.DoModal() == IDOK)
	{
		GetDlgItem(IDC_OUTPUT)->SetWindowText(dlg.m_strPath);
	}
	*/
}
