// Copyright (C) 2008 Borislav Trifonov
// Based on algorithm from "A Practical Nonblocking Queue Algorithm Using Compare-And-Swap"

// TODO: For 64-bit system, if cannot adapt to _InterlockedCompare64Exchange128() or _InterlockedCompareExchange128(), use instead algorithm from Fig.5 in "Non-Blocking Concurrent FIFO Queues with Single Word Synchronization Primitives"

#if !defined ATOMIC_QUEUE
#define ATOMIC_QUEUE

#include <exception>
#include "processor.h"

template<typename T>
class AtomicQueue
{
public:
	class Exc : public std::exception
	{
	public:
		inline Exc(const char []);
	};

	inline AtomicQueue(unsigned long);
	inline ~AtomicQueue(void);
	inline bool Push(T const); // Returns false if full
	inline bool Pop(T &); // Returns false if emtpy
	inline T operator[](unsigned long); // No error checking
private:
	inline AtomicQueue(void);
	AtomicQueue(AtomicQueue const &);
	AtomicQueue &operator=(AtomicQueue const &);
	__declspec(align(4)) unsigned long head, tail;
	unsigned long size;
	typedef Pack<T, unsigned long> AQPack;
	AQPack *buff;
};

template<typename T>
inline AtomicQueue<T>::AtomicQueue(unsigned long sz) : head(0), tail(0), size(sz), buff(0)
{
	STATIC_ASSERT(sizeof(T) == 4);
	STATIC_ASSERT(sizeof(unsigned long) == 4);
	STATIC_ASSERT(sizeof(unsigned long long) == 8);
	buff = static_cast<AQPack *>(_mm_malloc(size * sizeof(AQPack), 8));
	if (!buff) throw Exc("Not enough memory");
	for (unsigned long i = 0; i < size; ++i) buff[i].pack = 0ull;
}

#include <iostream>
template<typename T>
inline AtomicQueue<T>::~AtomicQueue(void)
{
	_mm_free(buff);
}

template<typename T>
inline bool AtomicQueue<T>::Push(T const item)
{
	while (true)
	{
		__declspec(align(4)) unsigned long const t(Acquire(tail)), h(Acquire(head));
		__declspec(align(8)) AQPack a(Acquire(buff[t % size].pack));
		if (t != Acquire(tail)) continue;
		if (t == Acquire(head) + size)
		{
			if (Acquire(buff[h % size].lower) && h == Acquire(head)) return false;
			_InterlockedCompareExchange(&head, h + 1, h);
			continue;
		}
		if (a.lower)
		{
			if (Acquire(buff[t % size].lower)) _InterlockedCompareExchange(&tail, t + 1, t);
		}
		else
		{
			__declspec(align(8)) AQPack b(item, a.upper + 1);
			if (_InterlockedCompareExchange64(&(buff[t % size].pack), b.pack, a.pack) == static_cast<long long>(a.pack))
			{
				_InterlockedCompareExchange(&tail, t + 1, t);
				return true;
			}
		}
	}
}

template<typename T>
inline bool AtomicQueue<T>::Pop(T &item)
{
	while (true)
	{
		__declspec(align(4)) unsigned long const h(Acquire(head)), t(Acquire(tail));
		__declspec(align(8)) AQPack a(Acquire(buff[h % size].pack));
		if (h != Acquire(head)) continue;
		if (h == Acquire(tail))
		{
			if (!Acquire(buff[t % size].lower) && t == Acquire(tail)) return false;
			_InterlockedCompareExchange(&tail, t + 1, t);
			continue;
		}
		if (!a.lower)
		{
			if (!Acquire(buff[h % size].lower)) _InterlockedCompareExchange(&head, h + 1, h);
		}
		else
		{
			__declspec(align(8)) AQPack b(0, a.upper + 1);
			if (_InterlockedCompareExchange64(&(buff[h % size].pack), b.pack, a.pack) == static_cast<long long>(a.pack))
			{
				_InterlockedCompareExchange(&head, h + 1, h);
				item = a.lower;
				return true;
			}
		}
	}
}

template<typename T>
inline T AtomicQueue<T>::operator[](unsigned long i)
{
	return buff[(head + i) % size].lower;
}

template<typename T>
inline AtomicQueue<T>::Exc::Exc(const char msg[]) : std::exception(msg)
{
}

#endif

