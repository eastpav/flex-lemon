#ifndef __FILTER_H__
#define __FILTER_H__
#include <glib.h>
#include "syntax-tree.h"
/*
typedef struct stnode {
    int type;  //运算、整数或字符串

    typedef struct opnode{
        test_op_t op;
        struct stnode* leftHand;
        struct stnode* rightHand;
    } opnode_t;

    union {
        opnode_t* op;
        char* stringValue;
        int value;
    } data;
} stnode_t;
*/

typedef struct {
	/* Syntax Tree stuff */
	stnode_t	*st_root;
	gboolean	syntax_error;
	gchar		*error_message;
	GPtrArray	*insns;
	GPtrArray	*consts;
	GHashTable	*loaded_fields;
	GHashTable	*interesting_fields;
	int		next_insn_id;
	int		next_const_id;
	int		next_register;
	int		first_constant; /* first register used as a constant */
} dfwork_t;

typedef struct {
	dfwork_t *dfw;
	GString* quoted_string;
} tf_scanner_state_t;

/* Scanner's lval */
extern stnode_t *tf_lval;

/* Return value for error in scanner. */
#define SCAN_FAILED	-1	/* not 0, as that means end-of-input */

/* Set dfw->error_message */
void tfilter_fail(dfwork_t *dfw, const char *format, ...) G_GNUC_PRINTF(2, 3);

#endif

