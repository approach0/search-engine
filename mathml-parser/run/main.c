#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mhook/mhook.h"
#include "tex-parser/head.h"
#include "tex-parser/y.tab.h"

struct optr_node *lexer_gen_node(const char *str)
{
	union YYSTYPE tmp;
	yy_scan_string(str);
	(void)yylex();
	tmp = yylval;
	// optr_print(tmp.nd, stdout);
	// optr_release(tmp.nd);
	yylex_destroy();
	return tmp.nd;
}

void print(xmlNodePtr cur, int level)
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
		print(cur->xmlChildrenNode, level + 1);
		cur = cur->next;
	}
}

struct optr_node *mathml2opt(xmlNodePtr cur, struct optr_node *parent, int rank)
{
	int cur_rank = 0;
	while (cur != NULL) {
		if (xmlIsBlankNode(cur)) {
			;
		} else {
			if (xmlNodeIsText(cur)) {
				struct optr_node *leaf;
				leaf = lexer_gen_node((char*)cur->content);
				optr_attach(leaf, parent);
			} else {
				const char *tag = (char*)cur->name;
				cur_rank += 1;
				if (rank && cur_rank != rank) {
					goto skip;
				}
				if (0 == strcmp(tag, "math")) {
					mathml2opt(cur->xmlChildrenNode, parent, 0);
				} else if (0 == strcmp(tag, "mfrac")) {
					struct optr_node *frac;
					frac = optr_alloc(S_frac, T_FRAC, WC_NONCOM_OPERATOR);
					mathml2opt(cur->xmlChildrenNode, frac, 0);
					optr_attach(frac, parent);
				} else if (0 == strcmp(tag, "mrow")) {
					struct optr_node *add;
					add = optr_alloc(S_plus, T_ADD, WC_COMMUT_OPERATOR);
					mathml2opt(cur->xmlChildrenNode, add, 0);
					optr_attach(add, parent);
				} else if (0 == strcmp(tag, "msubsup")) {
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
				} else if (0 == strcmp(tag, "mo")) {
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

int main()
{
	xmlNodePtr cur;
	xmlDocPtr doc = xmlParseFile("presentation.math.xml");

	cur = xmlDocGetRootElement(doc);
	// print(cur, 0);

	{
		struct optr_node *root;
		root = optr_alloc(S_root, T_ROOT, WC_COMMUT_OPERATOR);
		mathml2opt(cur, root, 0);
		optr_print(root, stdout);
		optr_release(root);
	}

	xmlDocDump(stdout, doc);
	xmlFreeDoc(doc);

	mhook_print_unfree();
	return 0;
}
