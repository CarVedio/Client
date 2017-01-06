#pragma once

#define	POP_MENU_RECORDING	10010
#define	POP_MENU_Stop	10011
#define	POP_MENU_SHOT	10012

typedef struct __CHANNEL_STATUS
{
	
	int			recording;

}CHANNELSTATUS;
// CDlgRender �Ի���
// 

class CDlgVideo;

class CDlgRender : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgRender)

public:
	CDlgRender(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDlgRender();

	void	SetChannelId(int _channelId)	{mChannelId = _channelId;}

	int		mChannelId;
	CHANNELSTATUS	channelStatus;
	HMENU	hMenu;
	void	ClosePopupMenu();

// �Ի�������
	enum { IDD = IDD_DIALOG_RENDER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	CImageEx	* m_pEasyLogo;
	CDlgVideo* m_pMainDlg;
public:
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	void SetMainDlg(CDlgVideo* pMainDlg);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};
