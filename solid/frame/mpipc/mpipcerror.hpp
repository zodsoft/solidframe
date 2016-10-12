// solid/frame/ipc/ipcerror.hpp
//
// Copyright (c) 2015 Valentin Palade (vipalade @ gmail . com) 
//
// This file is part of SolidFrame framework.
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.
//
#ifndef SOLID_FRAME_MPIPC_MPIPCERROR_HPP
#define SOLID_FRAME_MPIPC_MPIPCERROR_HPP

#include "solid/system/error.hpp"

namespace solid{
namespace frame{
namespace mpipc{

extern const ErrorConditionT error_connection_inactivity_timeout;
extern const ErrorConditionT error_connection_too_many_keepalive_packets_received;
extern const ErrorConditionT error_connection_killed;
extern const ErrorConditionT error_connection_delayed_closed;
extern const ErrorConditionT error_connection_enter_active;
extern const ErrorConditionT error_connection_stopping;
extern const ErrorConditionT error_connection_invalid_state;
extern const ErrorConditionT error_connection_logic;
extern const ErrorConditionT error_connection_resolve;

extern const ErrorConditionT error_message_canceled;
extern const ErrorConditionT error_message_connection;

extern const ErrorConditionT error_reader_invalid_packet_header;
extern const ErrorConditionT error_reader_invalid_message_switch;
extern const ErrorConditionT error_reader_too_many_multiplex;

extern const ErrorConditionT error_service_stopping;
extern const ErrorConditionT error_service_unknown_message_type;
extern const ErrorConditionT error_service_server_only;
extern const ErrorConditionT error_service_unknown_recipient;
extern const ErrorConditionT error_service_unknown_pool;
extern const ErrorConditionT error_service_pool_stopping;
extern const ErrorConditionT error_service_pool_full;
extern const ErrorConditionT error_service_unknown_connection;
extern const ErrorConditionT error_service_too_many_active_connections;
extern const ErrorConditionT error_service_bad_cast_request;
extern const ErrorConditionT error_service_bad_cast_response;
extern const ErrorConditionT error_service_start;
extern const ErrorConditionT error_service_start_listener;
extern const ErrorConditionT error_service_message_already_canceled;
extern const ErrorConditionT error_service_message_lost;
extern const ErrorConditionT error_service_unknown_message;

extern const ErrorConditionT error_compression_unavailable;


}//namespace mpipc
}//namespace frame
}//namespace solid

#endif