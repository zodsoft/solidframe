#include "clientserver/core/commandexecuter.h"
#include "clientserver/core/command.h"
#include "clientserver/core/server.h"
#include <deque>
#include <cassert>
#include "system/mutex.h"
#include "system/debug.h"
#include "system/timespec.h"
#include "utility/stack.h"
#include "utility/queue.h"


namespace clientserver{


struct CommandExecuter::Data{
	Data():pm(NULL), extsz(0), sz(0){}
	enum{
		Running, Stopping
	};
	void push(CmdPtr<Command> &_cmd);
	void eraseToutPos(uint32 _pos);
	struct CmdData{
		CmdData(const CmdPtr<Command>& _rcmd):cmd(_rcmd), toutidx(-1), uid(0),tout(0xffffffff){}
		CmdData(){}
		CmdPtr<Command>	cmd;
		int32			toutidx;
		uint32			uid;
		TimeSpec		tout;
	};
	typedef std::deque<int32>					TimeoutVectorTp;
	typedef std::deque<CmdData >				CommandDequeTp;
	typedef std::deque<CmdPtr<Command> >		CommandExtDequeTp;
	typedef Queue<uint32>						ExecQueueTp;
	typedef Stack<uint32>						FreeStackTp;
	Mutex 				*pm;
	uint32				extsz;
	uint32				sz;
	CommandDequeTp		cdq;
	CommandExtDequeTp	cedq;
	FreeStackTp			fs;
	FreeStackTp			fs2;
	ExecQueueTp			sq;
	ExecQueueTp			eq;
	TimeoutVectorTp		toutv;
	TimeSpec			tout;
};

void CommandExecuter::Data::push(CmdPtr<Command> &_cmd){
	if(fs.size()){
		cdq[fs.top()].cmd = _cmd;
		sq.push(fs.top());
		fs.pop();
	}else{
		cedq.push_back(_cmd);
		++extsz;
		sq.push(cdq.size() + extsz - 1);
	}
}

void CommandExecuter::Data::eraseToutPos(uint32 _pos){
	assert(_pos < toutv.size());
	toutv[_pos] = toutv.back();
	toutv.pop_back();
}

CommandExecuter::CommandExecuter():d(*(new Data)){
	state(Data::Running);
}

CommandExecuter::~CommandExecuter(){
	delete &d;
}

int CommandExecuter::signal(CmdPtr<Command> &_cmd){
	assert(d.pm->locked());
	if(this->state() != Data::Running){
		_cmd.clear();
		return 0;//no reason to raise the pool thread!!
	}
	d.push(_cmd);
	return Object::signal(S_CMD | S_RAISE);
}

int CommandExecuter::execute(ulong _evs, TimeSpec &_rtout){
	d.pm->lock();
	idbg("d.extsz = "<<d.extsz);
	if(d.extsz){
		for(Data::CommandExtDequeTp::const_iterator it(d.cedq.begin()); it != d.cedq.end(); ++it){
			d.cdq.push_back(Data::CmdData(*it));
		}
		d.sz += d.cedq.size();
		d.cedq.clear();
		d.extsz = 0;
	}
	if(signaled()){
		ulong sm = grabSignalMask(0);
		idbg("signalmask "<<sm);
		if(sm & S_KILL){
			state(Data::Stopping);
			if(!d.sz){//no command
				state(-1);
				d.pm->unlock();
				removeFromServer();
				idbg("~CommandExecuter");
				return BAD;
			}
		}
		if(sm & S_CMD){
			while(d.sq.size()){
				d.eq.push(d.sq.front());
				d.sq.pop();
			}
		}
	}
	d.pm->unlock();
	if(state() == Data::Running){
		idbg("d.eq.size = "<<d.eq.size());
		while(d.eq.size()){
			uint32 pos = d.eq.front();
			d.eq.pop();
			Data::CmdData &rcp(d.cdq[pos]);
			TimeSpec ts(_rtout);
			switch(rcp.cmd->execute(*this, CommandUidTp(pos, rcp.uid), ts)){
				case BAD: 
					++rcp.uid;
					rcp.cmd.clear();
					d.fs2.push(pos);
					if(rcp.toutidx >= 0){
						d.eraseToutPos(rcp.toutidx);
						rcp.toutidx = -1;
					}
					break;
				case OK:
					d.eq.push(pos);
					if(rcp.toutidx >= 0){
						d.eraseToutPos(rcp.toutidx);
						rcp.toutidx = -1;
					}
					break;
				case NOK:
					if(ts != _rtout){
						rcp.tout = ts;
						if(d.tout > ts){
							d.tout = ts;
						}
						if(rcp.toutidx < 0){
							d.toutv.push_back(pos);
							rcp.toutidx = d.toutv.size() - 1;
						}
					}else{
						//++rcp.uid;
						//rcp.cmd.clear();
						//d.fs2.push(pos);
						rcp.tout.set(0xffffffff);
						if(rcp.toutidx >= 0){
							d.eraseToutPos(rcp.toutidx);
							rcp.toutidx = -1;
						}
					}
					break;
				case LEAVE:
					++rcp.uid;
					rcp.cmd.release();
					d.fs2.push(pos);
					if(rcp.toutidx >= 0){
						d.eraseToutPos(rcp.toutidx);
						rcp.toutidx = -1;
					}
					break;
			}
		}
		if((_evs & TIMEOUT) && _rtout <= d.tout){
			TimeSpec tout(0xffffffff);
			for(Data::TimeoutVectorTp::const_iterator it(d.toutv.begin()); it != d.toutv.end();){
				Data::CmdData &rcp(d.cdq[*it]);
				if(_rtout >= rcp.tout){
					++rcp.uid;
					rcp.cmd.clear();
					d.fs2.push(*it);
					if(rcp.toutidx >= 0){
						d.eraseToutPos(rcp.toutidx);
						rcp.toutidx = -1;
					}
				}
			}
		}
	}else{
		for(Data::CommandDequeTp::iterator it(d.cdq.begin()); it != d.cdq.end(); ++it){
			if(it->cmd.ptr()){
				//TODO: should you use clear?!
				delete it->cmd.release();
			}
		}
		idbg("~CommandExecuter");
		removeFromServer();
		state(-1);
		return BAD;
	}
	if(d.fs2.size()){
		d.pm->lock();
		d.sz -= d.fs2.size();
		while(d.fs2.size()){
			d.fs.push(d.fs2.top());
			d.fs2.pop();
		}
		d.pm->unlock();
	}
	return NOK;
}

void CommandExecuter::mutex(Mutex *_pmut){
	d.pm = _pmut;
}

int CommandExecuter::execute(){
	assert(false);
}

void CommandExecuter::receiveCommand(
	CmdPtr<Command> &_rcmd,
	const RequestUidTp &_requid,
	int			_which,
	const ObjectUidTp& _from,
	const ipc::ConnectorUid *_conid
){
	idbg("_requid.first = "<<_requid.first<<" _requid.second = "<<_requid.second<<" uid = "<<d.cdq[_requid.first].uid);
	if(_requid.first < d.cdq.size() && d.cdq[_requid.first].uid == _requid.second){
		idbg("");
		if(d.cdq[_requid.first].cmd->receiveCommand(_rcmd, _which, _from, _conid) == OK){
			idbg("");
			d.eq.push(_requid.first);
		}
	}
}

void CommandExecuter::receiveIStream(
	StreamPtr<IStream> &_sp,
	const FileUidTp	& _fuid,
	const RequestUidTp &_requid,
	int			_which,
	const ObjectUidTp&_from,
	const ipc::ConnectorUid *_conid
){
	idbg("_requid.first = "<<_requid.first<<" _requid.second = "<<_requid.second<<" uid = "<<d.cdq[_requid.first].uid);
	if(_requid.first < d.cdq.size() && d.cdq[_requid.first].uid == _requid.second){
		idbg("");
		if(d.cdq[_requid.first].cmd->receiveIStream(_sp, _fuid, _which, _from, _conid) == OK){
			d.eq.push(_requid.first);
			idbg("");
		}
	}
}
void CommandExecuter::receiveOStream(
	StreamPtr<OStream> &_sp,
	const FileUidTp	&_fuid,
	const RequestUidTp &_requid,
	int			_which,
	const ObjectUidTp&_from,
	const ipc::ConnectorUid *_conid
){
	if(_requid.first < d.cdq.size() && d.cdq[_requid.first].uid == _requid.second){
		if(d.cdq[_requid.first].cmd->receiveOStream(_sp, _fuid, _which, _from, _conid) == OK){
			d.eq.push(_requid.first);
		}
	}
}
void CommandExecuter::receiveIOStream(
	StreamPtr<IOStream> &_sp,
	const FileUidTp	&_fuid,
	const RequestUidTp &_requid,
	int	  _which,
	const ObjectUidTp&_from,
	const ipc::ConnectorUid *_conid
){
	if(_requid.first < d.cdq.size() && d.cdq[_requid.first].uid == _requid.second){
		if(d.cdq[_requid.first].cmd->receiveIOStream(_sp, _fuid, _which, _from, _conid) == OK){
			d.eq.push(_requid.first);
		}
	}
}
void CommandExecuter::receiveString(
	const std::string &_str,
	const RequestUidTp &_requid,
	int			_which,
	const ObjectUidTp&_from,
	const ipc::ConnectorUid *_conid
){
	if(_requid.first < d.cdq.size() && d.cdq[_requid.first].uid == _requid.second){
		if(d.cdq[_requid.first].cmd->receiveString(_str, _which, _from, _conid) == OK){
			d.eq.push(_requid.first);
		}
	}
}
void CommandExecuter::receiveNumber(
	const int64 &_no,
	const RequestUidTp &_requid,
	int			_which,
	const ObjectUidTp&_from,
	const ipc::ConnectorUid *_conid
){
	if(_requid.first < d.cdq.size() && d.cdq[_requid.first].uid == _requid.second){
		if(d.cdq[_requid.first].cmd->receiveNumber(_no, _which, _from, _conid) == OK){
			d.eq.push(_requid.first);
		}
	}
}
void CommandExecuter::receiveError(
	int _errid, 
	const RequestUidTp &_requid,
	const ObjectUidTp&_from,
	const clientserver::ipc::ConnectorUid *_conid
){
	if(_requid.first < d.cdq.size() && d.cdq[_requid.first].uid == _requid.second){
		if(d.cdq[_requid.first].cmd->receiveError(_errid, _from, _conid) == OK){
			d.eq.push(_requid.first);
		}
	}
}


}//namespace clientserver


