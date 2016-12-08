#include <stdio.h>
#include <stdlib.h>
#include "filter.h"
#include "grammar.h"
#include "scanner.h"
#include "sttype-test.h"

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
    } else {
        printf("%s\n", (char*)node->data);
    }
}

void print(dfwork_t	*dfw)
{
    stnode_t *node = dfw->st_root;

    printnode(node);
}

int main(int argc, char** argv) {
    // Set up the scanner
    yyscan_t scanner;
    YY_BUFFER_STATE in_buffer;
    tf_scanner_state_t state;
    dfwork_t	*dfw;
    gboolean failure = FALSE;

    tf_lex_init(&scanner);
    //tf_set_in(stdin, scanner);

    // Set up the parser
    void* Parser = TfilterAlloc(g_malloc);

    //char* pattern = "devid == hussdsd";
    char* pattern = "devid == hussdsd || type != tcp && port ==123";
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
        printf("Token: %d Text:(m:%x t:%d d:%s)\n", lexToken, tf_lval->magic, tf_lval->type, (char*)tf_lval->data);
        Tfilter(Parser, lexToken, tf_lval, dfw);
    } 

    if (tf_lval) {
		stnode_free(tf_lval);
		tf_lval = NULL;
	}

    Tfilter(Parser, 0, NULL, dfw);
    
    if (dfw->syntax_error)
		failure = TRUE;

    if (state.quoted_string != NULL)
		g_string_free(state.quoted_string, TRUE);

    tf__delete_buffer(in_buffer, scanner);
    // Cleanup the scanner and parser
    tf_lex_destroy(scanner);
    if (failure)
		goto FAILURE;

    if (dfw->st_root == NULL)
        printf("no valid pattern\n");
    else
        print(dfw);

    TfilterFree(Parser, g_free);

    dfwork_free(dfw);
    return 0;

FAILURE:
    if (dfw) {
		g_free(dfw->error_message);
		dfwork_free(dfw);
	}

    return -1;
}


