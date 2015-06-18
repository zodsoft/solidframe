// frame/schedulerbase.hpp
//
// Copyright (c) 2007, 2008 Valentin Palade (vipalade @ gmail . com) 
//
// This file is part of SolidFrame framework.
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.
//
#ifndef SOLID_FRAME_SCHEDULER_BASE_HPP
#define SOLID_FRAME_SCHEDULER_BASE_HPP

#include "frame/common.hpp"
#include "system/error.hpp"
#include "system/function.hpp"

namespace solid{

class Thread;

namespace frame{

class Service;
class ReactorBase;
class ObjectBase;


//typedef FunctorReference<bool, ReactorBase&>	ScheduleFunctorT;
typedef FUNCTION<bool(ReactorBase&)>			ScheduleFunctionT;

//! A base class for all schedulers
class SchedulerBase{
public:
protected:
	typedef Thread* (*CreateWorkerF)(SchedulerBase &_rsch, const size_t);
	
	typedef FUNCTION<bool()>					ThreadEnterFunctionT;
	typedef FUNCTION<void()>					ThreadExitFunctionT;
	
	ErrorConditionT doStart(
		CreateWorkerF _pf,
		ThreadEnterFunctionT &_renf,
		ThreadExitFunctionT &_rexf,
		size_t _reactorcnt
	);

	void doStop(const bool _wait = true);
	
	ObjectUidT doStartObject(ObjectBase &_robj, Service &_rsvc, ScheduleFunctionT &_rfct, ErrorConditionT &_rerr);
	
protected:
	SchedulerBase();
	~SchedulerBase();
private:
	friend class ReactorBase;
	
	bool prepareThread(const size_t _idx, ReactorBase &_rsel, const bool _success);
	void unprepareThread(const size_t _idx, ReactorBase &_rsel);
	size_t doComputeScheduleReactorIndex();
private:
	struct Data;
	Data	&d;
};

}//namespace frame
}//namespace solid

#endif

