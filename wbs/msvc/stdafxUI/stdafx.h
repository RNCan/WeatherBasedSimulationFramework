// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once


#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers


#include <SDKDDKVer.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#define _AFX_ALL_WARNINGS




//#include <afxext.h>         // MFC extensions




#include <afxControlBars.h>     // prise en charge des MFC pour les rubans et les barres de contrôles
#include <afxTempl.h>
#include <afxWin.h>         // MFC core and standard components
#include <afxDlgs.h>
#include <afxDtCtl.h>
#include <afxEditBrowseCtrl.h>
#include <afxMenuButton.h>
#include <afxDialogEx.h>
#include <afxcontextmenumanager.h>


#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC support for Windows Common Controls
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


