#ifndef __SWAP_H__
#define __SWAP_H__

namespace TMPLEX
{
// swaps two elements
template<class T> inline void Swap(T& x, T& y)
{
	T temp;

	temp = x;
	x = y;
	y = temp;
}

// swaps two elements and returns value
// usefull for situation when need return old value
template<class T> inline T& SwapRet(T& x, T& y)
{
	T temp;

	temp = x;
	x = y;
	y = temp;

	return x;
}

// template function returns y if x < y
// To avoid conflicts with min and max in WINDEF.H
template<class T> inline const T& Max(const T& x, const T& y)
{
	if(x > y)
	{
		return x;
	}

	return y;
}


// template function returns y if x > y
// To avoid conflicts with min and max in WINDEF.H
template<class T> inline const T& Min(const T& x, const T& y)
{
	if(x < y)
	{
		return x;
	}

	return y;
}

}

#endif