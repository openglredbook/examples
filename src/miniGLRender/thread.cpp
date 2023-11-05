#include "thread.h"
#include <windows.h>

//// qqSysInterlockedInteger
//interlockedInt_t qqSysInterlockedInteger::Sys_InterlockedIncrement(interlockedInt_t& value)
//{
//	return InterlockedIncrementAcquire(&value);
//}
//
//interlockedInt_t qqSysInterlockedInteger::Sys_InterlockedDecrement(interlockedInt_t& value)
//{
//	return InterlockedDecrementRelease(&value);
//}
//
//interlockedInt_t qqSysInterlockedInteger::Sys_InterlockedAdd(interlockedInt_t& value, interlockedInt_t i)
//{
//	return InterlockedExchangeAdd(&value, i) + i;
//}
//
//interlockedInt_t qqSysInterlockedInteger::Sys_InterlockedSub(interlockedInt_t& value, interlockedInt_t i)
//{
//	return InterlockedExchangeAdd(&value, -i) - i;
//}
