// solid/frame/aio/openssl/aiosecurecontext.hpp
//
// Copyright (c) 2015 Valentin Palade (vipalade @ gmail . com) 
//
// This file is part of SolidFrame framework.
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.
//
#ifndef SOLID_FRAME_AIO_OPENSSL_SECURE_CONTEXT_HPP
#define SOLID_FRAME_AIO_OPENSSL_SECURE_CONTEXT_HPP

#include "openssl/ssl.h"
#include "solid/system/error.hpp"
#include "solid/system/function.hpp"

namespace solid{
namespace frame{
namespace aio{
namespace openssl{

enum struct PasswordPurpose{
	Read,
	Write
};

class Socket;
class Context{
public:
	using NativeContextT = SSL_CTX*;
	static Context create(const SSL_METHOD* = nullptr);
	
	Context();
	
	
	Context(Context const&) = delete;
	
	Context(Context && _rctx);
	
	Context& operator=(Context const&) = delete;
	
	Context& operator=(Context && _rctx);
	
	~Context();
	
	bool isValid()const;
	
	bool empty()const;
	
	//ErrorCodeT configure(const char *_filename = nullptr, const char *_appname = nullptr);
	
	
	ErrorCodeT loadDefaultVerifyPaths();
	
	//!Use it on client side to load the certificates
	ErrorCodeT loadVerifyFile(const char *_path);
	//!Use it on client side to load the certificates
	ErrorCodeT loadVerifyPath(const char *_path);
	
	//!Use it on server side to load the certificates
	ErrorCodeT loadCertificateFile(const char *_path);
	//!Use it on server side to load the certificates
	ErrorCodeT loadPrivateKeyFile(const char *_path);
	
	template <typename F>
	ErrorCodeT passwordCallback(F _f){
		pwdfnc = _f;
		return doSetPasswordCallback();
	}
	
	NativeContextT nativeContext()const{
		return pctx;
	}
private:
	static int on_password_cb(char *buf, int size, int rwflag, void *u);
	ErrorCodeT doSetPasswordCallback();
private:
	using PasswordFunctionT = FUNCTION<std::string (std::size_t, PasswordPurpose)>;
	
	friend class Socket;
	SSL_CTX				*pctx;
	PasswordFunctionT	pwdfnc;
};

}//namespace openssl
}//namespace aio
}//namespace frame
}//namespace solid


#endif
