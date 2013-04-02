#pragma once
//__________________________________________________________________
// FileName : KSRand.h
// Abstract : CKSRandクラスインライン
//------------------------------------------------------------------
// K.H 2013/04/02 23:32:27 First Edition
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
#include <stdlib.h>

namespace ksmath{
	//__________________________________________________________________
	// Class    : CKSRand
	// Abstract : 乱数発生器
	//------------------------------------------------------------------
	// K.H 2013/04/02 23:32:27 ほぼ同時刻に呼び出した場合にSeedが同じになるのを回避
	//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	class CKSRand
	{
	private:
		static unsigned int getSeed(){
			static int cnt = 0;
			return GetTickCount() + cnt++;
		}
	public:
		CKSRand(void){init();}
		~CKSRand(void){}
		void init(){init(getSeed());}
		void init(int seed){srand(seed);}
		int get(){return rand();}
		template<typename T> T get1(){ return rand()/static_cast<T>(RAND_MAX); }
	};

}