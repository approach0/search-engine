#include "term-lookup.h"
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>

int main() 
{
   /*
   test term look up open; print all tables in the database
   */
   char* dbname = "search_engine_db";
   MYSQL *conn;
   conn = (MYSQL *)term_lookup_open(dbname);
   MYSQL_RES *res;
   MYSQL_ROW row;
   char* testQuery = "show tables";
   if (mysql_query(conn, testQuery)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   }
   res = mysql_store_result(conn);
   while ((row = mysql_fetch_row(res)) != NULL){
      printf("%s \n", row[0]);
    }
   mysql_free_result(res);
   term_id_t test1 = term_lookup(conn, L"house");
   printf("%d\n", test1);
   term_lookup_close(conn);
   /*******************************/
}
