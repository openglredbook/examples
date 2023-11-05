#ifndef __THREAD_H__
#define __THREAD_H__

typedef int	interlockedInt_t;

class qqSysInterlockedInteger
{
public:
	qqSysInterlockedInteger() : value(0) {}

	int					Increment() { return Sys_InterlockedIncrement(value); }
	int					Decrement() { return Sys_InterlockedDecrement(value); }
	int					Add(int v) { return Sys_InterlockedAdd(value, (interlockedInt_t)v); }
	int					Sub(int v) { return Sys_InterlockedSub(value, (interlockedInt_t)v); }
	int					GetValue() const { return value; }
	void				SetValue(int v) { value = (interlockedInt_t)v; }

private:
	interlockedInt_t Sys_InterlockedIncrement(interlockedInt_t& value);
	interlockedInt_t Sys_InterlockedDecrement(interlockedInt_t& value);
	interlockedInt_t Sys_InterlockedAdd(interlockedInt_t& value, interlockedInt_t i);
	interlockedInt_t Sys_InterlockedSub(interlockedInt_t& value, interlockedInt_t i);

	interlockedInt_t	value;
};

#endif // __COMMON_H__
