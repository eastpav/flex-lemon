#ifndef __PROTO_H__
#define __PROTO_H__
#include <glib.h>

typedef enum {
    PROTO_VALUE_UNVALID,
	PROTO_VALUE_STRING,
	PROTO_VALUE_INTEGER
} proto_value_t;

gboolean protoInit(void);
gboolean protoExist(const char* fieldName);
gboolean protoTypeGetByName(const char* fieldName, proto_value_t *protoValue);
gboolean protoExplore(char* confPath);

#endif

