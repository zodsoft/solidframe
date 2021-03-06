// solid/frame/aio/aioresolver.hpp
//
// Copyright (c) 2014 Valentin Palade (vipalade @ gmail . com) 
//
// This file is part of SolidFrame framework.
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.
//
#ifndef SOLID_FRAME_AIO_RESOLVER_HPP
#define SOLID_FRAME_AIO_RESOLVER_HPP

#include "solid/utility/dynamictype.hpp"
#include "solid/system/function.hpp"
#include "solid/system/socketaddress.hpp"
#include "aioerror.hpp"
#include <string>

namespace solid{
namespace frame{
namespace aio{

struct ResolveBase: Dynamic<ResolveBase>{
	
	ErrorCodeT		error;
	int 			flags;
	
	ResolveBase(int _flags):flags(_flags){}
	
	virtual ~ResolveBase();
	virtual void run(bool _fail) = 0;
};

struct DirectResolve: ResolveBase{
	
	std::string		host;
	std::string		srvc;
	int				family;
	int				type;
	int				proto;
	
	DirectResolve(
		const char *_host, 
		const char *_srvc, 
		int _flags,
		int _family,
		int _type,
		int _proto
	):ResolveBase(_flags), host(_host), srvc(_srvc), family(_family), type(_type), proto(_proto){}
	
	ResolveData doRun();
};

struct ReverseResolve: ResolveBase{
	
	SocketAddressInet	addr;
	
	ReverseResolve(
		const SocketAddressStub &_rsa,
		int _flags
	):ResolveBase(_flags), addr(_rsa){}
	
	void doRun(std::string &_rhost, std::string &_rsrvc);
};

template <class Cbk>
struct DirectResolveCbk: DirectResolve{
	
	Cbk		cbk;
	
	DirectResolveCbk(
		Cbk	_cbk,
		const char *_host, 
		const char *_srvc, 
		int _flags,
		int _family,
		int _type,
		int _proto
	):DirectResolve(_host, _srvc, _flags, _family, _type, _proto), cbk(_cbk){
	}
	
	/*virtual*/ void run(bool _fail){
		
		ResolveData rd;
		
		if(!_fail){
			rd = this->doRun();
		}else{
			this->error = error_resolver_direct;
		}
		cbk(rd, this->error);
	}
};

template <class Cbk>
struct ReverseResolveCbk: ReverseResolve{
	
	Cbk		cbk;
	
	ReverseResolveCbk(
		Cbk	_cbk,
		const SocketAddressStub &_rsa,
		int _flags
	):ReverseResolve(_rsa, _flags), cbk(_cbk){}
	
	/*virtual*/ void run(bool _fail){
		if(!_fail){
			this->doRun(cbk.hostString(), cbk.serviceString());
		}else{
			this->error = error_resolver_reverse;
		}
		cbk(this->error);
	}
};

class Resolver{
public:
	Resolver(size_t _thrcnt = 0);
	~Resolver();
	
	ErrorConditionT start(ushort _thrcnt = 0);
	
	template <class Cbk>
	void requestResolve(
		Cbk _cbk,
		const char *_host, 
		const char *_srvc, 
		int _flags = 0,
		int _family = -1,
		int _type = -1,
		int _proto = -1
	){
		doSchedule(new DirectResolveCbk<Cbk>(_cbk, _host, _srvc, _flags, _family, _type, _proto));
	}
	
	template <class Cbk>
	void requestResolve(
		Cbk _cbk,
		const SocketAddressStub &_rsa,
		int _flags = 0
	){
		doSchedule(new ReverseResolveCbk<Cbk>(_rsa, _flags));
	}
	
	void stop();
private:
	void doSchedule(ResolveBase *_pb);
private:
	struct Data;
	Data	&d;
};


}//namespace aio
}//namespace frame
}//namespace solid



#endif
