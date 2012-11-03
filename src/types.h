/*
 * types.h
 *
 *  Created on: Oct 16, 2010
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <sys/time.h>

#include "util/uuid.h"
#include "datastructure/doublelinkedlist.h"
#include "constants.h"
#include "exception.h"
#include "sqlite3.h"

enum _GScale_Group_Setting {
	MIN,
	ConnectTimeout,
	DisconnectTimeout,
	MAX
};
typedef enum _GScale_Group_Setting GScale_Group_Setting;

typedef struct _GScale_Backend GScale_Backend;
typedef struct _GScale_Backend_IOCallback GScale_Backend_IOCallback;
typedef struct _GScale_Backend_InstanceCallback GScale_Backend_InstanceCallback;

typedef void (*GScale_Backend_Init)(GScale_Backend *_this);

typedef struct _GScale_GroupIdentifier GScale_GroupIdentifier;
typedef struct _GScale_Group GScale_Group;
typedef struct _GScale_NodeCallbacks GScale_NodeCallbacks;
typedef struct _GScale_Node_Identifier GScale_Node_Identifier;
typedef struct _GScale_Node_FQIdentifier GScale_Node_FQIdentifier;

typedef struct _GScale_Group_Settings GScale_Group_Settings;

typedef GScale_UUID GScale_Host_UUID;
typedef GScale_UUID GScale_Node_UUID;

typedef char GScale_Node_Alias[64];
typedef char GScale_Group_Name[64];
typedef char GScale_Group_Namespace[64];

typedef struct _GScale_Node_Remote GScale_Node_Remote;
typedef struct _GScale_Node_Local GScale_Node_Local;
typedef struct _GScale_Node GScale_Node;

typedef struct _GScale_Packet_Header GScale_Packet_Header;
typedef struct _GScale_Packet GScale_Packet;
typedef struct _GScale_Packet_Payload GScale_Packet_Payload;

typedef struct _GScale_FutureResult GScale_FutureResult;


struct _GScale_Backend_IOCallback {
    /* called when a node becomes available */
    void (*OnLocalNodeAvailable)(GScale_Backend *_this, GScale_Node *node);
    /* called when a node becomes unavailable */
    void (*OnLocalNodeUnavailable)(GScale_Backend *_this, GScale_Node *node);

    /* when a local node writes to the group */
    int32_t (*OnLocalNodeWritesToGroup)(GScale_Backend *_this,
            GScale_Packet *packet);
};

struct _GScale_Backend_InstanceCallback {
    void (*Worker)(GScale_Backend *_this, struct timeval *timeout);

    void (*SetOption)(GScale_Backend *_this, uint16_t optname,
            const void *optval, uint16_t optlen);
    void (*GetOption)(GScale_Backend *_this, uint16_t optname, void *optval,
            uint16_t optlen);
    void (*SetNodeOption)(GScale_Backend *_this, uint16_t optname,
    		const GScale_Node *node, const void *optval, uint16_t optlen);
    void (*GetNodeOption)(GScale_Backend *_this, uint16_t optname,
    		const GScale_Node *node, void *optval, uint16_t optlen);

    void (*Shutdown)(GScale_Backend *_this);
};

struct _GScale_Backend {
    GScale_Group *g;
    GScale_Backend_Init initcallback; /* used to identify the backend */

    GScale_Backend_IOCallback ioc;
    GScale_Backend_InstanceCallback instance;

    /* some backend specific data */
    void *backend_private;
};

struct _GScale_GroupIdentifier {
    GScale_Group_Namespace nspname; /* 64 = length of 2 uuids in hexadecimal notation without dashes */
    GScale_Group_Name name;
};

struct _GScale_Group_Settings{
    /* used for connect and disconnect */
	struct timeval connecttimeout;
	struct timeval disconnecttimeout;
    struct _toggle{
        int connecttimeout:1;
        int disconnecttimeout:1;
        int padding:6;
    } enable;
};

struct _GScale_Group{
	GScale_GroupIdentifier gid;
	uint32_t packetid;
	GScale_Group_Settings settings;

    DoubleLinkedList backends;

    /*DoubleLinkedList localnodes;*/
    /*DoubleLinkedList remotenodes;*/

    sqlite3 *dbhandle;

    /* eventdescriptor contains of 2 int's
     * 0: orignall epoll descriptor - internally used
     * 1: duplicate of epoll descriptor - externally used, e.g. by a library user
     *
     * in case the user of this library call's close() on the read side
     */
    int eventdescriptor[2];
    /* internally used pipe to force Worker enqueuing */
    int pipefds[2];
};

struct _GScale_NodeCallbacks {
    void (*OnRead)(GScale_Node *node, const GScale_Packet *packet);

    void (*OnNodeAvailable)(GScale_Node *dst,
            const GScale_Node_FQIdentifier *availnode);
    void (*OnNodeUnavailable)(GScale_Node *dst,
            const GScale_Node_FQIdentifier *unavailnode);
};

struct _GScale_Node_Identifier {
    GScale_GroupIdentifier gid;
    GScale_Node_UUID uuid; /* binary representation of a randomly generated uuid */
    GScale_Node_Alias alias;
};

struct _GScale_Node_FQIdentifier {
    GScale_Host_UUID hostuuid;
    GScale_Node_Identifier id;
};

struct _GScale_RemoteNode {
    GScale_Node_FQIdentifier id;
    GScale_Backend *backend;

    struct timeval added;

    void *tag;
};

struct _GScale_Node_Remote{
    GScale_Node_FQIdentifier id;
    void *tag;
};
struct _GScale_Node_Local{
    GScale_Node_Identifier id;
    GScale_Group *g;
    GScale_NodeCallbacks callbacks;
};

struct _GScale_Node {
    struct timeval added;
    GScale_Backend *backend;

    struct _nflags{
        int remote:1;
        int reserved:7;
    } flags;
    union _GScale_Locality{
        GScale_Node_Local local;
        GScale_Node_Remote remote;
    } locality;
};

struct _GScale_Packet_Header {
    GScale_Node_FQIdentifier src;
    GScale_Node_FQIdentifier dst;

    uint32_t packetid;
    uint8_t dstType;
};
struct _GScale_Packet_Payload {
    uint32_t length;
    char *buffer;
};
struct _GScale_Packet {
    GScale_Packet_Header header;

    GScale_Packet_Payload data;
};

typedef union _GScale_UnionStdtypes GScale_UnionStdtypes;

union _GScale_UnionStdtypes{
    uint8_t c;
    uint16_t s;
    uint32_t i;
    uint64_t l;
    float f;
    double d;
    long double ld;
    void *vp;
    char dat[1];
};

struct _GScale_FutureResult{
	GScale_UnionStdtypes marker;

	void (*resultcallback)(GScale_FutureResult *result, GScale_Group *g, GScale_Backend_Init initcallback);

	uint8_t success;
	GScale_Exception except;
};

#define GSCALE_INIT_GID {{'\0'},{'\0'}}
#define GSCALE_INIT_NID {GSCALE_INIT_GID,{'\0'}, {'\0'}}
#define GSCALE_INIT_LOCALNODE {GSCALE_INIT_NID, NULL, {NULL, NULL, NULL}}
#define GSCALE_INIT_NODE {{0,0}, NULL, {0,0}, {GSCALE_INIT_LOCALNODE}}

#endif /* TYPES_H_ */
