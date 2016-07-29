#include <stdint.h>

extern uint32_t file_offset_check_cnt;
int  file_offset_check_init(const char*);
void file_offset_check_free(void);
void file_offset_check_add(uint32_t, uint32_t);
void file_offset_check_print(void);
