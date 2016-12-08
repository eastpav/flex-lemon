/*
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include "syntax-tree.h"
#include "sttype-test.h"


#define STNODE_MAGIC	0xe9b00b9e

stnode_t*
stnode_new(sttype_id_t type_id, gpointer data)
{
	stnode_t	*node;
	node = (stnode_t*)malloc(sizeof(stnode_t));
	node->magic = STNODE_MAGIC;
	node->deprecated_token = NULL;
	node->inside_brackets = FALSE;

	if (type_id == STTYPE_UNINITIALIZED) {
		node->type = 0;
		node->data = NULL;
	}
	else {
		node->type = type_id;
		if (type_id == STTYPE_STRING 
			|| type_id == STTYPE_FIELD
			|| type_id == STTYPE_UNPARSED ) {
			node->data = g_strdup((char*) data);
		}
		else if (type_id == STTYPE_TEST){
		    node->data = test_new(data);
		} else {
		    node->data = data;
		}

	}

	return node;
}

void
stnode_set_bracket(stnode_t *node, gboolean bracket)
{
	node->inside_brackets = bracket;
}

stnode_t*
stnode_dup(const stnode_t *org)
{
	stnode_t	*node;

	if (!org)
		return NULL;

	sttype_id_t type = org->type;

	node = (stnode_t*)malloc(sizeof(stnode_t));
	node->magic = STNODE_MAGIC;
	node->deprecated_token = NULL;
	node->type = type;
	if (type == STTYPE_STRING 
		|| type == STTYPE_FIELD
		|| type == STTYPE_UNPARSED ) {
		node->data = g_strdup((char*) org->data);
	} else if (type == STTYPE_TEST){
	    node->data = test_dup(org->data);
	} else {
	    node->data = org->data;
	}
		
	node->value = org->value;
	node->inside_brackets = org->inside_brackets;

	return node;
}

void
stnode_init(stnode_t *node, sttype_id_t type_id, gpointer data)
{
	assert_magic(node, STNODE_MAGIC);
	g_assert(!node->data);

	node->type = type_id;
	if (type_id == STTYPE_STRING 
		|| type_id == STTYPE_FIELD
		|| type_id == STTYPE_UNPARSED ) {
	    node->data = g_strdup((char*)data);
	} else if(type_id == STTYPE_TEST){
	    node->data = test_new(data);
	} else {
	    node->data = data;
	}
}

void
stnode_init_int(stnode_t *node, sttype_id_t type_id, gint32 value)
{
	stnode_init(node, type_id, NULL);
	node->value = value;
}

void
stnode_free(stnode_t *node)
{
	assert_magic(node, STNODE_MAGIC);
	if (node->type == STTYPE_STRING 
		|| node->type == STTYPE_FIELD
		|| node->type == STTYPE_UNPARSED ) {
		g_free(node->data);
		
	} else if(node->type == STTYPE_TEST){
	    test_free(node->data);
	} else {
		g_assert(!node->data);
	}
	free(node);
}

const char*
stnode_type_name(stnode_t *node)
{
    char* name = NULL;

    assert_magic(node, STNODE_MAGIC);
    switch(node->type)
    {
        case STTYPE_STRING:
            name = "STRING";
	break;
	case STTYPE_FIELD:
	    name = "FIELD";
	break;
        case STTYPE_UNPARSED:
	    name = "UNPARSED";
	default:
	    name = "UNINITIALIZED";
	break;
    }
    return name;
}

sttype_id_t
stnode_type_id(stnode_t *node)
{
	assert_magic(node, STNODE_MAGIC);
	return node->type;
}

gpointer
stnode_data(stnode_t *node)
{
	assert_magic(node, STNODE_MAGIC);
	return node->data;
}

gint32
stnode_value(stnode_t *node)
{
	assert_magic(node, STNODE_MAGIC);
	return node->value;
}

const char *
stnode_deprecated(stnode_t *node)
{
	if (!node) {
		return NULL;
	}
	return node->deprecated_token;
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
