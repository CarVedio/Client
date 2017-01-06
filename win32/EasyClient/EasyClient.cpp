
// EasyPlayer.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "EasyClient.h"
#include "EasyClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEasyPlayerApp

BEGIN_MESSAGE_MAP(CEasyClientApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CEasyPlayerApp ����

CEasyClientApp::CEasyClientApp()
{
	// ֧����������������
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CEasyPlayerApp ����

CEasyClientApp theApp;


// CEasyPlayerApp ��ʼ��

BOOL CEasyClientApp::InitInstance()

{
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	InitHookScrollBar();

	// ���� shell ���������Է��Ի������
	// �κ� shell ����ͼ�ؼ��� shell �б���ͼ�ؼ���
	CShellManager *pShellManager = new CShellManager;

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

	CEasyClientDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȷ�������رնԻ���Ĵ���
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȡ�������رնԻ���Ĵ���
	}

	// ɾ�����洴���� shell ��������
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}



BOOL CEasyClientApp::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
	{
		return TRUE;
	}

	return CWinApp::PreTranslateMessage(pMsg);
}


int CEasyClientApp::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class
	RemoveHookScrollBar();

	return CWinApp::ExitInstance();
}

const ModuleFileInfomations& GetModuleFileInformations()
{
	static ModuleFileInfomations __s_mi;
	static BOOL bLoad = FALSE;

	if(!bLoad)
	{
		// Get application's full path.

		::GetModuleFileName(NULL, __s_mi.strFullPath.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
		__s_mi.strFullPath.ReleaseBuffer();

		// Break full path into seperate components.
		_tsplitpath_s(
			__s_mi.strFullPath, 
			__s_mi.strDrive.GetBufferSetLength(_MAX_DRIVE + 1), _MAX_DRIVE + 1,
			__s_mi.strDir.GetBufferSetLength(_MAX_DIR + 1), _MAX_DIR + 1,
			__s_mi.strName.GetBufferSetLength(_MAX_FNAME + 1), _MAX_FNAME + 1,
			__s_mi.strExt.GetBufferSetLength(_MAX_EXT + 1), _MAX_EXT + 1);

		__s_mi.strDrive.ReleaseBuffer();//�̷�
		__s_mi.strDir.ReleaseBuffer();//Ŀ¼�м�·��
		__s_mi.strName.ReleaseBuffer();//�ļ�����
		__s_mi.strExt.ReleaseBuffer();//�ļ���չ��(.exe)

		TCHAR   sDrive[_MAX_DRIVE];   
		TCHAR   sDir[_MAX_DIR];   
		TCHAR   sFilename[_MAX_FNAME],Filename[_MAX_FNAME];   
		TCHAR   sExt[_MAX_EXT];   

		GetModuleFileName(AfxGetInstanceHandle(),   Filename,   _MAX_PATH);   
		_tsplitpath_s(Filename,   sDrive,   sDir,   sFilename,   sExt); 

		CString  homeDir(CString(sDrive) + CString(sDir));   
		int      nLen = homeDir.GetLength(); 

		if(homeDir.GetAt(nLen-1) != _T('\\'))   
			homeDir   +=   _T('\\');   

		__s_mi.strPath = homeDir;

		bLoad = TRUE;
	}

	return __s_mi;
}