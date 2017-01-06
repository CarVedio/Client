/*
	Copyright (c) 2013-2016 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.EasyDarwin.org
*/

// IniFileLoad.cpp: implementation of the CIniFileLoad class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IniFileLoad.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIniFileLoad::CIniFileLoad()
{
	//InitIniFilePath();
}

CIniFileLoad::~CIniFileLoad()
{

}

/*
������InitIniFilePath()
���ܣ���ʼ�������ļ�·����
*/
void CIniFileLoad::InitIniFilePath()
{
	CString szFullPath;    // MAX_PATH��API�ж����˰ɣ�������128
	GetModuleFileName(NULL,szFullPath.GetBuffer(260),MAX_PATH);
	szFullPath.ReleaseBuffer((int)-1);
	szFullPath = szFullPath.Mid(0,szFullPath.ReverseFind('\\')+1);
	CString str=szFullPath;
	int index=str.Find(_T("Debug"));
	if(index<0)
		index=str.Find(_T("Release"));

	if(index>=0)
		m_strCurPath=str.Left(index);
	else
		m_strCurPath=str;

	m_strSettingPath = m_strCurPath+_T("Ini\\");
}

void CIniFileLoad::SetFileName(CString strFileName)
{
	m_strSettingPath = strFileName;

}

void CIniFileLoad::LoadString(CString strSection,CString strKey,CString &strValue)
{
	DWORD dwSize = 1000;
	GetPrivateProfileString(strSection,strKey,_T(""),
		strValue.GetBuffer(dwSize),dwSize,m_strSettingPath);
	strValue.ReleaseBuffer();
}


void CIniFileLoad::SetString(CString strSection,CString strKey,CString strValue)
{
	WritePrivateProfileString(strSection,strKey,strValue,m_strSettingPath);
}


/*
������������
GetPrivateProfileSectionNames - �� ini �ļ��л�� Section ������
��� ini �������� Section: [sec1] �� [sec2]���򷵻ص��� 'sec1',0,'sec2',0,0 �����㲻֪�� 
ini ������Щ section ��ʱ���������� api ����ȡ����
*/
int CIniFileLoad::GetSections(CStringArray& arrSection)
{

	int i; 
	int iPos=0; 
	int iMaxCount;
	TCHAR chSectionNames[MAX_ALLSECTIONS]={0}; //�ܵ���������ַ���
	TCHAR chSection[MAX_SECTION]={0}; //���һ��������
	GetPrivateProfileSectionNames(chSectionNames,MAX_ALLSECTIONS,m_strSettingPath);

	//����ѭ�����ضϵ�����������0
	for(i=0;i<MAX_ALLSECTIONS;i++)
	{
		if (chSectionNames[i]==0)
			if (chSectionNames[i]==chSectionNames[i+1])
				break;
	}

	iMaxCount=i+1; //Ҫ��һ��0��Ԫ�ء����ҳ�ȫ���ַ����Ľ������֡�
	arrSection.RemoveAll();//���ԭ����

	for(i=0;i<iMaxCount;i++)
	{
		chSection[iPos++]=chSectionNames[i];
		if(chSectionNames[i]==0)
		{ 
			arrSection.Add(chSection);
			memset(chSection,0,MAX_SECTION);
			iPos=0;
		}

	}

	return (int)arrSection.GetSize();
} 

/*
������������
GetPrivateProfileSection- �� ini �ļ��л��һ��Section��ȫ��������ֵ��
���ini����һ���Σ������� "��1=ֵ1" "��2=ֵ2"���򷵻ص��� '��1=ֵ1',0,'��2=ֵ2',0,0 �����㲻֪�� 
���һ�����е����м���ֵ�����������
*/
int CIniFileLoad::GetKeyValues(CStringArray& arrKey,CStringArray& arrValue, LPCTSTR lpSection)
{

	int i; 
	int iPos=0;
	CString strKeyValue;
	int iMaxCount;
	TCHAR chKeyNames[MAX_ALLKEYS]={0}; //�ܵ���������ַ���
	TCHAR chKey[MAX_KEY]={0}; //�������һ������

	GetPrivateProfileSection(lpSection,chKeyNames,MAX_ALLKEYS,m_strSettingPath);

	for(i=0;i<MAX_ALLKEYS;i++)
	{
		if (chKeyNames[i]==0)
			if (chKeyNames[i]==chKeyNames[i+1])
				break;
	}
	if(i==0)
	{
		return 0;
	}

	iMaxCount=i+1; //Ҫ��һ��0��Ԫ�ء����ҳ�ȫ���ַ����Ľ������֡�
	arrKey.RemoveAll();//���ԭ����
	arrValue.RemoveAll();

	for(i=0;i<iMaxCount;i++)
	{
		chKey[iPos++]=chKeyNames[i];
		if(chKeyNames[i]==0)
		{
			strKeyValue=chKey;
			arrKey.Add(strKeyValue.Left(strKeyValue.Find(_T("="))));
			//	CString strValueT=strKeyValue.Mid(strKeyValue.Find("=")+1);
			arrValue.Add(strKeyValue.Mid(strKeyValue.Find(_T("="))+1));
			memset(chKey,0,MAX_KEY);
			iPos=0;
		}
	}
	int nSize = arrKey.GetSize();
	return nSize;
}


void CIniFileLoad::LoadDataInt(CString strSection,CString strKey,int  &nValue)
{
	CString strValue=_T("");
	LoadString(strSection,strKey,strValue);

	if(strValue.IsEmpty())
	{
		nValue=-1;
	}
	else
	{
		nValue=_ttoi(strValue);
	}

}

void CIniFileLoad::LoadDataDouble(CString strSection,CString strKey,double  &fValue)
{
	CString strValue=_T("");
	LoadString(strSection,strKey,strValue);

	if(strValue.IsEmpty())
	{
		fValue=-1.0;
	}
	else
	{
		fValue=_ttof(strValue);
	}

}

void CIniFileLoad::SetDataInt(CString strSection,CString strKey,int nValue)
{
	CString strValue=_T("");
	strValue.Format(_T("%d"),nValue);
	SetString(strSection,strKey,strValue);
}

void CIniFileLoad::SetDataDouble(CString strSection,CString strKey,double fValue)
{
	CString strValue=_T("");
	strValue.Format(_T("%4f"),fValue);
	SetString(strSection,strKey,strValue);
}

int CIniFileLoad::GetParamData(CString strValue,CStringArray &arrayValue,CString strFlag)
{
	int nPos=0;
	while(nPos>=0)
	{
		nPos = strValue.Find(strFlag);
		if(nPos>=0)
		{
			CString strTmp;
			strTmp = strValue.Left(nPos);
			strTmp.TrimLeft();
			strTmp.TrimRight();
			arrayValue.Add(strTmp);
			strValue.Delete(0, nPos+1);
			strValue.TrimLeft();
			strValue.TrimRight();
		}
		else
		{
			strValue.TrimLeft();
			strValue.TrimRight();
			CString strTmp;
			strTmp = strValue;
			arrayValue.Add(strTmp);
		}
	}
	return arrayValue.GetSize();
}

CString CIniFileLoad::GetArrayData(CStringArray &array,int nIndex)
{
	int nSize=array.GetSize();
	if(nIndex<0||nIndex>=nSize)
		return _T("");
	return array.GetAt(nIndex);
}


void CIniFileLoad::LoadDataIntAngle(CString strSection,CString strKey,int &nYPos,int &nZPos,int &nFocusPos)
{
	CString strValue=_T("");
	LoadString(strSection,strKey,strValue);


	CString strYPos = GetFirstParam(strValue);
	CString strZPos = GetFirstParam(strValue);
	CString strFocusPos = GetFirstParam(strValue);
	if(strYPos.IsEmpty())
	{
		nYPos = 0xFFFF;
	}
	else
	{
		nYPos = _ttoi(strYPos);
	}
	if(strZPos.IsEmpty())
	{
		nZPos = 0xFFFF;
	}
	else
	{
		nZPos = _ttoi(strZPos);
	}
	if(strFocusPos.IsEmpty())
	{
		nFocusPos = 0xFFFF;
	}
	else
	{
		nFocusPos = _ttoi(strFocusPos);
	}

}

CString CIniFileLoad::GetFirstParam(CString &strInfo)
{
	int P = strInfo.Find(_T(","));

	if (P < 0)
	{
		return _T("");
	}
	TCHAR cRes[256];
	_stprintf_s(cRes, _T("%s"), strInfo);
	cRes[P] = '\0';

	strInfo.Delete(0, P + 1);
	strInfo.TrimLeft();
	strInfo.TrimRight();

	return cRes;
}

void CIniFileLoad::LoadDataIntMicPosDev(CString strSection,CString strKey,int &nMicPosIndex,int &nMicCamNum)
{
	CString strValue=_T("");
	LoadString(strSection,strKey,strValue);
	CString strMicPosIndex = _T("");
	CString strMicCamNum = _T("");
	int nCount = strValue.Find(_T(","));
	if(nCount<=0)
	{
		strMicPosIndex = strValue;
		strMicCamNum = _T("");
	}
	else
	{
		strMicPosIndex = strValue.Left(nCount);
		strMicCamNum = strValue.Mid(nCount+1,strValue.GetLength());
	}
	if(strMicPosIndex.IsEmpty())
	{
		nMicPosIndex = -1;
	}
	else
	{
		nMicPosIndex = _ttoi(strMicPosIndex);
	}
	if(strMicCamNum.IsEmpty())
	{
		nMicCamNum = 0;
	}
	else
	{
		nMicCamNum = _ttoi(strMicCamNum);
	}
}

