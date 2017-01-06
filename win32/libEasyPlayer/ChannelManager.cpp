/*
	Copyright (c) 2013-2014 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.easydarwin.org
	Author: Gavin@easydarwin.org
*/
#include "ChannelManager.h"
#include <time.h>
#include "vstime.h"
#include "trace.h"


#define		CHANNEL_ID_GAIN			1000

#define	__DELETE_ARRAY(x)	{if (NULL!=x) {delete []x;x=NULL;}}

int CALLBACK __RTSPSourceCallBack( int _channelId, void *_channelPtr, int _frameType, char *pBuf, RTSP_FRAME_INFO* _frameInfo);

CChannelManager	*pChannelManager = NULL;
CChannelManager::CChannelManager(void)
{
	pRealtimePlayThread	=	NULL;
	pAudioPlayThread	=	NULL;
	memset(&d3dAdapter, 0x00, sizeof(D3D_ADAPTER_T));

	InitializeCriticalSection(&crit);
}

CChannelManager::~CChannelManager(void)
{
	Release();
	DeleteCriticalSection(&crit);
}

int	CChannelManager::Initial()
{
	if (NULL == pRealtimePlayThread)
	{
		pRealtimePlayThread = new PLAY_THREAD_OBJ[MAX_CHANNEL_NUM];
		if (NULL == pRealtimePlayThread)		return -1;

		memset(&pRealtimePlayThread[0], 0x00, sizeof(PLAY_THREAD_OBJ)*MAX_CHANNEL_NUM);

		for (int i=0; i<MAX_CHANNEL_NUM; i++)
		{
			InitializeCriticalSection(&pRealtimePlayThread[i].crit);
			pRealtimePlayThread[i].renderFormat = GDI_FORMAT_RGB24;		//Ĭ��ΪGDI��ʾ
			pRealtimePlayThread[i].channelId = i+1;
		}
	}

	if (NULL == pChannelManager)		pChannelManager	=	this;

	return 0;
}

void CChannelManager::Release()
{
	if (NULL != pRealtimePlayThread)
	{
		for (int i=0; i<MAX_CHANNEL_NUM; i++)
		{
			if (NULL != pRealtimePlayThread[i].nvsHandle)
			{
				EasyRTSP_CloseStream(pRealtimePlayThread[i].nvsHandle);
				EasyRTSP_Deinit(&pRealtimePlayThread[i].nvsHandle);
			}

			ClosePlayThread(&pRealtimePlayThread[i]);
			DeleteCriticalSection(&pRealtimePlayThread[i].crit);
			for (int idx=0; idx<MAX_DECODER_NUM; idx++)
			{
				if (pRealtimePlayThread[i].decoderObj[idx].pIntelDecoder)
				{
					Release_IntelHardDecoder(pRealtimePlayThread[i].decoderObj[idx].pIntelDecoder);
				}
			}

		}
		__DELETE_ARRAY(pRealtimePlayThread);
	}
	//������Ƶ�����߳�
	if (NULL != pAudioPlayThread)
	{
		if (NULL != pAudioPlayThread->pSoundPlayer)
		{
			pAudioPlayThread->pSoundPlayer->Close();
			delete pAudioPlayThread->pSoundPlayer;
			pAudioPlayThread->pSoundPlayer = NULL;
		}
		delete pAudioPlayThread;
		pAudioPlayThread = NULL;
	}
}


int	CChannelManager::OpenStream(const char *url, HWND hWnd, RENDER_FORMAT renderFormat, int _rtpovertcp, const char *username, const char *password, MediaSourceCallBack callback, void *userPtr, bool bHardDecode)
{
	if (NULL == pRealtimePlayThread)			return -1;
	if ( (NULL == url) || (0==strcmp(url, "\0")))		return -1;

	int iNvsIdx = -1;
	EnterCriticalSection(&crit);
	do
	{
		for (int i=0; i<MAX_CHANNEL_NUM; i++)
		{
			if (NULL == pRealtimePlayThread[i].nvsHandle)
			{
				iNvsIdx = i;
				break;
			}
		}

		if (iNvsIdx == -1)		break;

		EasyRTSP_Init(&pRealtimePlayThread[iNvsIdx].nvsHandle);
		if (NULL == pRealtimePlayThread[iNvsIdx].nvsHandle)		break;	//�˳�whileѭ��
		
		memcpy(pRealtimePlayThread[iNvsIdx].url, url, strlen(url)+1);
		pRealtimePlayThread[iNvsIdx].pCallback = callback;
		pRealtimePlayThread[iNvsIdx].pUserPtr = userPtr;

		unsigned int mediaType = MEDIA_TYPE_VIDEO | MEDIA_TYPE_AUDIO;
		EasyRTSP_SetCallback(pRealtimePlayThread[iNvsIdx].nvsHandle, __RTSPSourceCallBack);
		EasyRTSP_OpenStream(pRealtimePlayThread[iNvsIdx].nvsHandle, iNvsIdx, (char*)url, 
			_rtpovertcp==0x01?EASY_RTP_OVER_TCP:EASY_RTP_OVER_UDP, mediaType, 
			(char*)username, (char*)password, (int*)&pRealtimePlayThread[iNvsIdx], 1000, 0, 0, 0);

		for (int nI=0; nI<MAX_DECODER_NUM; nI++)
		{
			pRealtimePlayThread[iNvsIdx].decoderObj[nI].bHardDecode = bHardDecode;
		}

		pRealtimePlayThread[iNvsIdx].hWnd = hWnd;
		pRealtimePlayThread[iNvsIdx].renderFormat = (D3D_SUPPORT_FORMAT)renderFormat;
		CreatePlayThread(&pRealtimePlayThread[iNvsIdx]);

	}while (0);
	LeaveCriticalSection(&crit);

	if (iNvsIdx >= 0)	iNvsIdx += CHANNEL_ID_GAIN;
	return iNvsIdx;
}

void CChannelManager::CloseStream(int channelId)
{
	if (NULL == pRealtimePlayThread)			return;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return;

	EnterCriticalSection(&crit);

	//�ر�rtsp client
	EasyRTSP_CloseStream(pRealtimePlayThread[iNvsIdx].nvsHandle);
	EasyRTSP_Deinit(&pRealtimePlayThread[iNvsIdx].nvsHandle);
	//�رղ����߳�
	ClosePlayThread(&pRealtimePlayThread[iNvsIdx]);

	LeaveCriticalSection(&crit);
}


int	CChannelManager::ShowStatisticalInfo(int channelId, int _show)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	pRealtimePlayThread[iNvsIdx].showStatisticalInfo = _show;
	return 0;
}
int	CChannelManager::SetFrameCache(int channelId, int _cache)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	pRealtimePlayThread[iNvsIdx].frameCache = _cache;
	return 0;
}
int	CChannelManager::SetShownToScale(int channelId, int ShownToScale)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	pRealtimePlayThread[iNvsIdx].ShownToScale = ShownToScale;
	return 0;
}
int	CChannelManager::SetDecodeType(int channelId, int _decodeKeyframeOnly)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	pRealtimePlayThread[iNvsIdx].decodeKeyFrameOnly = _decodeKeyframeOnly;
	return 0;
}
int	CChannelManager::SetRenderRect(int channelId, LPRECT lpSrcRect)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	if (NULL == lpSrcRect)	SetRectEmpty(&pRealtimePlayThread[iNvsIdx].rcSrcRender);
	else					CopyRect(&pRealtimePlayThread[iNvsIdx].rcSrcRender, lpSrcRect);

	return 0;
}
int	CChannelManager::DrawLine(int channelId, LPRECT lpRect)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	if (NULL == lpRect)		memset(&pRealtimePlayThread[iNvsIdx].d3d9Line, 0x00, sizeof(D3D9_LINE));
	else
	{
		memset(&pRealtimePlayThread[iNvsIdx].d3d9Line, 0x00, sizeof(D3D9_LINE));
		pRealtimePlayThread[iNvsIdx].d3d9Line.dwColor = RGB(0x0F, 0xF0, 0x00);
		pRealtimePlayThread[iNvsIdx].d3d9Line.pNodes[0].x = lpRect->left;
		pRealtimePlayThread[iNvsIdx].d3d9Line.pNodes[0].y  = lpRect->top;

		pRealtimePlayThread[iNvsIdx].d3d9Line.pNodes[1].x = lpRect->right;
		pRealtimePlayThread[iNvsIdx].d3d9Line.pNodes[1].y = lpRect->top;

		pRealtimePlayThread[iNvsIdx].d3d9Line.pNodes[2].x = lpRect->right;
		pRealtimePlayThread[iNvsIdx].d3d9Line.pNodes[2].y = lpRect->bottom;

		pRealtimePlayThread[iNvsIdx].d3d9Line.pNodes[3].x = lpRect->left;
		pRealtimePlayThread[iNvsIdx].d3d9Line.pNodes[3].y = lpRect->bottom;
		pRealtimePlayThread[iNvsIdx].d3d9Line.uiTotalNodes = 4;
	}

	return 0;
}

int		CChannelManager::SetDragStartPoint(int channelId, POINT pt)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	if (pRealtimePlayThread[iNvsIdx].renderFormat == GDI_FORMAT_RGB24)
	{
		RGB_SetDragStartPoint(pRealtimePlayThread[iNvsIdx].d3dHandle, pt);
	}
	else
	{
		D3D_SetStartPoint(pRealtimePlayThread[iNvsIdx].d3dHandle, pt);
	}
	return 0;
}
int		CChannelManager::SetDragEndPoint(int channelId, POINT pt)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	if (pRealtimePlayThread[iNvsIdx].renderFormat == GDI_FORMAT_RGB24)
	{
		RGB_SetDragEndPoint(pRealtimePlayThread[iNvsIdx].d3dHandle, pt);
	}
	else
	{
		D3D_SetEndPoint(pRealtimePlayThread[iNvsIdx].d3dHandle, pt);
	}
	return 0;
}
int		CChannelManager::ResetDragPoint(int channelId)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	if (pRealtimePlayThread[iNvsIdx].renderFormat == GDI_FORMAT_RGB24)
	{
		RGB_ResetDragPoint(pRealtimePlayThread[iNvsIdx].d3dHandle);
	}
	else
	{
		D3D_ResetSelZone(pRealtimePlayThread[iNvsIdx].d3dHandle);
	}
	return 0;
}

//��������		2014.12.01
int	CChannelManager::PlaySound(int channelId)
{
	int ret = -1;
	if (NULL == pAudioPlayThread)
	{
		pAudioPlayThread = new AUDIO_PLAY_THREAD_OBJ;
		if (NULL == pAudioPlayThread)		return ret;
		memset(pAudioPlayThread, 0x00, sizeof(AUDIO_PLAY_THREAD_OBJ));
	}

	ClearAllSoundData();		//�����ǰ���ڲ���������Ƶ,�����


	int iNvsIdx = channelId - CHANNEL_ID_GAIN + 1;

	if (pAudioPlayThread->channelId != iNvsIdx)
	{
		pAudioPlayThread->channelId = iNvsIdx;//channelId;
	
		if (NULL != pAudioPlayThread->pSoundPlayer)
		{
			pAudioPlayThread->pSoundPlayer->Close();
		}

		pAudioPlayThread->audiochannels	=	0;
		pAudioPlayThread->samplerate	=	0;
		pAudioPlayThread->bitpersample	=	0;
		ret = 0;
	}

	return ret;
}
int	CChannelManager::StopSound()
{
	if (NULL == pAudioPlayThread)		return 0;

	if (NULL != pAudioPlayThread->pSoundPlayer)
	{
		pAudioPlayThread->pSoundPlayer->Close();
	}
	pAudioPlayThread->audiochannels  = 0;
	pAudioPlayThread->channelId = -1;
	return 0;
}
void CChannelManager::ClearAllSoundData()
{
	if (NULL != pChannelManager)
	{
		if (NULL != pChannelManager->pAudioPlayThread)
		{
			if (NULL != pChannelManager->pAudioPlayThread->pSoundPlayer)
			{
				pChannelManager->pAudioPlayThread->pSoundPlayer->Clear();
			}
		}
	}
}

void CChannelManager::CreatePlayThread(PLAY_THREAD_OBJ	*_pPlayThread)
{
	if (NULL == _pPlayThread)		return;

	if (_pPlayThread->decodeThread.flag == 0x00)
	{
		//_pPlayThread->ch_tally = 0;
		_pPlayThread->decodeThread.flag = 0x01;

		_pPlayThread->decodeThread.hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_lpDecodeThread, _pPlayThread, 0, NULL);
		while (_pPlayThread->decodeThread.flag!=0x02 && _pPlayThread->decodeThread.flag!=0x00)	{Sleep(100);}
		if (NULL != _pPlayThread->decodeThread.hThread)
		{
			SetThreadPriority(_pPlayThread->decodeThread.hThread, THREAD_PRIORITY_HIGHEST);
		}
	}
	if (_pPlayThread->displayThread.flag == 0x00)
	{
		_pPlayThread->displayThread.flag = 0x01;
		_pPlayThread->displayThread.hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_lpDisplayThread, _pPlayThread, 0, NULL);
		while (_pPlayThread->displayThread.flag!=0x02 && _pPlayThread->displayThread.flag!=0x00)	{Sleep(100);}
		if (NULL != _pPlayThread->displayThread.hThread)
		{
			SetThreadPriority(_pPlayThread->displayThread.hThread, THREAD_PRIORITY_HIGHEST);
		}
	}
}
void CChannelManager::ClosePlayThread(PLAY_THREAD_OBJ	*_pPlayThread)
{
	if (NULL == _pPlayThread)		return;

	//�رս����߳�
	if (_pPlayThread->decodeThread.flag != 0x00)
	{
#ifdef _DEBUG
		_TRACE("�رղ����߳�[%d]\n", _pPlayThread->channelId);
#endif
		_pPlayThread->decodeThread.flag = 0x03;
		while (_pPlayThread->decodeThread.flag!=0x00)	{Sleep(100);}
	}
	if (NULL != _pPlayThread->decodeThread.hThread)
	{
		CloseHandle(_pPlayThread->decodeThread.hThread);
		_pPlayThread->decodeThread.hThread = NULL;
	}
	for (int i=0; i<MAX_DECODER_NUM; i++)		//�رս�����
	{
		if (NULL != _pPlayThread->decoderObj[i].ffDecoder)
		{
			FFD_Deinit(&_pPlayThread->decoderObj[i].ffDecoder);
			_pPlayThread->decoderObj[i].ffDecoder = NULL;
		}
		if (NULL != _pPlayThread->decoderObj[i].pIntelDecoder)
		{
			Release_IntelHardDecoder(_pPlayThread->decoderObj[i].pIntelDecoder);
			_pPlayThread->decoderObj[i].pIntelDecoder = NULL;
		}
		_pPlayThread->decoderObj[i].bHardDecode = false;
		memset(&_pPlayThread->decoderObj[i].codec, 0x00, sizeof(CODEC_T));
		_pPlayThread->decoderObj[i].yuv_size = 0;
	}

	//�رղ����߳�
	if (_pPlayThread->displayThread.flag != 0x00)
	{
		_pPlayThread->displayThread.flag = 0x03;
		while (_pPlayThread->displayThread.flag!=0x00)	{Sleep(100);}
	}
	if (NULL != _pPlayThread->displayThread.hThread)
	{
		CloseHandle(_pPlayThread->displayThread.hThread);
		_pPlayThread->displayThread.hThread = NULL;
	}
	for (int i=0; i<MAX_YUV_FRAME_NUM; i++)
	{
		__DELETE_ARRAY(_pPlayThread->yuvFrame[i].pYuvBuf);
		memset(&_pPlayThread->yuvFrame[i].frameinfo, 0x00, sizeof(MEDIA_FRAME_INFO));
		_pPlayThread->yuvFrame[i].Yuvsize = 0;
	}

	//�ͷŶ���
	if (NULL != _pPlayThread->pAVQueue)
	{
		SSQ_Deinit(_pPlayThread->pAVQueue);
		delete _pPlayThread->pAVQueue;
		_pPlayThread->pAVQueue = NULL;
	}
	_pPlayThread->initQueue = 0x00;
	_pPlayThread->frameCache	=	NULL;

	memset(&_pPlayThread->decodeThread, 0x00, sizeof(THREAD_OBJ));
	memset(&_pPlayThread->displayThread, 0x00, sizeof(THREAD_OBJ));
	_pPlayThread->hWnd	=	NULL;
}



int	CChannelManager::SetAudioParams(unsigned int _channel, unsigned int _samplerate, unsigned int _bitpersample)
{
	if (NULL == pAudioPlayThread)		return -1;

	WAVEFORMATEX wfx = {0,};
	wfx.cbSize = sizeof(WAVEFORMATEX);
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nSamplesPerSec = _samplerate;
	wfx.wBitsPerSample = _bitpersample;
	wfx.nChannels = _channel;
	wfx.nBlockAlign =  wfx.nChannels * ( wfx.wBitsPerSample / 8 );//(wfx.wBitsPerSample*wfx.nChannels)>>3; //wfx.nChannels * ( wfx.wBitsPerSample / 8 );
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

	wfx.nAvgBytesPerSec	= wfx.nSamplesPerSec * wfx.nChannels * wfx.wBitsPerSample / 8;
	wfx.nBlockAlign		= wfx.nChannels * wfx.wBitsPerSample / 8;

	if (NULL == pAudioPlayThread->pSoundPlayer)
	{
		pAudioPlayThread->pSoundPlayer = new CSoundPlayer();
	}
	if (NULL != pAudioPlayThread->pSoundPlayer)
	{
		pAudioPlayThread->pSoundPlayer->Close();
		pAudioPlayThread->pSoundPlayer->Open(wfx);
	}
	pAudioPlayThread->audiochannels	=	_channel;
	return 0;
}

void ParseDecoder2Render(int *_decoder, int _width, int _height, int renderFormat, int *yuvsize)
{
	if (NULL == _decoder)		return;
	int nDecoder = 0x00;
	int nYUVSize = 0x00;
	if (renderFormat == D3D_FORMAT_UYVY)
	{
		nDecoder	=	17;	//PIX_FMT_UYVY422
		nYUVSize	=	_width*_height*2+1;
	}
	else if (renderFormat == D3D_FORMAT_YV12)
	{
		nDecoder	=	0;//OUTPUT_PIX_FMT_YUV420P;
		nYUVSize	=	_width*_height*3/2+1;
	}
	else if (renderFormat == D3D_FORMAT_YUY2)
	{
		nDecoder	=	1;//OUTPUT_PIX_FMT_YUV422;
		nYUVSize	=	_width*_height*2+1;
	}
	else if (renderFormat == D3D_FORMAT_RGB565)				//23 ok
	{
		nDecoder	=	44;
		nYUVSize	=	_width*_height*2+1;
	}
	else if (renderFormat == D3D_FORMAT_RGB555)				//23 ok
	{
		nDecoder	=	46;
		nYUVSize	=	_width*_height*2+1;
	}
	else if (renderFormat == D3D_FORMAT_X8R8G8B8)
	{
		nDecoder	=	30;
		nYUVSize	=	_width*_height*4+1;
	}
	else if (renderFormat == D3D_FORMAT_A8R8G8B8)			//21 ok
	{
		nDecoder	=	30;
		nYUVSize	=	_width*_height*4+1;
	}
	else if (renderFormat == (D3D_SUPPORT_FORMAT)22)		//22	ok
	{
		nDecoder	=	28;
		nYUVSize	=	_width*_height*4+1;
	}
	else if (renderFormat == (D3D_SUPPORT_FORMAT)1498831189)
	{
		nDecoder	=	17;
		nYUVSize	=	_width*_height*4+1;
	}
	else if (renderFormat == (D3D_SUPPORT_FORMAT)36)
	{
		nDecoder	=	32;
		nYUVSize	=	_width*_height*4+1;
	}
	else if (renderFormat == (D3D_SUPPORT_FORMAT)20)
	{
		nDecoder	=	3;
		nYUVSize	=	_width*_height*3+1;
	}
	else if (renderFormat == GDI_FORMAT_RGB24)
	{
		nDecoder	=	3;
		nYUVSize	=	_width*_height*3+1;
	}
	else
	{
		nDecoder	=	28;
		nYUVSize	=	_width*_height*4+1;
	}

	if (NULL != _decoder)	*_decoder = nDecoder;
	if (NULL != yuvsize)	*yuvsize  = nYUVSize;
}

DECODER_OBJ	*GetDecoder(PLAY_THREAD_OBJ	*_pPlayThread, unsigned int mediaType, MEDIA_FRAME_INFO *_frameinfo)
{
	if (NULL == _pPlayThread || NULL==_frameinfo)									return NULL;


	int iIdx = -1;
	int iExist = -1;
	for (int i=0; i<MAX_DECODER_NUM; i++)
	{
		if (MEDIA_TYPE_VIDEO == mediaType)
		{
			if (_frameinfo->width < 1 || _frameinfo->height<1 || _frameinfo->codec<1)		return NULL;

			if (_pPlayThread->decoderObj[i].codec.vidCodec == 0x00 &&
				_pPlayThread->decoderObj[i].codec.width == 0x00 &&
				_pPlayThread->decoderObj[i].codec.height== 0x00 && iIdx==-1)
			{
				iIdx = i;
			}

			if (_pPlayThread->decoderObj[i].codec.vidCodec == _frameinfo->codec &&
				_pPlayThread->decoderObj[i].codec.width == _frameinfo->width &&
				_pPlayThread->decoderObj[i].codec.height== _frameinfo->height)
			{
				if (NULL == _pPlayThread->decoderObj[i].ffDecoder)
				{
					iExist = i;
					int nDecoder = OUTPUT_PIX_FMT_YUV420P;
					//ParseDecoder2Render(&nDecoder, _frameinfo->width, _frameinfo->height, _pPlayThread->renderFormat, &_pPlayThread->decoderObj[i].yuv_size);

					if (_pPlayThread->renderFormat == GDI_FORMAT_RGB24)
					{
						ParseDecoder2Render(&nDecoder, _frameinfo->width, _frameinfo->height, _pPlayThread->renderFormat, &_pPlayThread->decoderObj[i].yuv_size);
					}
					else
					{
#ifdef __ENABLE_SSE
						ParseDecoder2Render(&nDecoder, _frameinfo->width, _frameinfo->height, DISPLAY_FORMAT_YV12, &_pPlayThread->decoderObj[i].yuv_size);
#else
						ParseDecoder2Render(&nDecoder, _frameinfo->width, _frameinfo->height, _pPlayThread->renderFormat, &_pPlayThread->decoderObj[i].yuv_size);
#endif
					}

					FFD_Init(&_pPlayThread->decoderObj[i].ffDecoder);
					FFD_SetVideoDecoderParam(_pPlayThread->decoderObj[i].ffDecoder, _frameinfo->width, _frameinfo->height, _frameinfo->codec, nDecoder);
				}
				if (NULL == _pPlayThread->decoderObj[i].pIntelDecoder && _pPlayThread->decoderObj[i].bHardDecode)
				{
					//Ӳ�������ʼ��
					_pPlayThread->decoderObj[iIdx].pIntelDecoder = Create_IntelHardDecoder();
					int nRet =_pPlayThread->decoderObj[iIdx].pIntelDecoder->Init(_pPlayThread->hWnd);
					if (nRet<0)
					{
						Release_IntelHardDecoder(_pPlayThread->decoderObj[iIdx].pIntelDecoder);
						_pPlayThread->decoderObj[iIdx].pIntelDecoder = NULL;
					}
				}
				return &_pPlayThread->decoderObj[i];
			}

			if (iIdx>=0)
			{
				if (NULL == _pPlayThread->decoderObj[i].pIntelDecoder && _pPlayThread->decoderObj[i].bHardDecode)
				{
					//Ӳ�������ʼ��
					_pPlayThread->decoderObj[iIdx].pIntelDecoder = Create_IntelHardDecoder();
					int nRet =_pPlayThread->decoderObj[iIdx].pIntelDecoder->Init(_pPlayThread->hWnd);
					if (nRet<0)
					{
						Release_IntelHardDecoder(_pPlayThread->decoderObj[iIdx].pIntelDecoder);
						_pPlayThread->decoderObj[iIdx].pIntelDecoder = NULL;
					}
				}

				if (NULL == _pPlayThread->decoderObj[iIdx].ffDecoder)
				{
					int nDecoder = OUTPUT_PIX_FMT_YUV420P;
					if (_pPlayThread->renderFormat == GDI_FORMAT_RGB24)
					{
						ParseDecoder2Render(&nDecoder, _frameinfo->width, _frameinfo->height, _pPlayThread->renderFormat, &_pPlayThread->decoderObj[iIdx].yuv_size);
					}
					else
					{
#ifdef __ENABLE_SSE
						ParseDecoder2Render(&nDecoder, _frameinfo->width, _frameinfo->height, DISPLAY_FORMAT_YV12, &_pPlayThread->decoderObj[iIdx].yuv_size);
#else
						ParseDecoder2Render(&nDecoder, _frameinfo->width, _frameinfo->height, _pPlayThread->renderFormat, &_pPlayThread->decoderObj[iIdx].yuv_size);
#endif
					}

					FFD_Init(&_pPlayThread->decoderObj[iIdx].ffDecoder);
					FFD_SetVideoDecoderParam(_pPlayThread->decoderObj[iIdx].ffDecoder, _frameinfo->width, _frameinfo->height, _frameinfo->codec, nDecoder);

					if (NULL != _pPlayThread->decoderObj[iIdx].ffDecoder )
					{
						EnterCriticalSection(&_pPlayThread->crit);
						_pPlayThread->resetD3d	=	true;
						LeaveCriticalSection(&_pPlayThread->crit);

						_pPlayThread->decoderObj[iIdx].codec.vidCodec	= _frameinfo->codec;
						_pPlayThread->decoderObj[iIdx].codec.width	= _frameinfo->width;
						_pPlayThread->decoderObj[iIdx].codec.height = _frameinfo->height;
						
						return &_pPlayThread->decoderObj[iIdx];
					}
					else
					{
						return NULL;
					}
				}

			}
		}
		else if (MEDIA_TYPE_AUDIO == mediaType)
		{
			if (_frameinfo->sample_rate < 1 || _frameinfo->channels<1 || _frameinfo->codec<1)		return NULL;

			if (_pPlayThread->decoderObj[i].codec.audCodec == 0x00 &&
				_pPlayThread->decoderObj[i].codec.samplerate == 0x00 &&
				_pPlayThread->decoderObj[i].codec.channels== 0x00 && iIdx==-1)
			{
				iIdx = i;
			}

			if (_pPlayThread->decoderObj[i].codec.audCodec == _frameinfo->codec &&
				_pPlayThread->decoderObj[i].codec.samplerate == _frameinfo->sample_rate &&
				_pPlayThread->decoderObj[i].codec.channels== _frameinfo->channels)
			{
				if (NULL == _pPlayThread->decoderObj[i].ffDecoder)
				{
					iExist = i;

					FFD_Init(&_pPlayThread->decoderObj[i].ffDecoder);
					FFD_SetAudioDecoderParam(_pPlayThread->decoderObj[i].ffDecoder, _frameinfo->channels, _frameinfo->sample_rate, _frameinfo->codec);
				}
				return &_pPlayThread->decoderObj[i];
			}

			if (iIdx>=0)
			{
				if (NULL == _pPlayThread->decoderObj[iIdx].ffDecoder)
				{
					FFD_Init(&_pPlayThread->decoderObj[iIdx].ffDecoder);
					FFD_SetAudioDecoderParam(_pPlayThread->decoderObj[i].ffDecoder, _frameinfo->channels, _frameinfo->sample_rate, _frameinfo->codec);

					if (NULL != _pPlayThread->decoderObj[iIdx].ffDecoder)
					{
						_pPlayThread->decoderObj[iIdx].codec.audCodec	= _frameinfo->codec;
						_pPlayThread->decoderObj[iIdx].codec.samplerate	= _frameinfo->sample_rate;
						_pPlayThread->decoderObj[iIdx].codec.channels = _frameinfo->channels;

						return &_pPlayThread->decoderObj[iIdx];
					}
					else
					{
						return NULL;
					}
				}
				else
				{
					FFD_SetAudioDecoderParam(_pPlayThread->decoderObj[i].ffDecoder, _frameinfo->channels, _frameinfo->sample_rate, _frameinfo->codec);
					_pPlayThread->decoderObj[iIdx].codec.audCodec	= _frameinfo->codec;
					_pPlayThread->decoderObj[iIdx].codec.samplerate	= _frameinfo->sample_rate;
					_pPlayThread->decoderObj[iIdx].codec.channels = _frameinfo->channels;

					return &_pPlayThread->decoderObj[iIdx];
				}
			}
		}
	}

	return NULL;
}

LPTHREAD_START_ROUTINE CChannelManager::_lpDecodeThread( LPVOID _pParam )
{
	PLAY_THREAD_OBJ *pThread = (PLAY_THREAD_OBJ*)_pParam;
	if (NULL == pThread)			return 0;

	pThread->decodeThread.flag	=	0x02;

#ifdef _DEBUG
	_TRACE("�����߳�[%d]������. ThreadId:%d ...\n", pThread->channelId, GetCurrentThreadId());
#endif

	//SetThreadAffinityMask(GetCurrentThread(), 1);

	if (NULL != pThread->yuvFrame)
	{
		for (int i=0; i<MAX_YUV_FRAME_NUM; i++)
		{
			memset(&pThread->yuvFrame[i].frameinfo, 0x00, sizeof(MEDIA_FRAME_INFO));
		}
	}
	unsigned int channelid = 0;
	unsigned int mediatype = 0;
	MEDIA_FRAME_INFO	frameinfo;
	int buf_size = 1024*1024;
	//int buf_size = 1920*1080*2;

	char *pbuf = new char[buf_size];
	if (NULL == pbuf)
	{
		pThread->decodeThread.flag	=	0x00;
		return 0;
	}
	memset(pbuf, 0x00, buf_size);

	EasyAACEncoder_Handle m_pAACEncoderHandle = NULL;
	char* m_pAACEncBufer = new char[buf_size];
	memset(m_pAACEncBufer, 0x00, buf_size);

	pThread->decodeYuvIdx	=	0;
	memset(&frameinfo, 0x00, sizeof(MEDIA_FRAME_INFO));

	//#define AVCODEC_MAX_AUDIO_FRAME_SIZE	(192000)
#define AVCODEC_MAX_AUDIO_FRAME_SIZE	(64000)
	int audbuf_len = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
	unsigned char *audio_buf = new unsigned char[audbuf_len+1];
	memset(audio_buf, 0x00, audbuf_len);

    //FILE *fES = fopen("1920x1080.h264", "wb");

	while (1)
	{
		if (pThread->decodeThread.flag == 0x03)		break;

		if (pThread->initQueue == 0x00 || NULL==pThread->pAVQueue)
		{
			Sleep(1);
			continue;
		}

		int ret = SSQ_GetData(pThread->pAVQueue, &channelid, &mediatype, &frameinfo, pbuf);
		if (ret < 0)
		{
			_VS_BEGIN_TIME_PERIOD(1);
			__VS_Delay(1);
			_VS_END_TIME_PERIOD(1);
			continue;
		}

		if (mediatype == MEDIA_TYPE_VIDEO)
		{
#ifdef _DEBUG1
			_TRACE("�����߳�[%d]����...%d   %d x %d\n", pThread->id, channelid, frameinfo.width, frameinfo.height);
#endif
			//==============================================

            //if (NULL != fES)
            {
                //fwrite(pbuf, 1, frameinfo.length, fES);
            }

			//_TRACE("DECODE queue: %d\n", pChannelObj->pQueue->pQueHeader->videoframes);
			if (pThread->frameQueue > MAX_CACHE_FRAME)
			{
				//_TRACE("[ch%d]����֡��[%d]>�趨֡��[%d].  ��ն��в��ȴ���һ��Key frame.\n", pThread->renderCh, pThread->framequeue, MAX_CACHE_FRAME);
				_TRACE("[ch%d]����֡��[%d]>�趨֡��[%d].  ��ն��в��ȴ���һ��Key frame.\n", pThread->channelId, pThread->frameQueue, MAX_CACHE_FRAME);

				SSQ_Clear(pThread->pAVQueue);
				pThread->findKeyframe = 0x01;
				pThread->frameQueue = pThread->pAVQueue->pQueHeader->videoframes;
				continue;

				EnterCriticalSection(&pThread->crit);
				SSQ_Clear(pThread->pAVQueue);
				pThread->rtpTimestamp = 0;
				pThread->decodeYuvIdx = 0;
				pThread->findKeyframe = 0x01;
				for (int i=0; i<MAX_YUV_FRAME_NUM; i++)
				{
					memset(&pThread->yuvFrame[i].frameinfo, 0x00, sizeof(MEDIA_FRAME_INFO));
				}
				LeaveCriticalSection(&pThread->crit);
			}

			if ( (pThread->findKeyframe==0x01) && (frameinfo.type==EASY_SDK_VIDEO_FRAME_I) )
			{
				pThread->findKeyframe = 0x00;
			}
			else if (pThread->findKeyframe==0x01)
			{
#ifdef _DEBUG
				//_TRACE("[ch%d]Find Keyframe..\n", pThread->renderCh);
				_TRACE("[ch%d]Find Keyframe..\n", pThread->channelId);
#endif
				continue;
			}

			DECODER_OBJ *pDecoderObj = GetDecoder(pThread, MEDIA_TYPE_VIDEO, &frameinfo);		//��ȡ��Ӧ�Ľ�����
			if (NULL == pDecoderObj)
			{
#ifdef _DEBUG
				_TRACE("[ch%d]��ȡ������ʧ��.  %d x %d\n", pThread->channelId, frameinfo.width, frameinfo.height);
#endif
				_VS_BEGIN_TIME_PERIOD(1);
				__VS_Delay(1);
				_VS_END_TIME_PERIOD(1);
				continue;
			}

			//�����������ڴ�С�ڵ�ǰ��Ҫ���ڴ��С,����������
			if ( (pThread->yuvFrame[pThread->decodeYuvIdx].Yuvsize < pDecoderObj->yuv_size) && (NULL != pThread->yuvFrame[pThread->decodeYuvIdx].pYuvBuf))
			{
				__DELETE_ARRAY(pThread->yuvFrame[pThread->decodeYuvIdx].pYuvBuf);
			}
			if (NULL == pThread->yuvFrame[pThread->decodeYuvIdx].pYuvBuf)
			{
				pThread->yuvFrame[pThread->decodeYuvIdx].Yuvsize = pDecoderObj->yuv_size;//MAX_FRAME_SIZE;//pThread->yuv_size;
				pThread->yuvFrame[pThread->decodeYuvIdx].pYuvBuf = new char[pThread->yuvFrame[pThread->decodeYuvIdx].Yuvsize];
			}
			if (NULL == pThread->yuvFrame[pThread->decodeYuvIdx].pYuvBuf)		continue;

			while (pThread->yuvFrame[pThread->decodeYuvIdx].frameinfo.length > 0)		//�ȴ���֡��ʾ���
			{
				if (pThread->decodeThread.flag == 0x03)		break;

				_VS_BEGIN_TIME_PERIOD(1);
				__VS_Delay(1);
				_VS_END_TIME_PERIOD(1);
			}
			if (pThread->decodeThread.flag == 0x03)		break;

			//¼��
			if (pThread->manuRecording == 0x01 && NULL==pThread->m_pMP4Writer && frameinfo.type==EASY_SDK_VIDEO_FRAME_I)//����¼��
			{
				if (!pThread->m_pMP4Writer)
				{
					pThread->m_pMP4Writer = new EasyMP4Writer();
				}

				unsigned int timestamp = (unsigned int)time(NULL);
				time_t tt = timestamp;
				struct tm *_time = localtime(&tt);
				char szTime[64] = {0,};
				strftime(szTime, 32, "%Y%m%d%H%M%S", _time);

				int nRecordPathLen = strlen(pThread->manuRecordingPath);
				if (nRecordPathLen==0 || (pThread->manuRecordingPath[nRecordPathLen-1] != '/' && pThread->manuRecordingPath[nRecordPathLen-1] != '\\') )
				{
					pThread->manuRecordingPath[nRecordPathLen] = '/';
				}
				
				char sFileName[512] = {0,};
				sprintf(sFileName, "%sch%d_%s.mp4", pThread->manuRecordingPath, pThread->channelId, szTime);
				 // ע�⣺�����û����ƵӦ����ZOUTFILE_FLAG_VIDEO��û����ƵӦ����ZOUTFILE_FLAG_AUDIO��
				 // ����Media player�Ȳ��������ܲ����������� [10/10/2016 SwordTwelve]
				if (!pThread->m_pMP4Writer->CreateMP4File(sFileName,  ZOUTFILE_FLAG_FULL))//ZOUTFILE_FLAG_VIDEO
				{
					delete pThread->m_pMP4Writer;
					pThread->m_pMP4Writer = NULL;
					//return -1;
				}		
				else
				{
				}
			}
			if ( NULL != pThread->m_pMP4Writer)//¼��
			{
				if (pThread->manuRecording == 0x00 )//ֹͣ¼��
				{
					pThread->m_pMP4Writer->SaveFile();
					delete pThread->m_pMP4Writer;
					pThread->m_pMP4Writer=NULL;
				} 
				else//����MP4д����
				{
					pThread->m_pMP4Writer->WriteMp4File((unsigned char*)pbuf, frameinfo.length, 
						(frameinfo.type==EASY_SDK_VIDEO_FRAME_I)?true:false, 
						frameinfo.timestamp_sec*1000+frameinfo.timestamp_usec/1000, frameinfo.width, frameinfo.height);
				}
			}

			// ����֧��MP4Creator¼��mp4����������׳��ֱ����� [9/20/2016 dingshuai]
#if 0
			//�ֶ�¼��
			if (NULL != pThread->mp4cHandle)
			{
				//30fps * 60 seconds * 30 minutes  = �ֶ�¼�����С��30����
				if (pThread->vidFrameNum >= 30*60*30 || (pThread->manuRecording == 0x00) )
				{
					MP4C_CloseMp4File(pThread->mp4cHandle);
					MP4C_Deinit(&pThread->mp4cHandle);
					pThread->mp4cHandle = NULL;
					pThread->vidFrameNum = 0;
				}
				else
				{
					pThread->vidFrameNum ++;
					MP4C_AddFrame(pThread->mp4cHandle, MEDIA_TYPE_VIDEO, (unsigned char*)pbuf, frameinfo.length, frameinfo.type, frameinfo.timestamp_sec, frameinfo.timestamp_sec*1000+frameinfo.timestamp_usec/1000, frameinfo.fps);
				}
			}
			else if (pThread->manuRecording == 0x01 && NULL==pThread->mp4cHandle)
			{
				unsigned int timestamp = (unsigned int)time(NULL);
				time_t tt = timestamp;
				struct tm *_time = localtime(&tt);
				char szTime[64] = {0,};
				strftime(szTime, 32, "%Y%m%d %H%M%S", _time);

				memset(pThread->manuRecordingFile, 0x00, sizeof(pThread->manuRecordingFile));
				sprintf(pThread->manuRecordingFile, "ch%d_%s.mp4", pThread->channelId, szTime);
				//sprintf(pThread->manuRecordingFile, "ch%d.mp4", pThread->channelId);

				MP4C_Init(&pThread->mp4cHandle);
				MP4C_SetMp4VideoInfo(pThread->mp4cHandle, VIDEO_CODEC_H264, frameinfo.width, frameinfo.height, frameinfo.fps);
				MP4C_SetMp4AudioInfo(pThread->mp4cHandle, AUDIO_CODEC_AAC, 8000, 1);
				MP4C_CreateMp4File(pThread->mp4cHandle, pThread->manuRecordingFile, 1024*1024*2);
			}
#endif

			if (pThread->decodeKeyFrameOnly == 0x01)
			{
				if (frameinfo.type != EASY_SDK_VIDEO_FRAME_I)
				{
					pThread->findKeyframe = 0x01;
					continue;
				}
			}

			//����
			EnterCriticalSection(&pThread->crit);
			int nRet = 1;
			if (pDecoderObj->pIntelDecoder)
			{
				nRet = pDecoderObj->pIntelDecoder->Decode((unsigned char*)pbuf, frameinfo.length, (unsigned char*)/*pThread->yuvFrame[pThread->decodeYuvIdx].pYuvBuf*/NULL);
			}
			else
			{
				nRet = FFD_DecodeVideo3(pDecoderObj->ffDecoder, pbuf, frameinfo.length, pThread->yuvFrame[pThread->decodeYuvIdx].pYuvBuf, frameinfo.width, frameinfo.height);
				if (0 != nRet)
				{
					_TRACE("����ʧ��... framesize:%d   %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", frameinfo.length, 
						(unsigned char)pbuf[0], (unsigned char)pbuf[1], (unsigned char)pbuf[2], (unsigned char)pbuf[3], (unsigned char)pbuf[4],
						(unsigned char)pbuf[5], (unsigned char)pbuf[6], (unsigned char)pbuf[7], (unsigned char)pbuf[8], (unsigned char)pbuf[9]);

					if (frameinfo.type == EASY_SDK_VIDEO_FRAME_I)		//�ؼ�֡
					{
						_TRACE("[ch%d]��ǰ�ؼ�֡����ʧ��...\n", pThread->channelId);
#ifdef _DEBUG
						FILE *f = fopen("keyframe.txt", "wb");
						if (NULL != f)
						{
							fwrite(pbuf, 1, frameinfo.length, f);
							fclose(f);
						}
#endif
					}
					else
					{
#ifdef _DEBUG
						FILE *f = fopen("pframe.txt", "wb");
						if (NULL != f)
						{
							fwrite(pbuf, 1, frameinfo.length, f);
							fclose(f);
						}
#endif
					}
					pThread->findKeyframe = 0x01;
				}
				else
				{
					memcpy(&pThread->yuvFrame[pThread->decodeYuvIdx].frameinfo, &frameinfo, sizeof(MEDIA_FRAME_INFO));

					pThread->decodeYuvIdx ++;
					if (pThread->decodeYuvIdx >= MAX_YUV_FRAME_NUM)		pThread->decodeYuvIdx = 0;

					//ץͼ
					if (pThread->manuScreenshot == 0x01 && pThread->renderFormat == D3D_FORMAT_YUY2)//Just support YUY2->jpg
					{
						unsigned int timestamp = (unsigned int)time(NULL);
						time_t tt = timestamp;
						struct tm *_time = localtime(&tt);
						char szTime[64] = {0,};
						strftime(szTime, 32, "%Y%m%d-%H%M%S", _time);
	
// 						char strPath[512] = {0,};
// 						sprintf(strPath , "%sch%d_%s.jpg", pThread->strScreenCapturePath, pThread->channelId, szTime) ;

						PhotoShotThreadInfo* pShotThreadInfo = new PhotoShotThreadInfo;
						sprintf(pShotThreadInfo->strPath , "%sch%d_%s.jpg", pThread->strScreenCapturePath, pThread->channelId, szTime) ;

						int nYuvBufLen = frameinfo.width*frameinfo.height<<1;
						pShotThreadInfo->pYuvBuf = new unsigned char[nYuvBufLen];
						pShotThreadInfo->width = frameinfo.width;
						pShotThreadInfo->height = frameinfo.height;

						memcpy(pShotThreadInfo->pYuvBuf, pThread->yuvFrame[pThread->decodeYuvIdx].pYuvBuf, nYuvBufLen);
						CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_lpPhotoShotThread, pShotThreadInfo, 0, NULL);
						pThread->manuScreenshot = 0;
					}
				}
			}

			LeaveCriticalSection(&pThread->crit);
		}
		else if (MEDIA_TYPE_AUDIO == mediatype)		//��Ƶ
		{
#if 1
			if (NULL != pChannelManager)
			{
				if (NULL != pChannelManager->pAudioPlayThread && pChannelManager->pAudioPlayThread->channelId == pThread->channelId)
				{
					char* pDecBuffer = pbuf;
					unsigned int nDecBufLen = frameinfo.length;
					if (frameinfo.codec == EASY_SDK_AUDIO_CODEC_G711U || EASY_SDK_AUDIO_CODEC_G726 == frameinfo.codec || frameinfo.codec == EASY_SDK_AUDIO_CODEC_G711A) 
					{
						if (!m_pAACEncoderHandle)
						{
							InitParam initParam;
							initParam.u32AudioSamplerate=frameinfo.sample_rate;
							initParam.ucAudioChannel=frameinfo.channels;
							initParam.u32PCMBitSize=frameinfo.bits_per_sample;
							if (frameinfo.codec == EASY_SDK_AUDIO_CODEC_G711U)
							{
								initParam.ucAudioCodec = Law_ULaw;
							} 
							else if (frameinfo.codec == EASY_SDK_AUDIO_CODEC_G726)
							{
								initParam.ucAudioCodec = Law_G726;
							}
							else if (frameinfo.codec == EASY_SDK_AUDIO_CODEC_G711A)
							{
								initParam.ucAudioCodec = Law_ALaw;
							}
							m_pAACEncoderHandle = Easy_AACEncoder_Init( initParam);
						}
						unsigned int out_len = 0;
						int nRet = Easy_AACEncoder_Encode(m_pAACEncoderHandle, (unsigned char*)/*m_pG711EncBufer*/pbuf, /*m_nG711BufferLen*/frameinfo.length, (unsigned char*)m_pAACEncBufer, &out_len) ;
						if (nRet>0&&out_len>0)
						{
							pDecBuffer = m_pAACEncBufer;
							nDecBufLen = out_len;
							frameinfo.codec = EASY_SDK_AUDIO_CODEC_AAC;
						} 
						else
						{
							continue;
						}
					}

					if (pThread&&pThread->m_pMP4Writer && frameinfo.codec == EASY_SDK_AUDIO_CODEC_AAC)//��Ƶд��MP4�ļ�(now we just support AAC)
					{
						if (pThread->m_pMP4Writer->CanWrite())
						{
							pThread->m_pMP4Writer->WriteAACToMp4File((unsigned char*)pbuf, 
								frameinfo.length, frameinfo.timestamp_sec*1000+frameinfo.timestamp_usec/1000, frameinfo.sample_rate, frameinfo.channels, frameinfo.bits_per_sample);
						}
					}

					DECODER_OBJ *pDecoderObj = GetDecoder(pThread, MEDIA_TYPE_AUDIO, &frameinfo);
					if (NULL == pDecoderObj)
					{
#ifdef _DEBUG
						_TRACE("[ch%d]��ȡ��Ƶ������ʧ��: %d.\n", pThread->channelId, frameinfo.codec);
#endif
						continue;
					}

					memset(audio_buf, 0x00, audbuf_len);
					int pcm_data_size = 0;
					int ret = FFD_DecodeAudio(pDecoderObj->ffDecoder, (char*)pDecBuffer, nDecBufLen, (char *)audio_buf, &pcm_data_size);	//��Ƶ����(֧��g711(ulaw)��AAC)
					if (ret == 0)
					{
						//����
						if (pChannelManager->pAudioPlayThread->audiochannels == 0)
						{
							pChannelManager->SetAudioParams(pDecoderObj->codec.channels, pDecoderObj->codec.samplerate, 16);//32);// 16);
						}
						if (pChannelManager->pAudioPlayThread->audiochannels != 0 && NULL!=pChannelManager->pAudioPlayThread->pSoundPlayer)
						{
							pChannelManager->pAudioPlayThread->pSoundPlayer->Write((char *)audio_buf, pcm_data_size);
						}
					}
					else
					{
#ifdef _DEBUG
						_TRACE("[ERROR]������Ƶʧ��....\n");
#endif
					}
				}
			}
#endif
		}
		else if (MEDIA_TYPE_EVENT == mediatype)
		{
			if (frameinfo.type == 0xF1)		//Loss Packet
			{
				pThread->dwLosspacketTime	=	GetTickCount();
			}
			else if (frameinfo.type == 0xFF)	//Disconnect
			{
				pThread->dwDisconnectTime	=	GetTickCount();
			}
		}
	}

	if ( (NULL != pChannelManager) && (NULL != pChannelManager->pAudioPlayThread) && (pChannelManager->pAudioPlayThread->channelId == pThread->channelId) )
	{
		if (NULL!=pChannelManager->pAudioPlayThread->pSoundPlayer)
		{
			pChannelManager->pAudioPlayThread->pSoundPlayer->Close();
		}
		pChannelManager->pAudioPlayThread->channelId = -1;
		pChannelManager->pAudioPlayThread->audiochannels = 0;
	}

	if (NULL != pThread->mp4cHandle)
	{
		MP4C_CloseMp4File(pThread->mp4cHandle);
		MP4C_Deinit(&pThread->mp4cHandle);
		pThread->mp4cHandle = NULL;
		pThread->vidFrameNum = 0;
	}

	if (m_pAACEncoderHandle)
	{
		Easy_AACEncoder_Release(m_pAACEncoderHandle);
		m_pAACEncoderHandle = NULL;
	}

	if (m_pAACEncBufer)
	{
		delete[] m_pAACEncBufer ;
		m_pAACEncBufer = NULL;
	}

	delete []audio_buf;
	delete []pbuf;
	pbuf = NULL;

    //if (NULL != fES)    fclose(fES);

	pThread->decodeThread.flag	=	0x00;

#ifdef _DEBUG
	_TRACE("�����߳�[%d]���˳� ThreadId:%d.\n", pThread->channelId, GetCurrentThreadId());
#endif

	return 0;
}

LPTHREAD_START_ROUTINE CChannelManager::_lpPhotoShotThread( LPVOID _pParam )
{
	if (_pParam)
	{
		PhotoShotThreadInfo* pThreadInfo = (PhotoShotThreadInfo*)_pParam;
		LPSaveJpg	  pSaveImage=Create_SaveJpgDll();
		if(pSaveImage)
		{
			//YUV -> JPG
			pSaveImage->SaveBufferToJpg((BYTE*)pThreadInfo->pYuvBuf, pThreadInfo->width, pThreadInfo->height, pThreadInfo->strPath,-1,-1);

			Release_SaveJpgDll(pSaveImage);
			pSaveImage=NULL;
		}
		delete[] pThreadInfo->pYuvBuf;
		delete pThreadInfo;
	}
	return 0;
}


LPTHREAD_START_ROUTINE CChannelManager::_lpDisplayThread( LPVOID _pParam )
{
	PLAY_THREAD_OBJ *pThread = (PLAY_THREAD_OBJ*)_pParam;
	if (NULL == pThread)			return 0;

	pThread->displayThread.flag	=	0x02;

#ifdef _DEBUG
	_TRACE("��ʾ�߳�[%d]������. ThreadId:%d ...\n", pThread->channelId, GetCurrentThreadId());
#endif

	int width = 0;
	int height= 0;
	RECT	rcVideoRender;
	SetRectEmpty(&rcVideoRender);

	int iInitTimestamp = 0;

	pThread->rtpTimestamp = 0;
	unsigned int deviceLostTime = (unsigned int)time(NULL)-2;

	int iLastDisplayYuvIdx = -1;

	MEDIA_FRAME_INFO	lastFrameInfo;
	memset(&lastFrameInfo, 0x00, sizeof(MEDIA_FRAME_INFO));

//#ifdef _DEBUG
#if 1
	float	fDisplayTimes = 0.0f;		//��ʾ��ʱͳ��
	int		displayFrameNum = 0;
	int		nDisplayTotalTimes = 0;
	unsigned int uiLastTotalTime = 0;
#endif

	int	iDropFrame = 0;		//��֡����

	int iDelay = 0;

	_VS_BEGIN_TIME_PERIOD(1);
	QueryPerformanceFrequency(&pThread->cpuFreq);
	_VS_END_TIME_PERIOD(1);
	while (1)
	{
		if (pThread->displayThread.flag == 0x03)		break;

		int iDispalyYuvIdx = -1;
		int iYuvFrameNum = 0;
		unsigned int getNextFrame = 0;
		unsigned int rtpTimestamp = pThread->rtpTimestamp;
		do
		{
			if (pThread->displayThread.flag == 0x03)		break;

			for (int iYuvIdx=0; iYuvIdx<MAX_YUV_FRAME_NUM; iYuvIdx++)
			{
				if (pThread->displayThread.flag == 0x03)		break;
				if (pThread->yuvFrame[iYuvIdx].frameinfo.length < 1)		continue;

				unsigned int timestampTmp = pThread->yuvFrame[iYuvIdx].frameinfo.timestamp_sec*1000+pThread->yuvFrame[iYuvIdx].frameinfo.timestamp_usec/1000;

				if (timestampTmp > rtpTimestamp && getNextFrame==0)
				{
					rtpTimestamp = timestampTmp;
					iDispalyYuvIdx = iYuvIdx;
					getNextFrame = 1;
				}
				else if (timestampTmp <= rtpTimestamp)
				{
					rtpTimestamp = timestampTmp;
					iDispalyYuvIdx = iYuvIdx;
					//break;
				}
				/*
				else if (pThread->yuvFrame[iYuvIdx].frameinfo.rtptimestamp == rtpTimestamp)
				{
					memset(&pThread->yuvFrame[iYuvIdx].frameinfo, 0x00, sizeof(MEDIA_FRAME_INFO));
					iDispalyYuvIdx = iYuvIdx;
					break;
				}
				*/
				iYuvFrameNum ++;
			}
			_VS_BEGIN_TIME_PERIOD(1);
			__VS_Delay(1);
			_VS_END_TIME_PERIOD(1);

		}while (iDispalyYuvIdx == -1);

		pThread->dwDisconnectTime = 0;
		if (pThread->displayThread.flag == 0x03 )						break;
		if (iDispalyYuvIdx < 0 || iDispalyYuvIdx>=MAX_YUV_FRAME_NUM)	continue;

		if (pThread->yuvFrame[iDispalyYuvIdx].frameinfo.width < 1 ||
			pThread->yuvFrame[iDispalyYuvIdx].frameinfo.height< 1)
		{
			continue;
		}

		if ( (NULL==pThread->hWnd) || (NULL!=pThread->hWnd && (!IsWindow(pThread->hWnd))) || (NULL!=pThread->hWnd && (!IsWindowVisible(pThread->hWnd))))
		{
			pThread->rtpTimestamp = pThread->yuvFrame[iDispalyYuvIdx].frameinfo.timestamp_sec*1000+pThread->yuvFrame[iDispalyYuvIdx].frameinfo.timestamp_usec/1000;

			memset(&pThread->yuvFrame[iDispalyYuvIdx].frameinfo, 0x00, sizeof(MEDIA_FRAME_INFO));
			continue;
		}


#ifdef _DEBUG1
		static unsigned int uiTmpTimestamp = 0;
		if (uiTmpTimestamp == 0)		uiTmpTimestamp= pThread->yuvFrame[iDispalyYuvIdx].frameinfo.rtptimestamp;
		if (pThread->yuvFrame[iDispalyYuvIdx].frameinfo.rtptimestamp <= uiTmpTimestamp)
		{
			_TRACE("��ǰʱ���[%u] <= ���µ�ʱ���[%u].\n", pThread->yuvFrame[iDispalyYuvIdx].frameinfo.rtptimestamp, uiTmpTimestamp);
			uiTmpTimestamp = pThread->yuvFrame[iDispalyYuvIdx].frameinfo.rtptimestamp;
		}

#endif


		iLastDisplayYuvIdx = iDispalyYuvIdx;

		pThread->frameQueue = 0;
		if (NULL != pThread->pAVQueue && NULL!=pThread->pAVQueue->pQueHeader)
			//if (pThread->initQueue==0x01 && NULL!=pThread->avQueue.pQueHeader)
		{
			pThread->frameQueue = pThread->pAVQueue->pQueHeader->videoframes;
		}

		int iQue1_DecodeQueue = pThread->frameQueue;		//δ�����֡��
		int iQue2_DisplayQueue= iYuvFrameNum;				//�ѽ����֡��

		int nQueueFrame = iQue1_DecodeQueue + iQue2_DisplayQueue;

		RECT rcSrc;
		RECT rcDst;
		if (! IsRectEmpty(&pThread->rcSrcRender))
		{
			CopyRect(&rcSrc, &pThread->rcSrcRender);
		}
		else
		{
			SetRect(&rcSrc, 0, 0, width, height);
		}
		if (NULL!=pThread->hWnd && (IsWindow(pThread->hWnd)) )
		{
			GetClientRect(pThread->hWnd, &rcDst);
		}

		//�����ǰ�ֱ��ʺ�֮ǰ�Ĳ�ͬ,�����³�ʼ��d3d
		if (lastFrameInfo.width != pThread->yuvFrame[iDispalyYuvIdx].frameinfo.width ||
			lastFrameInfo.height!= pThread->yuvFrame[iDispalyYuvIdx].frameinfo.height)
		{
			pThread->resetD3d = true;
		}

		EnterCriticalSection(&pThread->crit);			//Lock

		if ( pThread->resetD3d )
		{
			pThread->resetD3d = false;
			//�ر�d3d
			if (pThread->renderFormat == GDI_FORMAT_RGB24)
			{
				RGB_DeinitDraw(&pThread->d3dHandle);
			}
			else
			{
				D3D_Release(&pThread->d3dHandle);
			}
		}
		width = pThread->yuvFrame[iDispalyYuvIdx].frameinfo.width;
		height= pThread->yuvFrame[iDispalyYuvIdx].frameinfo.height;

		LeaveCriticalSection(&pThread->crit);			//Unlock

		//����D3dRender
		if (pThread->renderFormat == GDI_FORMAT_RGB24)
		{
			if (NULL == pThread->d3dHandle)	RGB_InitDraw(&pThread->d3dHandle);
		}
		else if ( (NULL == pThread->d3dHandle) && ((unsigned int)time(NULL)-deviceLostTime >= 2) )
		{
			D3D_FONT	font;
			memset(&font, 0x00, sizeof(D3D_FONT));
			font.bold	=	0x00;
			wcscpy(font.name, TEXT("Arial Black"));
			font.size	=	(int)(float)((width)*0.02f);// 32;
			font.width	=	(int)(float)((font.size)/2.5f);//13;
			if (NULL!=pThread->hWnd && (IsWindow(pThread->hWnd)) )
			{
				D3D_Initial(&pThread->d3dHandle, pThread->hWnd, width, height, 0, 1, pThread->renderFormat, &font);

// 				D3D_SetDisplayFlag(pThread->d3dHandle, D3D_SHOW_ZONE|D3D_SHOW_SEL_BOX);
// 
// 				D3D9_LINE	d3d9Line;
// 				memset(&d3d9Line, 0x00, sizeof(D3D9_LINE));
// 				d3d9Line.usLineId = 1;
// 				strcpy(d3d9Line.strLineName, "Line1");
// 				d3d9Line.dwColor = RGB(0xff,0x80,0x00);
// 				d3d9Line.uiTotalNodes = 2;
// 				d3d9Line.pNodes[0].x = 20;
// 				d3d9Line.pNodes[0].y = 60;
// 
// 				d3d9Line.pNodes[1].x = 60;
// 				d3d9Line.pNodes[1].y = 60;
// 
// 				D3D_AddLine(pThread->d3dHandle, &d3d9Line);

			}
			if (NULL == pThread->d3dHandle)
			{
				_TRACE("DEVICE LOST...   times:%d\n", ((unsigned int)time(NULL)-deviceLostTime));
				deviceLostTime = (unsigned int)time(NULL);
				//���d3d ��ʼ��ʧ��,�����֡ͷ��Ϣ,�Ա�����̼߳���������һ֡
				pThread->rtpTimestamp = pThread->yuvFrame[iDispalyYuvIdx].frameinfo.timestamp_sec*1000+pThread->yuvFrame[iDispalyYuvIdx].frameinfo.timestamp_usec/1000;
				memset(&pThread->yuvFrame[iDispalyYuvIdx].frameinfo, 0x00, sizeof(MEDIA_FRAME_INFO));
				_VS_BEGIN_TIME_PERIOD(1);
				__VS_Delay(1);
				_VS_END_TIME_PERIOD(1);
				continue;
			}
		}

		int fps = pThread->yuvFrame[iDispalyYuvIdx].frameinfo.fps;
		int iOneFrameUsec = 1000/30;	//normal
		if (fps>0)	iOneFrameUsec = 1000 / fps;

		int iCache = 0;
		if (NULL!=pThread->frameCache) iCache = pThread->frameCache;
	
		pThread->rtpTimestamp = pThread->yuvFrame[iDispalyYuvIdx].frameinfo.timestamp_sec*1000+pThread->yuvFrame[iDispalyYuvIdx].frameinfo.timestamp_usec/1000;

		//ͳ����Ϣ:  �����ʽ �ֱ��� ֡�� ֡����  ����  ����֡��
		char sztmp[128] = {0,};
#if 0
		sprintf(sztmp, "%s[%d x %d]  FPS: %d[%s]    Bitrate: %.2fMbps   Cache: %d / %d",
			pThread->yuvFrame[iDispalyYuvIdx].frameinfo.codec==0x1C?"H264":"MPEG4",
			pThread->yuvFrame[iDispalyYuvIdx].frameinfo.width,
			pThread->yuvFrame[iDispalyYuvIdx].frameinfo.height,
			fps,
			pThread->yuvFrame[iDispalyYuvIdx].frameinfo.type==0x01?"I":"P",
			pThread->yuvFrame[iDispalyYuvIdx].frameinfo.bitrate/1024.0f,
			nQueueFrame, iCache);
#else
			sprintf(sztmp, "[%dx%d] fps[%d] Bitrate[%.2fMbps]Cache[%d(%d+%d) / %d]  AverageTime: %.2f  Delay: %d  totaltime:%d / %d dropframe:%d",
				pThread->yuvFrame[iDispalyYuvIdx].frameinfo.width,
				pThread->yuvFrame[iDispalyYuvIdx].frameinfo.height,
				pThread->yuvFrame[iDispalyYuvIdx].frameinfo.fps,
				pThread->yuvFrame[iDispalyYuvIdx].frameinfo.bitrate/1024.0f,
				nQueueFrame,  iQue1_DecodeQueue, iQue2_DisplayQueue,  iCache, 
				fDisplayTimes, iDelay, (int)fDisplayTimes+(iDelay>0?iDelay:0), iOneFrameUsec, iDropFrame);
#endif
		D3D_OSD	osd;
		memset(&osd, 0x00, sizeof(D3D_OSD));
		MByteToWChar(sztmp, osd.string, sizeof(osd.string)/sizeof(osd.string[0]));
		if (pThread->renderFormat == GDI_FORMAT_RGB24)
		{
			SetRect(&osd.rect, 2, 2, (int)wcslen(osd.string)*7, (int)(float)((height)*0.046f));//40);
		}
		else
		{
			SetRect(&osd.rect, 2, 2, (int)wcslen(osd.string)*20, (int)(float)((height)*0.046f));//40);
		}
		osd.color = RGB(0xff,0xff,0x00);
		osd.shadowcolor = RGB(0x15,0x15,0x15);
		osd.alpha = 180;

		int showOSD = pThread->showStatisticalInfo;

		int ret = 0;

		memset(&lastFrameInfo, 0x00, sizeof(MEDIA_FRAME_INFO));
		memcpy(&lastFrameInfo, &pThread->yuvFrame[iDispalyYuvIdx].frameinfo, sizeof(MEDIA_FRAME_INFO));

		if (nQueueFrame > iCache * 2)	iDropFrame ++;
		else							iDropFrame = 0;
		if (iDropFrame < 0x02)
		{
			if (pThread->renderFormat == GDI_FORMAT_RGB24)
			{
				RGB_DrawData(pThread->d3dHandle, pThread->hWnd, pThread->yuvFrame[iDispalyYuvIdx].pYuvBuf, width, height, &rcSrc, pThread->ShownToScale, RGB(0x3c,0x3c,0x3c), 0, showOSD, &osd);
				//D3D_RenderRGB24ByGDI(pThread->hWnd, pThread->yuvFrame[iDispalyYuvIdx].pYuvBuf, width, height, showOSD, &osd);
			
				//int	D3DRENDER_API  RGB_DrawData(D3D_HANDLE handle, HWND hWnd, char *pBuff, int width, int height, int ShownToScale, COLORREF bkColor, int flip=0, int OSDNum=0, D3D_OSD *_osd = NULL);

			}
			else if (NULL != pThread->d3dHandle)
			{
#ifdef _DEBUG0
				static FILE *fOutput = NULL;
				if (NULL == fOutput)
				{
					char sztmp[128] = {0,};
					sprintf(sztmp, "C:\\test\\%dx%d.yv12", width, height);
					fOutput = fopen(sztmp, "wb");
				}
				if (NULL != fOutput)	fwrite(pThread->yuvFrame[iDispalyYuvIdx].pYuvBuf, 1, width * height * 3 / 2, fOutput);
#endif

				D3D_UpdateData(pThread->d3dHandle, 0, (unsigned char*)pThread->yuvFrame[iDispalyYuvIdx].pYuvBuf, width, height, &rcSrc, NULL, showOSD, &osd);
				ret = D3D_Render(pThread->d3dHandle, pThread->hWnd, pThread->ShownToScale, &rcDst);
				if (ret < 0)
				{
					deviceLostTime = (unsigned int)time(NULL);
					pThread->resetD3d = true;						//��Ҫ�ؽ�D3D
				}
			}
		}
		else
		{
			_TRACE("��֡...%d\n", iDropFrame);
		}
		
		memset(&pThread->yuvFrame[iDispalyYuvIdx].frameinfo, 0x00, sizeof(MEDIA_FRAME_INFO));

		if (iInitTimestamp == 0x00)
		{
			iInitTimestamp = 0x01;
			_VS_BEGIN_TIME_PERIOD(1);
			__VS_Delay(50);
			QueryPerformanceCounter(&pThread->lastRenderTime);
			_VS_END_TIME_PERIOD(1);
		}
		else
		{
			unsigned int iInterval = 0;
			_LARGE_INTEGER	nowTime;


#ifdef _DEBUG1
			/*
			static int nRandTally = 0;
			if (nRandTally > 20)
			{
				Sleep(50);
				if (nRandTally > 30)	nRandTally = 0;
			}
			nRandTally ++;
			*/
			
			Sleep(30);
#endif


			_VS_BEGIN_TIME_PERIOD(1);
			QueryPerformanceCounter(&nowTime);
			_VS_END_TIME_PERIOD(1);
			if (pThread->cpuFreq.QuadPart < 1)	pThread->cpuFreq.QuadPart = 1;
			LONGLONG lInterval = (LONGLONG)(((nowTime.QuadPart - pThread->lastRenderTime.QuadPart) / (double)pThread->cpuFreq.QuadPart * (double)1000));
			iInterval = (int)lInterval;

			iDelay = iOneFrameUsec - iInterval;

			if (iDelay<1)	iDelay = 0;
			else if (iDelay>1500)	iDelay=1500;
			if (nQueueFrame<iCache && fps>0)
			{
				int ii = ((iOneFrameUsec * (iCache-nQueueFrame))/fps);
				iDelay += ii;
			}
			else if (nQueueFrame>iCache && fps>0)
			{
				int ii = ((iOneFrameUsec * (nQueueFrame-iCache))/fps);
				iDelay -= ii;
			}

//#ifdef _DEBUG
#if 1
			unsigned int uiCurrTime = (unsigned int)time(NULL);
			if (uiLastTotalTime == 0x00)		uiLastTotalTime = uiCurrTime;

			if (uiLastTotalTime != uiCurrTime)
			{
				fDisplayTimes = (float)nDisplayTotalTimes / (float)displayFrameNum;
				//fDisplayTimes = (float)nDisplayTotalTimes / iOneFrameUsec;
				uiLastTotalTime = uiCurrTime;
				nDisplayTotalTimes = iInterval;
				displayFrameNum = 1;
			}
			else
			{
				nDisplayTotalTimes += iInterval;
				displayFrameNum ++;
			}
#endif

			_VS_BEGIN_TIME_PERIOD(1);
			//_TRACE("[ch%d]����ʱ: %d\t��ʾ��ʱ:%d\t��ʱ:%d\t����֡��:%d\t��ǰ֡��С:%d  OneFrameUsec:%d\n", pThread->renderCh, iInterval+iDelay, iInterval, iDelay, nQueueFrame, frame_size, iOneFrameUsec);
			if (iDelay>0 && iDelay<1500 && iCache>0 && iCache*2>nQueueFrame && iDropFrame==0)
			{
				__VS_Delay(iDelay);
			}

			QueryPerformanceCounter(&pThread->lastRenderTime);
			_VS_END_TIME_PERIOD(1);
		}
	}

	if (pThread->renderFormat == GDI_FORMAT_RGB24)
	{
		RGB_DeinitDraw(&pThread->d3dHandle);
	}
	else
	{
		D3D_Release(&pThread->d3dHandle);
	}
	pThread->rtpTimestamp = 0;

	pThread->displayThread.flag	=	0x00;

#ifdef _DEBUG
	_TRACE("��ʾ�߳�[%d]���˳�. ThreadId:%d ..\n", pThread->channelId, GetCurrentThreadId());
#endif

	return 0;
}

int CALLBACK __NVSourceCallBack( int _chid, int *_chPtr, int _mediatype, char *pbuf, RTSP_FRAME_INFO *frameinfo)
{
	PLAY_THREAD_OBJ	*pPlayThread = (PLAY_THREAD_OBJ *)_chPtr;

	if (NULL == pPlayThread)			return -1;

	if (NULL == pChannelManager)	return -1;

	if (NULL != frameinfo)
	{
		//frameinfo->width = 640;
		//frameinfo->height = 360;

		if (frameinfo->height==3008)		frameinfo->height=3000;
		else if (frameinfo->height==1088)		frameinfo->height=1080;
		else if (frameinfo->height==544)	frameinfo->height=540;
	}

	pChannelManager->ProcessData(_chid, _mediatype, pbuf, frameinfo);

	return 0;
}

int CALLBACK __RTSPSourceCallBack( int _channelId, void *_channelPtr, int _frameType, char *pBuf, RTSP_FRAME_INFO* _frameInfo)
{
	PLAY_THREAD_OBJ	*pPlayThread = (PLAY_THREAD_OBJ *)_channelPtr;

	if (NULL == pPlayThread)			return -1;

	if (NULL == pChannelManager)	return -1;

	if (NULL != _frameInfo)
	{
		//frameinfo->width = 640;
		//frameinfo->height = 360;

		if (_frameInfo->height==3008)			_frameInfo->height=3000;
		else if (_frameInfo->height==1088)		_frameInfo->height=1080;
		else if (_frameInfo->height==544)		_frameInfo->height=540;
	}

	pChannelManager->ProcessData(_channelId, _frameType, pBuf, _frameInfo);

	return 0;
}


int	CChannelManager::ProcessData(int _chid, int mediatype, char *pbuf, RTSP_FRAME_INFO *frameinfo)
{
	if (NULL == pRealtimePlayThread)			return 0;


	MediaSourceCallBack pMediaCallback = (MediaSourceCallBack )pRealtimePlayThread[_chid].pCallback;
	if (NULL != pMediaCallback && (mediatype == EASY_SDK_VIDEO_FRAME_FLAG || mediatype == EASY_SDK_AUDIO_FRAME_FLAG || mediatype == EASY_SDK_MEDIA_INFO_FLAG))
	{
		pMediaCallback(_chid, (int*)pRealtimePlayThread[_chid].pUserPtr, mediatype, pbuf, frameinfo);
	}

	if (mediatype == EASY_SDK_VIDEO_FRAME_FLAG)
	{
		if ( (NULL == pRealtimePlayThread[_chid].pAVQueue) && (frameinfo->type==EASY_SDK_VIDEO_FRAME_I/*Key frame*/) )
		{
			pRealtimePlayThread[_chid].pAVQueue = new SS_QUEUE_OBJ_T();
			if (NULL != pRealtimePlayThread[_chid].pAVQueue)
			{
				memset(pRealtimePlayThread[_chid].pAVQueue, 0x00, sizeof(SS_QUEUE_OBJ_T));
				SSQ_Init(pRealtimePlayThread[_chid].pAVQueue, 0x00, _chid, TEXT(""), MAX_AVQUEUE_SIZE, 2, 0x01);
				SSQ_Clear(pRealtimePlayThread[_chid].pAVQueue);
				pRealtimePlayThread[_chid].initQueue = 0x01;
			}

			pRealtimePlayThread[_chid].dwLosspacketTime=0;
			pRealtimePlayThread[_chid].dwDisconnectTime=0;
		}
		if (NULL != pRealtimePlayThread[_chid].pAVQueue)
		{
			SSQ_AddData(pRealtimePlayThread[_chid].pAVQueue, _chid, MEDIA_TYPE_VIDEO, (MEDIA_FRAME_INFO*)frameinfo, pbuf);
		}
	}
	else if (mediatype == EASY_SDK_AUDIO_FRAME_FLAG)
	{
		if (NULL != pRealtimePlayThread[_chid].pAVQueue)
		{
			SSQ_AddData(pRealtimePlayThread[_chid].pAVQueue, _chid, MEDIA_TYPE_AUDIO, (MEDIA_FRAME_INFO*)frameinfo, pbuf);
		}
	}
// 	else if (mediatype == EASY_SDK_EVENT_FRAME_FLAG)
// 	{
// 		if (NULL == pbuf && NULL == frameinfo)
// 		{
// 			_TRACE("[ch%d]������...\n", _chid);
// 
// 			MEDIA_FRAME_INFO	frameinfo;
// 			memset(&frameinfo, 0x00, sizeof(MEDIA_FRAME_INFO));
// 			frameinfo.length = 1;
// 			frameinfo.type   = 0xFF;
// 			SSQ_AddData(pRealtimePlayThread[_chid].pAVQueue, _chid, MEDIA_TYPE_EVENT, (MEDIA_FRAME_INFO*)&frameinfo, "1");
// 		}
// 		else if (NULL!=frameinfo && frameinfo->type==0xF1)
// 		{
// 			_TRACE("[ch%d]����[%.2f]...\n", _chid, frameinfo->losspacket);
// 
// 			frameinfo->length = 1;
// 			SSQ_AddData(pRealtimePlayThread[_chid].pAVQueue, _chid, MEDIA_TYPE_EVENT, (MEDIA_FRAME_INFO*)frameinfo, "1");
// 		}
// 	}
	else if (mediatype == EASY_SDK_EVENT_FRAME_FLAG)//�ص�����״̬�¼�
	{
		// EasyRTSPClient��ʼ�������ӣ�����EasyRTSPClient�����߳�
		char*  fRTSPURL = pRealtimePlayThread[_chid].url;
		char sErrorString[512];
		if (NULL == pbuf && NULL == frameinfo)
		{
			sprintf(sErrorString, "Connecting:	%s ...\n", fRTSPURL);

			MEDIA_FRAME_INFO	tmpFrameinfo;
			memset(&tmpFrameinfo, 0x00, sizeof(MEDIA_FRAME_INFO));
			tmpFrameinfo.length = 1;
			tmpFrameinfo.type   = 0xFF;
			SSQ_AddData(pRealtimePlayThread[_chid].pAVQueue, _chid, MEDIA_TYPE_EVENT, (MEDIA_FRAME_INFO*)&tmpFrameinfo, sErrorString);
			if (pMediaCallback)
				pMediaCallback(_chid, (int*)pRealtimePlayThread[_chid].pUserPtr, mediatype, sErrorString,  (RTSP_FRAME_INFO*)&tmpFrameinfo);
		}
		else if (NULL!=frameinfo && frameinfo->type==0xF1)
		{
			sprintf(sErrorString, "Error:	 %s��	[ch%d]����[%.2f]...\n",  fRTSPURL, _chid, frameinfo->losspacket);

			frameinfo->length = 1;
			SSQ_AddData(pRealtimePlayThread[_chid].pAVQueue, _chid, MEDIA_TYPE_EVENT, (MEDIA_FRAME_INFO*)frameinfo, sErrorString);
			if (pMediaCallback)
				pMediaCallback(_chid, (int*)pRealtimePlayThread[_chid].pUserPtr, mediatype, sErrorString, frameinfo);
	}

		// EasyRTSPClient RTSPClient���Ӵ��󣬴�����ͨ��EasyRTSP_GetErrCode()�ӿڻ�ȡ������404
		else if (NULL != frameinfo && frameinfo->codec == EASY_SDK_EVENT_CODEC_ERROR)
		{
			sprintf(sErrorString, "Error:	  %s�� EasyRTSP_GetErrCode��%d :%s ...\n", fRTSPURL, EasyRTSP_GetErrCode(pRealtimePlayThread[_chid].nvsHandle), pbuf?pbuf:"null" );
			SSQ_AddData(pRealtimePlayThread[_chid].pAVQueue, _chid, MEDIA_TYPE_EVENT, (MEDIA_FRAME_INFO*)frameinfo, sErrorString);
			if (pMediaCallback)
				pMediaCallback(_chid, (int*)pRealtimePlayThread[_chid].pUserPtr, mediatype, sErrorString, frameinfo);
		}

		// EasyRTSPClient�����߳��˳�����ʱ�ϲ�Ӧ��ֹͣ��ص��ã���λ���Ӱ�ť��״̬
		else if (NULL != frameinfo && frameinfo->codec == EASY_SDK_EVENT_CODEC_EXIT)
		{
			sprintf(sErrorString, "Exit:	%s,   Error:%d ...\n", fRTSPURL, EasyRTSP_GetErrCode(pRealtimePlayThread[_chid].nvsHandle));
			SSQ_AddData(pRealtimePlayThread[_chid].pAVQueue, _chid, MEDIA_TYPE_EVENT, (MEDIA_FRAME_INFO*)frameinfo, sErrorString);
			if (pMediaCallback)
				pMediaCallback(_chid, (int*)pRealtimePlayThread[_chid].pUserPtr, mediatype, sErrorString, frameinfo);
		}
	}

	return 0;
}

int	CChannelManager::SetManuRecordPath(int channelId, const char* recordPath)
{
	if (NULL == pRealtimePlayThread)			return -1;
	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	if (recordPath)
	{
		int nPathLen = strlen(recordPath);
		if (nPathLen<=0)
		{
			return -1;
		}
		if (nPathLen>MAX_PATH)
		{
			nPathLen = MAX_PATH;
		}
		memcpy(pRealtimePlayThread[iNvsIdx].manuRecordingPath, recordPath, nPathLen);
	}
	return 1;
}

int	CChannelManager::SetManuPicShotPath(int channelId, const char* shotPath)
{
	if (NULL == pRealtimePlayThread)			return -1;
	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;
	if (shotPath)
	{
		int nPathLen = strlen(shotPath);
		if (nPathLen<=0)
		{
			return -1;
		}
		if (nPathLen>MAX_PATH)
		{
			nPathLen = MAX_PATH;
		}
		memcpy(pRealtimePlayThread[iNvsIdx].strScreenCapturePath, shotPath, nPathLen);
	}
	return 1;
}

int	CChannelManager::StartManuPicShot(int channelId)	
{
	//pThread->manuScreenshot
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	//if (NULL == pRealtimePlayThread[iNvsIdx].m_pSaveImage)
	{
		pRealtimePlayThread[iNvsIdx].manuScreenshot = 0x01;
	}
	return 1;
}

int	CChannelManager::StopManuPicShot(int channelId)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	pRealtimePlayThread[iNvsIdx].manuScreenshot = 0x00;
	return 1;
}


int	CChannelManager::StartManuRecording(int channelId)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	if (NULL == pRealtimePlayThread[iNvsIdx].mp4cHandle)
	{
		pRealtimePlayThread[iNvsIdx].manuRecording = 0x01;
	}

	return 1;
}
int	CChannelManager::StopManuRecording(int channelId)
{
	if (NULL == pRealtimePlayThread)			return -1;

	int iNvsIdx = channelId - CHANNEL_ID_GAIN;
	if (iNvsIdx < 0 || iNvsIdx>= MAX_CHANNEL_NUM)	return -1;

	if (pRealtimePlayThread[iNvsIdx].manuRecording == 0x01)
	{
		pRealtimePlayThread[iNvsIdx].manuRecording = 0x00;
	}

	return 1;
}
