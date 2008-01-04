/* Implementation file alphaconnection.cpp
	
	Copyright 2007, 2008 Valentin Palade 
	vipalade@gmail.com

	This file is part of SolidGround framework.

	SolidGround is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SolidGround.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "system/debug.h"
#include "system/timespec.h"

#include "utility/ostream.h"
#include "utility/istream.h"

#include "clientserver/tcp/channel.h"
#include "clientserver/ipc/ipcservice.h"

#include "core/server.h"
#include "core/command.h"

#include "alpha/alphaservice.h"

#include "alphaconnection.h"
#include "alphacommand.h"
#include "alphaprotocolfilters.h"

namespace cs=clientserver;
static char	*hellostr = "Welcome to alpha service!!!\r\n"; 
static char *sigstr = "Signaled!!!\r\n";

namespace test{
namespace alpha{

void Connection::initStatic(Server &_rs){
	Command::initStatic(_rs);
}

Connection::Connection(cs::tcp::Channel *_pch, SocketAddress *_paddr):
						 	BaseTp(_pch),
						 	wtr(*_pch),
						 	rdr(*_pch, wtr),
						 	paddr(_paddr){
	
	if(paddr){
		state(Connect);
	}else{
		state(Init);
	}
	
}

/*
NOTE: 
* Releasing the connection here and not in a base class destructor because
here we know the exact type of the object - so the service can do other things 
based on the type.
* Also it ensures a proper/safe visiting. Suppose the unregister would have taken 
place in a base destructor. If the visited object is a leaf, one may visit
destroyed data.
NOTE: 
* Visitable data must be destroyed after releasing the connection!!!
*/

Connection::~Connection(){
	idbg("destroy connection id "<<this->id()<<" pcmd "<<pcmd);
	delete pcmd; pcmd = NULL;
	test::Server &rs = test::Server::the();
	rs.service(*this).removeConnection(*this);
}
int Connection::execute(ulong _sig, TimeSpec &_tout){
	_tout.set(2400);//allways set it if it's not MAXTIMEOUT
	if(_sig & (cs::TIMEOUT | cs::ERRDONE)){
		if(state() == ConnectTout){
			state(Connect);
			return cs::UNREGISTER;
		}else 
			return BAD;
	}
	if(signaled()){
		test::Server &rs = test::Server::the();
		ulong sm(0);
		{
		Mutex::Locker	lock(rs.mutex(*this));
		sm = grabSignalMask(0);
		if(sm & cs::S_KILL) return BAD;
		if(sm & cs::S_CMD){
			grabCommands();
			channel().send(sigstr, strlen(sigstr));
		}
		}
		if(sm & cs::S_CMD){
			switch(execCommands(*this)){
				case BAD: 
					return BAD;
				case OK: //expected command received
					_sig |= cs::OKDONE;
				case NOK://unexpected command received
					break;
			}
		}
		if((sm & cs::S_ERR)){
			//NOTE: was I really expecting a commnad?
			if(pcmd && !pcmd->receiveError(true)){
				_sig |= cs::OKDONE;
			}
		}
		//now we determine if we return with NOK or we continue
		if(!_sig) return NOK;
	}
	if(_sig & cs::INDONE){
		assert(state() == ParseTout);
		state(Parse);
		//rcmdparser.doneRecv();
	}
	if(_sig & cs::OUTDONE){
		switch(state()){
			case IdleExecute:
			case Execute:
				break;
			case Connect:
				state(Init);
				break;
			default:
				state(Parse);
		}
		//rcmdresponder.doneSend();
	}
	int rc;
	switch(state()){
		case Init:{
			//channel().send(hellostr, strlen(hellostr));
			test::Server	&rs = test::Server::the();
			uint32			myport(rs.ipc().basePort());//rs.ipcService().myProcessId());
			uint32			objid(this->id());
			uint32			objuid(rs.uid(*this));
			writer()<<"* Hello from alpha server ("<<myport<<" "<<objid<<" "<<objuid<<")\r\n";
			writer().push(&Writer::flushAll);
			state(Execute);
			}break;
		case ParsePrepare:
			idbg("PrepareReader");
			prepareReader();
		case Parse:
			idbg("Parse");
			state(Parse);
			switch((rc = reader().run())){
				case OK: break;
				case NOK:
					state(ParseTout);
					return NOK;
				case BAD:return BAD;
			}
			if(reader().isError()){
				delete pcmd; pcmd = NULL;
				state(Execute);
				writer().push(Writer::putStatus);
				break;
			}
		case ExecutePrepare:
			idbg("PrepareExecute");
			pcmd->execute(*this);
			state(Execute);
		case IdleExecute:
			idbg("IdleExecute");
		case Execute:
			idbg("Execute");
			switch((rc = writer().run())){
				case NOK:
// 					if(state() != IdleExecute){
// 						state(ExecuteTout);
// 					}else{
// 					}
					return NOK;
				case OK:
					if(state() != IdleExecute){
						delete pcmd; pcmd = NULL;
						state(ParsePrepare); rc = OK;
					}else{
						state(Parse); rc = OK;
					}
				default:
					idbg("rc = "<<rc);
					return rc;
			}
			break;
		case Connect:
			/*
			switch(channel().connect(*paddr)){
				case BAD: return BAD;
				case OK:  state(Init);break;
				case NOK: state(ConnectTout); return REGISTER;
			};*/
			break;
		case ConnectTout:
			state(Init);
			//delete(paddr); paddr = NULL;
			break;
		case ParseTout:
			idbg("State: ParseTout");
			return NOK;
	}
	return OK;
}

int Connection::execute(){
	return BAD;
}

void Connection::prepareReader(){
	writer().clear();
	reader().clear();
	reader().push(&Reader::checkChar, protocol::Parameter('\n'));
	reader().push(&Reader::checkChar, protocol::Parameter('\r'));
	reader().push(&Reader::fetchKey<Reader, Connection, AtomFilter, Command>, protocol::Parameter(this, 64));
	reader().push(&Reader::checkChar, protocol::Parameter(' '));
	reader().push(&Reader::fetchFilteredString<TagFilter>, protocol::Parameter(&writer().tag(),64));
}

// int Command::execute(Listener &){
// 	assert(false);
// 	return BAD;
// }

int Connection::receiveIStream(
	StreamPtr<IStream> &_ps,
	const FromPairTp&_from,
	const clientserver::ipc::ConnectorUid *_conid
){
	if(pcmd && pcmd->receiveIStream(_ps, _from, _conid) == OK)
		state(IdleExecute);
	return OK;
}

int Connection::receiveOStream(
	StreamPtr<OStream> &_ps,
	const FromPairTp&_from,
	const clientserver::ipc::ConnectorUid *_conid
){
	if(pcmd) pcmd->receiveOStream(_ps, _from, _conid);
	return OK;
}

int Connection::receiveIOStream(
	StreamPtr<IOStream> &_ps, 
	const FromPairTp&_from,
	const clientserver::ipc::ConnectorUid *_conid
){
	if(pcmd) pcmd->receiveIOStream(_ps, _from, _conid);
	return OK;
}

int Connection::receiveString(
	const String &_str, 
	const FromPairTp&_from,
	const clientserver::ipc::ConnectorUid *_conid
){
	if(pcmd && pcmd->receiveString(_str, _from, _conid) == OK)
		state(IdleExecute);
	return OK;
}

int Connection::accept(cs::Visitor &_rov){
	//static_cast<TestInspector&>(_roi).inspectConnection(*this);
	return 0;
}

// int Connection::sendStatusResponse(cmd::Responder &_rr, int _opt){
//     //hassert_abort(!cmdparser.tag().empty());
//     if(_opt & 1){
//         _rr<<rcmdparser.tag()<<rcmdparser.errmsg;
//         rcmdparser.clearTag();
// 	}else{
//         _rr<<'*'<<' '<<rcmdparser.errmsg;
//     }
//     if(_opt & 2) _rr<<cmd::crlf;
//     return OK;
// }

}//namespace alpha
}//namespace test
