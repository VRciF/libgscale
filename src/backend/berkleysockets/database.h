/*
 * backend_berkleysockets_database.h
 *
 */

#ifndef BACKEND_BERKLEYSOCKETS_DATABASE_H_
#define BACKEND_BERKLEYSOCKETS_DATABASE_H_

void GScale_Backend_Berkleysockets_Database_Init(sqlite3 *db);

void GScale_Backend_Berkleysockets_Database_Shutdown(sqlite3 *db);
GScale_Backend_BerkleySockets_Socket* GScale_Backend_Berkleysockets_Database_getSocket(sqlite3 *db, const char *where, const char *orderby);
void GScale_Backend_Berkleysockets_Database_updateSocket(sqlite3 *db, GScale_Backend_BerkleySockets_Socket *socket);
void GScale_Backend_Berkleysockets_Database_insertSocket(sqlite3 *db, GScale_Backend_BerkleySockets_Socket *socket);
void GScale_Backend_Berkleysockets_Database_deleteSocket(sqlite3 *db, GScale_Backend_BerkleySockets_Socket *socket);

GScale_Backend_BerkleySockets_Socket*
GScale_Backend_Berkleysockets_Database_getSocketBySettings(sqlite3 *db, const GScale_Backend_BerkleySockets_Socket *socket,
                                                           const int fd, const int type, const int family, const int server, const GScale_Host_UUID *hostuuid);

void GScale_Backend_Berkleysockets_Database_insertPort(sqlite3 *db, const struct sockaddr_storage *addr, const int type, const int server, const GScale_FutureResult fr);
void GScale_Backend_Berkleysockets_Database_deletePort(sqlite3 *db, const uint64_t id, const struct sockaddr_storage *addr, const int type, const int server);
struct sockaddr_storage* GScale_Backend_Berkleysockets_Database_getNextBroadcastPort(sqlite3 *db, struct sockaddr_storage *addr);
int32_t GScale_Backend_Berkleysockets_Database_getNextMissingServer(sqlite3 *db, int32_t previd, struct sockaddr_storage *soaddr, int *type, GScale_FutureResult *fresult);

#endif /* BACKEND_BERKLEYSOCKETS_DATABASE_H_ */
