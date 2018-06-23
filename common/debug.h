#define prerr(_fmt, ...) \
	fprintf(stderr, C_RED "Error: " C_RST _fmt "\n", ##__VA_ARGS__);

#define prinfo(_fmt, ...) \
	fprintf(stderr, C_BLUE "Info: " C_RST _fmt "\n", ##__VA_ARGS__);

#define WHERE fprintf(stderr,"[WHERE]%s: %d\n", __FILE__, __LINE__);
