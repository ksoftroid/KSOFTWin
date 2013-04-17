#pragma once

#include <Winsock2.h>
#pragma comment( lib , "Ws2_32.lib")

namespace kssock{

class CKSWinSockInitializer
{
private:
	WSADATA	m_WinSockData;
public:
	static void WinSockInit(){
		static CKSWinSockInitializer inst;
	}
	~CKSWinSockInitializer(void){
		if( m_WinSockData.iMaxSockets == 0 )
			WSACleanup();
	}

private:
	CKSWinSockInitializer(void)
	{
		// 唯一回WinSockの初期化
		memset(&m_WinSockData,0,sizeof( WSADATA ));			// ソケットDLLの情報の初期化
		WSAStartup(MAKEWORD(2,0), &m_WinSockData);			// ウィンドウズソケットの初期化
		if(m_WinSockData.wVersion != MAKEWORD(2,0)){		// バージョンチェック
			WSACleanup();									// 初期化失敗
			m_WinSockData.iMaxSockets = 0;					// ソケットは一個も作れないことにする
		}
	}
};


}
