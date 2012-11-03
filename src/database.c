/*
 * database.c
 *
 */

#ifndef DATABASE_C_
#define DATABASE_C_

#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "util/log.h"
#include "database.h"

void
GScale_Database_Init(sqlite3 **db){
    int rval = SQLITE_OK;
    int i=0;
    char *statements[] = {
            "BEGIN",
            "CREATE TABLE nodes (uuid TEXT, gnspname TEXT, gname TEXT, alias TEXT, hostuuid TEXT, nodeptr BLOB, addedsecond INTEGER, addedmicrosecond INTEGER, remote INTEGER, PRIMARY KEY (uuid, hostuuid))",
            "CREATE INDEX nodes_uuid_idx ON nodes(uuid)",
            "CREATE INDEX nodes_gnspname_idx ON nodes(gnspname)",
            "CREATE INDEX nodes_gname_idx ON nodes(gname)",
            "CREATE INDEX nodes_alias_idx ON nodes(alias)",
            "CREATE INDEX nodes_hostuuid_idx ON nodes(hostuuid)",
            "CREATE INDEX nodes_addedsecond_idx ON nodes(addedsecond)",
            "CREATE INDEX nodes_addedmicrosecond_idx ON nodes(addedmicrosecond)",
            "CREATE INDEX nodes_remote_idx ON nodes(remote)",
            "COMMIT",
            NULL
    };

    if(db==NULL) ThrowInvalidArgumentException();

    if((rval=sqlite3_open(":memory:",db))!=SQLITE_OK)
    {
        *db = NULL;
        ThrowException(sqlite3_errmsg(*db), 0);
    }

    for(i=0;statements[i]!=NULL;i++){
        if(sqlite3_exec(*db, statements[i], 0,0,0)!=SQLITE_OK){
            const char *errmsg = sqlite3_errmsg(*db);
            sqlite3_exec(*db, "ROLLBACK", 0,0,0);

            sqlite3_close(*db);
            *db = NULL;
            ThrowException(errmsg, 0);
        }
    }
}
void
GScale_Database_Shutdown(sqlite3 *db){
    if(db==NULL){ return; }
    sqlite3_close(db);
}

int64_t GScale_Database_CountTable(sqlite3 *db, const char *tablename, const char *where){
    sqlite3_stmt *stmt = NULL;
    int count = 0;
    char query[1024] = {'\0'};

    if (db == NULL || tablename==NULL) ThrowInvalidArgumentException();

    if(where==NULL){
        where = "1=1";
    }
    snprintf(query, sizeof(query)-1, "SELECT count(*) FROM %s WHERE %s",tablename, where);
    if(sqlite3_prepare_v2(db,query,-1,&stmt,0)!=SQLITE_OK){
        const char *errmsg = sqlite3_errmsg(db);

        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);

        ThrowException(errmsg, 0);
    }
    if(sqlite3_step(stmt)==SQLITE_ROW){
        count = sqlite3_column_int(stmt,0);
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return count;
}

/****************************** BEGIN node operations ***********************************/

GScale_Node*
GScale_Database_getNode(sqlite3 *db, const char *where, const char *orderby){
    GScale_Node *next = NULL;
    char query[1024] = {'\0'};
    sqlite3_stmt *stmt = NULL;

    if(db==NULL) ThrowInvalidArgumentException();

    snprintf(query, sizeof(query)-1, "SELECT nodeptr FROM nodes");
    if(where!=NULL){
        snprintf(query+strlen(query),sizeof(query)-1-strlen(query)," WHERE %s",where);
    }
    if(orderby!=NULL){
        snprintf(query+strlen(query),sizeof(query)-1-strlen(query)," ORDER BY %s",orderby);
    }
    snprintf(query+strlen(query),sizeof(query)-1-strlen(query)," LIMIT 1");

    /*GSCALE_DEBUGP("getnode: %s\n", query);*/

    if(sqlite3_prepare_v2(db,query,-1,&stmt,0)!=SQLITE_OK){
        ThrowException(sqlite3_errmsg(db), 0);
    }
    if(sqlite3_step(stmt)==SQLITE_ROW)
    {
    	memcpy(&next, sqlite3_column_blob(stmt,0), sizeof(GScale_Node*));
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return next;
}

GScale_Node*
GScale_Database_getNodeByUUID(const GScale_Group *g, const GScale_Node *node, const int remote,
                              const GScale_Group_Namespace *nspname, const GScale_Group_Name *name,
                              const GScale_Node_UUID *nodeuuid, const GScale_Host_UUID *hostuuid)
{
    /*struct timeval tv = {0,0};*/
    char where[1024] = "1=1";

    if(g==NULL) ThrowInvalidArgumentException();
/*
    if(node!=NULL){
        tv = node->added;
    }
*/
    if(nspname!=NULL){
        snprintf(where+strlen(where), sizeof(where)-1-strlen(where), " AND gnspname='%s'",*nspname);
    }
    if(name!=NULL){
        snprintf(where+strlen(where), sizeof(where)-1-strlen(where), " AND gname='%s'",*name);
    }
    if(nodeuuid!=NULL){
        snprintf(where+strlen(where), sizeof(where)-1-strlen(where), " AND uuid='%s'",UUID_toString(*nodeuuid));
    }
    if(hostuuid!=NULL){
        snprintf(where+strlen(where), sizeof(where)-1-strlen(where), " AND hostuuid='%s'",UUID_toString(*hostuuid));
    }

    /*snprintf(where+strlen(where), sizeof(where)-1-strlen(where), "  AND remote=%d AND (addedsecond>%lu OR (addedsecond>=%lu AND addedmicrosecond>%lu))", remote, tv.tv_sec,tv.tv_sec,tv.tv_usec);*/
    snprintf(where+strlen(where), sizeof(where)-1-strlen(where), "  AND remote=%d", remote);
    node = GScale_Database_getNode(g->dbhandle, where, "addedsecond,addedmicrosecond DESC");

    return (GScale_Node*)node;
}

void
GScale_Database_updateNode(sqlite3 *db, GScale_Node *node){
    sqlite3_stmt* stmt = NULL;
    int rval = 0;
    const char *errmsg = NULL;
    char *struuid = NULL;

    if(db==NULL || node==NULL) ThrowInvalidArgumentException();

    sqlite3_prepare_v2(db, "UPDATE nodes SET gnspname=?1, gname=?2, alias=?3, hostuuid=?4, nodeptr=?5, addedsecond=?6, addedmicrosecond=?7, remote=?8 WHERE uuid=?9", -1, &stmt, NULL);
    if(node->flags.remote){
        sqlite3_bind_text(stmt, 1, node->locality.remote.id.id.gid.nspname, sizeof(node->locality.remote.id.id.gid.nspname), SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, node->locality.remote.id.id.gid.name, sizeof(node->locality.remote.id.id.gid.name), SQLITE_STATIC);
        sqlite3_bind_null(stmt, 3);
        struuid = UUID_toString(node->locality.remote.id.hostuuid);
        sqlite3_bind_text(stmt, 8, struuid, sizeof(GScale_StrUUID), SQLITE_TRANSIENT);

        struuid = UUID_toString(node->locality.remote.id.id.uuid);
        sqlite3_bind_text(stmt, 8, struuid, sizeof(GScale_StrUUID), SQLITE_TRANSIENT);
    }
    else{
        sqlite3_bind_text(stmt, 1, node->locality.local.id.gid.nspname, sizeof(node->locality.local.id.gid.nspname), SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, node->locality.local.id.gid.name, sizeof(node->locality.local.id.gid.name), SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, node->locality.local.id.alias, sizeof(node->locality.local.id.alias), SQLITE_STATIC);
        sqlite3_bind_null(stmt, 4);

        struuid = UUID_toString(node->locality.local.id.uuid);
        sqlite3_bind_text(stmt, 8, struuid, sizeof(GScale_StrUUID), SQLITE_TRANSIENT);
    }

    sqlite3_bind_blob(stmt, 5, &node, sizeof(GScale_Node*), SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 6, node->added.tv_sec);
    sqlite3_bind_int64(stmt, 7, node->added.tv_usec);
    sqlite3_bind_int(stmt, 8, node->flags.remote==GSCALE_NO ? GSCALE_NO : GSCALE_YES);

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
GScale_Database_insertNode(sqlite3 *db, GScale_Node *node){
    sqlite3_stmt* stmt = NULL;
    int rval = 0;
    const char *errmsg = NULL;
    char *struuid = NULL;

    if(db==NULL || node==NULL) ThrowInvalidArgumentException();

    sqlite3_prepare_v2(db, "INSERT INTO nodes (gnspname, gname, uuid, alias, nodeptr, addedsecond, addedmicrosecond, remote, hostuuid) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)", -1, &stmt, NULL);
    if(node->flags.remote){
        sqlite3_bind_text(stmt, 1, node->locality.remote.id.id.gid.nspname, sizeof(node->locality.remote.id.id.gid.nspname), SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, node->locality.remote.id.id.gid.name, sizeof(node->locality.remote.id.id.gid.name), SQLITE_STATIC);
        sqlite3_bind_null(stmt, 4);

        struuid = UUID_toString(node->locality.remote.id.hostuuid);
        sqlite3_bind_text(stmt, 9, struuid, sizeof(GScale_StrUUID), SQLITE_TRANSIENT);

        struuid = UUID_toString(node->locality.remote.id.id.uuid);
        sqlite3_bind_text(stmt, 3, struuid, sizeof(GScale_StrUUID), SQLITE_TRANSIENT);
    }
    else{
        sqlite3_bind_text(stmt, 1, node->locality.local.id.gid.nspname, sizeof(node->locality.local.id.gid.nspname), SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, node->locality.local.id.gid.name, sizeof(node->locality.local.id.gid.name), SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, node->locality.local.id.alias, sizeof(node->locality.local.id.alias), SQLITE_STATIC);

        struuid = UUID_toString(node->locality.local.id.uuid);
        sqlite3_bind_text(stmt, 3, struuid, sizeof(GScale_StrUUID), SQLITE_TRANSIENT);

        sqlite3_bind_null(stmt, 9);
    }

    sqlite3_bind_blob(stmt, 5, &node, sizeof(GScale_Node*), SQLITE_STATIC);

    sqlite3_bind_int64(stmt, 6, node->added.tv_sec);
    sqlite3_bind_int64(stmt, 7, node->added.tv_usec);
    sqlite3_bind_int(stmt, 8, node->flags.remote==GSCALE_NO ? GSCALE_NO : GSCALE_YES);

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
GScale_Database_deleteNode(sqlite3 *db, GScale_Node *node){
    char query[1024] = {'\0'};
    char *struuid = NULL;

    if(db==NULL || node==NULL) ThrowInvalidArgumentException();

    if(node->flags.remote){
        struuid = UUID_toString(node->locality.remote.id.id.uuid);
        snprintf(query, sizeof(query)-1, "DELETE FROM nodes WHERE uuid='%s'", struuid);
    }
    else{
    	struuid = UUID_toString(node->locality.local.id.uuid);
        snprintf(query, sizeof(query)-1, "DELETE FROM nodes WHERE uuid='%s'", struuid);
    }

    /*GSCALE_DEBUGP("delete query: %s", query);*/
    if(sqlite3_exec(db, query, NULL , NULL, NULL)!=SQLITE_OK){
        ThrowException(sqlite3_errmsg(db), 0);
    }
}

/****************************** END Node operations ***********************************/

void GScale_Database_PrintDBTable(sqlite3 *db, const char *tablename){
    sqlite3_stmt *stmt;
    char query[512]= {'\0'};
    int cols = 0, col=0;
    int rowcount = 0;
    int i=0, length=0;
    const char *buf = NULL;

    snprintf(query, sizeof(query)-1, "SELECT ROWID,* FROM %s", tablename);
    if(sqlite3_prepare_v2(db,query,-1,&stmt,0)!=SQLITE_OK)
    {
        printf("Select failed: %s\n",sqlite3_errmsg(db));
        return;
    }

    cols = sqlite3_column_count(stmt);
    while(sqlite3_step(stmt)==SQLITE_ROW)
    {
        rowcount++;

        /*printf("rowid: %s\t",sqlite3_column_text(stmt, ROWID));*/
        for(col=0 ; col<cols;col++)
        {
            buf = (const char*)sqlite3_column_text(stmt,col);
            length = sqlite3_column_bytes(stmt,col);

            printf("%s (%d) = ",sqlite3_column_name(stmt,col), length);
            for(i=0;i<length;i++){
                if ((buf[i] >= 0 && buf[i] <=31) || (buf[i] == 127)) {
                    printf("\\%o", buf[i]);
                }
                else {
                    printf("%c", buf[i]);
                }
            }
            printf("\t");
        }
        printf("\n");
    }
    printf("printed %d rows and %d cols\n", rowcount, cols);

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

}

#endif /* DATABASE_C_ */
