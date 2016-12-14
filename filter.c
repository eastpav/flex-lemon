#include <stdio.h>
#include <stdlib.h>
#include "filter.h"
#include "grammar.h"
#include "scanner.h"
#include "sttype-test.h"
#include "proto.h"
#include "semcheck.h"

#define PROTO_CONF_PATH "./proto.conf"

void* TfilterAlloc(void* (*)(gsize));
void* Tfilter(void*, int, stnode_t*, dfwork_t*);
void* TfilterFree(void*, void(*)(void*));

stnode_t *tf_lval;
void tfilter_fail(dfwork_t *dfw, const char *format, ...)
{
	va_list	args;

	/* If we've already reported one error, don't overwite it */
	if (dfw->error_message != NULL)
		return;

	va_start(args, format);
	dfw->error_message = g_strdup_vprintf(format, args);
	va_end(args);
}

static dfwork_t*
dfwork_new(void)
{
	dfwork_t	*dfw;

	dfw = g_new0(dfwork_t, 1);
	dfw->first_constant = -1;

	return dfw;
}

static void
dfwork_free(dfwork_t *dfw)
{
	if (dfw->st_root) {
		stnode_free(dfw->st_root);
	}

	/*
	 * We don't free the error message string; our caller will return
	 * it to its caller.
	 */
	g_free(dfw);
}

void printnode(stnode_t *node)
{
    if(node->type == STTYPE_TEST) {
        test_t	*test = (test_t*)stnode_data(node);
        printf("op:%s->\n", operandname(test->op));
        printnode(test->val1);
        printnode(test->val2);
    } else if(node->type == STTYPE_INTEGER){
        printf("%d\n", node->value);
    } else {
        printf("%s\n", (char*)node->data);
    }
}

void print(dfwork_t	*dfw)
{
    stnode_t *node = dfw->st_root;

    printnode(node);
}

char* testPattern[4] = {
        "[401CCCC udp 100 test1] testPattern1",
        "[401CCCC tcp 200 test2] testPattern2",
        "[402DDDD udp 300 test3] testPattern3",
        "[401CCCC tcp 300 test4] testPattern4"};

//    "devid == 401CCCC && type == tcp && port ==100";

gboolean stringTest(test_op_t op, char* val1, char* val2)
{
    gboolean ret = FALSE;
    switch(op)
    {
        case TEST_OP_EQ:
            if(strcmp(val1, val2) == 0) ret = TRUE;
        break;
        case TEST_OP_NE:
            if(strcmp(val1, val2) != 0) ret = TRUE;
        break;
        case TEST_OP_GE:
        case TEST_OP_LT:
        case TEST_OP_LE:
        default:
        break;
    }

    return ret;
}

gboolean integerTest(test_op_t op, int val1, int val2)
{
    gboolean ret = FALSE;
    switch(op)
    {
        case TEST_OP_EQ:
            if(val2 == val1) ret = TRUE;
        break;
        case TEST_OP_NE:
            if(val2 != val1) ret = TRUE;
        break;
        case TEST_OP_GT:
            if(val2 > val1) ret = TRUE;
        break;
        case TEST_OP_GE:
            if(val2 >= val1) ret = TRUE;
        break;
        case TEST_OP_LT:
            if(val2 < val1) ret = TRUE;
        break;
        case TEST_OP_LE:
            if(val2 <= val1) ret = TRUE;
        break;
        default:
        break;
    }

    return ret;
}

gboolean cmpPattern(stnode_t* keynode , stnode_t* valuenode, test_op_t op, char* devID, char* type, int port, char* test)
{
    char* key = keynode->data;
    int ivalue = 0;
    char* svalue = NULL;

    if(valuenode->type == STTYPE_INTEGER)
        ivalue = valuenode->value;
    else
        svalue = valuenode->data;

    if(strcmp(key, "devid") == 0)
    {
        return stringTest(op, svalue, devID);
    }
    else if(strcmp(key, "type") == 0)
    {
        return stringTest(op, svalue, type);
    }
    else if(strcmp(key, "port") == 0)
    {
        return integerTest(op, ivalue, port);
    }
    else if(strcmp(key, "test") == 0)
    {
        return stringTest(op, svalue, test);
    }
    else
        return FALSE;
}

gboolean testNode(stnode_t* node, char* devID, char* type, int port, char* testStr)
{
    gboolean ret = FALSE;

    if(node->type == STTYPE_TEST)
    {
        test_t	*test = (test_t*)stnode_data(node);
        switch(test->op)
        {
            case TEST_OP_EQ:
            case TEST_OP_NE:
            case TEST_OP_GT:
            case TEST_OP_GE:
            case TEST_OP_LT:
            case TEST_OP_LE:
                ret = cmpPattern(test->val1, test->val2, test->op, devID, type, port, testStr);
            break;
            case TEST_OP_AND:
                if(testNode(test->val1, devID, type, port, testStr) == TRUE)
                {
                    if(testNode(test->val2, devID, type, port, testStr) == TRUE)
                    {
                        ret = TRUE;
                    }
                    else
                    {
                        ret = FALSE;
                    }
                }
                else
                {
                    ret = FALSE;
                }
            break;
            case TEST_OP_OR:
                if(testNode(test->val1, devID, type, port, testStr) == TRUE)
                {
                    ret = TRUE;
                }
                else
                {
                    if(testNode(test->val2, devID, type, port, testStr) == TRUE)
                    {
                        ret = TRUE;
                    }
                    else
                    {
                        ret = FALSE;
                    } 
                }
            break;
            default:
            break;
        }
    }
    else
    {
        // 数据节点直接返回TRUE
        return TRUE;
    }

    return ret;
}

gboolean doFilter(dfwork_t* dfw, char* instance)
{
    char devID[20] = {0,};
    char type[4] = {0,};
    int port = 0;
    char test[8] = {0,};

    sscanf(instance, "[%s %s %d %s] %*s", devID, type, &port, test);

    return testNode(dfw->st_root, devID, type, port, test);
}

void doFilterTest(dfwork_t* dfw, char** testPattern)
{
    gboolean ret = FALSE;

    int i;
    for(i = 0; i < 4; i++)
    {
        ret = doFilter(dfw, testPattern[i]);
        if(ret == TRUE) {
            printf("%s\n", testPattern[i]);
        }
    }
    
}

int main(int argc, char** argv) {
    // Set up the scanner
    yyscan_t scanner;
    YY_BUFFER_STATE in_buffer;
    tf_scanner_state_t state;
    dfwork_t	*dfw;
    gboolean failure = FALSE;

    g_assert(protoInit() == TRUE);

    g_assert(access(PROTO_CONF_PATH, F_OK) == 0);

    g_assert(protoExplore(PROTO_CONF_PATH) == TRUE);
    

    tf_lex_init(&scanner);
    //tf_set_in(stdin, scanner);

    // Set up the parser
    void* Parser = TfilterAlloc(g_malloc);

    //char* pattern = "devid == hussdsd";
    char* pattern = "type != \"tcp\" && 200 >= port";
    // Do it!
    in_buffer = tf__scan_string(pattern, scanner);
    dfw = dfwork_new();
    state.dfw = dfw;
	state.quoted_string = NULL;

    tf_set_extra(&state, scanner);

    int lexToken;
    char* lexText = NULL;
    while(1) {

        tf_lval = stnode_new(STTYPE_UNINITIALIZED, NULL);
        lexToken = tf_lex(scanner);

        if (lexToken == 0) break;
        if (-1 == lexToken) {
            failure = TRUE;
            fprintf(stderr, "The scanner encountered an error.\n");
            break;
        }
        //lexText = tf_get_text(scanner);
        if(tf_lval->type == STTYPE_INTEGER)
            printf("Token: %d Text:(m:%x t:%d i:%d)\n", lexToken, tf_lval->magic, tf_lval->type, tf_lval->value);
        else
            printf("Token: %d Text:(m:%x t:%d d:%s)\n", lexToken, tf_lval->magic, tf_lval->type, (char*)tf_lval->data);
        Tfilter(Parser, lexToken, tf_lval, dfw);
    } 

    if (tf_lval) {
		stnode_free(tf_lval);
		tf_lval = NULL;
	}

    Tfilter(Parser, 0, NULL, dfw);
    
    if (dfw->syntax_error)
    {
        //printf("syntax_error\n");
		failure = TRUE;
    }
    if (state.quoted_string != NULL)
		g_string_free(state.quoted_string, TRUE);

    tf__delete_buffer(in_buffer, scanner);
    // Cleanup the scanner and parser
    tf_lex_destroy(scanner);
    if (failure)
		goto FAILURE;

    if (dfw->st_root == NULL)
    {
        printf("no valid pattern\n");
    }
    else
    {
        if(!dfw_semcheck(dfw))
        {
            printf("%s parse failed, please make sure the input correct\n", pattern);
            goto FAILURE;
        }
        print(dfw);
        doFilterTest(dfw, testPattern);
    }

    TfilterFree(Parser, g_free);

    dfwork_free(dfw);
    return 0;

FAILURE:
    if (dfw) {
        printf("%s\n", dfw->error_message);
		g_free(dfw->error_message);
		dfwork_free(dfw);
	}

    return -1;
}


