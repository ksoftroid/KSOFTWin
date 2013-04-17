#pragma once

namespace ks{
	typedef int KSRET;
	const int KSRET_SUC				= 0;
	const int KSRET_NOTSENDALL		= -1;
	const int KSRET_GRACEFULCLOSE	= -1;

	// π関連
	template<typename T> T TPIQ3(){ return static_cast<T>(2.3561944901923449288469825374596);}	// 3pi/4
	template<typename T> T TPIQ(){ return static_cast<T>(0.78539816339744830961566084581988);}	// pi/4
	template<typename T> T TPIH(){ return static_cast<T>(1.5707963267948966192313216916398);}	// pi/2
	template<typename T> T TPI(){ return static_cast<T>(3.1415926535897932384626433832795);}
	template<typename T> T TPI2(){ return static_cast<T>(6.283185307179586476925286766559);}

}