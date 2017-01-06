
// EasyPlayerDlg.h : ͷ�ļ�
//

#pragma once

#include "DlgVideo.h"
#include ".\EasySkinUI\skindialog.h"
// EasyProtocolЭ��֧�� [8/11/2016 Dingshuai]
#include "EasyProtocol.h"
using namespace EasyDarwin::Protocol;
#pragma comment(lib, "../bin/EasyProtocol.lib")
#pragma comment(lib, "../bin/libjson.lib")

#include <string.h>
using namespace std;


#define	_SURV_MAX_WINDOW_NUM		16

typedef struct __VIDEO_NODE_T
{
	bool		fullscreen;
	int			maximizedId;
	int			selectedId;
	int			channels;
	CDlgVideo	*pDlgVideo;
}VIDEO_NODE_T;

// CEasyClientDlg �Ի���
class CEasyClientDlg : public CSkinDialog
{
// ����
public:
	CEasyClientDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_EASYPLAYER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnCbnSelchangeComboSplitScreen();
	afx_msg void OnCbnSelchangeComboRenderFormat();
	afx_msg void OnBnClickedCheckShowntoscale();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	//�豸���Ͻڵ�˫����Ϣ
	afx_msg void	OnNMDBClickListDevices(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg long HandleButtonMessage(WPARAM wCmdID, LPARAM lParm);	DECLARE_MESSAGE_MAP()
	LRESULT OnWindowMaximized(WPARAM wParam, LPARAM lParam);
public:
	virtual BOOL DestroyWindow();
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	void SetActiveState(int nWndId, int nActiveState=1);
	int SendHttpReqDeviceList();
	static UINT WINAPI ReqDeviceListThread(LPVOID pParam);
	void ProcessReqDevListThread();
	void	InitialComponents();
	void	CreateComponents();
	void	UpdateComponents();
	void	DeleteComponents();
	void	UpdateVideoPosition(LPRECT lpRect);
	/*
	������DeleteChildren(HTREEITEM hItem)
	���ܣ�ɾ��hItem�µ����к��ӽڵ�
	*/
	UINT DeleteChildren(CSkinTreeCtrl *pTreeCtrl, HTREEITEM hItem);
	void ExpandAllItem(CSkinTreeCtrl *pTreeCtrl, HTREEITEM hTreeItem, bool bExpand = true);

private:
	//�ؼ���������
	CComboBox		*pComboxSplitScreen;
	CComboBox		*pComboxRenderFormat;	//IDC_COMBO_RENDER_FORMAT
	VIDEO_NODE_T	*pVideoWindow;		//��Ƶ����
	CSkinButton			pChkShownToScale;	//��������ʾ
	CStatic			*pStaticCopyright;	//IDC_STATIC_COPYRIGHT
	CSkinTreeCtrl* m_pTreeCtrDevList;
	HTREEITEM m_hRoot;
	//���ϵ�ͼ���б�
	CImageList	m_StatusImage;

	// EasyClient������չ [8/11/2016 Dingshuai]
	CInternetSession *m_pSession;
	EasyDarwin::Protocol::EasyDevicesInfo m_devicesInfo;
	CString m_strCMSIP; 
	int m_nCMSPort;
	int m_nCurSelWnd;
	HANDLE m_hReqDeviceListThread;

public:
	bool m_bReqDeviceListThreadRuning;
};
