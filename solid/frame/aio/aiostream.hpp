// solid/frame/aio/aiostream.hpp
//
// Copyright (c) 2014 Valentin Palade (vipalade @ gmail . com) 
//
// This file is part of SolidFrame framework.
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.
//
#ifndef SOLID_FRAME_AIO_STREAM_HPP
#define SOLID_FRAME_AIO_STREAM_HPP

#include "solid/system/common.hpp"
#include "solid/system/socketdevice.hpp"
#include "aiocompletion.hpp"
#include "aioerror.hpp"
#include "solid/system/debug.hpp"

namespace solid{
namespace frame{
namespace aio{

struct	ObjectProxy;
struct	ReactorContext;

template <class Sock>
class Stream: public CompletionHandler{
	using ThisT = Stream<Sock>;
	using RecvFunctionT = FUNCTION<void(ThisT&, ReactorContext&)>;
	using SendFunctionT = FUNCTION<void(ThisT&, ReactorContext&)>;
	
	static void on_init_completion(CompletionHandler& _rch, ReactorContext &_rctx){
		ThisT &rthis = static_cast<ThisT&>(_rch);
		rthis.completionCallback(&on_completion);
		rthis.addDevice(_rctx, rthis.s.device(), ReactorWaitReadOrWrite);
	}
	
	static void on_completion(CompletionHandler& _rch, ReactorContext &_rctx){
		ThisT &rthis = static_cast<ThisT&>(_rch);
		
		switch(rthis.s.filterReactorEvents(rthis.reactorEvent(_rctx))){
			case ReactorEventNone:
				break;
			case ReactorEventRecv:
				rthis.doRecv(_rctx);
				break;
			case ReactorEventSend:
				rthis.doSend(_rctx);
				break;
			case ReactorEventRecvSend:
				rthis.doRecv(_rctx);
				rthis.doSend(_rctx);
				break;
			case ReactorEventSendRecv:
				rthis.doSend(_rctx);
				rthis.doRecv(_rctx);
				break;
			case ReactorEventHangup:
			case ReactorEventError:
				rthis.doError(_rctx);
				break;
			case ReactorEventClear:
				rthis.doClear(_rctx);
				break;
			default:
				SOLID_ASSERT(false);
		}
	}
	
	//-------------
	static void on_posted_recv_some(ReactorContext &_rctx, Event const&){
		ThisT	&rthis = static_cast<ThisT&>(*completion_handler(_rctx));
		vdbgx(Debug::aio, "");
		rthis.recv_is_posted = false;
		rthis.doRecv(_rctx);
	}
	
	static void on_posted_send_all(ReactorContext &_rctx, Event const&){
		ThisT	&rthis = static_cast<ThisT&>(*completion_handler(_rctx));
		vdbgx(Debug::aio, "");
		rthis.send_is_posted = false;
		rthis.doSend(_rctx);
	}
	
	static void on_dummy(ThisT &_rthis, ReactorContext &_rctx){
		
	}
	
	//-------------
	template <class F>
	struct RecvSomeFunctor{
		F	f;
		
		RecvSomeFunctor(F &_rf):f{std::move(_rf)}{}
		
		void operator()(ThisT &_rthis, ReactorContext &_rctx){
			if(_rthis.doTryRecv(_rctx)){
				const size_t	recv_sz = _rthis.recv_buf_sz;
				F				tmp{std::move(f)};
				_rthis.doClearRecv(_rctx);
				tmp(_rctx, recv_sz);
			}
		}
	};
	
	template <class F>
	struct SendAllFunctor{
		F	f;
		
		SendAllFunctor(F &_rf):f{std::move(_rf)}{}
		
		void operator()(ThisT &_rthis, ReactorContext &_rctx){
			if(_rthis.doTrySend(_rctx)){
				if(_rthis.send_buf_sz == _rthis.send_buf_cp){
					F	tmp{std::move(f)};
					_rthis.doClearSend(_rctx);
					tmp(_rctx);
				}
			}
		}
	};
	
	template <class F>
	struct ConnectFunctor{
		F	f;
		
		ConnectFunctor(F &_rf):f{std::move(_rf)}{}
		
		void operator()(ThisT &_rthis, ReactorContext &_rctx){
			F	tmp{std::move(f)};
			_rthis.doClearSend(_rctx);
			tmp(_rctx);
		}
	};
	
	template <class F>
	struct SecureConnectFunctor{
		F	f;
		
		SecureConnectFunctor(F &_rf):f{std::move(_rf)}{}
		
		void operator()(ThisT &_rthis, ReactorContext &_rctx){
			_rthis.errorClear(_rctx);
			bool		can_retry;
			ErrorCodeT	err;
			if(_rthis.s.secureConnect(&_rctx, can_retry, err)){
			}else if(can_retry){
				return;
			}else{
				_rthis.error(_rctx, error_stream_system);
				_rthis.systemError(_rctx, err);
				SOLID_ASSERT(err);
			}
			F	tmp{std::move(f)};
			_rthis.doClearSend(_rctx);
			tmp(_rctx);
		}
	};
	
	template <class F>
	struct SecureAcceptFunctor{
		F	f;
		
		SecureAcceptFunctor(F &_rf):f{std::move(_rf)}{}
		
		void operator()(ThisT &_rthis, ReactorContext &_rctx){
			_rthis.errorClear(_rctx);
			bool		can_retry;
			ErrorCodeT	err;
			if(_rthis.s.secureAccept(&_rctx, can_retry, err)){
			}else if(can_retry){
				return;
			}else{
				_rthis.error(_rctx, error_stream_system);
				_rthis.systemError(_rctx, err);
				SOLID_ASSERT(err);
			}
			F	tmp{std::move(f)};
			_rthis.doClearRecv(_rctx);
			tmp(_rctx);
		}
	};
	
	template <class F>
	struct SecureShutdownFunctor{
		F	f;
		
		SecureShutdownFunctor(F &_rf):f{std::move(_rf)}{}
		
		void operator()(ThisT &_rthis, ReactorContext &_rctx){
			_rthis.errorClear(_rctx);
			bool		can_retry;
			ErrorCodeT	err;
			if(_rthis.s.secureShutdown(&_rctx, can_retry, err)){
			}else if(can_retry){
				return;
			}else{
				_rthis.error(_rctx, error_stream_system);
				_rthis.systemError(_rctx, err);
				SOLID_ASSERT(err);
			}
			
			F	tmp{std::move(f)};
			_rthis.doClearSend(_rctx);
			tmp(_rctx);
		}
	};
	
	template <class F>
	struct SecureVerifyFunctor{
		F f;
		
 		SecureVerifyFunctor(F &_rf):f{std::move(_rf)}{}
		
		bool operator()(void *_pctx, bool _preverified, typename Sock::VerifyContextT &_rverify_ctx){
			return f(*static_cast<ReactorContext*>(_pctx), _preverified, _rverify_ctx);
		}
	};
	
public:
	explicit Stream(
		ObjectProxy const &_robj, SocketDevice &&_rsd
	):	CompletionHandler(_robj, on_init_completion), s(std::move(_rsd)),
		recv_buf(nullptr), recv_buf_sz(0), recv_buf_cp(0), recv_is_posted(false),
		send_buf(nullptr), send_buf_sz(0), send_buf_cp(0), send_is_posted(false)
	{}
	
	template <class Ctx>
	Stream(
		ObjectProxy const &_robj, SocketDevice &&_rsd, const Ctx &_rctx
	):	CompletionHandler(_robj, on_init_completion), s(_rctx, std::move(_rsd)),
		recv_buf(nullptr), recv_buf_sz(0), recv_buf_cp(0), recv_is_posted(false),
		send_buf(nullptr), send_buf_sz(0), send_buf_cp(0), send_is_posted(false)
	{}
	
	Stream(
		ObjectProxy const &_robj
	):	CompletionHandler(_robj, on_dummy_completion),
		recv_buf(nullptr), recv_buf_sz(0), recv_buf_cp(0), recv_is_posted(false),
		send_buf(nullptr), send_buf_sz(0), send_buf_cp(0), send_is_posted(false)
	{}
	
	template <class Ctx>
	explicit Stream(
		ObjectProxy const &_robj, const Ctx &_rctx
	):	CompletionHandler(_robj, on_dummy_completion), s(_rctx),
		recv_buf(nullptr), recv_buf_sz(0), recv_buf_cp(0), recv_is_posted(false),
		send_buf(nullptr), send_buf_sz(0), send_buf_cp(0), send_is_posted(false)
	{}
	
	~Stream(){
		//MUST call here and not in the ~CompletionHandler
		this->deactivate();
	}
	
	bool hasPendingRecv()const{
		return !FUNCTION_EMPTY(recv_fnc);
	}
	
	bool hasPendingSend()const{
		return !FUNCTION_EMPTY(send_fnc);
	}
	SocketDevice& device(){
		return s.device();
	}
	SocketDevice const & device()const{
		return s.device();
	}
	
	Sock& socket(){
		return s;
	}
	
	SocketDevice reset(ReactorContext &_rctx, SocketDevice &&_rnewdev = std::move(dummy_socket_device())){
		if(s.device().ok()){
			remDevice(_rctx, s.device());
		}
		SocketDevice sd(s.reset(std::move(_rnewdev)));
		if(s.device().ok()){
			completionCallback(&on_completion);
			addDevice(_rctx, s.device(), ReactorWaitReadOrWrite);
		}
		return sd;
	} 
	
	template <typename F>
	bool postRecvSome(ReactorContext &_rctx, char *_buf, size_t _bufcp, F _f){
		if(FUNCTION_EMPTY(recv_fnc)){
			recv_fnc = RecvSomeFunctor<F>(_f);
			recv_buf = _buf;
			recv_buf_cp = _bufcp;
			recv_buf_sz = 0;
			recv_is_posted = true;
			doPostRecvSome(_rctx);
			errorClear(_rctx);
			return false;
		}else{
			error(_rctx, error_already);
			return true;
		}
	}
	
	template <typename F>
	bool recvSome(ReactorContext &_rctx, char *_buf, size_t _bufcp, F _f, size_t &_sz){
		if(FUNCTION_EMPTY(recv_fnc)){
			recv_buf = _buf;
			recv_buf_cp = _bufcp;
			recv_buf_sz = 0;
			
			if(doTryRecv(_rctx)){
				_sz = recv_buf_sz;
				return true;
			}else{
				recv_fnc = RecvSomeFunctor<F>(_f);
				return false;
			}
			
		}else{
			error(_rctx, error_already);
		}
		return true;
	}
	
	
	template <typename F>
	bool postSendAll(ReactorContext &_rctx, const char *_buf, size_t _bufcp, F _f){
		if(FUNCTION_EMPTY(send_fnc)){
			send_fnc = SendAllFunctor<F>(_f);
			send_buf = _buf;
			send_buf_cp = _bufcp;
			send_buf_sz = 0;
			send_is_posted = true;
			doPostSendAll(_rctx);
			errorClear(_rctx);
			return false;
		}else{
			error(_rctx, error_already);
			SOLID_ASSERT(false);
			return true;
		}
	}
	
	template <typename F>
	bool sendAll(ReactorContext &_rctx, char *_buf, size_t _bufcp, F _f){
		if(FUNCTION_EMPTY(send_fnc)){
			send_buf = _buf;
			send_buf_cp = _bufcp;
			send_buf_sz = 0;
			
			if(doTrySend(_rctx)){
				if(send_buf_sz == send_buf_cp){
					return true;
				}
			}
			send_fnc = SendAllFunctor<F>(_f);
			return false;
		}else{
			error(_rctx, error_already);
		}
		return true;
	}
	
	template <typename F>
	bool connect(ReactorContext &_rctx, SocketAddressStub const &_rsas, F _f){
		if(FUNCTION_EMPTY(send_fnc)){
			errorClear(_rctx);
			ErrorCodeT	err;
			if(s.create(_rsas, err)){
				completionCallback(&on_completion);
				addDevice(_rctx, s.device(), ReactorWaitReadOrWrite);
				
				bool		can_retry;
				bool		rv = s.connect(_rsas, can_retry, err);
				if(rv){
					
				}else if(can_retry){
					send_fnc = ConnectFunctor<F>(_f);
					return false;
				}else{
					error(_rctx, error_stream_system);
					systemError(_rctx, err);
					SOLID_ASSERT(err);
				}
			}else{
				error(_rctx, error_stream_system);
				systemError(_rctx, err);
				SOLID_ASSERT(err);
			}
			
		}else{
			error(_rctx, error_already);
		}
		return true;
	}
	
	void shutdown(ReactorContext &/*_rctx*/){
		s.shutdown();
	}
	
	template <typename F>
	bool secureConnect(ReactorContext &_rctx, F _f){
		if(FUNCTION_EMPTY(send_fnc)){
			errorClear(_rctx);
			bool		can_retry;
			ErrorCodeT	err;
			if(s.secureConnect(&_rctx, can_retry, err)){
			}else if(can_retry){
				send_fnc = SecureConnectFunctor<F>(_f);
				return false;
			}else{
				error(_rctx, error_stream_system);
				systemError(_rctx, err);
				SOLID_ASSERT(err);
			}
		}
		return true;
	}
	
	template <typename F>
	bool secureAccept(ReactorContext &_rctx, F _f){
		if(FUNCTION_EMPTY(recv_fnc)){
			errorClear(_rctx);
			bool		can_retry;
			ErrorCodeT	err;
			if(s.secureAccept(&_rctx, can_retry, err)){
			}else if(can_retry){
				recv_fnc = SecureAcceptFunctor<F>(_f);
				return false;
			}else{
				error(_rctx, error_stream_system);
				systemError(_rctx, err);
				SOLID_ASSERT(err);
			}
		}
		return true;
	}
	
	template <typename F>
	bool secureShutdown(ReactorContext &_rctx, F _f){
		if(FUNCTION_EMPTY(send_fnc)){
			errorClear(_rctx);
			bool		can_retry;
			ErrorCodeT	err;
			if(s.secureShutdown(&_rctx, can_retry, err)){
			}else if(can_retry){
				send_fnc = SecureShutdownFunctor<F>(_f);
				return false;
			}else{
				error(_rctx, error_stream_system);
				systemError(_rctx, err);
				SOLID_ASSERT(err);
			}
		}
		return true;
	}
	
	void secureRenegotiate(ReactorContext &_rctx){
		ErrorCodeT	err = s.renegotiate();
		if(err){
			error(_rctx, error_stream_system);
			systemError(_rctx, err);
			SOLID_ASSERT(err);
		}
	}
	
	template <typename F>
	void secureSetVerifyCallback(ReactorContext &_rctx, typename Sock::VerifyMaskT _mode, F _f){
		SecureVerifyFunctor<F>	fnc_wrap(_f);
		ErrorCodeT	err = s.setVerifyCallback(_mode, fnc_wrap);
		if(err){
			error(_rctx, error_stream_system);
			systemError(_rctx, err);
			SOLID_ASSERT(err);
		}
	}
	
	void secureSetVerifyDepth(ReactorContext &_rctx, const int _depth){
		ErrorCodeT	err = s.setVerifyDepth(_depth);
		if(err){
			error(_rctx, error_stream_system);
			systemError(_rctx, err);
			SOLID_ASSERT(err);
		}
	}
	
	void secureSetVerifyMode(ReactorContext &_rctx, typename Sock::VerifyMaskT _mode){
		ErrorCodeT	err = s.setVerifyMode(_mode);
		if(err){
			error(_rctx, error_stream_system);
			systemError(_rctx, err);
			SOLID_ASSERT(err);
		}
	}
	
	void secureSetCheckHostName(ReactorContext &_rctx, const std::string &_val){
		ErrorCodeT	err = s.setCheckHostName(_val);
		if(err){
			error(_rctx, error_stream_system);
			systemError(_rctx, err);
			SOLID_ASSERT(err);
		}
	}
	
private:
	void doPostRecvSome(ReactorContext &_rctx){
		reactor(_rctx).post(_rctx, on_posted_recv_some, Event(), *this);
	}
	void doPostSendAll(ReactorContext &_rctx){
		reactor(_rctx).post(_rctx, on_posted_send_all, Event(), *this);
	}
	
	
	void doRecv(ReactorContext &_rctx){
		if(!recv_is_posted and !FUNCTION_EMPTY(recv_fnc)){
			vdbgx(Debug::aio, "");
			errorClear(_rctx);
			recv_fnc(*this, _rctx);
		}
	}
	
	void doSend(ReactorContext &_rctx){
		if(!send_is_posted && !FUNCTION_EMPTY(send_fnc)){
			errorClear(_rctx);
			send_fnc(*this, _rctx);
		}
	}
	
	bool doTryRecv(ReactorContext &_rctx){
		bool		can_retry;
		ErrorCodeT	err;
		
		int			rv = s.recv(&_rctx, recv_buf, recv_buf_cp - recv_buf_sz, can_retry, err);
		
		vdbgx(Debug::aio, "recv ("<<(recv_buf_cp - recv_buf_sz)<<") = "<<rv);
		
		if(rv > 0){
			recv_buf_sz += rv;
			recv_buf += rv;
		}else if(rv == 0){
			error(_rctx, error_stream_shutdown);
			recv_buf_sz = recv_buf_cp = 0;
		}else if(rv < 0){
			if(can_retry){
				return false;
			}else{
				recv_buf_sz = recv_buf_cp = 0;
				error(_rctx, error_stream_system);
				systemError(_rctx, err);
				SOLID_ASSERT(err);
			}
		}
		return true;
	}
	
	bool doTrySend(ReactorContext &_rctx){
		bool		can_retry;
		ErrorCodeT	err;
		int			rv = s.send(&_rctx, send_buf, send_buf_cp - send_buf_sz, can_retry, err);
		
		vdbgx(Debug::aio, "send ("<<(send_buf_cp - send_buf_sz)<<") = "<<rv);
		
		if(rv > 0){
			send_buf_sz += rv;
			send_buf += rv;
		}else if(rv == 0){
			error(_rctx, error_stream_shutdown);
			send_buf_sz = send_buf_cp = 0;
		}else if(rv < 0){
			if(can_retry){
				return false;
			}else{
				send_buf_sz = send_buf_cp = 0;
				error(_rctx, error_stream_system);
				systemError(_rctx, err);
				SOLID_ASSERT(err);
			}
		}
		return true;
	}
	
	void doError(ReactorContext &_rctx){
		error(_rctx, error_stream_socket);
		systemError(_rctx, s.device().lastError());
		
		if(!FUNCTION_EMPTY(send_fnc)){
			send_buf_sz = send_buf_cp = 0;
			send_fnc(*this, _rctx);
		}
		if(!FUNCTION_EMPTY(recv_fnc)){
			recv_buf_sz = recv_buf_cp = 0;
			recv_fnc(*this, _rctx);
		}
	}
	
	void doClearRecv(ReactorContext &_rctx){
		FUNCTION_CLEAR(recv_fnc);
		SOLID_ASSERT(FUNCTION_EMPTY(recv_fnc));
		recv_buf = nullptr;
		recv_buf_sz = recv_buf_cp = 0;
	}
	
	void doClearSend(ReactorContext &_rctx){
		FUNCTION_CLEAR(send_fnc);
		SOLID_ASSERT(FUNCTION_EMPTY(send_fnc));
		send_buf = nullptr;
		send_buf_sz = send_buf_cp = 0;
	}
	
	void doClear(ReactorContext &_rctx){
		doClearRecv(_rctx);
		doClearSend(_rctx);
		remDevice(_rctx, s.device());
		recv_fnc = &on_dummy;//we prevent new send/recv calls
		send_fnc = &on_dummy;
	}
private:
	
	Sock			s;
	
	char 			*recv_buf;
	size_t			recv_buf_sz;
	size_t			recv_buf_cp;
	RecvFunctionT	recv_fnc;
	bool			recv_is_posted;
	
	const char		*send_buf;
	size_t			send_buf_sz;
	size_t			send_buf_cp;
	SendFunctionT	send_fnc;
	bool			send_is_posted;
};

}//namespace aio
}//namespace frame
}//namespace solid


#endif
