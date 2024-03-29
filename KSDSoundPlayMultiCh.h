#pragma once
//__________________________________________________________________
// FileName : KSDSoundPlayMultiCh.h
// Abstract : CKSDSoundPlayMultiChクラスインライン
//------------------------------------------------------------------
// K.Hishinuma 2013/03/28 22:02:08 First Edition
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
#include <mmsystem.h>
#pragma comment( lib, "winmm.lib" )

// DirectX
#include "dsound.h"
#pragma comment( lib, "dsound.lib" )
#pragma comment( lib, "dxguid.lib" )

#include "KSTChar.h"
#include "ksmath.h"
using ksmath::MIN;
using ksmath::LIM;

#include "./ks03/ksdxobject.h"
using ks03::dx::CKSDXObject;
#include "./ks03/ksdsoundlock.h"
#include "KSCriticalSection.h"
using kssync::CKSCriticalSection;
using kssync::CKSCriticalLock;

#ifndef DIRECTSOUND_TYPE
#define DIRECTSOUND_TYPE
	#if DIRECTSOUND_VERSION >= 0x0800
			const GUID DS_CLSID =			CLSID_DirectSound8;
			const GUID DS_IID =				IID_IDirectSound8;
			typedef LPDIRECTSOUND8			LPDSOUND;
	#else
			const GUID DS_CLSID =			CLSID_DirectSound;
			const GUID DS_IID =				IID_IDirectSound;
			typedef LPDIRECTSOUND			LPDSOUND;
	#endif
	typedef LPDIRECTSOUNDBUFFER		LPDSOUNDBUFFER;
#endif

namespace ksdx{

//__________________________________________________________________
// Class    : CKSDSoundPlayMultiCh
// Abstract : 複数のチャンネルをミキシングして出力する
//------------------------------------------------------------------
// K.Hishinuma 2013/03/28 22:02:08 First Edition
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
template< int chnum, int size, unsigned int flags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 >
class CKSDSoundPlayMultiCh
{
private:
	CKSCriticalSection			mCS;

private:
	CKSDXObject< LPDSOUND >		m_DSPlay;
	LPDSOUNDBUFFER				m_pDSBuffer[chnum];
	int							m_bufsize;
	int							m_buflen;
	int							m_freq;
	int							m_block;
	int							mCH[chnum];
	int							m_bytes;
	int							m_PlayPos[chnum];
	unsigned char				m_bData[chnum][size*2];		// ステレオ8bit出力用	
	unsigned short				m_sData[chnum][size*2];		// ステレオ16bit出力用
	unsigned int				m_lasterr;
public:
	CKSDSoundPlayMultiCh(void){
		timeBeginPeriod(1);
	}
public:
	~CKSDSoundPlayMultiCh(void){
		timeEndPeriod(1);
	}
public:
	// DSOUND生成
	bool Create(LPGUID lpGUID, HWND hParent, unsigned int dwLevel = DSSCL_NORMAL ){
		// ダイレクトサウンドオブジェクトの作成
		if( m_DSPlay.CreateInstance(DS_CLSID, DS_IID) != 0 ){
			MessageBox( NULL, _KT("ダイレクトサウンドキャプチャインスタンス生成エラー"), _KT("エラー"), MB_OK );
			return false;
		}
		// ダイレクトサウンドの初期化（NULL=さうんどカードデフォルト使用)
		if( m_DSPlay.p_pObj.get()->Initialize(lpGUID) != DS_OK ){
			m_DSPlay.Release();
			MessageBox( NULL, _KT("ダイレクトサウンドキャプチャ初期化エラー"), _KT("エラー"), MB_OK );
			return false;
		}
		// 協調レベルの設定(SECONDARY)
		if( m_DSPlay.p_pObj.get()->SetCooperativeLevel( hParent, dwLevel ) != DS_OK )
			return false ;
		return true;
	}

	void Release(){
		m_DSPlay.Release();
	}

	// 再生バッファの作成
	bool CreateBuffer(int idx0, int chnum, float sec, int bits, unsigned int freq){
		m_bytes = bits >> 3;

		DSBUFFERDESC bufdsc;
		WAVEFORMATEX waveform;
		waveform.cbSize = sizeof( WAVEFORMATEX );
		waveform.wFormatTag = WAVE_FORMAT_PCM;
		mCH[idx0] = waveform.nChannels = chnum;
		m_freq = waveform.nSamplesPerSec = freq;
		m_block = waveform.nBlockAlign = chnum * m_bytes;
		waveform.nAvgBytesPerSec = freq * waveform.nBlockAlign;
		waveform.wBitsPerSample = bits;

		m_buflen = static_cast<int>(freq * sec);
		memset( &bufdsc, 0, sizeof(DSBUFFERDESC) );
		bufdsc.dwSize = sizeof(DSBUFFERDESC);
		bufdsc.dwFlags = flags ;
		bufdsc.dwBufferBytes = m_bufsize = m_buflen * m_block;	// *2 はおまけバッファ。取得単位に対して十分に大きくないと、前のデータが消される
		bufdsc.guid3DAlgorithm = DS3DALG_DEFAULT;
		bufdsc.lpwfxFormat = &waveform;
		m_PlayPos[idx0] = -1;
		m_lasterr = m_DSPlay.p_pObj.get()->CreateSoundBuffer(
			reinterpret_cast<LPCDSBUFFERDESC>(&bufdsc),
			reinterpret_cast<LPDSOUNDBUFFER*>(&m_pDSBuffer[idx0]),
			NULL);
		switch( m_lasterr ){
		case DSERR_ALLOCATED:
			m_lasterr = 0;
			break;
		case DSERR_BADFORMAT:
			m_lasterr = 0;
			break;
		case DSERR_BUFFERTOOSMALL:
			m_lasterr = 0;
			break;
		case DSERR_CONTROLUNAVAIL:
			m_lasterr = 0;
			break;
		case DSERR_DS8_REQUIRED:
			m_lasterr = 0;
			break;
		case DSERR_INVALIDCALL:
			m_lasterr = 0;
			break;
		case DSERR_INVALIDPARAM:
			m_lasterr = 0;
			break;
		case DSERR_NOAGGREGATION:
			m_lasterr = 0;
			break;
		case DSERR_OUTOFMEMORY:
			m_lasterr = 0;
			break;
		case DSERR_UNINITIALIZED:
			m_lasterr = 0;
			break;
		case DSERR_UNSUPPORTED:
			m_lasterr = 0;
			break;
		}
		return m_lasterr == DS_OK;
	}

	bool StartPlay(int idx0){
		// セカンダリバッファをクリアしてから
		LPVOID lpvPtr1 ; 
		DWORD dwBytes1 ; 
		LPVOID lpvPtr2 ; 
		DWORD dwBytes2 ; 
		CKSCriticalLock cl(mCS);
		HRESULT hr = m_pDSBuffer[idx0]->Lock( 0, m_buflen, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		if( DSERR_BUFFERLOST == hr ){
			m_pDSBuffer[idx0]->Restore();
			hr = m_pDSBuffer[idx0]->Lock( 0, m_buflen, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		}
		if (SUCCEEDED(hr)) { 
			// ポインタの位置に書き込む。
			memset( lpvPtr1 , 0 , dwBytes1 ) ; 
			if ( NULL != lpvPtr2 )
				memset( lpvPtr2 , 0 , dwBytes2 ) ; 
			hr = m_pDSBuffer[idx0]->Unlock( lpvPtr1 , dwBytes1 , lpvPtr2 , dwBytes2 ) ;
			m_PlayPos[idx0] = -1;
			return true;
		}
		return false;
	}
	bool StopPlay(int idx0){
		CKSCriticalLock cl(mCS);
		return m_pDSBuffer[idx0]->Stop() == DS_OK;
	}
	void StopPlay(){
		for( int i = 0 ; i < chnum ; i++ )
			StopPlay(i);
	}

	bool SetMonoData( int idx0, const float* fdata, int len ){

		if( m_bytes == 1 ){
			for( int i = 0 ; i < MIN(len,size) ; i++ ){
				for( int j = 0 ; j < m_ch ; j++ ){
					m_bData[idx0][i*m_ch+j] = LIM<unsigned char>(static_cast<unsigned char>((fdata[i] * 128.0f) + 128.0f),0,255);
				}
			}
			return SetData( idx0, reinterpret_cast<const char*>(m_bData), len );
		}
		else if( m_bytes == 2 ){
			for( int i = 0 ; i < MIN(len,size) ; i++ ){
				for( int j = 0 ; j < m_ch ; j++ ){
					m_sData[idx0][i*m_ch+j] = LIM<unsigned short>(static_cast<unsigned short>((fdata[i] * 32768.0f) + 32768.0f), 0, 65535);
				}
			}
			return SetData( idx0, reinterpret_cast<const char*>(m_sData), len*2 );
		}
		return false;
	}
	bool PlayOneShot( int idx0, const char* data, int len ){
		LPVOID lpvPtr1 ; 
		DWORD dwBytes1 ; 
		LPVOID lpvPtr2 ; 
		DWORD dwBytes2 ; 
		// 現在再生中のポイントの0.5秒くらい先に書き込みますか・・・
		// 過去の書込みポイントがあれば、そこに書き込む。
		m_PlayPos[idx0] = 0;
		CKSCriticalLock cl(mCS);
		HRESULT hr = m_pDSBuffer[idx0]->Lock( m_PlayPos, len, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		if( DSERR_BUFFERLOST == hr ){
			m_pDSBuffer[idx0]->Restore();
			hr = m_pDSBuffer[idx0]->Lock( m_PlayPos, len, &lpvPtr1,  &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		}
		if (SUCCEEDED(hr)) { 
			// ポインタの位置に書き込む。
			memcpy( lpvPtr1 , data , dwBytes1 ) ; 
			if ( NULL != lpvPtr2 )
				memcpy( lpvPtr2 , data + dwBytes1 , dwBytes2 ) ; 
			hr = m_pDSBuffer[idx0]->Unlock( lpvPtr1 , dwBytes1 , lpvPtr2 , dwBytes2 ) ;
			m_pDSBuffer[idx0]->SetCurrentPosition(0);
			m_pDSBuffer[idx0]->Play( 0 , 0 , 0 ) ;
			return true;
		}
		return false;
	}

	bool SetData( int idx0, const char* data, int len, DWORD playmode = DSBPLAY_LOOPING, float delay = 0.10f ){
		LPVOID lpvPtr1 ; 
		DWORD dwBytes1 ; 
		LPVOID lpvPtr2 ; 
		DWORD dwBytes2 ; 
		// 現在再生中のポイントの0.05秒くらい先に書き込みますか・・・
		// 過去の書込みポイントがあれば、そこに書き込む。
		DWORD curpos;
		DWORD playpos;
		CKSCriticalLock cl(mCS);
		if( m_pDSBuffer[idx0]->GetCurrentPosition( &playpos, &curpos ) != DS_OK ){
			OutputDebugString(_KT("ERR1"));
			return false;
		}
		if( m_PlayPos[idx0] < 0 ){
			m_pDSBuffer[idx0]->SetCurrentPosition(0);
			if( m_pDSBuffer[idx0]->GetCurrentPosition( &playpos, &curpos ) != DS_OK ){
				OutputDebugString(_KT("ERR2"));
				return false;
			}
			m_PlayPos[idx0] = (curpos + static_cast<int>(m_freq * delay)) % m_bufsize;
			m_pDSBuffer[idx0]->Play( 0 , 0 , playmode ) ;
		}
		else if( ( static_cast<unsigned int>(m_PlayPos[idx0]) > playpos ) && ( static_cast<unsigned int>(m_PlayPos[idx0]) < curpos ) ){
			OutputDebugString(_KT("ERR3"));
			m_PlayPos[idx0] = (curpos + static_cast<int>(m_freq * delay)) % m_bufsize;
			return false;
		}
//		else
//			m_PlayPos[idx0] = curpos + static_cast<int>(m_freq * delay);
		HRESULT hr = m_pDSBuffer[idx0]->Lock( m_PlayPos[idx0], len, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		if( DSERR_BUFFERLOST == hr ){
			m_pDSBuffer[idx0]->Restore();
			hr = m_pDSBuffer[idx0]->Lock( m_PlayPos[idx0], len, &lpvPtr1,  &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		}
		if (SUCCEEDED(hr)) { 
			// ポインタの位置に書き込む。
			memcpy( lpvPtr1 , data , dwBytes1 ) ; 
			if ( NULL != lpvPtr2 )
				memcpy( lpvPtr2 , data + dwBytes1 , dwBytes2 ) ; 
			hr = m_pDSBuffer[idx0]->Unlock( lpvPtr1 , dwBytes1 , lpvPtr2 , dwBytes2 ) ;
			// 書込みポイント
			TRACE("time:%d ppos:%d pos:%d len:%d\n", timeGetTime(), playpos, m_PlayPos[idx0], dwBytes1+dwBytes2 );
			m_PlayPos[idx0] = (m_PlayPos[idx0] + dwBytes1 + dwBytes2 ) % m_bufsize;
			return true;
		}
		CKSString errstr(hr, _KT("ERR4:%d") );
		OutputDebugString(errstr.c_str());
		return false;
	}

};

}
