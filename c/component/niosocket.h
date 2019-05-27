//
// Created by hujianzhe on 18-8-13.
//

#ifndef	UTIL_C_COMPONENT_NIOSOCKET_H
#define	UTIL_C_COMPONENT_NIOSOCKET_H

#include "../syslib/atomic.h"
#include "../syslib/io.h"
#include "../syslib/socket.h"
#include "../datastruct/hashtable.h"
#include "dataqueue.h"

typedef struct NioLoop_t {
	unsigned char m_initok;
	Atom16_t m_wake;
	Reactor_t m_reactor;
	FD_t m_socketpair[2];
	void* m_readol;
	DataQueue_t* m_msgdq;
	CriticalSection_t m_msglistlock;
	List_t m_msglist;
	List_t m_sockcloselist;
	Hashtable_t m_sockht;
	HashtableNode_t* m_sockht_bulks[2048];
	long long m_event_msec;
} NioLoop_t;

typedef struct NioInternalMsg_t {
	ListNode_t m_listnode;
	int type;
} NioInternalMsg_t;
struct NioSocket_t;
typedef struct NioMsg_t {
	NioInternalMsg_t internal;
	struct NioSocket_t* sock;
} NioMsg_t;

enum {
	NIOSOCKET_TRANSPORT_NOSIDE,
	NIOSOCKET_TRANSPORT_CLIENT,
	NIOSOCKET_TRANSPORT_SERVER,
	NIOSOCKET_TRANSPORT_LISTEN
};

typedef struct NioSocketDecodeResult_t {
	char err;
	char incomplete;
	char is_reconnect_reply;
	char reconnect_reply_ok;
	int decodelen;
	int bodylen;
	unsigned char* bodyptr;
	NioMsg_t* msgptr;
} NioSocketDecodeResult_t;

typedef struct NioSocket_t {
/* public */
	FD_t fd;
	int domain;
	int socktype;
	int protocol;
	int sendprobe_timeout_sec;
	int keepalive_timeout_sec;
	int close_timeout_msec;
	const char* sessionid;
	void* userdata;
	unsigned int transport_side;
	union {
		struct sockaddr_storage local_listen_saddr;
		struct sockaddr_storage peer_listen_saddr;
	};
	void(*accept)(struct NioSocket_t*, FD_t, const struct sockaddr_storage*);
	void(*decode)(unsigned char*, size_t, NioSocketDecodeResult_t*);
	void(*recv)(struct NioSocket_t*, const struct sockaddr_storage*, NioSocketDecodeResult_t*);
	size_t(*hdrlen)(size_t bodylen);
	void(*encode)(unsigned char* hdrptr, size_t bodylen);
	void(*send_probe_to_server)(struct NioSocket_t*);
	void(*send_retransport_req_to_server)(struct NioSocket_t*, unsigned int, unsigned int);
	void(*send_retransport_ret_to_client)(struct NioSocket_t*, int);
	void(*reg_callback)(struct NioSocket_t*, int);
	void(*net_reconnect_callback)(struct NioSocket_t*, int);
	void(*shutdown_callback)(struct NioSocket_t*);
	void(*close_callback)(struct NioSocket_t*);
/* private */
	volatile char m_valid;
	volatile char m_sendaction;
	Atom16_t m_shutdown;
	int m_errno;
	NioInternalMsg_t m_regmsg;
	NioInternalMsg_t m_shutdownmsg;
	NioInternalMsg_t m_shutdownpostmsg;
	NioInternalMsg_t m_netreconnectmsg;
	NioInternalMsg_t m_closemsg;
	HashtableNode_t m_hashnode;
	NioLoop_t* m_loop;
	void(*m_free)(struct NioSocket_t*);
	void* m_readol;
	void* m_writeol;
	int m_writeol_has_commit;
	long long m_lastactive_msec;
	long long m_sendprobe_msec;
	unsigned char *m_inbuf;
	size_t m_inbufoffset;
	size_t m_inbuflen;
	unsigned short m_recvpacket_maxcnt;
	List_t m_recvpacketlist;
	List_t m_sendpacketlist;
	List_t m_sendpacketlist_bak;
	struct {
	/* public */
		struct sockaddr_storage peer_saddr;
		unsigned short mtu;
		unsigned short rto;
		unsigned char resend_maxtimes;
		unsigned char cwndsize;
		unsigned char enable;
	/* private */
		unsigned char m_status;
		unsigned char m_synrcvd_times;
		unsigned char m_synsent_times;
		unsigned char m_reconnect_times;
		unsigned char m_fin_times;
		long long m_synrcvd_msec;
		long long m_synsent_msec;
		long long m_reconnect_msec;
		long long m_fin_msec;
		unsigned int m_cwndseq;
		unsigned int m_recvseq;
		unsigned int m_sendseq;
		unsigned int m_recvseq_bak;
	} reliable;
} NioSocket_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll NioSocket_t* niosocketCreate(FD_t fd, int domain, int type, int protocol, NioSocket_t*(*pmalloc)(void), void(*pfree)(NioSocket_t*));
__declspec_dll void niosocketManualClose(NioSocket_t* s);
__declspec_dll NioSocket_t* niosocketSend(NioSocket_t* s, const void* data, unsigned int len, const struct sockaddr_storage* saddr);
__declspec_dll NioSocket_t* niosocketSendv(NioSocket_t* s, const Iobuf_t iov[], unsigned int iovcnt, const struct sockaddr_storage* saddr);
__declspec_dll void niosocketClientReconnect(NioSocket_t* s);
__declspec_dll void niosocketTcpTransportReplace(NioSocket_t* old_s, NioSocket_t* new_s, int new_recvseq, int new_cwndseq);
__declspec_dll void niosocketShutdown(NioSocket_t* s);
__declspec_dll NioLoop_t* nioloopCreate(NioLoop_t* loop, DataQueue_t* msgdq);
__declspec_dll NioLoop_t* nioloopWake(NioLoop_t* loop);
__declspec_dll int nioloopHandler(NioLoop_t* loop, NioEv_t e[], int n, long long timestamp_msec, int wait_msec);
__declspec_dll void nioloopReg(NioLoop_t* loop, NioSocket_t* s[], size_t n);
__declspec_dll void nioloopDestroy(NioLoop_t* loop);
__declspec_dll void niomsgHandler(DataQueue_t* dq, int max_wait_msec, void (*user_msg_callback)(NioMsg_t*, void*), void* arg);
__declspec_dll void niomsgClean(DataQueue_t* dq, void(*deleter)(NioMsg_t*));

#ifdef __cplusplus
}
#endif

#endif
