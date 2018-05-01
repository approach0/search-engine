#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "head.h"
#include "y.tab.h"

static
void _debug_print(xmlNodePtr cur, int level)
{
	int cur_rank = 0;
	while (cur != NULL) {
		if (xmlIsBlankNode(cur)) {
			;
		} else {
			if (xmlNodeIsText(cur)) {
				printf("%*s '%s'", level*2, "", cur->content);
			} else {
				printf("%*s <%s> rank#%d", level*2, "", cur->name, ++cur_rank);
			}
			printf(" level#%d\n", level);
		}
		_debug_print(cur->xmlChildrenNode, level + 1);
		cur = cur->next;
	}
}

static
struct optr_node *lexer_gen_node(const char *str)
{
	union YYSTYPE val;
	enum yytokentype tok;
	yylval.nd = NULL; /* important */
	yy_scan_string(str);
	tok = yylex();
	val = yylval;

	if (NULL != val.nd) {
		goto skip;
	}

	switch (tok) {
	case _L_BRACKET:
	case _R_BRACKET:
		val.nd = optr_alloc(S_bracket, T_GROUP, WC_COMMUT_OPERATOR);
		break;
	case _L_DOT:
	case _R_DOT:
		val.nd = optr_alloc(S_array, T_GROUP, WC_COMMUT_OPERATOR);
		break;
	case _L_ANGLE:
	case _R_ANGLE:
		val.nd = optr_alloc(S_angle, T_GROUP, WC_COMMUT_OPERATOR);
		break;
	case _L_SLASH:
	case _R_SLASH:
		val.nd = optr_alloc(S_slash, T_GROUP, WC_COMMUT_OPERATOR);
		break;
	case _L_HAIR:
	case _R_HAIR:
		val.nd = optr_alloc(S_hair, T_GROUP, WC_COMMUT_OPERATOR);
		break;
	case _L_ARROW:
	case _R_ARROW:
		val.nd = optr_alloc(S_arrow, T_GROUP, WC_COMMUT_OPERATOR);
		break;
	case _L_TEX_BRACE: /* I doubt it can reach this case */
	case _R_TEX_BRACE:
		val.nd = NULL;
		break;
	case _L_TEX_BRACKET:
	case _R_TEX_BRACKET:
		val.nd = optr_alloc(S_bracket, T_GROUP, WC_COMMUT_OPERATOR);
		break;
	case _OVER:
	case _OF:
	case _QVAR:
	case _BEGIN_MAT:
	case _END_MAT:
	case _STACKREL:
	case _BUILDREL:
	case _SET_REL:
		val.nd = NULL;
		break;
	default:
		/* "ab" will result a empty <mo></mo> */
		val.nd = optr_alloc(S_times, T_TIMES, WC_COMMUT_OPERATOR); /* important */
	}

skip:
	/* reset yylval to ensure next it is NULL */
	yylex_destroy();

	// printf("scan: %s\n", str);
	// optr_print(val.nd, stdout);

	return val.nd;
}


static void
lift_and_replace_parent(struct optr_node *parent, char *op_name)
{
	struct optr_node* op_nd = lexer_gen_node(op_name);
	if (op_nd == NULL)
		return;
	parent->symbol_id = op_nd->symbol_id;
	parent->token_id  = op_nd->token_id;
	parent->wildcard  = op_nd->wildcard;
	optr_release(op_nd);
}

static char *get_tag_str(xmlNodePtr cur, int rank)
{
	int cur_rank = 0;
	while (cur != NULL) {
		if (xmlIsBlankNode(cur)) {
			;
		} else {
			if (xmlNodeIsText(cur)) {
				return NULL;
			} else {
				char *tag = (char*)cur->name;
				cur_rank += 1;
				if (cur_rank == rank) {
					return tag;
				}
			}
		}
		cur = cur->next;
	}
	return NULL;
}

static struct optr_node *
mathml2opt(xmlNodePtr cur, struct optr_node *parent, int rank)
{
	int cur_rank = 0;
	while (cur != NULL) {
		if (xmlIsBlankNode(cur)) {
			;
		} else {
			if (xmlNodeIsText(cur)) {
				struct optr_node *leaf;
				leaf = lexer_gen_node((char*)cur->content);
				if (leaf)
					optr_attach(leaf, parent);
			} else {
				const char *tag = (char*)cur->name;
				cur_rank += 1;
				if (rank && cur_rank != rank) {
					goto skip;
				}
				if (0 == strcmp(tag, "math")) {
					mathml2opt(cur->xmlChildrenNode, parent, 0);
				} else if (0 == strcmp(tag, "mpadded")) {
					mathml2opt(cur->xmlChildrenNode, parent, 0);
				} else if (0 == strcmp(tag, "mfrac")) {
					struct optr_node *frac;
					frac = optr_alloc(S_frac, T_FRAC, WC_NONCOM_OPERATOR);
					mathml2opt(cur->xmlChildrenNode, frac, 0);
					optr_attach(frac, parent);
				} else if (0 == strcmp(tag, "mroot")) {
					struct optr_node *root;
					root = optr_alloc(S_root, T_ROOT, WC_NONCOM_OPERATOR);
					mathml2opt(cur->xmlChildrenNode, root, 1);
					mathml2opt(cur->xmlChildrenNode, root, 2);
					/* attach hanger to parent */
					optr_attach(root, parent);
				} else if (0 == strcmp(tag, "msqrt")) {
					struct optr_node *root;
					root = optr_alloc(S_root, T_ROOT, WC_NONCOM_OPERATOR);
					mathml2opt(cur->xmlChildrenNode, root, 1);
					/* attach hanger to parent */
					optr_attach(root, parent);
				//} else if (0 == strcmp(tag, "mfenced")) {
				} else if (0 == strcmp(tag, "mrow")) {
					struct optr_node *times; /* it is assuming TIMES by default */
					times = optr_alloc(S_times, T_TIMES, WC_COMMUT_OPERATOR);
					mathml2opt(cur->xmlChildrenNode, times, 0);
					optr_attach(times, parent);
				} else if (0 == strcmp(tag, "msup")) {
					struct optr_node *hanger, *base, *sup;
					hanger = optr_alloc(S_hanger, T_HANGER, WC_COMMUT_OPERATOR);
					/* base */
					base = optr_alloc(S_base, T_BASE, WC_COMMUT_OPERATOR);
					optr_attach(base, hanger);
					mathml2opt(cur->xmlChildrenNode, base, 1);
					/* superscript */
					sup = optr_alloc(S_supscript, T_SUPSCRIPT, WC_COMMUT_OPERATOR);
					optr_attach(sup, hanger);
					mathml2opt(cur->xmlChildrenNode, sup, 2);
					/* attach hanger to parent */
					optr_attach(hanger, parent);
				} else if (0 == strcmp(tag, "msub")) {
					struct optr_node *hanger, *base, *sub;
					hanger = optr_alloc(S_hanger, T_HANGER, WC_COMMUT_OPERATOR);
					/* base */
					base = optr_alloc(S_base, T_BASE, WC_COMMUT_OPERATOR);
					optr_attach(base, hanger);
					mathml2opt(cur->xmlChildrenNode, base, 1);
					/* subscript */
					sub = optr_alloc(S_subscript, T_SUBSCRIPT, WC_COMMUT_OPERATOR);
					optr_attach(sub, hanger);
					mathml2opt(cur->xmlChildrenNode, sub, 2);
					/* attach hanger to parent */
					optr_attach(hanger, parent);
				} else if (0 == strcmp(tag, "msubsup")) {
					struct optr_node *hanger, *base, *sup, *sub;
					hanger = optr_alloc(S_hanger, T_HANGER, WC_COMMUT_OPERATOR);
					/* base */
					base = optr_alloc(S_base, T_BASE, WC_COMMUT_OPERATOR);
					mathml2opt(cur->xmlChildrenNode, base, 1);
					optr_attach(base, hanger);
					/* subscript */
					sub = optr_alloc(S_subscript, T_SUBSCRIPT, WC_COMMUT_OPERATOR);
					optr_attach(sub, hanger);
					mathml2opt(cur->xmlChildrenNode, sub, 2);
					/* superscript */
					sup = optr_alloc(S_supscript, T_SUPSCRIPT, WC_COMMUT_OPERATOR);
					optr_attach(sup, hanger);
					mathml2opt(cur->xmlChildrenNode, sup, 3);
					/* attach hanger to parent */
					optr_attach(hanger, parent);
				} else if (0 == strcmp(tag, "munderover")) {
					struct optr_node *hanger, *base, *sup, *sub;
					hanger = optr_alloc(S_hanger, T_HANGER, WC_COMMUT_OPERATOR);
					/* base */
					base = optr_alloc(S_base, T_BASE, WC_COMMUT_OPERATOR);
					optr_attach(base, hanger);
					mathml2opt(cur->xmlChildrenNode, base, 1);
					/* subscript */
					sub = optr_alloc(S_subscript, T_SUBSCRIPT, WC_COMMUT_OPERATOR);
					optr_attach(sub, hanger);
					mathml2opt(cur->xmlChildrenNode, sub, 2);
					/* superscript */
					sup = optr_alloc(S_supscript, T_SUPSCRIPT, WC_COMMUT_OPERATOR);
					optr_attach(sup, hanger);
					mathml2opt(cur->xmlChildrenNode, sup, 3);
					/* attach hanger to parent */
					optr_attach(hanger, parent);
				} else if (0 == strcmp(tag, "munder")) {
					struct optr_node *hanger, *base, *sub;
					hanger = optr_alloc(S_hanger, T_HANGER, WC_COMMUT_OPERATOR);
					/* base */
					base = optr_alloc(S_base, T_BASE, WC_COMMUT_OPERATOR);
					optr_attach(base, hanger);
					mathml2opt(cur->xmlChildrenNode, base, 1);
					/* subscript */
					sub = optr_alloc(S_subscript, T_SUBSCRIPT, WC_COMMUT_OPERATOR);
					optr_attach(sub, hanger);
					mathml2opt(cur->xmlChildrenNode, sub, 2);
					/* attach hanger to parent */
					optr_attach(hanger, parent);
				} else if (0 == strcmp(tag, "mmultiscripts")) {
					struct optr_node *hanger, *base, *sup, *sub;
					char *sep_tag = get_tag_str(cur->xmlChildrenNode, 2);
					hanger = optr_alloc(S_hanger, T_HANGER, WC_COMMUT_OPERATOR);
					/* base */
					base = optr_alloc(S_base, T_BASE, WC_COMMUT_OPERATOR);
					optr_attach(base, hanger);
					mathml2opt(cur->xmlChildrenNode, base, 1);
					/* test special syntax of mmultiscripts */
					if (0 == strcmp(sep_tag, "mprescripts")) {
						/* pre-subscript */
						sub = optr_alloc(S_subscript, T_PRE_SUBSCRIPT, WC_COMMUT_OPERATOR);
						optr_attach(sub, hanger);
						mathml2opt(cur->xmlChildrenNode, sub, 3);
						/* pre-superscript */
						sup = optr_alloc(S_supscript, T_PRE_SUPSCRIPT, WC_COMMUT_OPERATOR);
						optr_attach(sup, hanger);
						mathml2opt(cur->xmlChildrenNode, sup, 4);
					} else {
						sep_tag = get_tag_str(cur->xmlChildrenNode, 4);
						if (0 == strcmp(sep_tag, "mprescripts")) {
							/* subscript */
							sub = optr_alloc(S_subscript, T_SUBSCRIPT, WC_COMMUT_OPERATOR);
							optr_attach(sub, hanger);
							mathml2opt(cur->xmlChildrenNode, sub, 2);
							/* superscript */
							sup = optr_alloc(S_supscript, T_SUPSCRIPT, WC_COMMUT_OPERATOR);
							optr_attach(sup, hanger);
							mathml2opt(cur->xmlChildrenNode, sup, 3);
							/* pre-subscript */
							sub = optr_alloc(S_subscript, T_PRE_SUBSCRIPT, WC_COMMUT_OPERATOR);
							optr_attach(sub, hanger);
							mathml2opt(cur->xmlChildrenNode, sub, 5);
							/* pre-superscript */
							sup = optr_alloc(S_supscript, T_PRE_SUPSCRIPT, WC_COMMUT_OPERATOR);
							optr_attach(sup, hanger);
							mathml2opt(cur->xmlChildrenNode, sup, 6);
						}
					}
					/* attach hanger to parent */
					optr_attach(hanger, parent);
				} else if (0 == strcmp(tag, "mo")) {
					char *op_name = (char*)cur->xmlChildrenNode->content;
					lift_and_replace_parent(parent, op_name);
				} else if (0 == strcmp(tag, "mi")) {
					mathml2opt(cur->xmlChildrenNode, parent, 0);
				} else if (0 == strcmp(tag, "mn")) {
					mathml2opt(cur->xmlChildrenNode, parent, 0);
				} else {
					fprintf(stderr, "unable to handle <%s>\n", tag);
				}
skip:
				;
			}
		}
		cur = cur->next;
	}

	return NULL;
}

struct optr_node *mathml_parse_file(const char *fname)
{
	struct optr_node *mathml_root;
	xmlNodePtr cur;
	xmlDocPtr doc = xmlParseFile(fname);
	// xmlDocDump(stdout, doc);

	cur = xmlDocGetRootElement(doc);
	{
		mathml_root = optr_alloc(S_root, T_MATHML_ROOT, WC_COMMUT_OPERATOR);
		mathml2opt(cur, mathml_root, 0);
	}

	xmlFreeDoc(doc);
	return mathml_root;
}
