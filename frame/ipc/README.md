# SolidFrame IPC module

A process-to-process communication (message exchange) engine via plain/secured TCP.

Enabled for two scenarios:
* client-server
* peer-to-peer

## Characteristics

* C++ only (no IDLs)
* Per destination connection pool.
* Buffer oriented message serialization engine. This means that the messages gets serialized (marshaled) one fixed sized buffer (e.g. 4KB) at a time which further enables sending messages that are bigger than the system memory (e.g. a message with an 100GB file).
* Message multiplexing. Multiple messages from the send queue are sent in parallel on the same connection. This means for example that one can send multiple small messages while also sending one (or more) huge message(s) like the one above.
* Support for buffer level compression. The library can compress (using a pluggable algorithm) a buffer before writing it on the socket.

## Implemented Functionality

### Basic protocol functionality

**Description**
* Serialize messages at one end and deserialize at the other

**Test**
* test_protocol_basic
	* directly use ipcmessagewriter and ipcmessagereader to serialize and deserialize messages
* test_protocol_synchronous
	* directly use ipcmessagewriter and ipcmessagereader to serialize and deserialize synchronous messages

### Basic IPC functionality

**Description**

* Client-Server
* Peer2Peer
* Send messages and wait for response
* Connection pool

**Test**

* test_clientserver_basic
	* multiple messages are sent from client to server
	* the server respond back with the received message (echo)
	* messages are sent asynchronously and have different sizes
	* test paramenter: the number of connections for client connection pool.
	* test verify received message integrity
	* test verify that all responses are received during a certain amount of time
* test_clientserver_sendrequest
	* Mostly like above, only that there are two types of message: Request and Response
	* The client side uses ipc::Service::sendRequest() with callback for Response
* test_peer2peer_basic
	* multiple message are sent from both sides and the response is expected also on both sides
	* messages are sent asynchronously and have different sizes
	* test paramenter: the number of connections per pool
	* test verify received message integrity
	* test verify that all responses are received during a certain amount of time

### Prevent DOS made possible by sending lots of KeepAlive packets

**Description**

* set timer for non-active connections
* count the number of received keep alive packets per interval

**Test**

* test_keepalive_fail
	* client sends one message to server
	* test parameter: scenario 0: test that server closes connection with error_too_many_keepalive_packets_received
	* test parameter: scenario 1: test that server closes connection with error_inactivity_timeout
	* scenario 0: client is configured to send multiple keep-alive packets
	* scenario 0: server is configured so that it should close connection because of multime keep-alive packets
	* scenario 1: client is configured to send keep-alive packet very rarely
	* scenario 1: server is configured so that is should close connection because of inactivity
* test_keepalive_success
	* scenario 0: verify that connection is not closed during inactivity period
	* scenario 0: a message is send from client after an inactivity period and a response is expected from server


## Pending Functionality

### Support for One-Shot-Delivery and Message Canceling

**Description**

* Normal behavior: ipc::service will continuously try to send a message until:
	* Normal message: fully or partially sent
	* Idempotent messages:
		* when fully sent
		* or, if expecting response, until receiving the response
* Normal behavior: in other words: it will wait for the peer side to become available then send the messages
* For Messages with OneShotSendFlag, ipc::service will try to call complete asap - will imediately fail if no connection to server
* Pending send message can be canceled - not all messages can be canceled (i.e. no cancel for messages that are currently being sent)
* Messages will be canceled only when they return to pool.

**Test**

* test_clientserver_delayed
	* start client and send message
	* wait a while
	* start the server and expect a response on the client side
	* try to cancel the sent message - expect fail
* test_clientserver_noserver
	* start client and send message
	* wait for a while - expect message did not complete somehow
	* cancel the message - expect message to complete with error_canceled
* test_clientserver_oneshot
	* start client and send one shot message
	* expect message canceled in certain, short amount of time

### Support for packet compression

**Test**

* test_clientserver_basic
	* add support for compression using Google Snappy (http://google.github.io/snappy/)

### Support for SSL

### Support for SOCKS5