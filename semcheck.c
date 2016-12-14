#include <stdio.h>
#include "semcheck.h"
#include "filter.h"
#include "proto.h"
#include "syntax-tree.h"
#include "sttype-test.h"

gboolean semcheck(dfwork_t *dfw, stnode_t *stNode);

test_op_t flipOp(test_op_t op)
{
    switch(op)
    {
        case TEST_OP_EQ:
            return TEST_OP_EQ;
        case TEST_OP_NE:
            return TEST_OP_NE;
        case TEST_OP_GT:
            return TEST_OP_LT;
        case TEST_OP_GE:
            return TEST_OP_LE;
        case TEST_OP_LT:
            return TEST_OP_GT;
        case TEST_OP_LE:
            return TEST_OP_GE;
        default:
            g_assert_not_reached();
        break;
    }

    return op;
}

gboolean checkProtoValue(stnode_t *keynode, stnode_t *valuenode)
{
    proto_value_t pv;
    char* protoName = stnode_data(keynode);

    if(protoTypeGetByName(protoName, &pv) == TRUE)
    {
        sttype_id_t tp = stnode_type_id(valuenode);
        switch(pv)
        {
            case PROTO_VALUE_STRING:
                return ((tp == STTYPE_STRING)
                        || (tp == STTYPE_UNPARSED)
                        || (tp == STTYPE_FVALUE));
            case PROTO_VALUE_INTEGER:
                return tp == STTYPE_INTEGER;
            break;
        }
    }

    return FALSE;
}

/* exchange stArg1 stArg2 of stNode if stArg2 is FIELD*/
gboolean checkRelation(dfwork_t *dfw, test_op_t stOp, stnode_t *stNode, stnode_t *stArg1, stnode_t *stArg2)
{
    gboolean ret = FALSE;

    if((stnode_type_id(stArg1) == STTYPE_FIELD)
        && (stnode_type_id(stArg2) != STTYPE_FIELD))
    {
        ret = checkProtoValue(stArg1, stArg2);
    }
    else if((stnode_type_id(stArg1) != STTYPE_FIELD)
            && (stnode_type_id(stArg2) == STTYPE_FIELD))
    {
        if(checkProtoValue(stArg2, stArg1) == TRUE)
        {
            test_op_t exOp;
            exOp = flipOp(stOp);
            sttype_test_set2(stNode, exOp, stArg2, stArg1);
            ret = TRUE;
        }
    }
    else
        ret = FALSE;

    return ret;
}

gboolean checkTest(dfwork_t *dfw, stnode_t *stNode)
{
    test_op_t stOp, stArgOp;
	stnode_t *stArg1, *stArg2;
    gboolean ret = FALSE;

    sttype_test_get(stNode, &stOp, &stArg1, &stArg2);

    switch(stOp)
    {
        case TEST_OP_AND:
        case TEST_OP_OR:
            if(semcheck(dfw, stArg1) == TRUE)
            {
                ret = semcheck(dfw, stArg2);
            }
        break;
        case TEST_OP_EQ:
        case TEST_OP_NE:
        case TEST_OP_GT:
        case TEST_OP_GE:
        case TEST_OP_LT:
        case TEST_OP_LE:
            ret = checkRelation(dfw, stOp, stNode, stArg1, stArg2);
        break;
        default:
            g_assert_not_reached();
        break;
    }

    return ret;
}

gboolean semcheck(dfwork_t *dfw, stnode_t *stNode)
{
    gboolean ret = FALSE;

    switch(stnode_type_id(stNode))
    {
        case STTYPE_TEST:
            ret = checkTest(dfw, stNode);
            break;
        default:
            break;
    }

    return ret;
}


gboolean dfw_semcheck(dfwork_t *dfw)
{
    return semcheck(dfw, dfw->st_root);
}

