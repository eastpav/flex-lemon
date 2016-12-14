
%include {

#include <assert.h>
#include <glib.h>

#include "filter.h"
#include "syntax-tree.h"
#include "sttype-test.h"
#include "grammar.h"

#ifdef _WIN32
#pragma warning(disable:4671)
#endif

/* End of C code */
}

/* Parser Information */
%name		Tfilter
%token_prefix	TOKEN_
%extra_argument	{dfwork_t *dfw}

/* Terminal and Non-Terminal types and destructors */
%token_type			{stnode_t*}
%token_destructor		{
	(void) dfw; /* Mark unused, similar to Q_UNUSED */
	stnode_free($$);
}

%type		sentence	{stnode_t*}


%type		expr		{stnode_t*}
%destructor	expr		{stnode_free($$);}

%type		entity		{stnode_t*}
%destructor	entity		{stnode_free($$);}

%type		relation_test	{stnode_t*}
%destructor	relation_test	{stnode_free($$);}

%type		logical_test	{stnode_t*}
%destructor	logical_test	{stnode_free($$);}

%type		rel_op2		{test_op_t}

%type		range		{stnode_t*}
%destructor	range		{stnode_free($$);}


/* This is called as soon as a syntax error happens. After that, 
any "error" symbols are shifted, if possible. */
%syntax_error {
	if (!TOKEN) {
		tfilter_fail(dfw, "Unexpected end of filter string.");
		dfw->syntax_error = TRUE;
		return;
	}

	switch(stnode_type_id(TOKEN)) {
		case STTYPE_UNINITIALIZED:
			tfilter_fail(dfw, "Syntax error.");
			break;
		case STTYPE_TEST:
			tfilter_fail(dfw, "Syntax error, TEST.");
			break;
		case STTYPE_STRING:
			tfilter_fail(dfw, "The string \"%s\" was unexpected in this context.",
				(char *)stnode_data(TOKEN));
			break;
		case STTYPE_UNPARSED:
			tfilter_fail(dfw, "\"%s\" was unexpected in this context.",
				(char *)stnode_data(TOKEN));
			break;
		case STTYPE_FIELD:
			tfilter_fail(dfw, "Syntax error near \"%s\".", (char *)stnode_data(TOKEN));
			break;
        case STTYPE_INTEGER:
		    tfilter_fail(dfw, "The integer %d was unexpected in this context.", stnode_value(TOKEN));
			break;

		/* These aren't handed to use as terminal tokens from
		   the scanner, so was can assert that we'll never
		   see them here. */
		case STTYPE_NUM_TYPES:
		case STTYPE_RANGE:
		case STTYPE_FVALUE:
			g_assert_not_reached();
			break;
	}
	dfw->syntax_error = TRUE;
}

/* When a parse fails, mark an error. This occurs after
the above syntax_error code and after the parser fails to
use error recovery, shifting an "error" symbol and successfully
shifting 3 more symbols. */
%parse_failure {
	dfw->syntax_error = TRUE;
}

/* ----------------- The grammar -------------- */

/* Associativity */
%left TEST_AND.
%left TEST_OR.
%nonassoc TEST_EQ TEST_NE TEST_LT TEST_LE TEST_GT TEST_GE .

/* Top-level targets */
sentence ::= expr(X).		{ dfw->st_root = X; }
sentence ::= .			{ dfw->st_root = NULL; }

expr(X) ::= relation_test(R).	{ X = R; }
expr(X) ::= logical_test(L).	{ X = L; }


/* Logical tests */
logical_test(T) ::= expr(E) TEST_AND expr(F).
{
	T = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set2(T, TEST_OP_AND, E, F);
}

logical_test(T) ::= expr(E) TEST_OR expr(F).
{
	T = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set2(T, TEST_OP_OR, E, F);
}

logical_test(T) ::= entity(E).
{
	T = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set1(T, TEST_OP_EXISTS, E);
}



/* Entities, or things that can be compared/tested/checked */
entity(E) ::= FIELD(F).		{ E = F; }
entity(E) ::= STRING(S).	{ E = S; }
entity(E) ::= UNPARSED(U).	{ E = U; }
entity(E) ::= INTEGER(I).	{ E = I; }


/* Relational tests */
relation_test(T) ::= entity(E) rel_op2(O) entity(F).
{
	T = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set2(T, O, E, F);
}

/* 'a == b == c' or 'a < b <= c <= d < e' */
relation_test(T) ::= entity(E) rel_op2(O) relation_test(R).
{
	stnode_t *L, *F;
	/* for now generate it like E O F  TEST_OP_AND  F P G, later it could be optimized
	   or semantically checked (to make a <= b >= c or a == b != c invalid)?
	 */

	F = R;
	do {
		g_assert(F != NULL && stnode_type_id(F) == STTYPE_TEST);
		sttype_test_get(F, NULL, &F, NULL);
	} while (stnode_type_id(F) == STTYPE_TEST);

	L = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set2(L, O, E, stnode_dup(F));

	T = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set2(T, TEST_OP_AND, L, R);
}

rel_op2(O) ::= TEST_EQ.  { O = TEST_OP_EQ; }
rel_op2(O) ::= TEST_NE.  { O = TEST_OP_NE; }
rel_op2(O) ::= TEST_GT.  { O = TEST_OP_GT; }
rel_op2(O) ::= TEST_GE.  { O = TEST_OP_GE; }
rel_op2(O) ::= TEST_LT.  { O = TEST_OP_LT; }
rel_op2(O) ::= TEST_LE.  { O = TEST_OP_LE; }

/* Any expression inside parens is simply that expression */
expr(X) ::= LPAREN expr(Y) RPAREN.
{
	X = Y;
	stnode_set_bracket(X, TRUE);
}

