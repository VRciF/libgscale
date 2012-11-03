/*
 * database.h
 *
 */

#ifndef DATABASE_H_
#define DATABASE_H_

#include "sqlite3.h"
#include "types.h"

void GScale_Database_Init(sqlite3 **db);
void GScale_Database_Shutdown(sqlite3 *db);
int64_t GScale_Database_CountTable(sqlite3 *db, const char *tablename, const char *where);

void GScale_Database_insertNode(sqlite3 *db, GScale_Node *node);
void GScale_Database_updateNode(sqlite3 *db, GScale_Node *node);
GScale_Node* GScale_Database_getNode(sqlite3 *db, const char *where, const char *orderby);
GScale_Node* GScale_Database_getNodeByUUID(const GScale_Group *g, const GScale_Node *node, const int remote,
                                           const GScale_Group_Namespace *nspname, const GScale_Group_Name *name,
                                           const GScale_Node_UUID *nodeuuid, const GScale_Host_UUID *hostuuid);
void GScale_Database_deleteNode(sqlite3 *db, GScale_Node *node);

void GScale_Database_PrintDBTable(sqlite3 *db, const char *tablename);

#endif /* DATABASE_H_ */
