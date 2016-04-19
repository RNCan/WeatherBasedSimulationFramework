//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "UI/Common/CommonCtrl.h"
#include "FileManager/DirectoryManager.h"


#include "WeatherBasedSimulationString.h"

namespace WBSF
{

	class CFileNameProperty : public CStdGridProperty
	{
		friend class CMFCPropertyGridCtrl;

		// Construction
	public:

		CFileNameProperty(const std::string& strName, const std::string& strFileName, const CDirectoryManager& manager, const std::string& description, size_t dwData) :
			CStdGridProperty(strName, strFileName, description, dwData),
			m_manager(manager)
		{
			WBSF::StringVector list = m_manager.GetFilesList();

			AddOption(_T(""));

			for (size_t i = 0; i < list.size(); i++)
			{
				AddOption(CString(list[i].c_str()));
			}

			AllowEdit(FALSE);
		}

		virtual ~CFileNameProperty()
		{
		}

		virtual BOOL HasButton() const{ return TRUE; }
		// Overrides
	public:

		const CDirectoryManager& m_manager;
	};



	template<UINT RES_STRING_ID, int BASE_INDEX = 0, bool ADD_EMPTY = false>
	class CGeneralIndexProperty : public CMFCPropertyGridProperty
	{
	public:

		//static CStringArrayEx OPTIONS_VALUES;

		CGeneralIndexProperty(const CString& strName, int index = 0, LPCTSTR lpszDescr = NULL, DWORD_PTR dwData = 0) :
			CMFCPropertyGridProperty(strName, _T(""), lpszDescr, dwData)
		{
			CStringArrayEx OPTIONS_VALUES(UtilWin::GetCString(RES_STRING_ID));

			m_bAllowEdit = false;
			if (ADD_EMPTY)
				AddOption(_T(""));

			for (int i = 0; i < OPTIONS_VALUES.GetSize(); i++)
			{
				AddOption(OPTIONS_VALUES[i]);
			}

			AllowEdit(FALSE);

			SetOriginalValue(GetOptionText(index - BASE_INDEX));
		}

		CString GetOptionText(int index)
		{
			ASSERT(index >= 0 && index < m_lstOptions.GetSize());
			POSITION pos = m_lstOptions.FindIndex(index);
			return m_lstOptions.GetAt(pos);
		}

		int GetIndex()const
		{
			int index = m_pWndCombo->GetCurSel();
			return index - BASE_INDEX;
		}

		void SetIndex(int index)
		{
			CMFCPropertyGridProperty::SetValue(GetOptionText(index - BASE_INDEX));
		}

	};

	typedef CGeneralIndexProperty < IDS_STR_WEATHER_VARIABLES_TITLE, -1, true > CWeatherVariableProperty;
	typedef CGeneralIndexProperty < IDS_STR_STATISTIC > CStatisticProperty;
	typedef CGeneralIndexProperty < IDS_WG_GRAPH_SERIE_TYPE_TITLE > CSerieTypeProperty;
	typedef CGeneralIndexProperty < IDS_STR_LEFT_RIGHT > CYAxisTypeProperty;
	typedef CGeneralIndexProperty < IDS_WG_GRAPH_FILL_DIR_TITLE > CFillDirectionProperty;
	typedef CGeneralIndexProperty < IDS_WG_GRAPH_HIST_DIR_TITLE > CHistDirectionProperty;


}