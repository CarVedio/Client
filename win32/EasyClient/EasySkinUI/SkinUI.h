#ifndef SKINUI_HEAD_FILE
#define SKINUI_HEAD_FILE

//////////////////////////////////////////////////////////////////////////

//ϵͳͷ�ļ�
#include <GdiPlus.h>
#include <Shlwapi.h>
#include <afxwin.h>
#include <atlimage.h>
#include <vector>
#include <map>

using namespace Gdiplus;
using namespace std;

//FALSH ����
#import "Flash.ocx" named_guids
//////////////////////////////////////////////////////////////////////////
#ifndef SKINUI_CLASS
#define SKINUI_CLASS
#endif//SKINUI_CLASS


//////////////////////////////////////////////////////////////////////////
interface IFlashContrlSink
{
public:
	//
	virtual void OnDraw(CDC*pDC,int nWidth,int nHeight)=NULL;
};
//////////////////////////////////////////////////////////////////////////
#define ON_RBN_DBLCLK(id, memberFxn) \
	ON_CONTROL(BN_CLICKED, id, memberFxn)

#define WM_EDIT_CLICK					WM_USER+0x50

#define CHECK_IMAGE(image)				((image != NULL) && (!image->IsNull()))
#define CHECK_HWND(pWnd)				((pWnd != NULL) && (!pWnd->GetSafeHwnd()))
//////////////////////////////////////////////////////////////////////////
//ListBox Item
struct tagItemStruct
{
	UINT       itemID;
	UINT       itemWidth;
	UINT       itemHeight;
};

//λ�ö�λ
enum
{
	en_LTop,
	en_RTop,
	en_LBottom,
	en_RBottom,
	en_ScaleSize,//λ�úʹ�С��������������
	en_ScalePos//���������ţ�ֻ��λ��
};

struct tagPositionData
{
	int			nFixedPostion[2];
	CPoint		ptPosition;
	CSize		szSize;
};

//������ͷ�ļ�
//////////////////////////////////////////////////////////////////////////
//ƽ̨ͷ�ļ�
#include "Macro.h"
#include "Function.h"
#include "MemoryDC.h"
#include "ImageEx.h"
#include "RenderManager.h"
#include "GifImage.h"
#include "BuildDialog.h"

#include "ISkinControl.h"
#include "XmlManager.h"
#include "SkinButton.h"
#include "SkinComboBox.h"
#include "SkinDialog.h"
#include "SkinEdit.h"
#include "SkinListBox.h"
#include "SkinSliderCtrl.h"
#include "SkinProgressCtrl.h"
#include "SkinListCtrl.h"
#include "SkinTreeCtrl.h"
#include "SkinTabCtrl.h"
#include "SkinRichEdit.h"
#include "SkinUIMenu.h"
#include "FlashControl.h"
#include "SkinToolBar.h"
#include "SkinStatic.h"

//////////////////////////////////////////////////////////////////////////

#endif