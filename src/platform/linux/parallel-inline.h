#pragma once

#include <pthread.h>
#include <semaphore.h>

#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64) || defined(__i386)
	#include <xmmintrin.h>
#endif

namespace agge
{
	class hybrid_event
	{
	public:
		enum { max_spin = 20000 };

	public:
		hybrid_event()
			: _lock_state(0)
		{	sem_init(&_native, 0, 0);	}

		~hybrid_event()
		{	sem_destroy(&_native);	}

		void set()
		{
			if (__sync_val_compare_and_swap(&_lock_state, 0 /*if was not locked...*/, 1 /*... flag*/) == 2 /*was blocked*/)
				sem_post(&_native);
		}

		void wait()
		{
			for (bool ready = false; !ready; )
			{
				for (long i = max_spin; !ready && i; --i)
				{
					ready = !!__sync_lock_test_and_set(&_lock_state, 0);
#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64) || defined(__i386)
					_mm_pause();
#elif defined(__arm__) && (defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_7__))
					asm volatile ("yield" ::: "memory");
#endif
				}
				if (!ready && __sync_val_compare_and_swap(&_lock_state, 0 /*if was not flagged...*/, 2 /*... block*/) == 0 /*was not flagged*/)
					sem_wait(&_native);
			}
		}

	private:
		sem_t _native;
		volatile long _lock_state;
	};

	struct parallel::thread : noncopyable
	{
	public:
		explicit thread(count_t id);
		~thread();

		hybrid_event ready;
		hybrid_event done;

	public:
		parallel::kernel_function *kernel;

	private:
		static void *thread_proc(void *data);

	private:
		const count_t _id;
		pthread_t _thread;
	};



	parallel::thread::thread(count_t id)
		: kernel(0), _id(id)
	{
		if (int error = pthread_create(&_thread, 0, &parallel::thread::thread_proc, this))
			throw std::bad_alloc();
	}

	parallel::thread::~thread()
	{
		kernel = 0;
		ready.set();
		pthread_join(_thread, 0);
	}

	void *parallel::thread::thread_proc(void *data)
	{
		thread *this_ = static_cast<thread *>(data);

		for (; this_->ready.wait(), this_->kernel; this_->kernel = 0, this_->done.set())
			(*this_->kernel)(this_->_id);
		return 0;
	}
}
