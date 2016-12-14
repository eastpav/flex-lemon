#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "proto.h"

static GHashTable *g_protoNameMap = NULL;
static char *g_lastFieldName = NULL;
static char *g_lastInfo = NULL;

gboolean protoInit(void)
{
    g_protoNameMap = g_hash_table_new(g_str_hash, g_str_equal);
    
    return TRUE;
}

gboolean protoRegister(char *protoName, char *type)
{
    proto_value_t pv;

    if(g_strcmp0(type, "string") == 0)
        pv = PROTO_VALUE_STRING;
    else if(g_strcmp0(type, "integer") == 0)
        pv = PROTO_VALUE_INTEGER;
    else
        return FALSE;

    
    return g_hash_table_insert(g_protoNameMap, g_strdup(protoName), (gpointer)pv);
}

gboolean protoExist(const char* fieldName)
{
    gboolean existed = FALSE;
    if(!fieldName) return FALSE;
    
    if (g_strcmp0(fieldName, g_lastFieldName) == 0) 
    {
		return TRUE;
	}

    existed = g_hash_table_contains(g_protoNameMap, fieldName);
    if(existed)
    {
        g_free(g_lastFieldName);
        g_lastFieldName = g_strdup(fieldName);
        return TRUE;
    }

    return FALSE;
}

gboolean protoTypeGetByName(const char* fieldName, proto_value_t *protoValue)
{
    gpointer v = NULL;
    
    if(!fieldName) return FALSE;

    /* 使用g_hash_table_lookup函数时，要求key对应的value值必须大于0，否则返回0不能区分是失败还是数据 */
    v = g_hash_table_lookup(g_protoNameMap, fieldName);
    if(v)
    {
        *protoValue = (proto_value_t)v;
        return TRUE;
    }

    return FALSE;
}

gboolean protoExplore(char* confPath)
{
    FILE* fp = fopen(confPath, "r");
    if(fp)
    {
        char* line = NULL;
        char* striped = NULL;
        size_t length = 0;
        int read = 0;

        char** section = NULL;
        char** proto = NULL;

        while ((read = getline(&line, &length, fp)) != -1) {
            //printf("(%d:%ld)%s\n", read, strlen(line), line);
            section = g_strsplit(line, "#", 2);
            striped = g_strstrip(*section);

            if(strlen(striped) > 0)
            {
                proto = g_strsplit(striped, " ", 2);
                g_assert(*proto != 0 && *(proto+1) != 0);
                protoRegister(g_strstrip(*proto), g_strstrip(*(proto+1)));
                g_strfreev(proto);
                proto = NULL;
            }

            g_strfreev(section);
            section = NULL;
        }

        free(line);
        fclose(fp);
        return TRUE;
    }

    return FALSE;
}

