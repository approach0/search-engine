#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include "mhook/mhook.h"

void traverse(xmlNodePtr cur, int level)
{
	while (cur != NULL) {
		if (xmlIsBlankNode(cur)) {
			;
		} else {
			if (xmlNodeIsText(cur)) {
				printf("%*s '%s'", level*2, "", cur->content);
			} else {
				printf("%*s <%s>", level*2, "", cur->name);
			}
			printf(" level#%d\n", level);
		}
		traverse(cur->xmlChildrenNode, level + 1);
		cur = cur->next;
	}
}

int main()
{
	xmlNodePtr cur;
	xmlDocPtr doc = xmlParseFile("presentation.math.xml");
	xmlDocDump(stderr, doc);

	cur = xmlDocGetRootElement(doc);

	traverse(cur, 0);

	xmlFreeDoc(doc);

	mhook_print_unfree();
	return 0;
}
