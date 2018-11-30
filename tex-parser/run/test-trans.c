#include "head.h"

int main()
{
	enum token_id  t_id = T_NIL;
	enum symbol_id s_id = S_NIL;

	for (s_id = S_NIL; s_id <= SYMBOL_ID_VOLUME; s_id++) {
		if (s_id > SYMBOL_ID_NUM_BEGIN && s_id < SYMBOL_ID_NUM_END)
			continue;
		printf("s_id=%d: %s\n", s_id, trans_symbol(s_id));
	}
	printf("\n");

	for (t_id = T_NIL; t_id < T_N + 2; t_id++) {
		printf("t_id=%d: %s\n", t_id, trans_token(t_id));
	}
}
