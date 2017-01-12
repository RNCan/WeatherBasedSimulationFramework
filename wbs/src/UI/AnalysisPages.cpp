//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include <vector>

#include "FileManager/FileManager.h"


#include "UI/Common/EnableGroupboxControls.h"
#include "UI/Common/HTMLTree/XMessageBox.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWin.h"
#include "UI/AnalysisPages.h"

#include "WeatherBasedSimulationString.h"


namespace WBSF
{

	using namespace DIMENSION;

#ifdef _DEBUG
#undef THIS_FILE
	static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


	//**************************************************************
	BEGIN_MESSAGE_MAP(CTemporalReferenceEdit, CMFCMaskedEdit)
	END_MESSAGE_MAP()

	CTemporalReferenceEdit::CTemporalReferenceEdit(CTM TM)
	{
		m_TM = TM;
		//m_type=CTRef::UNKNOWN;
		//	m_TM=CTRef::GetTM(DAILY,FOR_EACH_YEAR);
	}

	CTRef CTemporalReferenceEdit::GetTemporalReference()const
	{
		CString text;
		CWnd::GetWindowText(text);
		CTRef ref(UtilWin::ToUTF8(text), m_TM);

		return ref;
	}

	void CTemporalReferenceEdit::PreSubclassWindow()
	{
		UpdateMask();
	}

	void CTemporalReferenceEdit::SetTemporalReference(CTRef ref)
	{
		m_TM = ref.GetTM();
		UpdateMask();

		CString str;
		short m = short(ref.GetMonth() + 1);
		short d = short(ref.GetDay() + 1);
		short h = short(ref.GetHour());
		long r = long(ref.GetRef());


		CString y;
		char sep = '/';
		char sepy = ' ';
		if (ref.IsAnnual())
		{
			sepy = sep;
			y.Format(_T("%04d"), short(ref.GetYear()));
		}

		switch (ref.GetType())
		{
		case CTRef::HOURLY:	str.Format(_T("%s%c%02d%c%02d%c%02d"), y, sepy, m, sep, d, sep, h); break;
		case CTRef::DAILY:	str.Format(_T("%s%c%02d%c%02d"), y, sepy, m, sep, d); break;
		case CTRef::MONTHLY:str.Format(_T("%s%c%02d"), y, sepy, m); break;
		case CTRef::ANNUAL:	str.Format(_T("%s"), y); break;
		case CTRef::ATEMPORAL:str.Format(_T("%d"), r); break;
		case CTRef::UNKNOWN:	break;
		default: ASSERT(false);
		}

		str.Trim();

		SetWindowText(str);
	}

	void CTemporalReferenceEdit::UpdateMask()
	{
		//CMFCMaskedEdit& edit = (CMFCMaskedEdit& )rControl.GetWnd();
		switch (m_TM.Mode())
		{
		case CTM::FOR_EACH_YEAR:
		{
			switch (m_TM.Type())
			{
			case CTM::ANNUAL:	EnableMask(_T("****"), _T("____"), _T(' ')); break;
			case CTM::MONTHLY:	EnableMask(_T("**** DD"), _T("____/__"), _T(' ')); break;
			case CTM::DAILY:	EnableMask(_T("**** DD DD"), _T("____/__/__"), _T(' ')); break;
			case CTM::HOURLY:	EnableMask(_T("**** DD DD DD"), _T("____/__/__/__"), _T(' ')); break;
			case CTM::ATEMPORAL:EnableMask(_T("AAAAAAAAAA"), _T("__________"), _T(' ')); break;
			default: ASSERT(false);
			}
			break;
		}
		case CTM::OVERALL_YEARS:
		{
			switch (m_TM.Mode())
			{
			case CTM::ANNUAL:		SetReadOnly(true); break;
			case CTM::MONTHLY:		EnableMask(_T("DD"), _T("__"), _T(' ')); break;
			case CTM::DAILY:		EnableMask(_T("DD DD"), _T("__/__"), _T(' ')); break;
			case CTM::HOURLY:		EnableMask(_T("DD DD DD"), _T("__/__/__"), _T(' ')); break;
			case CTM::ATEMPORAL:	EnableMask(_T("AAAAAAAAAA"), _T("__________"), _T(' ')); break;
			default: ASSERT(false);
			}
			break;
		}
		default: assert(false);
		}

		EnableSelectByGroup(FALSE);
		EnableGetMaskedCharsOnly(FALSE);
		EnableSetMaskedCharsOnly(FALSE);
		SetValidChars(_T(" 0123456789-"));

	}

	//************************************************************
	//CAnalysisGeneralPage property page

	BEGIN_MESSAGE_MAP(CAnalysisPage, CMFCPropertyPage)
	END_MESSAGE_MAP()

	CAnalysisPage::CAnalysisPage(UINT nIDTemplate, UINT nIDCaption) :
		CMFCPropertyPage(nIDTemplate, nIDCaption)
	{
		m_pAnalysis = NULL;
	}

	CAnalysisPage::~CAnalysisPage()
	{
	}

	void CAnalysisPage::Initialise(const CExecutablePtr& pParent, CAnalysis& analysis)
	{
		m_pParent = pParent;
		m_pAnalysis = &analysis;
	}



	//************************************************************
	//CAnalysisGeneralPage
	BEGIN_MESSAGE_MAP(CAnalysisGeneralPage, CAnalysisPage)
	END_MESSAGE_MAP()

	CAnalysisGeneralPage::CAnalysisGeneralPage() :
		CAnalysisPage(CAnalysisGeneralPage::IDD)
	{
	}

	CAnalysisGeneralPage::~CAnalysisGeneralPage()
	{
	}

	void CAnalysisGeneralPage::DoDataExchange(CDataExchange* pDX)
	{
		CAnalysisPage::DoDataExchange(pDX);

		//initiale dialog ctrl
		DDX_Control(pDX, IDC_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_INTERNAL_NAME, m_internalNameCtrl);
		DDX_Control(pDX, IDC_DESCRIPTION, m_descriptionCtrl);


		if (pDX->m_bSaveAndValidate)
			GetAnalysisFromInterface(*m_pAnalysis);
		else SetAnalysisToInterface(*m_pAnalysis);

	}

	void CAnalysisGeneralPage::GetAnalysisFromInterface(CAnalysis& analysis)
	{
		analysis.SetName(m_nameCtrl.GetString());
		analysis.SetDescription(m_descriptionCtrl.GetString());
	}

	void CAnalysisGeneralPage::SetAnalysisToInterface(const CAnalysis& analysis)
	{
		m_nameCtrl.SetWindowText(analysis.GetName());
		m_internalNameCtrl.SetWindowText(analysis.GetInternalName());
		m_descriptionCtrl.SetWindowText(analysis.GetDescription());
	}



	//************************************************************
	//CAnalysisWherePage property page
	BEGIN_MESSAGE_MAP(CAnalysisWherePage, CAnalysisPage)
		ON_BN_CLICKED(IDC_SELECT_LOCATION, &OnSelectChange)
	END_MESSAGE_MAP()

	CAnalysisWherePage::CAnalysisWherePage() :
		CAnalysisPage(CAnalysisWherePage::IDD)
	{}

	CAnalysisWherePage::~CAnalysisWherePage()
	{}

	void CAnalysisWherePage::DoDataExchange(CDataExchange* pDX)
	{
		CAnalysisPage::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_SELECT_LOCATION, m_selectCtrl);
		DDX_Control(pDX, IDC_LOCATION, m_locationCtrl);

		if (pDX->m_bSaveAndValidate)
			GetAnalysisFromInterface(*m_pAnalysis);
		else SetAnalysisToInterface(*m_pAnalysis);

	}

	void CAnalysisWherePage::GetAnalysisFromInterface(CAnalysis& analysis)
	{
		m_pAnalysis->GetWindow().m_locations.clear();

		analysis.GetWindow().m_bSelectLocation = m_selectCtrl.GetCheck();
		if (analysis.GetWindow().m_bSelectLocation)
			m_pAnalysis->GetWindow().m_locations = m_locationCtrl.GetSelection();

	}

	void CAnalysisWherePage::SetAnalysisToInterface(const CAnalysis& analysis)
	{
		bool bSelection = analysis.GetWindow().m_bSelectLocation;

		m_selectCtrl.SetCheck(bSelection);
		if (bSelection && !m_locationCtrl.IsInit())
		{
			FillLocation();
			m_locationCtrl.SetSelection(analysis.GetWindow().m_locations);
		}

		UpdateCtrl();
	}

	void CAnalysisWherePage::FillLocation()
	{
		if (!m_locationCtrl.IsInit())
		{
			m_pParent->GetParentInfo(WBSF::GetFM(), m_info, LOCATION);
			m_locationCtrl.SetPossibleValues(m_info.GetDimensionStr(DIMENSION::LOCATION));
		}
	}

	void CAnalysisWherePage::OnSelectChange()
	{
		if (!m_locationCtrl.IsInit())
		{
			FillLocation();
			m_locationCtrl.SetSelection(m_pAnalysis->GetWindow().m_locations);
		}

		UpdateCtrl();
	}


	void CAnalysisWherePage::UpdateCtrl()
	{
		bool bSelection = m_selectCtrl.GetCheck();
		m_locationCtrl.EnableWindow(bSelection);
	}
	//************************************************************
	// CAnalysisWhenPage property page

	BEGIN_MESSAGE_MAP(CAnalysisWhenPage, CAnalysisPage)
		ON_EN_CHANGE(IDC_BEGIN, &UpdateCtrl)
		ON_EN_CHANGE(IDC_END, &UpdateCtrl)
		ON_BN_CLICKED(IDC_SELECT_PERIOD, &UpdateCtrl)
		ON_BN_CLICKED(IDC_USE_CURRENT_DATE1, &UpdateCtrl)
		ON_BN_CLICKED(IDC_USE_CURRENT_DATE2, &UpdateCtrl)
	END_MESSAGE_MAP()

	CAnalysisWhenPage::CAnalysisWhenPage() :
		CAnalysisPage(CAnalysisWhenPage::IDD)
	{
		m_inversed = -1;
	}

	void CAnalysisWhenPage::InitialiseCtrl()
	{

		ERMsg msg = m_pParent->GetParentInfo(WBSF::GetFM(), m_info, TIME_REF);
		CTM TM = m_info.m_period.GetTM();

		//if begin and end are not initialize 
		//we initialize it with the simulation
		CTPeriod& period = m_pAnalysis->GetWindow().m_period;
		if (!period.IsInit())
		{
			period = m_info.m_period;
		}
		//else 

		if (period.GetTM() != TM)
		{
			period.Transform(TM);
		}

		//a faire attention: on doit mettre les model et les
		//reference sur la meme longeur d'onde
		//	short outputType = period.Begin().m_type;//CTRef::ANNUAL_DAILY; //m_pParent ->GetOutputType();

		//m_beginCtrl.SetType(outputType);
		//m_endCtrl.SetType(outputType);

		if (!msg)
			UtilWin::SYShowMessage(msg, this);
	}

	CAnalysisWhenPage::~CAnalysisWhenPage()
	{
	}

	void CAnalysisWhenPage::DoDataExchange(CDataExchange* pDX)
	{
		CAnalysisPage::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_SELECT_PERIOD, m_selectPeriodCtrl);
		DDX_Control(pDX, IDC_BEGIN, m_beginCtrl);
		DDX_Control(pDX, IDC_END, m_endCtrl);
		DDX_Control(pDX, IDC_USE_CURRENT_DATE1, m_useCurrentDataCtrl1);
		DDX_Control(pDX, IDC_SHIFT1, m_shiftCtrl1);
		DDX_Control(pDX, IDC_USE_CURRENT_DATE2, m_useCurrentDataCtrl2);
		DDX_Control(pDX, IDC_SHIFT2, m_shiftCtrl2);
		DDX_Control(pDX, IDC_CONTINUOUS, m_continuousCtrl);
		DDX_Control(pDX, IDC_YEAR_BY_YEAR, m_yearByYearCtrl);

		if (pDX->m_bSaveAndValidate)
			GetAnalysisFromInterface(*m_pAnalysis);
		else SetAnalysisToInterface(*m_pAnalysis);

	}


	void CAnalysisWhenPage::GetAnalysisFromInterface(CAnalysis& analysis)
	{
		CAnalysisWindow& window = analysis.GetWindow();

		window.m_bSelectPeriod = m_selectPeriodCtrl.GetCheck();
		window.m_period.Begin() = m_beginCtrl.GetTemporalReference();
		window.m_period.End() = m_endCtrl.GetTemporalReference();

		short type = GetCheckedRadioButton(IDC_CONTINUOUS, IDC_YEAR_BY_YEAR) - IDC_CONTINUOUS;
		window.m_period.SetType(type);

		window.m_bUseCurrentDate1 = m_useCurrentDataCtrl1.GetCheck();
		window.m_shift1 = WBSF::ToInt(m_shiftCtrl1.GetString());

		window.m_bUseCurrentDate2 = m_useCurrentDataCtrl2.GetCheck();
		window.m_shift2 = WBSF::ToInt(m_shiftCtrl2.GetString());
	}

	void CAnalysisWhenPage::SetAnalysisToInterface(const CAnalysis& analysis)
	{
		InitialiseCtrl();

		const CAnalysisWindow& window = analysis.GetWindow();

		m_selectPeriodCtrl.SetCheck(window.m_bSelectPeriod);
		m_beginCtrl.SetTemporalReference(window.m_period.Begin());
		m_endCtrl.SetTemporalReference(window.m_period.End());

		short ID = IDC_CONTINUOUS + window.m_period.GetType();
		CheckRadioButton(IDC_CONTINUOUS, IDC_YEAR_BY_YEAR, ID);

		m_useCurrentDataCtrl1.SetCheck(window.m_bUseCurrentDate1);
		m_shiftCtrl1.SetWindowText(ToString(window.m_shift1));

		m_useCurrentDataCtrl2.SetCheck(window.m_bUseCurrentDate2);
		m_shiftCtrl2.SetWindowText(ToString(window.m_shift2));

		UpdateCtrl();
	}

	void CAnalysisWhenPage::UpdateCtrl()
	{
		bool bEnable = m_selectPeriodCtrl.GetCheck();
		bool bEnableBegin = bEnable && !m_useCurrentDataCtrl1.GetCheck();
		bool bEnableEnd = bEnable && !m_useCurrentDataCtrl2.GetCheck();
		bool bEnableShift1 = bEnable && m_useCurrentDataCtrl1.GetCheck();
		bool bEnableShift2 = bEnable && m_useCurrentDataCtrl2.GetCheck();

		m_beginCtrl.EnableWindow(bEnableBegin);
		m_endCtrl.EnableWindow(bEnableEnd);

		m_continuousCtrl.EnableWindow(bEnable);
		m_yearByYearCtrl.EnableWindow(bEnable);

		m_useCurrentDataCtrl1.EnableWindow(bEnable);
		m_shiftCtrl1.EnableWindow(bEnableShift1);
		m_useCurrentDataCtrl2.EnableWindow(bEnable);
		m_shiftCtrl2.EnableWindow(bEnableShift2);

		CTRef begin(m_beginCtrl.GetTemporalReference());
		CTRef end(m_endCtrl.GetTemporalReference());

		short inversed = begin.IsInversed(end) ? 1 : 0;
		//if( m_inversed==-1 || inversed!=m_inversed)
		{
			m_inversed = inversed;
			UINT ID1 = m_inversed ? IDB_CONTINUOUS_PERIOD2 : IDB_CONTINUOUS_PERIOD1;
			UINT ID1C = m_inversed ? IDB_CONTINUOUS_PERIOD2_CHECKED : IDB_CONTINUOUS_PERIOD1_CHECKED;
			UINT ID2 = m_inversed ? IDB_YEAR_BY_YEAR2 : IDB_YEAR_BY_YEAR1;
			UINT ID2C = m_inversed ? IDB_YEAR_BY_YEAR2_CHECKED : IDB_YEAR_BY_YEAR1_CHECKED;


			m_continuousCtrl.m_bTopImage = TRUE;
			m_continuousCtrl.m_nAlignStyle = CMFCButton::ALIGN_CENTER;
			m_continuousCtrl.SetImage(ID1);
			m_continuousCtrl.SetCheckedImage(ID1C);
			m_continuousCtrl.SizeToContent();
			//m_continuousCtrl.Invalidate();
			m_continuousCtrl.RedrawWindow();

			m_yearByYearCtrl.m_bTopImage = TRUE;
			m_yearByYearCtrl.m_nAlignStyle = CMFCButton::ALIGN_CENTER;
			m_yearByYearCtrl.SetImage(ID2);
			m_yearByYearCtrl.SetCheckedImage(ID2C);
			m_yearByYearCtrl.SizeToContent();
			//m_yearByYearCtrl.Invalidate();
			m_yearByYearCtrl.RedrawWindow();
			//UpdateWindow();
		}

		int v = NB_STATIC;
		for (int i = 0; i < NB_STATIC; i++)
			GetStatic(i).EnableWindow(bEnable);
	}

	void CAnalysisWhenPage::OnUpdateCtrl(NMHDR *pNMHDR, LRESULT *pResult)
	{
		UpdateCtrl();
		*pResult = 0;
	}


	/////////////////////////////////////////////////////////////////////////////
	// CAnalysisWhatPage property page
	BEGIN_MESSAGE_MAP(CAnalysisWhatPage, CAnalysisPage)
		ON_BN_CLICKED(IDC_SELECT_VARIABLE, &UpdateCtrl)
	END_MESSAGE_MAP()

	CAnalysisWhatPage::CAnalysisWhatPage() :
		CAnalysisPage(CAnalysisWhatPage::IDD)
	{
	}

	CAnalysisWhatPage::~CAnalysisWhatPage()
	{
	}

	void CAnalysisWhatPage::DoDataExchange(CDataExchange* pDX)
	{
		CAnalysisPage::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_SELECT_VARIABLE, m_selectCtrl);
		DDX_Control(pDX, IDC_VARIABLE, m_parametersetCtrl);

		if (pDX->m_bSaveAndValidate)
			GetAnalysisFromInterface(*m_pAnalysis);
		else
			SetAnalysisToInterface(*m_pAnalysis);

	}

	void CAnalysisWhatPage::GetAnalysisFromInterface(CAnalysis& analysis)
	{
		m_pAnalysis->GetWindow().m_parametersVariations.clear();

		analysis.GetWindow().m_bSelectParametersVariations = m_selectCtrl.GetCheck();
		if (analysis.GetWindow().m_bSelectParametersVariations)
		{
			m_pAnalysis->GetWindow().m_parametersVariations = m_parametersetCtrl.GetSelection();
		}

	}

	std::vector<int> A;

	void CAnalysisWhatPage::SetAnalysisToInterface(const CAnalysis& analysis)
	{
		bool bSelection = analysis.GetWindow().m_bSelectParametersVariations;
		m_selectCtrl.SetCheck(bSelection);

		if (!m_parametersetCtrl.IsInit())
		{
			FillParametersVariations();
			m_parametersetCtrl.SetSelection(analysis.GetWindow().m_parametersVariations);
		}

		UpdateCtrl();
	}


	void CAnalysisWhatPage::FillParametersVariations()
	{
		m_pParent->GetParentInfo(WBSF::GetFM(), m_info, PARAMETER);
		m_parametersetCtrl.SetPossibleValues(m_info.GetDimensionStr(PARAMETER));
	}


	void CAnalysisWhatPage::UpdateCtrl()
	{
		bool bSelection = m_selectCtrl.GetCheck();
		m_parametersetCtrl.EnableWindow(bSelection);
	}

	/////////////////////////////////////////////////////////////////////////////
	// CAnalysisWhichPage property page
	BEGIN_MESSAGE_MAP(CAnalysisWhichPage, CAnalysisPage)
		ON_BN_CLICKED(IDC_SELECT_VARIABLE, &UpdateCtrl)
	END_MESSAGE_MAP()

	CAnalysisWhichPage::CAnalysisWhichPage() :
		CAnalysisPage(CAnalysisWhichPage::IDD)
	{
	}

	CAnalysisWhichPage::~CAnalysisWhichPage()
	{
	}

	void CAnalysisWhichPage::DoDataExchange(CDataExchange* pDX)
	{
		CAnalysisPage::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_SELECT_VARIABLE, m_selectCtrl);
		DDX_Control(pDX, IDC_VARIABLE, m_variableCtrl);


		if (pDX->m_bSaveAndValidate)
			GetAnalysisFromInterface(*m_pAnalysis);
		else SetAnalysisToInterface(*m_pAnalysis);

	}

	void CAnalysisWhichPage::GetAnalysisFromInterface(CAnalysis& analysis)
	{
		m_pAnalysis->GetWindow().m_variables.clear();

		analysis.GetWindow().m_bSelectVariable = m_selectCtrl.GetCheck();
		if (analysis.GetWindow().m_bSelectVariable)
		{
			m_pAnalysis->GetWindow().m_variables = m_variableCtrl.GetSelection();
		}

	}

	void CAnalysisWhichPage::SetAnalysisToInterface(const CAnalysis& analysis)
	{
		bool bSelection = analysis.GetWindow().m_bSelectVariable;

		m_selectCtrl.SetCheck(bSelection);

		if (!m_variableCtrl.IsInit())
		{
			FillVariable();
			m_variableCtrl.SetSelection(analysis.GetWindow().m_variables);
		}

		UpdateCtrl();
	}


	void CAnalysisWhichPage::FillVariable()
	{
		m_pParent->GetParentInfo(WBSF::GetFM(), m_info, VARIABLE);
		m_variableCtrl.SetPossibleValues(m_info.GetDimensionStr(VARIABLE));
	}


	void CAnalysisWhichPage::UpdateCtrl()
	{
		bool bSelection = m_selectCtrl.GetCheck();
		m_variableCtrl.EnableWindow(bSelection);
	}


	//************************************************************
	BEGIN_MESSAGE_MAP(CMFCImage, CMFCButton)
		ON_WM_LBUTTONDOWN()
		ON_WM_MOUSEMOVE()
	END_MESSAGE_MAP()

	//************************************************************
	// CAnalysisHowPage property page
	BEGIN_MESSAGE_MAP(CAnalysisHowPage, CAnalysisPage)
		ON_BN_CLICKED(IDC_SELECT_STATISTIC, &UpdateCtrl)
		ON_BN_CLICKED(IDC_STATISTIC, &UpdateCtrl)
		ON_BN_CLICKED(IDC_EVENT, &UpdateCtrl)
		ON_CBN_SELCHANGE(IDC_EVENT_TYPE, &UpdateCtrl)
		ON_CBN_SELCHANGE(IDC_TT_TYPE, &OnTTypeChange)
		ON_CBN_SELCHANGE(IDC_TT_MODE, &UpdateCtrl)
		ON_BN_CLICKED(IDC_SELECT_TIME_TRANSFORMATION, &UpdateCtrl)
		ON_WM_CTLCOLOR()
	END_MESSAGE_MAP()


	CAnalysisHowPage::CAnalysisHowPage() :
		CAnalysisPage(CAnalysisHowPage::IDD),
		m_previousStatisticCtrl(true),
		m_statisticTypeCtrl(true)
	{
	}

	CAnalysisHowPage::~CAnalysisHowPage()
	{
	}

	void CAnalysisHowPage::DoDataExchange(CDataExchange* pDX)
	{
		CAnalysisPage::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_SELECT_TIME_TRANSFORMATION, m_selectCtrl);
		DDX_Control(pDX, IDC_CT_TYPE, m_CTTypeCtrl);
		DDX_Control(pDX, IDC_CT_MODE, m_CTModeCtrl);
		DDX_Control(pDX, IDC_TT_TYPE, m_TTTypeCtrl);
		DDX_Control(pDX, IDC_TT_MODE, m_TTModeCtrl);
		DDX_Control(pDX, IDC_PREVIOUS_STATISTIC_TYPE, m_previousStatisticCtrl);


		DDX_Control(pDX, IDC_STATISTIC, m_statisticCtrl);
		DDX_Control(pDX, IDC_EVENT, m_eventCtrl);

		DDX_Control(pDX, IDC_STATISTIC_TYPE, m_statisticTypeCtrl);
		DDX_Control(pDX, IDC_EVENT_TYPE, m_eventTypeCtrl);
		DDX_Control(pDX, IDC_K, m_KCtrl);
		DDX_Control(pDX, IDC_DROP_YEAR, m_bDropYearCtrl);
		DDX_Control(pDX, IDC_MEAN_OVER_REPLICATION, m_meanOverReplicationCtrl);
		DDX_Control(pDX, IDC_MEAN_OVER_PARAMETERSET, m_meanOverParameterSetCtrl);
		DDX_Control(pDX, IDC_MEAN_OVER_LOCATION, m_meanOverLocationCtrl);
		DDX_Control(pDX, IDC_MODE_IMAGE, m_imageModeCtrl);



		if (pDX->m_bSaveAndValidate)
		{
			if (m_TTTypeCtrl.GetCount() == 0 && m_selectCtrl.GetCheck())
			{
				AfxMessageBox(IDS_INVALID_TEMPORAL_TRANS, MB_ICONEXCLAMATION);
				pDX->Fail();
			}

			GetAnalysisFromInterface(*m_pAnalysis);
		}
		else SetAnalysisToInterface(*m_pAnalysis);

	}




	void CAnalysisHowPage::GetAnalysisFromInterface(CAnalysis& analysis)
	{
		CAnalysisComputation& computation = analysis.GetComputation();

		computation.m_bSelectTimeTransformation = m_selectCtrl.GetCheck();

		computation.m_previousStatisticType = m_previousStatisticCtrl.GetCurSel();
		computation.m_TM = GetTM();

		computation.m_kind = GetComputationKind();

		computation.m_statisticType2 = m_statisticTypeCtrl.GetCurSel();
		computation.m_eventType = m_eventTypeCtrl.GetCurSel();
		computation.m_K = ToFloat(m_KCtrl.GetString());
		computation.m_bDropYear = m_bDropYearCtrl.GetCheck();

		computation.m_bMeanOverReplication = m_meanOverReplicationCtrl.GetCheck();
		computation.m_bMeanOverParameterSet = m_meanOverParameterSetCtrl.GetCheck();
		computation.m_bMeanOverLocation = m_meanOverLocationCtrl.GetCheck();
	}

	void CAnalysisHowPage::SetAnalysisToInterface(const CAnalysis& analysis)
	{
		const CAnalysisComputation& computation = analysis.GetComputation();
		m_selectCtrl.SetCheck(computation.m_bSelectTimeTransformation);

		if (!m_info.m_period.IsInit())
			m_pParent->GetParentInfo(WBSF::GetFM(), m_info, TIME_REF);


		m_sourceTM = m_info.m_period.GetTM();
		m_CTTypeCtrl.SetCurSel((int)m_sourceTM.Type());
		m_CTModeCtrl.SetCurSel((int)m_sourceTM.Mode());

		FillType();
		FillMode((int)computation.m_TM.Type());
		SetTM(computation.m_TM);

		SetComputationKind(computation.m_kind);

		m_previousStatisticCtrl.SetCurSel(computation.m_previousStatisticType);
		m_statisticTypeCtrl.SetCurSel(computation.m_statisticType2);
		m_eventTypeCtrl.SetCurSel(computation.m_eventType);
		m_KCtrl.SetWindowText(ToString(computation.m_K));
		m_bDropYearCtrl.SetCheck(computation.m_bDropYear);

		m_meanOverReplicationCtrl.SetCheck(computation.m_bMeanOverReplication);
		m_meanOverParameterSetCtrl.SetCheck(computation.m_bMeanOverParameterSet);
		m_meanOverLocationCtrl.SetCheck(computation.m_bMeanOverLocation);

		UpdateCtrl();
	}

	CTM CAnalysisHowPage::GetTM()
	{
		short TType = m_TTTypeCtrl.GetCurItemData();
		short TMode = m_TTModeCtrl.GetCurItemData();

		return CTM(TType, TMode);// (TType != -1 && TMode != -1) ? CTRef::GetTM(TType, TMode) : CTRef::GetTM(CTRef::ANNUAL, CTRef::FOR_EACH_YEAR);
	}

	void CAnalysisHowPage::SetTM(CTM TM)
	{
		m_TTTypeCtrl.SelectFromItemData((int)TM.Type());
		m_TTModeCtrl.SelectFromItemData((int)TM.Mode());
	}

	void CAnalysisHowPage::FillType()
	{
		WBSF::StringVector TYPE_NAME = Tokenize(GetString(IDS_CMN_OUTPUT_TYPE_NAME), ";|");
		m_TTTypeCtrl.ResetContent();

		for (int t = 0; t < CTRef::NB_REFERENCE; t++)
		{
			if (m_sourceTM.IsTypeAvailable(t))
			{
				int pos = m_TTTypeCtrl.AddString(TYPE_NAME[t].c_str());
				m_TTTypeCtrl.SetItemData(pos, t);
			}
		}
	}

	void CAnalysisHowPage::FillMode(short t)
	{
		WBSF::StringVector MODE_NAME = Tokenize(GetString(IDS_CMN_TIME_FORMAT_DATAHEAD1), ";|");

		int curSel = m_TTModeCtrl.GetCurItemData();
		m_TTModeCtrl.ResetContent();

		for (int m = 0; m < CTM::NB_MODE; m++)
		{
			if (m_sourceTM.IsModeAvailable(CTM(t, m)))
			{
				int pos = m_TTModeCtrl.AddString(MODE_NAME[m].c_str());
				m_TTModeCtrl.SetItemData(pos, m);
			}
		}

		m_TTModeCtrl.SelectFromItemData(curSel);
	}

	void CAnalysisHowPage::OnTTypeChange()
	{
		FillMode(m_TTTypeCtrl.GetCurItemData());
		UpdateCtrl();
	}

	short CAnalysisHowPage::GetComputationKind()
	{
		short kind = GetCheckedRadioButton(IDC_STATISTIC, IDC_EVENT) - IDC_STATISTIC;
		ASSERT(kind >= 0 && kind < CAnalysisComputation::NB_KIND);

		return kind;
	}

	void CAnalysisHowPage::SetComputationKind(short kind)
	{
		ASSERT(kind >= 0 && kind < CAnalysisComputation::NB_KIND);
		CheckRadioButton(IDC_STATISTIC, IDC_EVENT, IDC_STATISTIC + kind);
	}


	void CAnalysisHowPage::UpdateCtrl()
	{

		//this->GetCheckedRadioButton(IDC_TIME_TRANSFORMATION_RADIO, IDC_FINAL_COMPUTATION_RADIO);
		bool bEnable = m_selectCtrl.GetCheck();
		//EnableGroupboxControls(::GetDlgItem(m_hWnd, IDC_SELECT_TIME_TRANSFORMATION), bEnable);
		//EnableGroupboxControls(::GetDlgItem(m_hWnd, IDC_SELECT_FINAL_COMPUTATION), !bEnable);

		bool bEnableEvent = bEnable && GetComputationKind() == CAnalysisComputation::EVENT;
		bool bEnableK = bEnableEvent && CAnalysisComputation::HaveK(m_eventTypeCtrl.GetCurSel());
		bool bEnablePreviousStat = m_pParent->GetDatabaseType() == CBioSIMDatabase::DATA_STATISTIC;
		bool bEnableStat = bEnable && !bEnableEvent;

		m_previousStatisticCtrl.EnableWindow(bEnablePreviousStat);

		m_TTTypeCtrl.EnableWindow(bEnable);
		m_TTModeCtrl.EnableWindow(bEnable);

		m_statisticCtrl.EnableWindow(bEnable);
		m_statisticTypeCtrl.EnableWindow(bEnableStat);
		m_eventCtrl.EnableWindow(bEnable);
		m_eventTypeCtrl.EnableWindow(bEnableEvent);
		m_KCtrl.EnableWindow(bEnableK);
		m_bDropYearCtrl.EnableWindow(bEnableEvent);

		for (int i = 0; i < NB_STATIC; i++)
			GetStatic(i).EnableWindow(bEnable);

		GetDlgItem(IDC_CT_TYPE)->EnableWindow(FALSE);
		GetDlgItem(IDC_CT_MODE)->EnableWindow(FALSE);

	

		//Update Image
		short type = m_TTTypeCtrl.GetCurItemData();
		short TMode = m_TTModeCtrl.GetCurItemData();
		UINT ID = NULL;
		if (type == CTRef::ANNUAL)
			ID = TMode ? IDB_OVER_ALL_YEARS_A : IDB_FOR_EACH_YEARS_A;
		else if (type == CTRef::MONTHLY)
			ID = TMode ? IDB_OVER_ALL_YEARS_M : IDB_FOR_EACH_YEARS_M;
		else if (type == CTRef::DAILY)
			ID = TMode ? IDB_OVER_ALL_YEARS_D : IDB_FOR_EACH_YEARS_D;

		m_imageModeCtrl.ShowWindow(ID ? SW_SHOW : SW_HIDE);
		m_imageModeCtrl.m_bTopImage = TRUE;
		m_imageModeCtrl.m_nAlignStyle = CMFCButton::ALIGN_CENTER;
		m_imageModeCtrl.SetImage(ID);
		m_imageModeCtrl.SizeToContent();
		m_imageModeCtrl.EnableWindow(bEnable);
		m_imageModeCtrl.RedrawWindow();


	}


	HBRUSH CAnalysisHowPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
	{
		HBRUSH hbr = CAnalysisPage::OnCtlColor(pDC, pWnd, nCtlColor);
		return hbr;
	}

	void CAnalysisHowPage::OnOK()
	{
		CAnalysisComputation& computation = m_pAnalysis->GetComputation();
		if (computation.m_bSelectTimeTransformation &&
			computation.m_bMeanOverReplication  &&
			computation.m_kind == CAnalysisComputation::STATISTIC &&
			computation.m_statisticType2 == -1)
		{

			XMSGBOXPARAMS xmb;
			_tcscpy_s(xmb.szCompanyName, 260, _T("NRCan"));
			xmb.lpszModule = _T("HowPage");
			xmb.nLine = IDS_SIM_HOW_WARNING;

			XMessageBox(m_hWnd,
				UtilWin::GetCString(IDS_SIM_HOW_WARNING),
				NULL,
				MB_OK | MB_DONOTTELLAGAIN | MB_ICONINFORMATION | MB_NORESOURCE,
				&xmb);
		}
		//if (rc & MB_DONOTTELLAGAIN) 
		//{ 
		//	TRACE(_T("MB_DONOTTELLAGAIN\n")); 
		//} 
	}
}