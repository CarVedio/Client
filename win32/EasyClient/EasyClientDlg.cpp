
// EasyPlayerDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "EasyClient.h"
#include "EasyClientDlg.h"
#include "afxdialogex.h"
#include "CreateDump.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//�쳣������
LONG CrashHandler_Player(EXCEPTION_POINTERS *pException)
{
	SYSTEMTIME	systemTime;
	GetLocalTime(&systemTime);

	wchar_t wszFile[MAX_PATH] = {0,};
	wsprintf(wszFile, TEXT("Player%04d%02d%02d %02d%02d%02d.dmp"), systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
	CreateDumpFile(wszFile, pException);

	return EXCEPTION_EXECUTE_HANDLER;		//����ֵEXCEPTION_EXECUTE_HANDLER	EXCEPTION_CONTINUE_SEARCH	EXCEPTION_CONTINUE_EXECUTION
}


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CEasyClientDlg �Ի���


CEasyClientDlg::CEasyClientDlg(CWnd* pParent /*=NULL*/)
	: CSkinDialog(CEasyClientDlg::IDD, TEXT("Main_config.xml"), pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	InitialComponents();

	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)CrashHandler_Player);
	m_pSession = NULL;
	m_strCMSIP = _T("121.40.50.44"); //121.40.50.44 192.168.1.121 124.193.152.42
	m_nCMSPort = 10000;
	m_nCurSelWnd = -1;
	m_hReqDeviceListThread = INVALID_HANDLE_VALUE;
	m_bReqDeviceListThreadRuning = false;
	m_pTreeCtrDevList = NULL;
}

void CEasyClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CSkinDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_SHOWNTOSCALE, pChkShownToScale);
}

BEGIN_MESSAGE_MAP(CEasyClientDlg, CSkinDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_WINDOW_MAXIMIZED, OnWindowMaximized)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_SCREEN, &CEasyClientDlg::OnCbnSelchangeComboSplitScreen)
	ON_CBN_SELCHANGE(IDC_COMBO_RENDER_FORMAT, &CEasyClientDlg::OnCbnSelchangeComboRenderFormat)
	ON_BN_CLICKED(IDC_CHECK_SHOWNTOSCALE, &CEasyClientDlg::OnBnClickedCheckShowntoscale)
	ON_WM_LBUTTONDOWN()
	ON_WM_DESTROY()
	ON_NOTIFY(NM_DBLCLK, 5010, &CEasyClientDlg::OnNMDBClickListDevices)
	ON_NOTIFY(NM_DBLCLK, 7000, &CEasyClientDlg::OnNMDBClickListDevices)

	//��ť��Ϣ��Ӧ
	ON_MESSAGE( WM_BUSEDDOWNUP_BUTTON, HandleButtonMessage )

END_MESSAGE_MAP()

// CEasyClientDlg ��Ϣ�������
BOOL CEasyClientDlg::OnInitDialog()
{
	CSkinDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	CreateComponents();

	if (NULL != pVideoWindow && NULL!=pVideoWindow->pDlgVideo)
	{
		FILE *f = fopen("rtsp.txt", "rb");
		if (NULL != f)
		{
			int idx = 0;
			char szURL[128] = {0,};
			while (! feof(f) && idx+1<_SURV_MAX_WINDOW_NUM)
			{
				memset(szURL, 0x00, sizeof(szURL));
				fgets(szURL, sizeof(szURL), f);

				if (0 != strcmp(szURL, "\0"))
				{
					pVideoWindow->pDlgVideo[idx++].SetURL(szURL);
				}
			}
		}
	}

	//Init Devices Tree root
	m_pTreeCtrDevList = (CSkinTreeCtrl*)GetItemByName(TEXT("DevListTree"));
	if (m_pTreeCtrDevList)
	{
		//����ͼ��
	if (m_StatusImage.GetSafeHandle()==NULL)
	{
		CBitmap Image;
		BITMAP ImageInfo;
		Image.LoadBitmap(IDB_TREE_LIST_IMAGE);
		Image.GetBitmap(&ImageInfo);
		m_StatusImage.Create(18,ImageInfo.bmHeight,ILC_COLOR16|ILC_MASK,0,0);
		m_StatusImage.Add(&Image,RGB(255,0,255));
		m_pTreeCtrDevList->SetImageList(&m_StatusImage,TVSIL_NORMAL);
	}

		// ��ʼ�����ؼ�
		TVINSERTSTRUCT tvInsert;
		//m_pTreeCtrDevList->ModifyStyle(NULL,TVS_HASBUTTONS|TVS_HASLINES/*|TVS_CHECKBOXES*/|TVS_SHOWSELALWAYS); 
		m_pTreeCtrDevList->ModifyStyle(NULL,TVS_HASBUTTONS|TVS_HASLINES|TVS_SHOWSELALWAYS); 

		tvInsert.hParent = NULL;
		tvInsert.hInsertAfter = TVI_LAST;
		tvInsert.item.mask = TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_TEXT|TVIF_STATE;
		tvInsert.item.hItem = NULL;
		tvInsert.item.state = INDEXTOSTATEIMAGEMASK( 1 );
		tvInsert.item.stateMask = TVIS_STATEIMAGEMASK;
		tvInsert.item.cchTextMax = 6;
		tvInsert.item.cchTextMax = 64;
		tvInsert.item.cChildren = 1;
		tvInsert.item.lParam = 0;

		tvInsert.item.iImage = 1;
		tvInsert.item.iSelectedImage = 6;

		tvInsert.item.pszText = _T("�豸�б�");
		//��Ӹ�Ŀ¼
		m_hRoot = m_pTreeCtrDevList->InsertItem(&tvInsert);
		m_pTreeCtrDevList->SetItemData(m_hRoot, 0);//��Ŀ¼����Ϊ0����ʶΪ���ڵ�
	}

	//��ȡһ�����µ�CMS��ַ
	CSkinEdit* pCMSEdit = (CSkinEdit*)GetDlgItem(5100);
	if (pCMSEdit)
	{
		pCMSEdit->SetWindowText(_T("121.40.50.44"));
	}
	CSkinEdit* pCMSPort= (CSkinEdit*)GetDlgItem(5101);
	if (pCMSPort)
	{
		pCMSPort->SetWindowText(_T("10000"));
	}

	// �����豸���� [8/11/2016 Dingshuai]
	SendHttpReqDeviceList();
	OnCbnSelchangeComboRenderFormat();

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

/*
������DeleteChildren(HTREEITEM hItem)
���ܣ�ɾ��hItem�µ����к��ӽڵ�
*/
UINT CEasyClientDlg::DeleteChildren(CSkinTreeCtrl *pTreeCtrl, HTREEITEM hItem)
{
	UINT nCount = 0;
	if (pTreeCtrl)
	{
		HTREEITEM hChild = pTreeCtrl->GetChildItem (hItem);

		while (hChild != NULL) 
		{
			HTREEITEM hNextItem = pTreeCtrl->GetNextSiblingItem (hChild);
			pTreeCtrl->DeleteItem (hChild);
			hChild = hNextItem;
			nCount++;
		}
	}
	return nCount;
}

void CEasyClientDlg::ExpandAllItem(CSkinTreeCtrl *pTreeCtrl,HTREEITEM hTreeItem,bool bExpand)
{
	if( pTreeCtrl == NULL && pTreeCtrl->GetSafeHwnd() == NULL ) return;
	if(!pTreeCtrl->ItemHasChildren(hTreeItem)) return;

	HTREEITEM hNextItem = pTreeCtrl->GetChildItem(hTreeItem);
	while (hNextItem != NULL)
	{
		ExpandAllItem(pTreeCtrl, hNextItem, true);
		hNextItem = pTreeCtrl->GetNextItem(hNextItem, TVGN_NEXT);
	}

	pTreeCtrl->Expand(hTreeItem,bExpand?TVE_EXPAND:TVE_COLLAPSE);
}

int CEasyClientDlg::SendHttpReqDeviceList()
{
	if (!m_bReqDeviceListThreadRuning)
	{
		m_bReqDeviceListThreadRuning = true;
		//�����߳�
		m_hReqDeviceListThread=(HANDLE)_beginthreadex(NULL,0,(&CEasyClientDlg::ReqDeviceListThread),
			this,THREAD_PRIORITY_NORMAL,NULL);
		if (!m_hReqDeviceListThread)
		{
			m_bReqDeviceListThreadRuning = false;
			return -1;
		}
	}
	return 1;
}

UINT CEasyClientDlg::ReqDeviceListThread(LPVOID pParam)
{
	if (pParam)
	{
		CEasyClientDlg* pMaster = (CEasyClientDlg*)pParam;
		if (pMaster)
		{
			pMaster->ProcessReqDevListThread();
			pMaster->m_bReqDeviceListThreadRuning = false;
		}
	}
	return 0;
}

void CEasyClientDlg::ProcessReqDevListThread()
{
	//For list
#if 0
	CSkinListCtrl*pListCtrl = (CSkinListCtrl*)GetDlgItem(5010);
	if( pListCtrl )
	{		
		// 		pListCtrl->InsertImage(7,TEXT("Res\\ListCtrl\\RarType.png"));
		pListCtrl->DeleteAllItems();
#endif

	if (m_pTreeCtrDevList)
	{
		DeleteChildren(m_pTreeCtrDevList, m_hRoot);

		//  [10/10/2016 SwordTwelve]
		//��ȡһ�����µ�CMS��ַ
		CSkinEdit* pCMSEdit = (CSkinEdit*)GetDlgItem(5100);
		if (pCMSEdit)
		{
			pCMSEdit->GetWindowText(m_strCMSIP);
		} 
		CString strCMSPort = _T("10000");
		CSkinEdit* pCMSPort= (CSkinEdit*)GetDlgItem(5101);
		if (pCMSPort)
		{
			pCMSPort->GetWindowText(strCMSPort);
		}
		char szPort[128] = {0,};
		__WCharToMByte(strCMSPort, szPort, sizeof(szPort)/sizeof(szPort[0]));
		m_nCMSPort = atoi(szPort);

		//1. �����豸�б�
		CString strReqURL =_T(""); 
		strReqURL.Format(_T("http://%s:%d/API/getdevicelist"), m_strCMSIP, m_nCMSPort);
		if (!m_pSession)
		{
			m_pSession = new CInternetSession(/*_T("Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)")*/);
		}

		std::string strData;
		CHttpFile *pFile = (CHttpFile *)m_pSession->OpenURL(strReqURL, 1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD);
		if (pFile)
		{
			char ch;
			while (pFile->Read(&ch, 1))
			{
				strData += ch;
			}
			pFile->Close();
			delete pFile;
			pFile = NULL;
		}

		//CString  m_strCode = (CString)strData.c_str();
		if (!strData.empty())
		{
			m_devicesInfo.clear();
			//����Json�ִ�
			EasyDarwin::Protocol::EasyMsgSCDeviceListACK deviceListAck(strData.c_str());
			EasyDarwin::Protocol::EasyDevices m_devices = deviceListAck.GetDevices();
			int i = 0;
			CString strTrmp = _T("");
			for (EasyDevices::iterator it = m_devices.begin(); it != m_devices.end(); it++)
			{
#if 0
				strTrmp = it->second.serial_.c_str();
				pListCtrl->InsertItem(i, strTrmp);
				strTrmp = it->second.name_.c_str();
				pListCtrl->SetItemText(i,1,strTrmp);
				strTrmp = it->second.appType_.c_str();
				pListCtrl->SetItemText(i,2,strTrmp);
#endif 
				CString strAppType = (CString)it->second.appType_.c_str();
				CString strName = (CString)it->second.name_.c_str();
				CString strSerial = (CString)it->second.serial_.c_str();
				strTrmp.Format(_T("%s(%s)"), strName, strSerial);
				//�������ڵ�
				TVINSERTSTRUCT tvinsert;
				tvinsert.hParent = m_hRoot;
				tvinsert.hInsertAfter = TVI_LAST;
				tvinsert.item.mask = TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_TEXT|TVIF_STATE;
				tvinsert.item.hItem = NULL;
				tvinsert.item.state = INDEXTOSTATEIMAGEMASK( 1 );
				tvinsert.item.stateMask = TVIS_STATEIMAGEMASK;
				tvinsert.item.cchTextMax = 6;
				tvinsert.item.cchTextMax = 64;
				tvinsert.item.cChildren = 1;
				tvinsert.item.lParam = 0;
				tvinsert.item.pszText = (LPTSTR)strTrmp.GetBuffer(strTrmp.GetLength());
				tvinsert.item.iImage = 9;//FOR Image
				tvinsert.item.iSelectedImage = 6;//FOR Image

// 				string serial_;//�豸���к�
// 				string name_;//�豸����
// 				string password_;//����
// 				string tag_;//��ǩ
// 				string channelCount_;//���豸����������ͷ����
// 				string snapJpgPath_;//���µĿ���·��
				EasyDarwin::Protocol::strDevice deviceInfo;
				deviceInfo.serial_ = it->second.serial_;
				deviceInfo.name_ = it->second.name_;
				deviceInfo.eDeviceType = static_cast<EasyDarwinTerminalType>(EasyProtocol::GetTerminalType( it->second.terminalType_));
				deviceInfo.eAppType = static_cast<EasyDarwinAppType>(EasyProtocol::GetAppType(it->second.appType_));
				deviceInfo.snapJpgPath_ = it->second.snapJpgPath_;

				//nvr�ӽڵ�
				if (strAppType == _T("EasyNVR"))
				{
					//��ȡ�豸��Ϣ
					CString strReqURL =_T(""); 
					strReqURL.Format(_T("http://%s:%d/API/getdeviceinfo?device=%s"), m_strCMSIP, m_nCMSPort, strSerial);
					if (!m_pSession)
					{
						m_pSession = new CInternetSession(/*_T("Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)")*/);
					}

					std::string strData;
					CHttpFile *pFile = (CHttpFile *)m_pSession->OpenURL(strReqURL, 1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD);
					if (pFile)
					{
						char ch;
						while (pFile->Read(&ch, 1))
						{
							strData += ch;
						}
						pFile->Close();
						delete pFile;
						pFile = NULL;
					}

					//CString  m_strCode = (CString)strData.c_str();
					if (!strData.empty())
					{
						//����Json�ִ�
						//EasyDarwin::Protocol::EasyMsgSCDeviceInfoACK deviceInfoAck(strData.c_str());
						bool bRet = deviceInfo.GetDevInfo(strData.c_str());
						if(bRet)
						{
							//�ɹ�����
						}
					}
					//��ӵ��豸��Ϣ�б�
					m_devicesInfo[it->second.serial_] = deviceInfo;

					HTREEITEM DeviceNo = NULL;
					DeviceNo  = m_pTreeCtrDevList->InsertItem(&tvinsert);	
					//������ڵ�����(��ǰ�豸ID)
					m_pTreeCtrDevList->SetItemData(DeviceNo, (DWORD_PTR)m_devicesInfo[it->second.serial_].serial_.c_str());//�豸Ϊ���������ֱ�Ӹ���ID���к�

					for (EasyDevices::iterator itor = deviceInfo.channels_.begin(); itor != deviceInfo.channels_.end(); itor++)
					{
						tvinsert.hParent = DeviceNo;
						tvinsert.hInsertAfter  = TVI_LAST;
						tvinsert.item.iImage = 0;
						tvinsert.item.iSelectedImage = 6;
						CString strChannel = (CString)itor->second.channel_.c_str();
						CString strChannelNode = _T("");
						strChannelNode.Format(_T("Channel%s"), strChannel);
						tvinsert.item.pszText = strChannelNode.GetBuffer(strChannelNode.GetLength());

						HTREEITEM NVRMember = m_pTreeCtrDevList->InsertItem(&tvinsert);
						//������Ա�ڵ�������(��ǰ��ԱIP)
						m_pTreeCtrDevList->SetItemData(NVRMember, (DWORD_PTR)m_devicesInfo[it->second.serial_].channels_[itor->second.channel_].channel_.c_str());
					}
				}
				else
				{
					//��ӵ��豸��Ϣ�б�
					m_devicesInfo[it->second.serial_] = deviceInfo;
					HTREEITEM DeviceNo = NULL;
					DeviceNo  = m_pTreeCtrDevList->InsertItem(&tvinsert);	
					//������ڵ�����(��ǰ�豸ID)
					m_pTreeCtrDevList->SetItemData(DeviceNo, (DWORD_PTR)m_devicesInfo[it->second.serial_].serial_.c_str());//�豸Ϊ���������ֱ�Ӹ���ID���к�
				}
				i++;
			}
			//չ�����������ӽڵ�
			ExpandAllItem(m_pTreeCtrDevList, m_pTreeCtrDevList->GetRootItem(), true);
				
			// �������� [8/12/2016 SwordTwelve]
			//��ȡ����JPGͼƬ
#if 1
			for (EasyDevices::iterator it = m_devices.begin(); it != m_devices.end(); it++)
			{
				CString strImgURL = (CString)it->second.snapJpgPath_.c_str();
				if (!strImgURL.IsEmpty())
				{
					pFile = (CHttpFile *)m_pSession->OpenURL(strImgURL, 1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD);

					BYTE b;
					strData.clear();
					while (pFile->Read(&b, 1))
					{
						strData += b;
					}
					pFile->Close();
					delete pFile;
					pFile = NULL;

					if (!strData.empty())
					{
						CString strTime = _T("");
						CTime CurrentTime=CTime::GetCurrentTime(); 
						strTime.Format(_T("%04d%02d%02d-%02d%02d%02d"),CurrentTime.GetYear(),CurrentTime.GetMonth(),
							CurrentTime.GetDay(),CurrentTime.GetHour(),  CurrentTime.GetMinute(),
							CurrentTime.GetSecond());

						CString lpszPath = _T("");
						CString strSerial =  (CString)it->second.serial_.c_str();
						lpszPath.Format(_T("./snap/%s-%s.jpg") , strSerial, strTime);
						CFile file;
						if (!file.Open(lpszPath, CFile::modeCreate | CFile::modeWrite))
						{
							return ;
						}
						file.Write((LPCTSTR)strData.c_str(), strData.length());
						file.Close();

						//pListCtrl->InsertImage(i,lpszPath);		
					}
				}
			}
#endif
		}
		else
		{
			//not get any devices
		}
	}
}

//���˫���豸�б�
void CEasyClientDlg::OnNMDBClickListDevices(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if(pNMItemActivate->iItem != -1)//����ڿհ״�
	{
#if 0
		CSkinListCtrl*pListCtrl = (CSkinListCtrl*)GetDlgItem(5010);
		if( pListCtrl )
#endif
		if( m_pTreeCtrDevList )	
		{
			HTREEITEM hTreeItem = m_pTreeCtrDevList->GetSelectedItem();
			if(m_pTreeCtrDevList->ItemHasChildren(hTreeItem))//�к��ӽڵ�˵����NVR??
				return;

			HTREEITEM hParentTreeItem = m_pTreeCtrDevList->GetParentItem(hTreeItem);//û�и��ڵ㣬 ˵���Ǹ�
			if (!hParentTreeItem)
			{
				return;
			}
			char* pSerial = (char*) m_pTreeCtrDevList->GetItemData(hParentTreeItem);
			char* pData = NULL;
			if (pSerial == 0)
			{
				pSerial = (char*)m_pTreeCtrDevList->GetItemData(hTreeItem);
			}
			else
			{
				pData = (char*)m_pTreeCtrDevList->GetItemData(hTreeItem);
			}
			
			CString strDeviceId=  (CString)pSerial;//pListCtrl->GetItemText(pNMItemActivate->iItem,0);
			CString strChannel =  (CString)pData;
			if (strChannel.IsEmpty())
			{
				strChannel = _T("0");
			}
			if (strDeviceId)
			{
				//http�����豸����
				//http://121.40.50.44:10000/api/getdevicestream?device=001001000010&channel=01&protocol=RTSP&reserve=1
				CString strReqURL =_T(""); 
				if (pData ==NULL)
				{
					strReqURL.Format(_T("http://%s:%d/API/getdevicestream?device=%s&channel=%s&protocol=RTSP&reserve=1"), m_strCMSIP, m_nCMSPort, strDeviceId,strChannel);
				}
				else
				{
					strReqURL.Format(_T("http://%s:%d/API/getdevicestream?device=%s&channel=%s&protocol=RTSP&reserve=1"), m_strCMSIP, m_nCMSPort, strDeviceId, strChannel);
				}

				std::string strData;
				CHttpFile *pFile = (CHttpFile *)m_pSession->OpenURL(strReqURL, 1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD);
				if (pFile)
				{
					char ch;
					while (pFile->Read(&ch, 1))
					{
						strData += ch;
					}
					pFile->Close();
					delete pFile;
					pFile = NULL;
				}
				if (!strData.empty())
				{
					//����Json�ִ�
					EasyDarwin::Protocol::EasyMsgSCGetStreamACK getSreamAck(strData.c_str());
					std::string strURL = getSreamAck.GetBodyValue("URL");
					if(!strURL.empty())
					{
						int nSelWnd = m_nCurSelWnd;
						if (nSelWnd <  0)
						{
							nSelWnd = 0;
						}
						pVideoWindow->pDlgVideo[nSelWnd].SetDeviceSerial(strDeviceId, strChannel);
						pVideoWindow->pDlgVideo[nSelWnd].SetURL((char*)strURL.c_str());
						if (!pVideoWindow->pDlgVideo[nSelWnd].Preview())
						{
							pVideoWindow->pDlgVideo[nSelWnd].Preview();
						}
					}
				}
			}
		}
	}
	*pResult = 0;

}

void CEasyClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CSkinDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CEasyClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CSkinDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CEasyClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CEasyClientDlg::DestroyWindow()
{
	EasyPlayer_Release();
	DeleteComponents();

	return CSkinDialog::DestroyWindow();
}


LRESULT CEasyClientDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (WM_PAINT == message || WM_SIZE==message)
	{
		UpdateComponents();
	}

	return CSkinDialog::WindowProc(message, wParam, lParam);
}


void	CEasyClientDlg::InitialComponents()
{
	pComboxSplitScreen	=	NULL;
	pComboxRenderFormat	=	NULL;
	pVideoWindow	=	NULL;

	pStaticCopyright	=	NULL;

	RenderFormat	=	DISPLAY_FORMAT_RGB565;//RGB565
	EasyPlayer_Init();
}

void	CEasyClientDlg::CreateComponents()
{
	__CREATE_WINDOW(pComboxSplitScreen, CComboBox,		IDC_COMBO_SPLIT_SCREEN);
	__CREATE_WINDOW(pComboxRenderFormat, CComboBox,		IDC_COMBO_RENDER_FORMAT);
	//__CREATE_WINDOW(pChkShownToScale, CSkinButton,		IDC_CHECK_SHOWNTOSCALE);
	__CREATE_WINDOW(pStaticCopyright, CStatic,		IDC_STATIC_COPYRIGHT);

// 		checkimgtickn="Res\CheckBox\checkbox_tick_normal.png" checkimgtickh="Res\CheckBox\checkbox_tick_highlight.png" 
// 		size="100,15" trans="true" checked="true" fontnormalcolor="#FFFFFF"/>
	pChkShownToScale.SetCheckImage(TEXT("Res\\CheckBox\\checkbox_normal.png"), TEXT("Res\\CheckBox\\checkbox_highlight.png"), TEXT("Res\\CheckBox\\checkbox_tick_normal.png"), TEXT("Res\\CheckBox\\checkbox_tick_highlight.png") );
	pChkShownToScale.SetButtonType(en_CheckButton);
	HDC hParentDC = GetBackDC();

	pChkShownToScale.SetParentBack(hParentDC);
	pChkShownToScale.SetSize(68,15);
	//pChkShownToScale.SetCheck(BST_CHECKED);


		pChkShownToScale.SetWindowText(TEXT("��������ʾ"));

	if (NULL == pVideoWindow)
	{
		pVideoWindow = new VIDEO_NODE_T;
		pVideoWindow->fullscreen    = false;
		pVideoWindow->maximizedId	=	-1;
		pVideoWindow->selectedId	=	-1;
		pVideoWindow->channels		=	4;
		if (pVideoWindow->channels>_SURV_MAX_WINDOW_NUM)	pVideoWindow->channels=_SURV_MAX_WINDOW_NUM;
		pVideoWindow->pDlgVideo	=	new CDlgVideo[_SURV_MAX_WINDOW_NUM];//gAppInfo.maxchannels
		for (int i=0; i<_SURV_MAX_WINDOW_NUM; i++)
		{
			pVideoWindow->pDlgVideo[i].SetMainDlg(this);
			pVideoWindow->pDlgVideo[i].Create(IDD_DIALOG_VIDEO, this);
			pVideoWindow->pDlgVideo[i].SetWindowId(i);
			pVideoWindow->pDlgVideo[i].ShowWindow(SW_HIDE);
		}
	}

	if (NULL != pComboxSplitScreen)
	{
		pComboxSplitScreen->AddString(TEXT("4����"));
		pComboxSplitScreen->AddString(TEXT("8����"));
		pComboxSplitScreen->AddString(TEXT("9����"));
		pComboxSplitScreen->AddString(TEXT("16����"));
		pComboxSplitScreen->SetCurSel(0);
	}
	if (NULL != pComboxRenderFormat)
	{
		pComboxRenderFormat->AddString(TEXT("YUY2"));
		pComboxRenderFormat->AddString(TEXT("YV12"));
		pComboxRenderFormat->AddString(TEXT("RGB565"));
		pComboxRenderFormat->AddString(TEXT("GDI"));

		pComboxRenderFormat->SetCurSel(0);
	}
}
void	CEasyClientDlg::UpdateComponents()
{
	CRect	rcClient;
	GetClientRect(&rcClient);
	if (rcClient.IsRectEmpty())		return;

	CRect	rcVideo;
	rcVideo.SetRect(rcClient.left+302, rcClient.top+36, rcClient.right-20, rcClient.bottom-80);
	UpdateVideoPosition(&rcVideo);

	CRect	rcSplitScreen;
	rcSplitScreen.SetRect(rcClient.left+10, rcVideo.bottom+3, rcClient.left+100, rcVideo.bottom+3+90);
	__MOVE_WINDOW(pComboxSplitScreen, rcSplitScreen);

	CRect	rcRenderFormat;
	rcRenderFormat.SetRect(rcSplitScreen.right+5, rcSplitScreen.top, rcSplitScreen.right+5+100, rcSplitScreen.bottom);
	__MOVE_WINDOW(pComboxRenderFormat, rcRenderFormat);

	CRect	rcShownToScale;
	rcShownToScale.SetRect(rcRenderFormat.right+10, rcRenderFormat.top, rcRenderFormat.right+10+80, rcRenderFormat.top+30);
	//__MOVE_WINDOW(pChkShownToScale, rcShownToScale);
	if (pChkShownToScale.GetSafeHwnd())
	{
		pChkShownToScale.MoveWindow(&rcShownToScale);
	}

	CRect	rcCopyright;
	rcCopyright.SetRect(rcClient.right-200, rcSplitScreen.top+5, rcClient.right-2, rcClient.bottom);
	__MOVE_WINDOW(pStaticCopyright, rcCopyright);
}
void	CEasyClientDlg::DeleteComponents()
{
	if (NULL != pVideoWindow)
	{
		if (NULL != pVideoWindow->pDlgVideo)
		{
			for (int i=0; i<_SURV_MAX_WINDOW_NUM; i++)
			{
				pVideoWindow->pDlgVideo[i].DestroyWindow();
			}
			delete []pVideoWindow->pDlgVideo;
			pVideoWindow->pDlgVideo = NULL;
		}
		delete pVideoWindow;
		pVideoWindow = NULL;
	}
}


void	CEasyClientDlg::UpdateVideoPosition(LPRECT lpRect)
{
	CRect rcClient;
	if (NULL == lpRect)
	{
		GetClientRect(&rcClient);
		lpRect = &rcClient;
	}

	if (NULL == pVideoWindow)		return;

	//CRect rcClient;
	rcClient.CopyRect(lpRect);

	CRect rcTmp;
	rcTmp.SetRect(rcClient.left, rcClient.top, rcClient.left+rcClient.Width()/2, rcClient.top+rcClient.Height()/2);

	//

	if (pVideoWindow->maximizedId==-1)
	{
		int nTimes = 2;
		int nLeft = lpRect->left;
		int nTop  = lpRect->top;

		for (int i=pVideoWindow->channels; i<_SURV_MAX_WINDOW_NUM; i++)
		{
			if (pVideoWindow->pDlgVideo[i].IsWindowVisible())
				pVideoWindow->pDlgVideo[i].ShowWindow(SW_HIDE);
		}

		switch (pVideoWindow->channels)
		{
		case 4:
		case 9:
		case 16:
		case 25:
		case 36:
		case 64:
		default:
			{
				nTimes = 2;
				if (pVideoWindow->channels == 4)		nTimes	=	2;
				if (pVideoWindow->channels == 9)		nTimes	=	3;
				if (pVideoWindow->channels == 16)		nTimes	=	4;
				if (pVideoWindow->channels == 25)		nTimes	=	5;
				if (pVideoWindow->channels == 36)		nTimes	=	6;
				if (pVideoWindow->channels == 64)		nTimes	=	8;

				RECT rcTmp;
				SetRectEmpty(&rcTmp);

				int n = 0;//videoPatrol.patrolStartId;
				for (int i = 0; i < nTimes; i++)
				{
					for (int j = 0; j < nTimes; j ++)
					{
						//SetRect(&rcTmp, nLeft, nTop, nLeft + imgSize.cx / nTimes, nTop + imgSize.cy / nTimes);
						SetRect(&rcTmp, nLeft, nTop, nLeft + rcClient.Width() / nTimes, nTop + rcClient.Height() / nTimes);
						//CopyRect(&vidRenderHandle[n].drawvid.rect, &rcTmp);

						if (j+1==nTimes && rcTmp.right<rcClient.right)
						{
							rcTmp.right = rcClient.right;
						}
						if (i+1==nTimes && rcTmp.bottom<rcClient.bottom)
						{
							rcTmp.bottom = rcClient.bottom;
						}


						pVideoWindow->pDlgVideo[n].MoveWindow(&rcTmp);
						if (! pVideoWindow->pDlgVideo[n].IsWindowVisible())
							pVideoWindow->pDlgVideo[n].ShowWindow(SW_SHOW);


						n ++;

						nLeft += rcClient.Width() / nTimes;
					}
					nLeft = rcClient.left;
					nTop  += (rcClient.Height()) / nTimes;
				}
			}
			break;
		case 6:		//6��??��
			{
				int nWidth = rcClient.Width() / 3;
				int nHeight= rcClient.Height()/ 3;

				int nRight = 0;
				int nBottom= 0;
				if (rcClient.right > nWidth*3)	nRight = rcClient.Width()-nWidth*3;
				if (rcClient.bottom> nHeight*3)	nBottom= rcClient.Height()-nHeight*3;
			
				nLeft = rcClient.left;
				nTop  = rcClient.top+nHeight*2;
				for (int i=3; i<6; i++)
				{
					rcTmp.SetRect(nLeft, nTop, nLeft+nWidth, nTop+nHeight);
					if (i+1==6)			rcTmp.right += nRight;
					if (nBottom > 0)	rcTmp.bottom += nBottom;
					pVideoWindow->pDlgVideo[i].MoveWindow(&rcTmp);
					if (! pVideoWindow->pDlgVideo[i].IsWindowVisible())
						pVideoWindow->pDlgVideo[i].ShowWindow(SW_SHOW);
				
					nLeft += nWidth;
				}
				nLeft -= nWidth;
				nTop  = rcClient.top;
				for (int i=1; i<3; i++)
				{
					rcTmp.SetRect(nLeft, nTop, nLeft+nWidth, nTop+nHeight);
					if (nRight>0)	rcTmp.right += nRight;
					pVideoWindow->pDlgVideo[i].MoveWindow(&rcTmp);
					if (! pVideoWindow->pDlgVideo[i].IsWindowVisible())
						pVideoWindow->pDlgVideo[i].ShowWindow(SW_SHOW);
					nTop += nHeight;
				}
			
				rcTmp.SetRect(rcClient.left, rcClient.top, rcTmp.left, rcTmp.bottom);
				pVideoWindow->pDlgVideo[0].MoveWindow(&rcTmp);
				if (! pVideoWindow->pDlgVideo[0].IsWindowVisible())
					pVideoWindow->pDlgVideo[0].ShowWindow(SW_SHOW);
			}
			break;
		case 8:		//8����
			{

				int nWidth = rcClient.Width() / 4;
				int nHeight= rcClient.Height()/ 4;

				int nRight = 0;
				int nBottom= 0;
				if (rcClient.right > nWidth*4)	nRight = rcClient.Width()-nWidth*4;
				if (rcClient.bottom> nHeight*4)	nBottom= rcClient.Height()-nHeight*4;

				nLeft = rcClient.left;
				nTop  = rcClient.top+nHeight*3;
				for (int i=4; i<8; i++)
				{
					rcTmp.SetRect(nLeft, nTop, nLeft+nWidth, nTop+nHeight);
					if (i+1==8)			rcTmp.right += nRight;
					if (nBottom > 0)	rcTmp.bottom += nBottom;
					pVideoWindow->pDlgVideo[i].MoveWindow(&rcTmp);
					if (! pVideoWindow->pDlgVideo[i].IsWindowVisible())
							pVideoWindow->pDlgVideo[i].ShowWindow(SW_SHOW);

					nLeft += nWidth;
				}
				nLeft -= nWidth;
				nTop  = rcClient.top;
				for (int i=1; i<4; i++)
				{
					rcTmp.SetRect(nLeft, nTop, nLeft+nWidth, nTop+nHeight);
					if (nRight>0)	rcTmp.right += nRight;
					pVideoWindow->pDlgVideo[i].MoveWindow(&rcTmp);
					if (! pVideoWindow->pDlgVideo[i].IsWindowVisible())
						pVideoWindow->pDlgVideo[i].ShowWindow(SW_SHOW);
					nTop += nHeight;
				}

				rcTmp.SetRect(rcClient.left, rcClient.top, rcTmp.left, rcTmp.bottom);
				pVideoWindow->pDlgVideo[0].MoveWindow(&rcTmp);
				if (! pVideoWindow->pDlgVideo[0].IsWindowVisible())
					pVideoWindow->pDlgVideo[0].ShowWindow(SW_SHOW);

			}
			break;
		}

		for (int vid=0; vid<_SURV_MAX_WINDOW_NUM; vid++)
		{
			//pVideoWindow->pDlgVideo[vid].SetSelectedChannel(pVideoWindow->selectedId==vid);
		}
	}
	else
	{
		for (int i=0; i<_SURV_MAX_WINDOW_NUM; i++)
		{
			if (pVideoWindow->pDlgVideo[i].IsWindowVisible() && i!=pVideoWindow->maximizedId)
			{
				pVideoWindow->pDlgVideo[i].ShowWindow(SW_HIDE);
			}
		}
		rcTmp.SetRect(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
		pVideoWindow->pDlgVideo[pVideoWindow->maximizedId].MoveWindow(&rcTmp);
		pVideoWindow->pDlgVideo[pVideoWindow->maximizedId].ShowWindow(SW_SHOW);
	}
}

LRESULT CEasyClientDlg::OnWindowMaximized(WPARAM wParam, LPARAM lParam)
{
	int nCh = (int)wParam;

	if (pVideoWindow->maximizedId == -1)
	{
		pVideoWindow->maximizedId = nCh;
	}
	else
	{
		pVideoWindow->maximizedId = -1;
	}
	UpdateComponents();

	return 0;
}

void CEasyClientDlg::OnCbnSelchangeComboSplitScreen()
{
	if (NULL == pVideoWindow)		return;

	int nSplitWindow = 4;
	int nIdx = pComboxSplitScreen->GetCurSel();
	if (nIdx == 0)	nSplitWindow = 4;
	else if (nIdx == 1)	nSplitWindow = 8;
	else if (nIdx == 2)	nSplitWindow = 9;
	else if (nIdx == 3)	nSplitWindow = 16;

	pVideoWindow->channels		=	nSplitWindow;
	UpdateComponents();
}


void CEasyClientDlg::OnCbnSelchangeComboRenderFormat()
{
	if (NULL == pComboxRenderFormat)		return;

	int iIdx = pComboxRenderFormat->GetCurSel();
	if (iIdx == 0)	RenderFormat	=	DISPLAY_FORMAT_YUY2;//YV12
	else if (iIdx == 1)	RenderFormat	=	DISPLAY_FORMAT_YV12;//YUY2
	else if (iIdx == 2)	RenderFormat	=	DISPLAY_FORMAT_RGB565;//RGB565
	else if (iIdx == 3)	RenderFormat	=	DISPLAY_FORMAT_RGB24_GDI;//GDI
}

void CEasyClientDlg::OnBnClickedCheckShowntoscale()
{
	//IDC_CHECK_SHOWNTOSCALE
	if (NULL == pVideoWindow)					return;
	if (NULL == pVideoWindow->pDlgVideo)		return;

	static int shownToScale = 0x00;
	shownToScale = (shownToScale==0x00?0x01:0x00);

	for (int i=0; i<_SURV_MAX_WINDOW_NUM; i++)
	{
		pVideoWindow->pDlgVideo[i].SetShownToScale(shownToScale);
	}
}

void CEasyClientDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	PostMessage(WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(point.x,point.y));

	CSkinDialog::OnLButtonDown(nFlags, point);
}


void CEasyClientDlg::OnDestroy()
{

	if (NULL != m_pSession)
	{
		m_pSession->Close();
		delete m_pSession;
	}

	CSkinDialog::OnDestroy();

	// TODO: Add your message handler code here
}

void CEasyClientDlg::SetActiveState(int nWndId, int nActiveState)
{
	m_nCurSelWnd = nWndId;
	for (int i=0; i<_SURV_MAX_WINDOW_NUM; i++)
	{
		if (nWndId== i)
		{
			//pVideoWindow->pDlgVideo[i].SetActiveState(1);
		} 
		else
		{
			pVideoWindow->pDlgVideo[i].SetActiveState(0);
		}
	}
}

LRESULT CEasyClientDlg::HandleButtonMessage(WPARAM wParam, LPARAM lParam)
{
	DWORD dwCtrlID = (DWORD)wParam;
	int nType = (UINT)lParam;
// 	if(nType!=2)//1-down 2-up
// 		return 0;
	CSkinButton *pSkinButton=NULL;
	CString strItemName=GetItemNameByID(dwCtrlID,&pSkinButton);
	if(pSkinButton==NULL||strItemName.IsEmpty())
		return 0;
	CString sPtzCmd = _T("");
	if(strItemName==_T("Refresh") && nType==2)
	{
		SendHttpReqDeviceList();
	}
	if (_T("StopServerRecord") == strItemName && nType==2)
	{
		//δѡ�񴰿�
		if (m_nCurSelWnd<0)
		{
			CString strError = _T("����ѡ��һ����Ƶ���ڡ�");
			AfxMessageBox(strError);
			return 0;
		}
		//ƽ̨¼��
		//����http����PTZ����
		CString strDeviceId= _T("");
		CString strChannel= _T("");
		strDeviceId = pVideoWindow->pDlgVideo[m_nCurSelWnd].GetDeviceSerial();
		strChannel = pVideoWindow->pDlgVideo[m_nCurSelWnd].GetDeviceChannel();
		CString strRMSIP = _T("");
		CString strRMSPort = _T("");
		strRMSIP = pVideoWindow->pDlgVideo[m_nCurSelWnd].GetRMSIP();
		strRMSPort = pVideoWindow->pDlgVideo[m_nCurSelWnd].GetRMSPort();
		if (strDeviceId.IsEmpty()||strChannel.IsEmpty()||strRMSPort.IsEmpty()||strRMSIP.IsEmpty())
		{
			return 0;
		}
		CString strName =  _T("");
		strName.Format(_T("%s_%s"), strDeviceId,  strChannel);
		CString strReqURL =_T(""); //Continuous / single
		strReqURL.Format(_T("http://%s:%s/api/easyrecordmodule?name=%s&cmd=stop"), 
			strRMSIP, strRMSPort, strName);
		if (!m_pSession)
		{
			m_pSession = new CInternetSession(/*_T("Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)")*/);
		}

		std::string strTempData;
		CHttpFile *pFile = (CHttpFile *)m_pSession->OpenURL(strReqURL, 1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD);
		if (pFile)
		{
			char ch;
			while (pFile->Read(&ch, 1))
			{
				strTempData += ch;
			}
			pFile->Close();
			delete pFile;
			pFile = NULL;
		}

		if (!strTempData.empty())
		{
		}
	}
	if (_T("ServerRecord") == strItemName && nType==2)
	{
		//δѡ�񴰿�
		if (m_nCurSelWnd<0)
		{
			CString strError = _T("����ѡ��һ����Ƶ���ڡ�");
			AfxMessageBox(strError);
			return 0;
		}
		//ƽ̨¼��
		//����http����PTZ����
		CString strDeviceId= _T("");
		CString strChannel= _T("");
		strDeviceId = pVideoWindow->pDlgVideo[m_nCurSelWnd].GetDeviceSerial();
		strChannel = pVideoWindow->pDlgVideo[m_nCurSelWnd].GetDeviceChannel();
		if (strDeviceId.IsEmpty()||strChannel.IsEmpty())
		{
			return 0;
		}
		CString strReqURL =_T(""); //Continuous / single
		strReqURL.Format(_T("http://%s:%d/api/associatedRMS?device=%s&channel=%s"), 
			m_strCMSIP, m_nCMSPort, strDeviceId, strChannel);
		if (!m_pSession)
		{
			m_pSession = new CInternetSession(/*_T("Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)")*/);
		}

		std::string strData;
		CHttpFile *pFile = (CHttpFile *)m_pSession->OpenURL(strReqURL, 1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD);
		if (pFile)
		{
			char ch;
			while (pFile->Read(&ch, 1))
			{
				strData += ch;
			}
			pFile->Close();
			delete pFile;
			pFile = NULL;
		}

		if (!strData.empty())
		{
			//����Json�ִ�
			//EasyRMSAssociated" : "false", "EasyRMSIP" : "192.168.1.172", "EasyRMSPort" : "10100", "Serial" : "001002000002", 
			EasyDarwin::Protocol::EasyMsgSCRMSAssociateACK associateRMSAck(strData.c_str());

			std::string strbAssociatedRMS = associateRMSAck.GetBodyValue("EasyRMSAssociated");
			std::string strEasyRMSIP = associateRMSAck.GetBodyValue("EasyRMSIP");
			std::string strEasyRMSPort = associateRMSAck.GetBodyValue("EasyRMSPort");

			std::string strURL = associateRMSAck.GetBodyValue("url");
			if(!strURL.empty()&&!strEasyRMSIP.empty()&&!strEasyRMSPort.empty() && strbAssociatedRMS != "true")
			{
				//��RMS����¼������
				if (strDeviceId.IsEmpty()||strChannel.IsEmpty())
				{
					return 0;
				}
				CString sEasyRMSIP = (CString)strEasyRMSIP.c_str();
				CString sEasyRMSPort = (CString)strEasyRMSPort.c_str();
				CString sEasyStreamUrl = (CString) strURL.c_str();

				pVideoWindow->pDlgVideo[m_nCurSelWnd].SetRMSIPPort(sEasyRMSIP, sEasyRMSPort);

				CString strName =  _T("");
				strName.Format(_T("%s_%s"), strDeviceId,  strChannel);
				CString strReqURL =_T(""); //Continuous / single
				strReqURL.Format(_T("http://%s:%s/api/easyrecordmodule?name=%s&url=%s"), 
					sEasyRMSIP, sEasyRMSPort, strName, sEasyStreamUrl);
				if (!m_pSession)
				{
					m_pSession = new CInternetSession(/*_T("Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)")*/);
				}

				std::string strTempData;
				CHttpFile *pFile = (CHttpFile *)m_pSession->OpenURL(strReqURL, 1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD);
				if (pFile)
				{
					char ch;
					while (pFile->Read(&ch, 1))
					{
						strTempData += ch;
					}
					pFile->Close();
					delete pFile;
					pFile = NULL;
				}

				if (!strTempData.empty())
				{

				}
			}

		}
	}

// EASY_PTZ_CMD_TYPE_STOP,                         "STOP",
// EASY_PTZ_CMD_TYPE_UP,                           "UP",
// EASY_PTZ_CMD_TYPE_DOWN,                         "DOWN",
// EASY_PTZ_CMD_TYPE_LEFT,                         "LEFT",
// EASY_PTZ_CMD_TYPE_RIGHT,                        "RIGHT",
// EASY_PTZ_CMD_TYPE_LEFTUP,                       "LEFTUP",
// EASY_PTZ_CMD_TYPE_LEFTDOWN,                     "LEFTDOWN",
// EASY_PTZ_CMD_TYPE_RIGHTUP,                      "RIGHTUP",
// EASY_PTZ_CMD_TYPE_RIGHTDOWN,                    "RIGHTDOWN",
// EASY_PTZ_CMD_TYPE_ZOOMIN,                       "ZOOMIN",
// EASY_PTZ_CMD_TYPE_ZOOMOUT,                      "ZOOMOUT",
// EASY_PTZ_CMD_TYPE_FOCUSIN,                      "FOCUSIN",
// EASY_PTZ_CMD_TYPE_FOCUSOUT,                     "FOCUSOUT",
// EASY_PTZ_CMD_TYPE_APERTUREIN,                   "APERTUREIN",
// EASY_PTZ_CMD_TYPE_APERTUREOUT,                  "APERTUREOUT"

	else if(strItemName==_T("LeftUpper"))
		(nType==1) ? sPtzCmd=_T("leftup") : sPtzCmd=_T("stop");
	else if(strItemName==_T("Left"))
		(nType==1) ? sPtzCmd=_T("left") : sPtzCmd=_T("stop");
	else if(strItemName==_T("LeftDown"))
		(nType==1) ? sPtzCmd=_T("leftdown") : sPtzCmd=_T("stop");
	else if(strItemName==_T("Upper"))
		(nType==1) ? sPtzCmd=_T("up") : sPtzCmd=_T("stop");
	else if(strItemName==_T("Center"))
		sPtzCmd=_T("");
	else if(strItemName==_T("Down"))
		(nType==1) ? sPtzCmd=_T("down") : sPtzCmd=_T("stop");
	else if(strItemName==_T("RightUpper"))
		(nType==1) ? sPtzCmd=_T("rightup") : sPtzCmd=_T("stop");
	else if(strItemName==_T("Right"))
		(nType==1) ? sPtzCmd=_T("right") : sPtzCmd=_T("stop");
	else if(strItemName==_T("RightDown"))
		(nType==1) ? sPtzCmd=_T("rightdown") : sPtzCmd=_T("stop");
	else if(strItemName==_T("ZoomIn"))
		(nType==1) ? sPtzCmd=_T("zoomin") : sPtzCmd=_T("stop");
	else if(strItemName==_T("FocusIn"))
		(nType==1) ? sPtzCmd=_T("focusin") : sPtzCmd=_T("stop");
	else if(strItemName==_T("ApertureIn"))
		(nType==1) ? sPtzCmd=_T("aperturein") : sPtzCmd=_T("stop");
	else if(strItemName==_T("ZoomOut"))
		(nType==1) ? sPtzCmd=_T("zoomout") : sPtzCmd=_T("stop");
	else if(strItemName==_T("FocusOut"))
		(nType==1) ? sPtzCmd=_T("focusout") : sPtzCmd=_T("stop");
	else if(strItemName==_T("ApertureOut"))
		(nType==1) ? sPtzCmd=_T("apertureout") : sPtzCmd=_T("stop");

	if (!sPtzCmd.IsEmpty() && m_nCurSelWnd>-1)
	{
		//����http����PTZ����
		CString strDeviceId= _T("");
		CString strChannel= _T("");
		strDeviceId = pVideoWindow->pDlgVideo[m_nCurSelWnd].GetDeviceSerial();
		strChannel = pVideoWindow->pDlgVideo[m_nCurSelWnd].GetDeviceChannel();
		//http://[ip]:[port]/api/ptzcontrol?device=001001000058&channel=0&actiontype=single&c ommand=down&speed=5&protocol=onvif
		CString strReqURL =_T(""); //Continuous / single
		strReqURL.Format(_T("http://%s:%d/api/ptzcontrol?device=%s&channel=%s&actiontype=Continuous&command=%s&speed=5&protocol=onvif"), 
			m_strCMSIP, m_nCMSPort, strDeviceId, strChannel, sPtzCmd);
		if (!m_pSession)
		{
			m_pSession = new CInternetSession(/*_T("Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)")*/);
		}

		std::string strData;
		CHttpFile *pFile = (CHttpFile *)m_pSession->OpenURL(strReqURL, 1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD);
		if (pFile)
		{
			char ch;
			while (pFile->Read(&ch, 1))
			{
				strData += ch;
			}
			pFile->Close();
			delete pFile;
			pFile = NULL;
		}

		//CString  m_strCode = (CString)strData.c_str();
		if (!strData.empty())
		{
			//PTZ���Ƴɹ�~~
		}
	}

	return 0;
}

