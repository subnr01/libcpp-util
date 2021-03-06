//============================================================================
//                                  libcpp-util
//                   A simple odds-n-ends library for C++11
//
//         Licensed under modified BSD license. See LICENSE for details.
//============================================================================

#ifndef LIBCPP_UTIL_SEMAPHORE_H
#define LIBCPP_UTIL_SEMAPHORE_H

#include "libcpp-util/smp/spinlock.h"

#include <condition_variable>

namespace cpputil {

class semaphore {
private:
	cacheline_spinlock s;
	std::condition_variable_any cv;
	unsigned count;
	unsigned waiters;

	semaphore(const semaphore&) = delete;
	semaphore& operator=(const semaphore&) = delete;
public:
	semaphore(unsigned initial_count) 
		: count(initial_count), waiters(0) {
	}

	~semaphore() = default;

	void post() {
		std::unique_lock<cacheline_spinlock> lock(s);
		count++;
		if (waiters > 0) {
			waiters--;
			lock.unlock();
			cv.notify_one();
		}
	}

	void post_all() {
		std::lock_guard<cacheline_spinlock> lock(s);
		count += waiters;
		waiters = 0;
		cv.notify_all();
	}

	void wait() {
		std::unique_lock<cacheline_spinlock> lock(s);
		if (count > 0) {
			count--;
		} else {
			waiters++;
			while (count == 0)
				cv.wait(lock);
			count--;
		}
	}

	bool try_wait() {
		std::lock_guard<cacheline_spinlock> lock(s);
		if (count > 0) {
			count--;
			return true;
		} else {
			return false;
		}
	}

	unsigned value() {
		std::lock_guard<cacheline_spinlock> lock(s);
		return count;
	}
};

}
#endif
