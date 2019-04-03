/* handy ASCII/VT100 color control sequences */
#define C_RST     "\033[0m"
#define C_RED     "\033[1m\033[31m"      /* Bold Red */
#define C_GREEN   "\033[1m\033[32m"      /* Bold Green */
#define C_BLUE    "\033[1m\033[34m"      /* Bold Blue */
#define C_MAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define C_CYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define C_GRAY    "\033[1m\033[30m"      /* Bold Gray */
#define C_BROWN   "\033[1m\033[0;33m"    /* brown */

#define ES_RESET_LINE    "\33[2K\r"      /* erase line and reset cursor */
#define ES_RESET_CONSOLE "\e[1;1H\e[2J"  /* reset console */

#define ES_INVERTED_COLOR "\e[7m"        /* invert foreground and background color */
