/* get position array callbacks */
char *getposarr_for_termpost(char*, size_t*);
char *getposarr_for_termpost_with_pos(char*, size_t*);

/* on flush callbacks */
uint32_t onflush_for_plainpost(char*, uint32_t*);
uint32_t onflush_for_termpost(char*, uint32_t*);
uint32_t onflush_for_termpost_with_pos(char*, uint32_t*);

/* on rebuf callbacks */
void onrebuf_for_plainpost(char*, uint32_t*);
void onrebuf_for_termpost(char*, uint32_t*);
void onrebuf_for_termpost_with_pos(char*, uint32_t*);
