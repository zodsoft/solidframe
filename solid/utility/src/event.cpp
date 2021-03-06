// solid/frame/src/event.cpp
//
// Copyright (c) 2015 Valentin Palade (vipalade @ gmail . com) 
//
// This file is part of SolidFrame framework.
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.
//
#include "solid/system/cassert.hpp"
#include "solid/utility/event.hpp"

namespace solid{

//-----------------------------------------------------------------------------

	const EventCategory<GenericEvents>	generic_event_category{
	"solid::generic_event_category",
	[](const GenericEvents _evt){
		switch(_evt){
			case GenericEvents::Default:
				return "default";
			case GenericEvents::Start:
				return "start";
			case GenericEvents::Stop:
				return "stop";
			case GenericEvents::Raise:
				return "raise";
			case GenericEvents::Message:
				return "message";
			case GenericEvents::Timer:
				return "timer";
			case GenericEvents::Pause:
				return "pause";
			case GenericEvents::Resume:
				return "resume";
			case GenericEvents::Update:
				return "update";
			case GenericEvents::Kill:
				return "kill";
			default:
				return "unknown";
		}
	}
};

//-----------------------------------------------------------------------------

Event::Event():pcategory_(&generic_event_category), id_(static_cast<size_t>(GenericEvents::Default)){}

//-----------------------------------------------------------------------------

Event::Event(Event &&_uevt):pcategory_(_uevt.pcategory_), id_(_uevt.id_), any_(std::move(_uevt.any_)){
	_uevt.pcategory_ = &generic_event_category;
	_uevt.id_ = static_cast<size_t>(GenericEvents::Default);
}

//-----------------------------------------------------------------------------

Event::Event(const Event &_revt):pcategory_(_revt.pcategory_), id_(_revt.id_), any_(_revt.any_){}

//-----------------------------------------------------------------------------

Event& Event::operator=(const Event &_revt){
	pcategory_ = _revt.pcategory_;
	id_ = _revt.id_;
	any_ = _revt.any_;
	return *this;
}

//-----------------------------------------------------------------------------

Event& Event::operator=(Event&& _uevt){
	pcategory_ = _uevt.pcategory_;
	id_ = _uevt.id_;
	any_ = std::move(_uevt.any_);
	_uevt.pcategory_ = &generic_event_category;
	_uevt.id_ = static_cast<size_t>(GenericEvents::Default);
	return *this;
}

//-----------------------------------------------------------------------------

bool Event::operator==(const Event &_revt)const{
	return (pcategory_ == _revt.pcategory_) and (id_ == _revt.id_);
}

//-----------------------------------------------------------------------------

bool Event::isDefault()const{
	return pcategory_ == &generic_event_category and id_ == static_cast<size_t>(GenericEvents::Default);
}

//-----------------------------------------------------------------------------

void Event::clear(){
	pcategory_ = &generic_event_category;
	id_ = static_cast<size_t>(GenericEvents::Default);
	any_.clear();
}

//-----------------------------------------------------------------------------

std::ostream& Event::print(std::ostream &_ros)const{
	return _ros<<pcategory_->name()<<':'<<':'<<pcategory_->name(*this);
}

//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &_ros, Event const&_re){
	return _re.print(_ros);
}

//-----------------------------------------------------------------------------

}//namespace solid

