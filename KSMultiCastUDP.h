#pragma once
//__________________________________________________________________
// FileName : KSTCP.h
// Abstract : CKSTCPクラスインライン
//------------------------------------------------------------------
// K.Hishinuma 2013/03/22 23:54:13 First Edition
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <vector>
using std::vector;
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
#include "ksgeneric.h"
using ksgen::Int2Type;
#include "KSConst.h"
using namespace ks;
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
#include "./ks04/ksthreadex2.h"
using kssys::CKSThreadEx2;
#include "kswinsockinitializer.h"
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
namespace kssock{
//__________________________________________________________________
// Class    : CKSTCP
// Abstract : TCP通信クラス
//------------------------------------------------------------------
// K.Hishinuma 2013/03/22 23:54:13 First Edition
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
class CKSMultiCastUDP : public CKSThreadEx2{
private:
	int			mConnected;
	SOCKET		mSock;
	sockaddr_in	mSockAddr;
	HANDLE		maListen[2];
	int						mTimeOut;
	CKSSockEventReceiverEx*	mpReceiver;
	void*					mpArg;
	WSANETWORKEVENTS		mEvent;

public:
	CKSMultiCastUDP() : mConnected(0){
		CKSWinSockInitializer::WinSockInit();
	}
	~CKSMultiCastUDP(){
	}

public:
	// くらいあんとソケット生成
	KSRET CreateSocket( const char* recvipaddr, unsigned short port ){
		mSock = socket( AF_INET, SOCK_DGRAM, 0 );
		if( mSock == INVALID_SOCKET )
			return WSAGetLastError();
		unsigned int ipaddr = inet_addr(recvipaddr);
		if( setsockopt( mSock, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast<char*>(&ipaddr), sizeof(unsigned int) ) == SOCKET_ERROR)
			return WSAGetLastError();

		mSockAddr.sin_family = AF_INET;
		mSockAddr.sin_port = htons(port);
		mSockAddr.sin_addr.S_un.S_addr = ipaddr;
		return KSRET_SUC;
	}

	// サーバソケット生成
	KSRET CreateSocket( unsigned short port, const char* multicastaddr, unsigned int ifaddr = INADDR_ANY ){
		mSock = socket( AF_INET, SOCK_DGRAM, 0 );
		if( mSock == INVALID_SOCKET )
			return WSAGetLastError();
		
		// 受信用ポート
		mSockAddr.sin_family = AF_INET;
		mSockAddr.sin_port = htons(port);
		mSockAddr.sin_addr.S_un.S_addr = INADDR_ANY;
		if( bind(mSock, (const sockaddr*)&mSockAddr, sizeof(mSockAddr)) == SOCKET_ERROR )
			return WSAGetLastError();

		// マルチキャストグループへジョイン。これで指定したマルチキャストグループのメッセージを受信できるようになる
		ip_mreq mreq;
		memset(&mreq, 0, sizeof(mreq));
		mreq.imr_interface.S_un.S_addr = ifaddr;
		mreq.imr_multiaddr.S_un.S_addr = inet_addr(multicastaddr);
		if (setsockopt(mSock,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mreq, sizeof(mreq)) != 0) 
			return WSAGetLastError();
		return KSRET_SUC;
	}

	// スレッドの停止
	void StopSockThread(bool inthread)
	{
		// ソケットのクローズ
		unsigned int value;
		if( !m_threadValid.get(value)  )
			return;
		m_threadValid = FALSE;			// スレッド制御フラグ
		if(maListen[1]!=0)
			WSASetEvent(maListen[1]);		// 停止イベント発生
		if( inthread == 0 )
			StopThread(3000);
	}

	// マルチキャストからの脱退
	KSRET LeaveMultiCastGroup( const char* multicastaddr, unsigned int ifaddr = INADDR_ANY){
		ip_mreq mreq;
		memset(&mreq, 0, sizeof(mreq));
		mreq.imr_interface.S_un.S_addr = ifaddr;
		mreq.imr_multiaddr.S_un.S_addr = inet_addr(multicastaddr);
		if( setsockopt( mSock, IPPROTO_IP,IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) != 0 )
			return WSAGetLastError();
		return KSRET_SUC;
	}

	// 切断（Graceful Close処理)
	void GracefulClose(){
		shutdown( mSock, SD_BOTH );						// 切断通知
		Close();
	}

	// データの送信
	KSRET SendData( const char* data, int len) const{
		int ret = sendto( mSock, data, len, 0, (LPSOCKADDR)&mSockAddr, sizeof(sockaddr_in) );			// データ送信
		// 送信結果
		if( ret == SOCKET_ERROR )
			return WSAGetLastError();
		return ret-len;
	}

	// データの受信
	KSRET RecvData( char* buf, int buflen ){
		int ret = recv( mSock, buf, buflen, 0 );
		if( ret == SOCKET_ERROR )
			return  WSAGetLastError();
		else if( ret == 0 ){
			closesocket(mSock);
			mSock = NULL;
			return KSRET_GRACEFULCLOSE;
		}
		return ret;			// 受信バイト数
	}

	// Close
	void Close(){
		closesocket(mSock);
		mSock=NULL;
	}

private:
	// イベントの初期化
	KSRET InitEvent(CKSSockEventReceiverEx& receiver, int TimeOut, void* arg){
		// イベントの作成
		StopSockThread(0);				// 既にあれば停止して削除
		maListen[0] = (void*)WSACreateEvent();	// 作成
		maListen[1] = WSACreateEvent();			// Exit用
		if( WSAEventSelect( mSock, maListen[0], FD_READ | FD_CLOSE ) != 0)
			return WSAGetLastError();

		// 引数の設定
		mTimeOut	= TimeOut;
		mpArg		= arg;
		mpReceiver	= &receiver;
	}

	bool PreThread(){return true;}
	bool IsCondition(){ return WaitMessage(mTimeOut) == 0; }
	bool MainThread(){
		// イベント発生時処理する
		if( WSAEnumNetworkEvents( mSock, maListen[0], &mEvent) != 0 ){		// イベント解析
			int err = WSAGetLastError();
			if( err == WSAENOTSOCK ){
				m_hThread = INVALID_HANDLE_VALUE;
				return false;						// こんなこと言われたら終了でしょ
			}
			return false;
		}
		else{
			if( mpReceiver->OnEvent(mEvent,mpArg) != MSG_PROC ){
				if(mEvent.lNetworkEvents & FD_CLOSE )
					return false;
			}
		}
		WSAResetEvent( maListen[0] );
		return true;
	}
	void FinThread(){
		GracefulClose();
		WSACloseEvent(maListen[0]);
		WSACloseEvent(maListen[1]);
	}

private:
	// メッセージ待ち
	int WaitMessage(const int nTime)
	{
		// ソケットからメッセージがくるまでウェイト。
		//	 但し時間指定があればその間ウェイト
		unsigned int dwResult;
		if(nTime == 0)
			dwResult = WSAWaitForMultipleEvents(2, maListen, FALSE, WSA_INFINITE, FALSE);
		else
			dwResult = WSAWaitForMultipleEvents(2, maListen, FALSE, nTime, FALSE);

		switch(dwResult){
		case WSA_WAIT_EVENT_0:		// 所有権が獲得できた
		case WAIT_IO_COMPLETION:	// ＩＯが使いたいって
			return 0;
		case WSA_WAIT_EVENT_0+1:
			return 1;
		case WSA_WAIT_TIMEOUT:		// タイムアウト
			return -1;			
		default:
			return -2;
		}
	}
};

}	// namespace kssock