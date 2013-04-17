#pragma once
//__________________________________________________________________
// FileName : KSRand.h
// Abstract : CKSRandクラスインライン
//------------------------------------------------------------------
// K.H 2013/04/02 23:32:27 First Edition
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
#include <stdlib.h>
#include <math.h>
#include "KSConst.h"
//''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

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

	//--------------------------------------------------------------------------/
	// Class Name	/ CKSRandNorm
	//--------------------------------------------------------------------------/
	// Abstruct		/ 正規分布乱数
	// History		/ 2013.04.02 Kei Hishinuma First Edition					
	//--------------------------------------------------------------------------/
	class CKSRandNorm
	{
	private:
		double	mAvg;			// 平均
		double	mNorm;			// sqrt(分散)=標準偏差
		CKSRand	mRand;			// 乱数生成
		bool	mbSin;			// ２つのうちどっちを取得するか
		double	mAlpha;			// ボックスミューラの係数１
		double	mBeta;			// ボックスミューラの係数２

	public:
		//--------------------------------------------------------------------------/
		// Function		/ コンストラクタ
		//--------------------------------------------------------------------------/
		// Abstruct		/ 正規分布の平均値と分散を指定すること
		// History		/ 2013.04.02 Kei Hishinuma First Edition					/
		//--------------------------------------------------------------------------/
		CKSRandNorm(double avg, double variance )
			: mAvg(avg)
			, mNorm(sqrt(variance))
			, mbSin(true)
			, mAlpha(0.0)
			, mBeta(0.0)
		{}
		~CKSRandNorm(){}
		void init(){mRand.init();}
		void init(int seed){mRand.init(seed);}
		double get(){
			if(mbSin){
				mbSin = !mbSin;
				mAlpha = sqrt( -2.0 * log( mRand.get1<double>() ) );
				mBeta = ks::TPI2<double>() * mRand.get1<double>();
				return mAvg + mNorm * sin( mBeta ) * mAlpha;
			}
			mbSin = !mbSin;
			return mAvg + mNorm * cos( mBeta ) * mAlpha;
		}
	};

}