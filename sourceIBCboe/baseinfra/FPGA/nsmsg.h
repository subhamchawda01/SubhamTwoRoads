/** @file
 * NovaSparks Messaging API 2.
 *
 * This is the API for program using NSMSG.
 * 
 * (c) NovaSparks 2008-2013
 */

/**
 * \mainpage NSMSG API 2 reference.
 * 
 * The API described here in covers both new and legacy API found in NSMSG API 1.
 * These APIs are interchangeable but primitives from current API may not be mixed 
 * with primitives from deprecated API.
 *
 * The API provides the tools to receive messages from NovaSparks Feed-handlers.  
 * This API is part of NovaSparks Messaging, or mnemonically nsmsg.
 *
 * Before using this API it is recommended to read NovaSparks Messaging 2 User 
 * Guide.
 *
 * Further details about the individual routines in the API maybe found below.
 */

#ifndef NS_MESSAGING2_H
#define NS_MESSAGING2_H

#include <time.h>

#pragma GCC visibility push(default)

#ifdef __cplusplus
extern "C" {
#endif

/* some flags */
#define NSMSG_NONBLOCK 0x1
#define NSMSG_RESET 0x2

/* structures available to user but hidden */

/** nsmsg context */
typedef unsigned long nsmsg_t;
/** nsmsg socket */
typedef unsigned long nsmsg_socket_t;
/**
 * \typedef nsmsg message, opaque size correspond to actual message structure 
 * size for allocation by user. Though user should not rely on that size for 
 * anything else than allocation.
 */
typedef struct {char opaque[32];} nsmsg_msg_t;
/** nsmsg subject id */
typedef unsigned int nsmsg_subject_t;

/**
 * Initialize messaging context.
 * @param  reserved Reserved for future use, must be NULL.
 * @return A handle to a fully initialized messaging environment,
 *         NULL on failure. <BR>
 * @return ENOMEM Not enough memory <BR>
 */
nsmsg_t * nsmsg_init(const char *reserved);

/**
 * Terminate messaging context.
 * @param  ctx  the messaging environment
 * @return 0 on success, -1 on failure. <BR>
 */
int nsmsg_term(nsmsg_t *ctx);

/**
 * Make a new messaging socket.
 * Socket connects thread to nsmsg interface for future I/O operation.
 * Socket configures I/O to be active blocking (spinning) as default. They 
 * can be made non-blocking with NSMSG_NONBLOCK flag.
 *
 * @param  ctx   the messaging environment
 * @param  uri   the messaging interface uri, proto://iface-name (pcie://pcie0)
 * @param  flags socket options
 * @return 0 on success, NULL on failure. <BR>
 * @return EINVAL when URI is malformed <BR>
 * @return ENOMEM not enough memory <BR>
 */
nsmsg_socket_t * nsmsg_socket(nsmsg_t *ctx, const char *uri, int flags);

/**
 * Close messaging socket.
 *
 * @param  socket the messaging socket
 * @return 0 on success, -1 on failure. <BR>
 */
int nsmsg_close(nsmsg_socket_t *socket);

/**
 * Subscribe to a symbol within the current messaging environment.
 * Subscriptions are per context not per socket. <BR>
 *
 * Symbol are subject ids of the form "feed_id:category_id:instrument_id".
 * 
 * User private data is a thunk that user pass to associate its object with 
 * a symbol. So on message reception the object may be retrieved.
 *
 * @param ctx    pointer to a messaging environment
 * @param symbol symbol to subscribe to
 * @param priv   user private data
 * @return 0 on success, -1 on failure <BR>
 * @return EINVAL invalid symbol <BR>
 */
int nsmsg_subscribe(nsmsg_t *ctx, const char *symbol, void *priv);

/**
 * Unsubscribe to a symbol within the current messaging environment.
 * @param ctx    pointer to a messaging environment
 * @param symbol symbol to unsubscribe to
 * @return 0 on success, -1 on failure <BR>
 * @return EINVAL invalid symbol <BR>
 */
int nsmsg_unsubscribe(nsmsg_t *ctx, const char *symbol);

/* reserved for future use */

/**
 * Allocate and initialize a new message descriptor.
 * @param s the messaging environment
 * @param symbol symbol associated with message, NULL when message is used 
 *        for reception
 * @param len length of message, set to 0 when message is used for reception
 * @return new message on success, NULL on failure
 */
nsmsg_msg_t * nsmsg_msg_new(nsmsg_socket_t *s, const char *symbol, size_t len);

/**
 * Increment reference counter on the message descriptor.
 * @param msg the message
 */
void nsmsg_msg_ref(nsmsg_msg_t *msg);

/* end of reserved */

/**
 * Receive a message from socket sock.
 * Fill message descriptor msg on message reception or return error.
 * @param sock          the messaging socket
 * @param msg        messaging message
 * @param flags      same as socket flags but on a call basis
 * @return size of message payload on success, -1 on failure <BR>
 * @return EBADF interface down <BR>
 * @return EAGAIN occurs in non blocking mode when no messages
 *                are in the incoming queue.
 * @return ENOBUFS occurs when control flow cannot be handled correctly 
 *                 because incoming queue is full.
 */
ssize_t nsmsg_recv(nsmsg_socket_t *sock, nsmsg_msg_t *msg, int flags);

/**
 * Decrement reference counter on the message, free ressources when no more 
 * reference.
 * @param msg the message
 */
void nsmsg_msg_unref(nsmsg_msg_t *msg);

/* reserved for future use */

/**
 * Send/Publish a message.
 * @param ctx the messaging environment
 * @param msg user pointer where the message is
 * @return length of message on success, -1 on failure
 */
ssize_t nsmsg_send(nsmsg_socket_t *s, nsmsg_msg_t *msg);

/* end of reserved */

#include <errno.h>
#ifndef ENOBUFS
#define ENOBUFS 105
#endif

/**
 * Error description function.
 * Give a string describing the error number given in parameter.
 * @param  e  error code (errno)
 * @return string error description
 */
const char * nsmsg_strerror(int e);

/* Message accessors */

/**
 * Get private data associated with the symbol of this message.
 * @param msg the message
 * @return user private data
 */
void * nsmsg_msg_priv(const nsmsg_msg_t *msg);

/**
 * Get raw message (including nswf message header).
 * @param msg the message
 * @return raw message data
 */
const void * nsmsg_msg_raw(const nsmsg_msg_t *msg);

/**
 * Get raw message length of the message.
 * @param msg the message
 * @return length of message payload (including header)
 */
size_t nsmsg_msg_raw_len(const nsmsg_msg_t *msg);

/**
 * Get payload data of the message (excluding header).
 * @param msg the message
 * @return message data
 */
const void * nsmsg_msg_data(const nsmsg_msg_t *msg);

/**
 * Get payload data length of the message.
 * @param msg the message
 * @return length of message payload (excluding header)
 */
size_t nsmsg_msg_len(const nsmsg_msg_t *msg);

/**
 * Get Subject id of the message.
 * @param msg the message
 * @return subject identifier
 */
nsmsg_subject_t nsmsg_msg_subject(const nsmsg_msg_t *msg);

/**
 * Get end to end message latency
 * @param msg the message
 * @param ts latency in timespec format
 * @return 0 on success and -1 when message does not contain e2e timestamp
 */
int nsmsg_msg_latency(const nsmsg_msg_t *msg, struct timespec *ts);

#ifdef __cplusplus
}
#endif

#pragma GCC visibility pop

#endif /* NS_MESSAGING2_H */
