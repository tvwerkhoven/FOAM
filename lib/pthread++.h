/*
    pthread++.h -- C++ wrappers around pthread
    Copyright (C) 2006  Guus Sliepen <guus@sliepen.eu.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef PTHREAD_H_WRAPPER
#define PTHREAD_H_WRAPPER

#include <cstdio>
#include <pthread.h>
#include <signal.h>
#include <sigc++/slot.h>
#include <sys/time.h>
#include <time.h>

// Via https://issues.asterisk.org/view.php?id=1411
#ifndef PTHREAD_MUTEX_RECURSIVE_NP
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif
#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP PTHREAD_MUTEX_INITIALIZER
#endif

// Via http://www.haskell.org/pipermail/cvs-ghc/2009-January/047181.html
#ifndef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP PTHREAD_MUTEX_ERRORCHECK
#endif




#ifdef sigmask
#undef sigmask
#endif

namespace pthread {
	class attr {
		friend class thread;
		pthread_attr_t pthread_attr;

		public:
		attr() { pthread_attr_init(&pthread_attr); }
		~attr() { pthread_attr_destroy(&pthread_attr); }
		int setdetachstate(int detachstate) { return pthread_attr_setdetachstate(&pthread_attr, detachstate); }
		int getdetachstate(int *detachstate) { return pthread_attr_getdetachstate(&pthread_attr, detachstate); }
		int setschedpolicy(int schedpolicy) { return pthread_attr_setschedpolicy(&pthread_attr, schedpolicy); }
		int getschedpolicy(int *schedpolicy) { return pthread_attr_getschedpolicy(&pthread_attr, schedpolicy); }
		int setschedparam(const struct sched_param *schedparam) { return pthread_attr_setschedparam(&pthread_attr, schedparam); }
		int getschedparam(struct sched_param *schedparam) { return pthread_attr_getschedparam(&pthread_attr, schedparam); }
		int setinheritsched(int inheritsched) { return pthread_attr_setinheritsched(&pthread_attr, inheritsched); }
		int getinheritsched(int *inheritsched) { return pthread_attr_getinheritsched(&pthread_attr, inheritsched); }
		int setscope(int scope) { return pthread_attr_setscope(&pthread_attr, scope); }
		int getscope(int *scope) { return pthread_attr_getscope(&pthread_attr, scope); }
		int setstacksize(size_t stacksize) { return pthread_attr_setstacksize(&pthread_attr, stacksize); }
		int getstacksize(size_t *stacksize) { return pthread_attr_getstacksize(&pthread_attr, stacksize); }
	};

	class thread {
		pthread_t pthread;

		static void *start_slot(void *arg) {
			sigc::slot<void> *tmp = (sigc::slot<void> *)arg, slot = *tmp;
			delete tmp;
			slot();
			return 0;
		}

		public:
		thread(): pthread(0) {}
		thread(pthread_t initialiser): pthread(initialiser) {}
		thread(attr *attr, void *(*start_routine)(void *), void *arg = NULL) { create(attr, start_routine, arg); }
		thread(void *(*start_routine)(void *), void *arg = NULL) { create(start_routine, arg); }
		thread(attr *attr, sigc::slot<void> slot) { create(attr, slot); }
		thread(sigc::slot<void> slot) { create(slot); }
		//~thread() { join(); }

		int create(attr *attr, void *(*start_routine)(void *), void *arg = NULL) { return pthread_create(&pthread, &attr->pthread_attr, start_routine, arg); }
		int create(void *(*start_routine)(void *), void *arg = NULL) { return create(NULL, start_routine, arg); }
		int create(attr *attr, sigc::slot<void> slot) { return create(attr, start_slot, new sigc::slot<void>(slot)); }
		int create(sigc::slot<void> slot) { return create(NULL, start_slot, new sigc::slot<void>(slot)); }
		int join(void **thread_return = NULL) { return pthread_join(pthread, thread_return); }
		int cancel() { return pthread_cancel(pthread); }
		int kill(int signo) { return pthread_kill(pthread, signo); }
		int detach() { return pthread_detach(pthread); }
		bool isself() { return pthread_equal(this->pthread, pthread_self()); }
		bool operator ==(thread other) { return pthread_equal(this->pthread, other.pthread); }
		bool operator !=(thread other) { return !pthread_equal(this->pthread, other.pthread); }

		int getschedparam(int *policy, struct sched_param *param) { return pthread_getschedparam(pthread, policy, param); }
		int setschedparam(int policy, const struct sched_param *param) { return pthread_setschedparam(pthread, policy, param); }
    // int setschedprio(int prio) { return pthread_setschedprio(pthread, prio); }
    // int getcpuclockid(clockid_t *clock_id) { return pthread_getcpuclockid(pthread, clock_id); }
	};

	static inline int setcancelstate(int state, int *oldstate = NULL) { return pthread_setcancelstate(state, oldstate); }
	static inline int setcanceltype(int type, int *oldtype = NULL) { return pthread_setcanceltype(type, oldtype); }
	static inline void testcancel() { pthread_testcancel(); }
	static inline thread self() { thread self(pthread_self()); return self; }
	static inline int sigmask(int how, const sigset_t *newmask = NULL, sigset_t *oldmask = NULL) { sigset_t all; if(!newmask) {sigfillset(&all); newmask = &all;} return pthread_sigmask(how, newmask, oldmask); }
	static inline int sigwait(const sigset_t *set, int *sig) { return ::sigwait(set, sig); }
	static inline void exit(void *retval = NULL) { pthread_exit(retval); }
//	static inline void yield() { pthread_yield(); }
	static inline void yield() { sched_yield(); }
	static inline int atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void)) { return pthread_atfork(prepare, parent, child); }	
  // static inline int getaffinity(cpu_set_t *mask) { return sched_getaffinity(0, sizeof *mask, mask); }
  // static inline int setaffinity(cpu_set_t *mask) { return sched_setaffinity(0, sizeof *mask, mask); }

	class mutexattr {
		friend class mutex;
		pthread_mutexattr_t pthread_mutexattr;

		public:
		mutexattr(int kind) { pthread_mutexattr_init(&pthread_mutexattr); settype(kind); }
		mutexattr() { pthread_mutexattr_init(&pthread_mutexattr); }
		~mutexattr() { pthread_mutexattr_destroy(&pthread_mutexattr); }
		int settype(int kind) { return pthread_mutexattr_settype(&pthread_mutexattr, kind); }
		int gettype(int *kind) { return pthread_mutexattr_gettype(&pthread_mutexattr, kind); }
	};

	static const pthread_mutex_t MUTEX_INITIALIZER = PTHREAD_MUTEX_INITIALIZER;
  static const pthread_mutex_t RECURSIVE_MUTEX_INITIALIZER = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
  // static const pthread_mutex_t ERRORCHECK_MUTEX_INITIALIZER = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;

	class mutex {
		friend class cond;

		pthread_mutex_t pthread_mutex;

		public:
		mutex(const pthread_mutex_t initializer = MUTEX_INITIALIZER): pthread_mutex(initializer) {}
		mutex(const mutexattr *attr) { pthread_mutex_init(&pthread_mutex, &attr->pthread_mutexattr); }
		~mutex() { pthread_mutex_destroy(&pthread_mutex); }

		int lock() { return pthread_mutex_lock(&pthread_mutex); }
		int trylock() { return pthread_mutex_trylock(&pthread_mutex); }
    // int timedlock(const struct timespec *abstime) { return pthread_mutex_timedlock(&pthread_mutex, abstime); }
		int unlock() { return pthread_mutex_unlock(&pthread_mutex); }
	};

	class recursivemutex: public mutex {
		public:
		recursivemutex(): mutex(RECURSIVE_MUTEX_INITIALIZER) {}
	};

  // class errorcheckmutex: public mutex {
  //  public:
  //  errorcheckmutex(): mutex(ERRORCHECK_MUTEX_INITIALIZER) {}
  // };

	class mutexholder {
		mutex *m;
		public:
		mutexholder(mutex *m): m(m) { m->lock(); }
		~mutexholder() { m->unlock(); }
	};

	static const pthread_cond_t COND_INITIALIZER = PTHREAD_COND_INITIALIZER;
	
	class cond {
		pthread_cond_t pthread_cond;

		public:
		cond(const pthread_cond_t initialiser = COND_INITIALIZER): pthread_cond(initialiser) {}
		cond(pthread_condattr_t *cond_attr) { pthread_cond_init(&pthread_cond, cond_attr); }
		~cond() { pthread_cond_destroy(&pthread_cond); }

		int signal() { return pthread_cond_signal(&pthread_cond); }
		int broadcast() { return pthread_cond_broadcast(&pthread_cond); }
		int wait(mutex &mutex) { return pthread_cond_wait(&pthread_cond, &mutex.pthread_mutex); }
		int timedwait(mutex &mutex, const struct timespec *abstime) { return pthread_cond_timedwait(&pthread_cond, &mutex.pthread_mutex, abstime); }
		int timedwait(mutex &mutex, long usec) { 
			struct timeval now;
			struct timespec timeout;
			gettimeofday(&now, 0);
			timeout.tv_sec = now.tv_sec + usec / 1000000;
			timeout.tv_nsec = (now.tv_usec + usec % 1000000) * 1000;
			if(timeout.tv_nsec >= 1000000000) {
				timeout.tv_nsec -= 1000000000;
				timeout.tv_sec++;
			}
			return pthread_cond_timedwait(&pthread_cond, &mutex.pthread_mutex, &timeout);
		}
	};

  // class once {
  //  pthread_once_t once_control;
  // 
  //  public:
  //  once(void (*init_routine)(void)): once_control(PTHREAD_ONCE_INIT) { pthread_once(&once_control, init_routine); }
  //  once(): once_control(PTHREAD_ONCE_INIT) {}
  // 
  //  void run(void (*init_routine)(void)) { pthread_once(&once_control, init_routine); }
  // };

	static const pthread_rwlock_t RWLOCK_INITIALIZER = PTHREAD_RWLOCK_INITIALIZER;
  // static const pthread_rwlock_t RWLOCK_WRITER_NONRECURSIVE_INITIALIZER = PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP;

	class rwlock {
		pthread_rwlock_t pthread_rwlock;

		public:
		rwlock(const pthread_rwlock_t initialiser = RWLOCK_INITIALIZER): pthread_rwlock(initialiser) {}
		rwlock(pthread_rwlockattr_t *rwlock_attr) { pthread_rwlock_init(&pthread_rwlock, rwlock_attr); }
		~rwlock() { pthread_rwlock_destroy(&pthread_rwlock); }

		int rdlock() { return pthread_rwlock_rdlock(&pthread_rwlock); }
		int tryrdlock() { return pthread_rwlock_tryrdlock(&pthread_rwlock); }
    // int timedrdlock(const struct timespec *abstime) { return pthread_rwlock_timedrdlock(&pthread_rwlock, abstime); }
		int wrlock() { return pthread_rwlock_wrlock(&pthread_rwlock); }
    // int trywrlock() { return pthread_rwlock_trywrlock(&pthread_rwlock); }
    // int timedwrlock(const struct timespec *abstime) { return pthread_rwlock_timedwrlock(&pthread_rwlock, abstime); }
		int unlock() { return pthread_rwlock_unlock(&pthread_rwlock); }
	};

	class rdlockholder {
		rwlock *l;
		public:
		rdlockholder(rwlock *l): l(l) { l->rdlock(); }
		~rdlockholder() { l->unlock(); }
	};

	class wrlockholder {
		rwlock *l;
		public:
		wrlockholder(rwlock *l): l(l) { l->wrlock(); }
		~wrlockholder() { l->unlock(); }
	};

  // class spinlock {
  //  pthread_spinlock_t pthread_spinlock;
  // 
  //  public:
  //  spinlock(bool shared = false) { pthread_spin_init(&pthread_spinlock, shared); }
  //  ~spinlock() { pthread_spin_destroy(&pthread_spinlock); }
  // 
  //  int lock() { return pthread_spin_lock(&pthread_spinlock); }
  //  int trylock() { return pthread_spin_trylock(&pthread_spinlock); }
  //  int unlock() { return pthread_spin_unlock(&pthread_spinlock); }
  // };

  // class spinlockholder {
  //  spinlock *l;
  //  public:
  //  spinlockholder(spinlock *l): l(l) { l->lock(); }
  //  ~spinlockholder() { l->unlock(); }
  // };

  // class barrier {
  //  pthread_barrier_t pthread_barrier;
  // 
  //  public:
  //  barrier(const pthread_barrierattr_t *barrierattr, unsigned int count) { pthread_barrier_init(&pthread_barrier, barrierattr, count); }
  //  barrier(unsigned int count) { pthread_barrier_init(&pthread_barrier, NULL, count); }
  //  ~barrier() { pthread_barrier_destroy(&pthread_barrier); }
  // 
  //  int wait() { return pthread_barrier_wait(&pthread_barrier); }
  // };

	class key {
		pthread_key_t pthread_key;

		public:
		key(void (*destr_function)(void *) = NULL) { pthread_key_create(&pthread_key, destr_function); }
		key(void (*destr_function)(void *), void *pointer) { pthread_key_create(&pthread_key, destr_function); setspecific(pointer); }
		~key() { pthread_key_delete(pthread_key); }

		void *getspecific() { return pthread_getspecific(pthread_key); }
		int setspecific(void *pointer) { return pthread_setspecific(pthread_key, pointer); }
	};
}

#endif
