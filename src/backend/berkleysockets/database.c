/*
 * backend_berkleysockets_database.c
 *
 */

#ifndef BACKEND_BERKLEYSOCKETS_DATABASE_C_
#define BACKEND_BERKLEYSOCKETS_DATABASE_C_

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "types.h"
#include "sqlite3.h"
#include "util/log.h"
#include "backend/berkleysockets/types.h"
#include "backend/berkleysockets/util.h"
#include "constants.h"
#include "exception.h"
#include "debug.h"
#include "../../database.h"

void
GScale_Backend_Berkleysockets_Database_Init(sqlite3 *db){
    int i=0;
    const char *errmsg;
    char *statements[] = {
            "BEGIN",
            "CREATE TABLE sockets (id INTEGER PRIMARY KEY, ifaceip TEXT, peerip TEXT, port INTEGER, fd INTEGER, type INTEGER, family INTEGER, server INTEGER, hostuuid TEXT, socketptr BLOB, createdsecond INTEGER, createdmicrosecond INTEGER)",
            "CREATE INDEX sockets_fd_idx ON sockets(fd)",
            "CREATE INDEX sockets_peerip_idx ON sockets(peerip)",
            "CREATE INDEX sockets_port_idx ON sockets(port)",
            "CREATE INDEX sockets_type_idx ON sockets(type)",
            "CREATE INDEX sockets_family_idx ON sockets(family)",
            "CREATE INDEX sockets_server_idx ON sockets(server)",
            "CREATE INDEX sockets_hostuuid_idx ON sockets(hostuuid)",
            "CREATE INDEX sockets_createdsecond_idx ON sockets(createdsecond)",
            "CREATE INDEX sockets_createdmicrosecond_idx ON sockets(createdmicrosecond)",

            "CREATE TABLE listenports (id INTEGER PRIMARY KEY, ifaceip TEXT, port INTEGER, domain INTEGER, type INTEGER, server INTEGER, createdsecond INTEGER, createdmicrosecond INTEGER, resultcallback BLOB)",
            "CREATE UNIQUE INDEX listenports_primaryip_idx ON listenports(ifaceip,port,domain,type,server)",
            "CREATE INDEX listenports_ifaceip_idx ON listenports(ifaceip)",
            "CREATE INDEX listenports_port_idx ON listenports(port)",
            "CREATE INDEX listenports_domain_idx ON listenports(domain)",
            "CREATE INDEX listenports_createdsecond_idx ON listenports(createdsecond)",
            "CREATE INDEX listenports_createdmicrosecond_idx ON listenports(createdmicrosecond)",
            "COMMIT",
            NULL
    };
    /*SELECT port FROM (SELECT port FROM listenports WHERE server=0
     *EXCEPT
     *SELECT port FROM listenports WHERE server=1) WHERE port>0 ORDER BY port;
     */

    if(db==NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    for(i=0;statements[i]!=NULL;i++){
        if(sqlite3_exec(db, statements[i], 0,0,0)!=SQLITE_OK){
            errmsg = sqlite3_errmsg(db);
            sqlite3_exec(db, "ROLLBACK", 0,0,0);

            ThrowException(errmsg, EINVAL);
        }
    }
}

void
GScale_Backend_Berkleysockets_Database_Shutdown(sqlite3 *db){
    if(db==NULL){
        ThrowException("invalid argument given", EINVAL);
    }


    if(sqlite3_exec(db, "DROP TABLE sockets", 0,0,0)!=SQLITE_OK){
        ThrowException(sqlite3_errmsg(db), 0);
    }
}

/******************************** SOCKET Operations ************************************/

GScale_Backend_BerkleySockets_Socket*
GScale_Backend_Berkleysockets_Database_getSocket(sqlite3 *db, const char *where, const char *orderby){
    GScale_Backend_BerkleySockets_Socket *next = NULL;
    char query[1024] = {'\0'};
    sqlite3_stmt *stmt = NULL;

    if(db==NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    snprintf(query, sizeof(query)-1, "SELECT socketptr FROM sockets");
    if(where!=NULL){
        snprintf(query+strlen(query),sizeof(query)-1-strlen(query)," WHERE %s",where);
    }
    if(orderby!=NULL){
        snprintf(query+strlen(query),sizeof(query)-1-strlen(query)," ORDER BY %s",orderby);
    }
    snprintf(query+strlen(query),sizeof(query)-1-strlen(query)," LIMIT 1");
    /*GSCALE_DEBUGP("query socket: %s\n", query);*/

    if(sqlite3_prepare_v2(db,query,-1,&stmt,0)!=SQLITE_OK){
        ThrowException(sqlite3_errmsg(db), 0);
    }
    if(sqlite3_step(stmt)==SQLITE_ROW)
    {
    	memcpy(&next, sqlite3_column_blob(stmt,0), sizeof(GScale_Backend_BerkleySockets_Socket*));
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return next;
}

void
GScale_Backend_Berkleysockets_Database_updateSocket(sqlite3 *db, GScale_Backend_BerkleySockets_Socket *socket){
    sqlite3_stmt* stmt = NULL;
    int rval = 0;
    const char *errmsg;
    char query[1024] = {'\0'};

    if(db==NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    /*printf("UPDATE socket: %p\n", socket);*/

    snprintf(query, sizeof(query)-1, "UPDATE sockets SET type=?1, family=?2, server=?3, hostuuid=?5, socketptr=?6, createdsecond=?7, createdmicrosecond=?8 WHERE fd=%d", socket->fd);
    /*GSCALE_DEBUGP("update socket: %s | %s", query, UUID_toString(socket->hostuuid));*/
    sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, socket->type);
    sqlite3_bind_int(stmt, 2, socket->family);
    sqlite3_bind_int(stmt, 3, socket->flags.server==0 ? 0 : 1);

    sqlite3_bind_text(stmt, 5, UUID_toString(socket->hostuuid), sizeof(GScale_StrUUID), SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 6, &socket, sizeof(GScale_Backend_BerkleySockets_Socket*), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, socket->created.tv_sec);
    sqlite3_bind_int(stmt, 8, socket->created.tv_usec);

    rval = sqlite3_step(stmt);
    if (rval != SQLITE_DONE){
        errmsg = sqlite3_errmsg(db);
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    if (rval != SQLITE_DONE){
        ThrowException(errmsg, 0);
    }
}

void
GScale_Backend_Berkleysockets_Database_insertSocket(sqlite3 *db, GScale_Backend_BerkleySockets_Socket *socket){
    sqlite3_stmt* stmt = NULL;
    int rval = 0;
    struct sockaddr_storage addr;
    char ip[INET6_ADDRSTRLEN+1] = {'\0'};
    char dstip[INET6_ADDRSTRLEN+1] = {'\0'};
    size_t size = 0;
    const char *errmsg;

    if(db==NULL || socket==NULL){
        ThrowException("invalid argument given",  EINVAL);
    }

    size=sizeof(struct sockaddr_storage);
    getsockname(socket->fd, (struct sockaddr*)&addr, &size);
    GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr_r(&addr, ip, sizeof(ip));
    size=sizeof(struct sockaddr_storage);
    if(getpeername(socket->fd, (struct sockaddr*)&addr, &size) != -1){
    	GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr_r(&addr, dstip, sizeof(dstip));
    }

    sqlite3_prepare_v2(db, "INSERT INTO sockets (fd, ifaceip, peerip, port, type, family, server, hostuuid, socketptr, createdsecond, createdmicrosecond) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, socket->fd);
    sqlite3_bind_text(stmt, 2, ip, strlen(ip), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, dstip, strlen(dstip), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, socket->port);
    sqlite3_bind_int(stmt, 5, socket->type);
    sqlite3_bind_int(stmt, 6, socket->family);
    sqlite3_bind_int(stmt, 7, socket->flags.server==0 ? 0 : 1);

    sqlite3_bind_text(stmt, 8, UUID_toString(socket->hostuuid), sizeof(GScale_StrUUID), SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 9, &socket, sizeof(GScale_Backend_BerkleySockets_Socket*), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 10, socket->created.tv_sec);
    sqlite3_bind_int(stmt, 11, socket->created.tv_usec);

    /*printf("inserting socket: %s, %p, %d, %d, %d, %d\n\n", ip, socket, socket->port, socket->type, socket->family, socket->flags.server);*/

    rval = sqlite3_step(stmt);
    if (rval != SQLITE_DONE){
        errmsg = sqlite3_errmsg(db);
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    if (rval != SQLITE_DONE){
        ThrowException(errmsg,  0);
    }
}

void
GScale_Backend_Berkleysockets_Database_deleteSocket(sqlite3 *db, GScale_Backend_BerkleySockets_Socket *socket){
    char query[1024] = {'\0'};

    if(db==NULL || socket==NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    snprintf(query, sizeof(query)-1, "DELETE FROM sockets WHERE fd=%d", socket->fd);
    if(sqlite3_exec(db, query, NULL , NULL, NULL)!=SQLITE_OK){
        ThrowException(sqlite3_errmsg(db), 0);
    }
}

GScale_Backend_BerkleySockets_Socket*
GScale_Backend_Berkleysockets_Database_getSocketBySettings(sqlite3 *db, const GScale_Backend_BerkleySockets_Socket *socket,
                                                           const int fd, const int type, const int family, const int server, const GScale_Host_UUID *hostuuid)
{
    struct timeval tv = {0,0};
    char where[1024] = "1=1";

    if(db==NULL){
        ThrowException("invalid argument given", 0);
    }

    if(socket!=NULL){
        tv = socket->created;
    }

    if(fd>=0){
        snprintf(where+strlen(where), sizeof(where)-1-strlen(where), " AND fd=%d",fd);
    }
    if(type>=0){
        snprintf(where+strlen(where), sizeof(where)-1-strlen(where), " AND type=%d",type);
    }
    if(family>=0){
        snprintf(where+strlen(where), sizeof(where)-1-strlen(where), " AND family=%d",family);
    }
    if(server>=0){
        snprintf(where+strlen(where), sizeof(where)-1-strlen(where), " AND server=%d",server);
    }
    if(hostuuid!=NULL){
        snprintf(where+strlen(where), sizeof(where)-1-strlen(where), " AND hostuuid='%s'",UUID_toString(*hostuuid));
    }

    snprintf(where+strlen(where), sizeof(where)-1-strlen(where), "  AND (createdsecond>%lu OR (createdsecond>=%lu AND createdmicrosecond>%lu))", tv.tv_sec,tv.tv_sec,tv.tv_usec);
    socket = GScale_Backend_Berkleysockets_Database_getSocket(db, where, "createdsecond,createdmicrosecond DESC");

    return (GScale_Backend_BerkleySockets_Socket*)socket;
}

void GScale_Backend_Berkleysockets_Database_insertPort(sqlite3 *db, const struct sockaddr_storage *addr, const int type, const int server, const GScale_FutureResult fr){
    char query[1024] = {'\0'};
    sqlite3_stmt *stmt = NULL;
    struct timeval now = {0,0};
    int count = 0, rval = 0;
    char ip[NI_MAXHOST] = {'\0'};   /* e.g. 255.255.255.255, 2001:0db8:85a3:0000:0000:8a2e:0370:7334 (from wikipedia) */
    uint16_t port = 0;
    int domain = 0;
    const char *errmsg;

    if(db==NULL || addr==NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    domain = addr->ss_family;
    GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr_r(addr, ip, sizeof(ip));
    switch(addr->ss_family){
        case AF_INET:
            port = ntohs(((struct sockaddr_in*)addr)->sin_port);
            break;
        case AF_INET6:
            port = ntohs(((struct sockaddr_in6*)addr)->sin6_port);
            break;
    }

    snprintf(query, sizeof(query)-1, "SELECT count(*) FROM listenports WHERE ifaceip='%s' AND port=%u AND domain=%u AND type=%u AND server=%u", ip, port, domain, type, server);
    /*GSCALE_DEBUGP("query listenport: %s\n\n",query);*/
    if(sqlite3_prepare_v2(db,query,-1,&stmt,0)!=SQLITE_OK){
        ThrowException(sqlite3_errmsg(db), 0);
    }
    if(sqlite3_step(stmt)==SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt,0);
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    if(count>0){
        return;
    }

    gettimeofday(&now, NULL);

    /*GSCALE_DEBUGP("%s: INSERT INTO listenports(ifaceip,port,domain,type,server) (%s,%d,%d,%d,%d)", __func__, ip, port,domain,type,server);*/
    sqlite3_prepare_v2(db, "INSERT INTO listenports (ifaceip, port, domain, type, server, createdsecond, createdmicrosecond, resultcallback) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)",
    		               -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, ip, strlen(ip), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, port);
    sqlite3_bind_int(stmt, 3, domain);
    sqlite3_bind_int(stmt, 4, type);
    sqlite3_bind_int(stmt, 5, server);
    sqlite3_bind_int(stmt, 6, now.tv_sec);
    sqlite3_bind_int(stmt, 7, now.tv_usec);
    sqlite3_bind_blob(stmt, 8, &fr, sizeof(GScale_FutureResult), SQLITE_STATIC);

    /*printf("inserting listenport: ip:%s, port:%d, domain:%d, type:%d, server:%d\n",ip, port, domain, type, server);*/
    rval = sqlite3_step(stmt);
    if (rval != SQLITE_DONE){
        errmsg = sqlite3_errmsg(db);
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    if(rval!=SQLITE_DONE){
        ThrowException(errmsg, 0);
    }
}
void GScale_Backend_Berkleysockets_Database_deletePort(sqlite3 *db, const uint64_t id, const struct sockaddr_storage *addr, const int type, const int server){
    char delete[1024] = {'\0'};
    char ip[NI_MAXHOST] = {'\0'};
    uint16_t port = 0;
    int domain = 0;

    if(db==NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    if(addr==NULL){
        snprintf(delete, sizeof(delete)-1,
                 "DELETE FROM listenports WHERE id=%llu", id);
    }
    else{
    	GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr_r(addr, ip, sizeof(ip));
        switch(addr->ss_family){
            case AF_INET:
                port = ntohs(((struct sockaddr_in*)addr)->sin_port);
                domain = ((struct sockaddr_in*)addr)->sin_family;
                break;
            case AF_INET6:
                port = ntohs(((struct sockaddr_in6*)addr)->sin6_port);
                domain = ((struct sockaddr_in6*)addr)->sin6_family;
                break;
        }
        snprintf(delete, sizeof(delete)-1,
                 "DELETE FROM listenports WHERE ifaceip='%s' AND port=%u AND domain=%u AND type=%u AND server=%u",
                 ip, port, domain, type, server);
    }
    /*printf("deleteport,  %s\n\n",delete);*/
    if(sqlite3_exec(db, delete, 0,0,0)!=SQLITE_OK){
        ThrowException(sqlite3_errmsg(db), 0);
    }
}

struct sockaddr_storage* GScale_Backend_Berkleysockets_Database_getNextBroadcastPort(sqlite3 *db, struct sockaddr_storage *addr){
    char select[1024] = {'\0'};
    sqlite3_stmt *stmt = NULL;
    int port = 0;
    char ip[NI_MAXHOST+1] = {'\0'};

    if(db==NULL || addr==NULL){
        ThrowException("invalid argument given", EINVAL);
    }
    switch(addr->ss_family){
        case AF_INET:
            port = ntohs(((struct sockaddr_in*)addr)->sin_port);
            GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr_r(addr, ip, sizeof(ip));
            break;
        case AF_INET6:
            port = ntohs(((struct sockaddr_in6*)addr)->sin6_port);
            GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr_r(addr, ip, sizeof(ip));
            break;
    }

    snprintf(select, sizeof(select)-1, "SELECT port, domain, ifaceip FROM listenports WHERE server=0 AND type=%d AND (port>%u OR (port=%u AND ifaceip>'%s')) ORDER BY port,ifaceip LIMIT 1", SOCK_DGRAM, port, port, ip);
    /*GSCALE_DEBUGP("%s: %s", __func__, select);*/
    if(sqlite3_prepare_v2(db,select,-1,&stmt,0)!=SQLITE_OK){
        ThrowException(sqlite3_errmsg(db), EINVAL);
    }
    if(sqlite3_step(stmt)==SQLITE_ROW)
    {
        addr->ss_family = sqlite3_column_int(stmt,1);
        switch(addr->ss_family){
            case AF_INET:
            	GScale_Backend_BerkleySockets_Util_GetSockaddrFromIp(AF_INET, (const char*)sqlite3_column_text(stmt,2), addr);
            	((struct sockaddr_in*)addr)->sin_port = htons(sqlite3_column_int(stmt,0));
                break;
            case AF_INET6:
            	GScale_Backend_BerkleySockets_Util_GetSockaddrFromIp(AF_INET6, (const char*)sqlite3_column_text(stmt,2), addr);
            	((struct sockaddr_in6*)addr)->sin6_port = htons(sqlite3_column_int(stmt,0));
                break;
        }
    }
    else{
        addr = NULL;
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return addr;
}


int32_t GScale_Backend_Berkleysockets_Database_getNextMissingServer(sqlite3 *db, int32_t previd, struct sockaddr_storage *soaddr, int *type, GScale_FutureResult *fresult){
    char select[1024] = {'\0'};
    sqlite3_stmt *stmt = NULL;
    struct sockaddr_in *inaddr = (struct sockaddr_in*)soaddr;
    struct sockaddr_in6 *in6addr = (struct sockaddr_in6*)soaddr;
    const void *sqlitefresult = NULL;

    if(db==NULL || soaddr==NULL || type==NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    snprintf(select, sizeof(select)-1, "SELECT l.id,l.ifaceip,l.port,l.domain,l.type,l.resultcallback from listenports l"
                                       " LEFT JOIN sockets s ON l.ifaceip=s.ifaceip AND l.port=s.port AND l.type=s.type AND l.domain=s.family"
                                       " WHERE s.port IS NULL AND l.id>%d AND l.server=1 ORDER BY l.port,l.domain,l.ifaceip LIMIT 1",
                                       previd);
    if(sqlite3_prepare_v2(db,select,-1,&stmt,0)!=SQLITE_OK){
        ThrowException(sqlite3_errmsg(db), 0);
    }
    if(sqlite3_step(stmt)==SQLITE_ROW)
    {
        previd = sqlite3_column_int(stmt,0);
        *type = sqlite3_column_int(stmt,4);
        if(fresult!=NULL && (sqlitefresult=sqlite3_column_blob(stmt, 5))!=NULL){
            memcpy(fresult, sqlitefresult, sizeof(GScale_FutureResult));
        }

        soaddr->ss_family = sqlite3_column_int(stmt,3);
        if(soaddr->ss_family == AF_INET){
            inaddr->sin_port = htons(sqlite3_column_int(stmt,2));
            inet_pton(soaddr->ss_family, (const char *)sqlite3_column_text(stmt,1), &(inaddr->sin_addr));
        }
        else{
            in6addr->sin6_port = htons(sqlite3_column_int(stmt,2));
            inet_pton(soaddr->ss_family, (const char *)sqlite3_column_text(stmt,1), &(in6addr->sin6_addr));
        }
    }
    else{
        previd=-1;
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return previd;

}

#endif /* BACKEND_BERKLEYSOCKETS_DATABASE_C_ */
