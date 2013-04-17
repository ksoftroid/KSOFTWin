#pragma once
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

template< int size, unsigned int flags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 >
class CKSDSoundPlay
{
private:
	CKSCriticalSection			mCS;
	CKSDXObject< LPDSOUND >		m_DSPlay;
	LPDSOUNDBUFFER				m_pDSBuffer;
	int							m_bufsize;
	int							m_buflen;
	int							m_freq;
	int							m_block;
	int							m_ch;
	int							m_bytes;
	int							m_PlayPos;
	unsigned char				m_bData[size*2];		// ステレオ8bit出力用	
	unsigned short				m_sData[size*2];		// ステレオ16bit出力用
	unsigned int				m_lasterr;
public:
	CKSDSoundPlay(void){
		timeBeginPeriod(1);
	}
public:
	~CKSDSoundPlay(void){
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
	bool CreateBuffer(int chnum, float sec, int bits, unsigned int freq){
		m_bytes = bits >> 3;

		DSBUFFERDESC bufdsc;
		WAVEFORMATEX waveform;
		waveform.cbSize = sizeof( WAVEFORMATEX );
		waveform.wFormatTag = WAVE_FORMAT_PCM;
		m_ch = waveform.nChannels = chnum;
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
		m_PlayPos = -1;
		m_lasterr = m_DSPlay.p_pObj.get()->CreateSoundBuffer(
			reinterpret_cast<LPCDSBUFFERDESC>(&bufdsc),
			reinterpret_cast<LPDSOUNDBUFFER*>(&m_pDSBuffer),
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

	bool StartPlay(){
		// セカンダリバッファをクリアしてから
		LPVOID lpvPtr1 ; 
		DWORD dwBytes1 ; 
		LPVOID lpvPtr2 ; 
		DWORD dwBytes2 ; 
		CKSCriticalLock cl(mCS);
		HRESULT hr = m_pDSBuffer->Lock( 0, m_buflen, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		if( DSERR_BUFFERLOST == hr ){
			m_pDSBuffer->Restore();
			hr = m_pDSBuffer->Lock( 0, m_buflen, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		}
		if (SUCCEEDED(hr)) { 
			// ポインタの位置に書き込む。
			memset( lpvPtr1 , 0 , dwBytes1 ) ; 
			if ( NULL != lpvPtr2 )
				memset( lpvPtr2 , 0 , dwBytes2 ) ; 
			hr = m_pDSBuffer->Unlock( lpvPtr1 , dwBytes1 , lpvPtr2 , dwBytes2 ) ;
			m_PlayPos = -1;
			return true;
		}
		return false;
	}
	bool StopPlay(){
		CKSCriticalLock cl(mCS);
		return m_pDSBuffer->Stop() == DS_OK;
	}

	bool SetMonoData( const float* fdata, int len ){

		if( m_bytes == 1 ){
			for( int i = 0 ; i < MIN(len,size) ; i++ ){
				for( int j = 0 ; j < m_ch ; j++ ){
					m_bData[i*m_ch+j] = LIM<unsigned char>(static_cast<unsigned char>((fdata[i] * 128.0f) + 128.0f),0,255);
				}
			}
			return SetData( reinterpret_cast<const char*>(m_bData), len );
		}
		else if( m_bytes == 2 ){
			for( int i = 0 ; i < MIN(len,size) ; i++ ){
				for( int j = 0 ; j < m_ch ; j++ ){
					m_sData[i*m_ch+j] = LIM<unsigned short>(static_cast<unsigned short>((fdata[i] * 32768.0f) + 32768.0f), 0, 65535);
				}
			}
			return SetData( reinterpret_cast<const char*>(m_sData), len*2 );
		}
		return false;
	}
	bool PlayOneShot( const char* data, int len ){
		LPVOID lpvPtr1 ; 
		DWORD dwBytes1 ; 
		LPVOID lpvPtr2 ; 
		DWORD dwBytes2 ; 
		// 現在再生中のポイントの0.5秒くらい先に書き込みますか・・・
		// 過去の書込みポイントがあれば、そこに書き込む。
		m_PlayPos = 0;
		CKSCriticalLock cl(mCS);
		HRESULT hr = m_pDSBuffer->Lock( m_PlayPos, len, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		if( DSERR_BUFFERLOST == hr ){
			m_pDSBuffer->Restore();
			hr = m_pDSBuffer->Lock( m_PlayPos, len, &lpvPtr1,  &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		}
		if (SUCCEEDED(hr)) { 
			// ポインタの位置に書き込む。
			memcpy( lpvPtr1 , data , dwBytes1 ) ; 
			if ( NULL != lpvPtr2 )
				memcpy( lpvPtr2 , data + dwBytes1 , dwBytes2 ) ; 
			hr = m_pDSBuffer->Unlock( lpvPtr1 , dwBytes1 , lpvPtr2 , dwBytes2 ) ;
			m_pDSBuffer->SetCurrentPosition(0);
			m_pDSBuffer->Play( 0 , 0 , 0 ) ;
			return true;
		}
		return false;
	}

	bool SetData( const char* data, int len, DWORD playmode = DSBPLAY_LOOPING, float delay = 0.10f ){
		LPVOID lpvPtr1 ; 
		DWORD dwBytes1 ; 
		LPVOID lpvPtr2 ; 
		DWORD dwBytes2 ; 
		// 現在再生中のポイントの0.05秒くらい先に書き込みますか・・・
		// 過去の書込みポイントがあれば、そこに書き込む。
		DWORD curpos;
		DWORD playpos;
		CKSCriticalLock cl(mCS);
		if( m_pDSBuffer->GetCurrentPosition( &playpos, &curpos ) != DS_OK )
			return false;
		if( m_PlayPos < 0 ){
			m_PlayPos = curpos + static_cast<int>(m_freq * delay);
			m_pDSBuffer->SetCurrentPosition(0);
			m_pDSBuffer->Play( 0 , 0 , playmode ) ;
		}
		else if( ( static_cast<unsigned int>(m_PlayPos) > playpos ) && ( static_cast<unsigned int>(m_PlayPos) < curpos ) )
			return false;
//		else
//			m_PlayPos = curpos + static_cast<int>(m_freq * 0.025);
		HRESULT hr = m_pDSBuffer->Lock( m_PlayPos, len, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		if( DSERR_BUFFERLOST == hr ){
			m_pDSBuffer->Restore();
			hr = m_pDSBuffer->Lock( m_PlayPos, len, &lpvPtr1,  &dwBytes1, &lpvPtr2, &dwBytes2, 0 );
		}
		if (SUCCEEDED(hr)) { 
			// ポインタの位置に書き込む。
			memcpy( lpvPtr1 , data , dwBytes1 ) ; 
			if ( NULL != lpvPtr2 )
				memcpy( lpvPtr2 , data + dwBytes1 , dwBytes2 ) ; 
			hr = m_pDSBuffer->Unlock( lpvPtr1 , dwBytes1 , lpvPtr2 , dwBytes2 ) ;
			// 書込みポイント
			TRACE("time:%d ppos:%d pos:%d len:%d\n", timeGetTime(), playpos, m_PlayPos, dwBytes1+dwBytes2 );
			m_PlayPos = (m_PlayPos + dwBytes1 + dwBytes2 ) % m_bufsize;
			return true;
		}
		return false;
	}

};

}
