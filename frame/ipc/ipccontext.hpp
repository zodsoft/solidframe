// frame/ipc/ipccontext.hpp
//
// Copyright (c) 2015 Valentin Palade (vipalade @ gmail . com) 
//
// This file is part of SolidFrame framework.
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.
//
#ifndef SOLID_FRAME_IPC_IPC_CONTEXT_HPP
#define SOLID_FRAME_IPC_IPC_CONTEXT_HPP

#include "system/socketaddress.hpp"

#include "utility/dynamicpointer.hpp"
#include "boost/any.hpp"

#include <ostream>

#include "frame/common.hpp"

namespace solid{
namespace frame{
namespace ipc{

//! A structure to uniquely indetify an IPC connection pool
/*!
	<b>Overview:</b><br>
	
	<b>Usage:</b><br>
	
*/
struct ConnectionPoolUid: UniqueId{
	ConnectionPoolUid(
		const size_t _idx = InvalidIndex(),
		const uint32 _uid = InvalidIndex()
	):UniqueId(_idx, _uid){}
	
};

struct ConnectionUid{
	
	ConnectionUid(){}
	
	ConnectionUid(const ConnectionUid &_rconuid): poolid(_rconuid.poolid), connectionid(_rconuid.connectionid){}
	
	
	bool isInvalid()const{
		return isInvalidConnection() || isInvalidPool();
	}
	
	bool isInvalidConnection()const{
		return connectionid.isInvalid();
	}
	
	bool isInvalidPool()const{
		return poolid.isInvalid();
	}
	ConnectionPoolUid const& poolId()const{
		return poolid;
	}
	ObjectUidT const& connectionId()const{
		return connectionid;
	}
	
private:
	friend class Service;
	friend class Connection;
	friend class ConnectionContext;
	
	ConnectionUid(
		const ConnectionPoolUid &_rpoolid,
		const ObjectUidT &_rconid
	):poolid(_rpoolid), connectionid(_rconid){}
	
	ConnectionPoolUid	poolid;
	ObjectUidT			connectionid;
};


std::ostream& operator<<(std::ostream &_ros, ConnectionUid const &_con_id);

struct RequestUid{
	uint32	index;
	uint32	unique;
	
	RequestUid(
		const uint32 _idx = InvalidIndex(),
		const uint32 _uid = InvalidIndex()
	):index(_idx), unique(_uid){}
};

std::ostream& operator<<(std::ostream &_ros, RequestUid const &_msguid);

struct MessageUid{
	MessageUid(): index(InvalidIndex()), unique(0){}
	MessageUid(MessageUid const &_rmsguid): index(_rmsguid.index), unique(_rmsguid.unique){}
	
	bool isInvalid()const{
		return index == InvalidIndex();
	}
	
	bool isValid()const{
		return !isInvalid();
	}
private:
	friend class Service;
	friend class Connection;
	friend std::ostream& operator<<(std::ostream &_ros, MessageUid const &_msguid);
	size_t		index;
	uint32		unique;

	MessageUid(
		const size_t _idx,
		const uint32 _uid
	):index(_idx), unique(_uid){}
	
};


std::ostream& operator<<(std::ostream &_ros, MessageUid const &_msguid);

class Service;
class Connection;

struct ConnectionContext{
	Service& service()const{
		return rservice;
	}
	
	ConnectionUid	connectionId()const;
	
	SocketDevice const & device()const;
	
	ulong messageFlags()const{
		return message_flags;
	}
	RequestUid const& requestUid()const{
		return request_uid;
	}
	//! Keep any connection data
	boost::any& any();
private:
	friend class Connection;
	friend class MessageWriter;
	friend struct Message;
	friend class TestEntryway;
	
	Service				&rservice;
	Connection			&rconnection;
	
	ulong				message_flags;
	uint8				message_state;
	RequestUid			request_uid;
	
	
	
	ConnectionContext(
		Service &_rsrv, Connection &_rcon
	):rservice(_rsrv), rconnection(_rcon), message_flags(0), message_state(0){}
	
	ConnectionContext(ConnectionContext const&);
	ConnectionContext& operator=(ConnectionContext const&);
};

}//namespace ipc
}//namespace frame
}//namespace solid
#endif
