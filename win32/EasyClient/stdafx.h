
// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // �� Windows ͷ���ų�����ʹ�õ�����
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

// �ر� MFC ��ĳЩ�����������ɷ��ĺ��Եľ�����Ϣ������
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC ��������ͱ�׼���
#include <afxext.h>         // MFC ��չ


#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC �� Internet Explorer 4 �����ؼ���֧��
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC �� Windows �����ؼ���֧��
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // �������Ϳؼ����� MFC ֧��
#include <afxinet.h>
#include "IniFileLoad.h"

#include "resource.h"

// EasySkinUI��֧��
//ͼƬ�ؼ�
#import "ImageOle.dll" named_guids
//ͷ�ļ�
#include "./EasySkinUI/detours.h"
#pragma comment (lib,"./EasySkinUI/Lib/Detours.lib")

#include "./EasySkinUI/SkinUI_ScrollBar.h"
#pragma comment(lib,"Gdiplus.lib")
#include "./EasySkinUI/SkinUI.h"

#include "PublicClass.h"

#define	WM_WINDOW_MAXIMIZED		(WM_USER+2001)
#define	WM_MANU_RECORDING		(WM_USER+2002)

#define __CREATE_WINDOW(_x, _class, _id) {if (NULL == _x) {_x = (_class*)GetDlgItem(_id);}}
#define __MOVE_WINDOW(x, _rect)	{if (NULL != x) {x->MoveWindow(&_rect);}}
#define __MOVE_WINDOW_INVALIDATE(x, _rect)	{if (NULL != x) {x->MoveWindow(&_rect); x->Invalidate();}}
#define __DELETE_WINDOW(x)	{if (NULL != x) {x->DestroyWindow(); delete x; x=NULL;}}

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) {if(x != NULL) {delete x; x = NULL;}}
#endif

extern int		RenderFormat;

#define _CS

bool __WCharToMByte(LPCWSTR lpcwszStr, LPSTR lpszStr, DWORD dwSize);
bool __MByteToWChar(LPCSTR lpcszStr, LPWSTR lpwszStr, DWORD dwSize);


#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


