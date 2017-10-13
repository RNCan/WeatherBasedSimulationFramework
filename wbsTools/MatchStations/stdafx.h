
// stdafx.h�: fichier Include pour les fichiers Include syst�me standard,
// ou les fichiers Include sp�cifiques aux projets qui sont utilis�s fr�quemment,
// et sont rarement modifi�s

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclure les en-t�tes Windows rarement utilis�s
#endif

#include <SDKDDKVer.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // certains constructeurs CString seront explicites

// d�sactive le masquage MFC de certains messages d'avertissement courants et par ailleurs souvent ignor�s
#define _AFX_ALL_WARNINGS






#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // Prise en charge MFC pour les contr�les communs Internet Explorer�4
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // Prise en charge des MFC pour les contr�les communs Windows
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // prise en charge des MFC pour les rubans et les barres de contr�les
#include <afxdlgs.h>



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


