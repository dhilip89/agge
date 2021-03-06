#pragma once

#include "handle.h"

#include <agge/types.h>

namespace agge
{
	class semaphore : noncopyable
	{
	public:
		semaphore();
		~semaphore();

		void signal();
		void wait();

	private:
		handle _handle;
	};

	class hybrid_event
	{
	public:
		enum { max_spin = 20000 };
		enum { state_free, state_set, state_blocked };

	public:
		hybrid_event();

		void signal();
		void wait();

	private:
		volatile long _state;
		semaphore _semaphore;
	};
}
