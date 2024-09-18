// drumkv1_sched.cpp
//
/****************************************************************************
   Copyright (C) 2012-2024, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "drumkv1_sched.h"

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <QHash>


//-------------------------------------------------------------------------
// drumkv1_sched_thread - worker/schedule thread decl.
//

class drumkv1_sched_thread : public QThread
{
public:

	// ctor.
	drumkv1_sched_thread(uint32_t nsize = 32);

	// dtor.
	~drumkv1_sched_thread();

	// schedule processing and wake from wait condition.
	void schedule(drumkv1_sched *sched);

	// process all pending runs immediately.
	void sync_pending();

	// clear all pending runs immediately.
	void sync_reset();

protected:

	// main thread executive.
	void run();
	void run_process();

	// clear all.
	void clear();

private:

	// sync queue instance reference.
	uint32_t m_nsize;
	uint32_t m_nmask;

	drumkv1_sched **m_items;

	volatile uint32_t m_iread;
	volatile uint32_t m_iwrite;

	// whether the thread is logically running.
	volatile bool m_running;

	// thread synchronization objects.
	QMutex m_mutex;
	QWaitCondition m_cond;
};


static drumkv1_sched_thread *g_sched_thread = nullptr;
static uint32_t g_sched_refcount = 0;

static QHash<drumkv1 *, QList<drumkv1_sched::Notifier *> > g_sched_notifiers;


//-------------------------------------------------------------------------
// drumkv1_sched_thread - worker/schedule thread impl.
//

// ctor.
drumkv1_sched_thread::drumkv1_sched_thread ( uint32_t nsize ) : QThread()
{
	m_nsize = (4 << 1);
	while (m_nsize < nsize)
		m_nsize <<= 1;
	m_nmask = (m_nsize - 1);
	m_items = new drumkv1_sched * [m_nsize];

	m_iread  = 0;
	m_iwrite = 0;

	::memset(m_items, 0, m_nsize * sizeof(drumkv1_sched *));

	m_running = false;
}


// dtor.
drumkv1_sched_thread::~drumkv1_sched_thread (void)
{
	// fake sync and wait
	if (m_running && isRunning()) do {
		if (m_mutex.tryLock()) {
			m_running = false;
			m_cond.wakeAll();
			m_mutex.unlock();
		}
	} while (!wait(100));

	delete [] m_items;
}


// schedule processing and wake from wait condition.
void drumkv1_sched_thread::schedule ( drumkv1_sched *sched )
{
	if (!sched->sync_wait()) {
		const uint32_t w = (m_iwrite + 1) & m_nmask;
		if (w != m_iread) {
			m_items[m_iwrite] = sched;
			m_iwrite = w;
		}
	}

	if (m_mutex.tryLock()) {
		m_cond.wakeAll();
		m_mutex.unlock();
	}
}


// process all pending runs, immediately.
void drumkv1_sched_thread::sync_pending (void)
{
	QMutexLocker locker(&m_mutex);

	run_process();
}


// clear all pending runs, immediately.
void drumkv1_sched_thread::sync_reset (void)
{
	QMutexLocker locker(&m_mutex);

	clear();
}


void drumkv1_sched_thread::clear (void)
{
	m_iread  = 0;
	m_iwrite = 0;

	::memset(m_items, 0, m_nsize * sizeof(drumkv1_sched *));
}


// main thread executive.
void drumkv1_sched_thread::run (void)
{
	m_mutex.lock();

	m_running = true;

	while (m_running) {
		// do whatever we must...
		run_process();
		// wait for sync...
		m_cond.wait(&m_mutex);
	}

	m_mutex.unlock();
}


void drumkv1_sched_thread::run_process (void)
{
	uint32_t r = m_iread;
	while (r != m_iwrite) {
		drumkv1_sched *sched = m_items[r];
		if (sched) {
			sched->sync_process();
			m_items[r] = nullptr;
		}
		++r &= m_nmask;
	}
	m_iread = r;
}


//-------------------------------------------------------------------------
// drumkv1_sched - worker/scheduled stuff (pure virtual).
//

// ctor.
drumkv1_sched::drumkv1_sched ( drumkv1 *pDrumk, Type stype, uint32_t nsize )
	: m_pDrumk(pDrumk), m_stype(stype), m_sync_wait(false)
{
	m_nsize = (4 << 1);
	while (m_nsize < nsize)
		m_nsize <<= 1;
	m_nmask = (m_nsize - 1);
	m_items = new int [m_nsize];

	m_iread  = 0;
	m_iwrite = 0;

	::memset(m_items, 0, m_nsize * sizeof(int));

	if (++g_sched_refcount == 1 && g_sched_thread == nullptr) {
		g_sched_thread = new drumkv1_sched_thread();
		g_sched_thread->start();
	}
}


// dtor (virtual).
drumkv1_sched::~drumkv1_sched (void)
{
	delete [] m_items;

	if (--g_sched_refcount == 0) {
		if (g_sched_thread) {
			delete g_sched_thread;
			g_sched_thread = nullptr;
		}
	}
}


// instance access.
drumkv1 *drumkv1_sched::instance (void) const
{
	return m_pDrumk;
}


// schedule process.
void drumkv1_sched::schedule ( int sid )
{
	const uint32_t w = (m_iwrite + 1) & m_nmask;
	if (w != m_iread) {
		m_items[m_iwrite] = sid;
		m_iwrite = w;
	}

	if (g_sched_thread)
		g_sched_thread->schedule(this);
}


// test-and-set.
bool drumkv1_sched::sync_wait (void)
{
	const bool sync_wait = m_sync_wait;

	if (!sync_wait)
		m_sync_wait = true;

	return sync_wait;
}


// scheduled processor.
void drumkv1_sched::sync_process (void)
{
	// do whatever we must...
	uint32_t r = m_iread;
	while (r != m_iwrite) {
		const int sid = m_items[r];
		process(sid);
		sync_notify(m_pDrumk, m_stype, sid);
		m_items[r] = 0;
		++r &= m_nmask;
	}
	m_iread = r;

	m_sync_wait = false;
}


// signal broadcast (static).
void drumkv1_sched::sync_notify ( drumkv1 *pDrumk, Type stype, int sid )
{
	if (g_sched_notifiers.contains(pDrumk)) {
		const QList<Notifier *>& list
			= g_sched_notifiers.value(pDrumk);
		QListIterator<Notifier *> iter(list);
		while (iter.hasNext())
			iter.next()->notify(stype, sid);
	}
}


// process/clear pending schedules, immediately. (static)
void drumkv1_sched::sync_pending (void)
{
	if (g_sched_thread)
		g_sched_thread->sync_pending();
}


void drumkv1_sched::sync_reset (void)
{
	if (g_sched_thread)
		g_sched_thread->sync_reset();
}


//-------------------------------------------------------------------------
// drumkv1_sched::Notifier - worker/schedule proxy decl.
//

// ctor.
drumkv1_sched::Notifier::Notifier ( drumkv1 *pDrumk )
	: m_pDrumk(pDrumk)
{
	g_sched_notifiers[pDrumk].append(this);
}


// dtor.
drumkv1_sched::Notifier::~Notifier (void)
{
	if (g_sched_notifiers.contains(m_pDrumk)) {
		QList<Notifier *>& list = g_sched_notifiers[m_pDrumk];
		list.removeAll(this);
		if (list.isEmpty())
			g_sched_notifiers.remove(m_pDrumk);
	}
}


// end of drumkv1_sched.cpp
