// drumkv1_sched.h
//
/****************************************************************************
   Copyright (C) 2012-2014, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __drumkv1_sched_h
#define __drumkv1_sched_h


// forward decls.
class drumkv1_sched_notifier;


//-------------------------------------------------------------------------
// drumkv1_sched - worker/scheduled stuff (pure virtual).
//

class drumkv1_sched
{
public:

	// ctor.
	drumkv1_sched();

	// virtual dtor.
	virtual ~drumkv1_sched();

	// schedule process.
	void schedule();

	// test-and-set wait.
	bool sync_wait();
	
	// scheduled processor.
	void sync_process();

	// (pure) virtual processor.
	virtual void process() = 0;

	// signal/slot proxy accessor (static).
	static drumkv1_sched_notifier *notifier();

private:

	// instance variables.
	volatile bool m_sync_wait;
};


//-------------------------------------------------------------------------
// drumkv1_sched_notifier - worker/schedule proxy decl.
//

#include <QObject>

class drumkv1_sched_notifier : public QObject
{
	Q_OBJECT

public:

	drumkv1_sched_notifier(QObject *parent = NULL);

	void sync_notify();

signals:

	void notify();
};


#endif	// __drumkv1_sched_h

// end of drumkv1_sched.h
