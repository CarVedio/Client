// DlgRender.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "EasyClient.h"
#include "DlgRender.h"
#include "afxdialogex.h"
#include "DlgVideo.h"
#include "../libEasyPlayer/libEasyPlayerAPI.h"

// CDlgRender �Ի���

IMPLEMENT_DYNAMIC(CDlgRender, CDialogEx)

CDlgRender::CDlgRender(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgRender::IDD, pParent)
{
	memset(&channelStatus, 0x00, sizeof(CHANNELSTATUS));
	hMenu		=	NULL;
	m_pEasyLogo = NULL;
	mChannelId	=	0;
	m_pMainDlg = NULL;
}

CDlgRender::~CDlgRender()
{
	ClosePopupMenu();
	UIRenderEngine->RemoveImage(m_pEasyLogo);

}

void CDlgRender::SetMainDlg(CDlgVideo* pMainDlg)
{
	m_pMainDlg = pMainDlg;
}

void CDlgRender::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgRender, CDialogEx)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_PAINT()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


// CDlgRender ��Ϣ�������
void CDlgRender::ClosePopupMenu()
{
	if (NULL != hMenu)
	{
		DestroyMenu(hMenu);
		hMenu = NULL;
	}
}

void CDlgRender::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_pMainDlg)
	{
		m_pMainDlg->SetActiveState(1,true);
	} 
	::PostMessage(GetParent()->GetSafeHwnd(), WM_LBUTTONDBLCLK, 0, 0);

	CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CDlgRender::OnRButtonUp(UINT nFlags, CPoint point)
{
	ClosePopupMenu();

	hMenu = CreatePopupMenu();
	if (NULL != hMenu)
	{
		AppendMenu(hMenu, MF_STRING, POP_MENU_Stop, TEXT("ֹͣԤ��"));
		
		AppendMenu(hMenu, MF_STRING|(channelStatus.recording==0x01?MF_CHECKED:MF_UNCHECKED), POP_MENU_RECORDING, TEXT("¼��"));
		AppendMenu(hMenu, MF_STRING, POP_MENU_SHOT, TEXT("ץͼ"));

		CPoint	pMousePosition;
		GetCursorPos(&pMousePosition);
		SetForegroundWindow();
		TrackPopupMenu(hMenu, TPM_LEFTALIGN, pMousePosition.x, pMousePosition.y, 0, GetSafeHwnd(), NULL);
	}

	CDialogEx::OnRButtonUp(nFlags, point);
}


BOOL CDlgRender::OnCommand(WPARAM wParam, LPARAM lParam)
{
	WORD	wID = (WORD)wParam;
	switch (wID)
	{
	case POP_MENU_RECORDING:
		{
			//channelStatus.recording = (channelStatus.recording==0x00?0x01:0x00);
			if (mChannelId > 0)
			{
				channelStatus.recording = (channelStatus.recording==0x00?0x01:0x00);

				if (channelStatus.recording == 0x01)			EasyPlayer_StartManuRecording(mChannelId);
				else											EasyPlayer_StopManuRecording(mChannelId);
			}
		}
		break;
	case	POP_MENU_Stop:
		{
			if (mChannelId > 0)
			{
				if (m_pMainDlg)
				{
					m_pMainDlg->Preview();
				}			
			}
		}
		break;
	case POP_MENU_SHOT:
		{
			if (mChannelId > 0)
			{
				EasyPlayer_StartManuPicShot(mChannelId);
				// 				channelStatus.shoting = (channelStatus.shoting==0x00?0x01:0x00);
				// 
				// 				if (channelStatus.shoting == 0x01)			EasyPlayer_StartManuPicShot(mChannelId);
				// 				else											EasyPlayer_StopManuPicShot(mChannelId);
			}
		}

	default:
		break;
	}


	return CDialogEx::OnCommand(wParam, lParam);
}


void CDlgRender::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: �ڴ˴������Ϣ����������
	// ��Ϊ��ͼ��Ϣ���� CDialogEx::OnPaint()
	CBrush brushBkgnd; 
	CRect rcClient;
	brushBkgnd.CreateSolidBrush(RGB(0, 0, 0));
	GetClientRect(&rcClient);
	dc.FillRect(&rcClient, &brushBkgnd);
	brushBkgnd.DeleteObject(); //�ͷŻ�ˢ 
	CDC* pDC = CDC::FromHandle(dc.m_hDC);
	if ( m_pEasyLogo != NULL && !m_pEasyLogo->IsNull() )
	{
		int nStartX = (rcClient.Width()-184)/2;
		int nStartY =  (rcClient.Height()-184)/2;
		m_pEasyLogo->DrawImage(pDC,nStartX,nStartY);
	}
}


BOOL CDlgRender::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pEasyLogo = UIRenderEngine->GetImage(TEXT("Res\\EasyTeam\\Easylogo.png"));

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CDlgRender::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CDialogEx::OnLButtonUp(nFlags, point);
}


void CDlgRender::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (m_pMainDlg)
	{
		m_pMainDlg->SetActiveState(1,true);
	} 

	CDialogEx::OnLButtonDown(nFlags, point);
}
