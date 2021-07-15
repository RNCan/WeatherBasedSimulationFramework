
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include <SDKDDKVer.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS


#include <afxControlBars.h>     // prise en charge des MFC pour les rubans et les barres de contrôles
#include <afxTempl.h>
#include <afxWin.h>         // MFC core and standard components
#include <afxDlgs.h>
#include <afxDtCtl.h>
#include <afxEditBrowseCtrl.h>
#include <afxMenuButton.h>
#include <afxDialogEx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxcontextmenumanager.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


#include <vector>
#include <deque>
#include <list>
#include <set>
#include <array>
#include <map>
#include <unordered_map>
#include <string>
#include <exception>
#include <algorithm>
#include <memory>
#include <sstream>
#include <functional> 
#include <fstream>
#include <codecvt>
#include <iomanip>
#include <random>
#include <locale>
#include <cmath>
#include <climits>
#include <cstring>
#include <cctype>
#include <cstddef>
#include <stdexcept>

#include <stdlib.h>
#include <crtdbg.h>
#include <math.h>
#include <sys/stat.h>
#include <windows.h>
#include <wtypes.h>
#include <float.h>
#include <assert.h>
#define _INTSAFE_H_INCLUDED_
#define NOMINMAX 

#undef min
#undef max



#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif




