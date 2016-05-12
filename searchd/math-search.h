/* essential step before a math query can be searched */
int math_search_prepare_qry(struct subpaths*);

/* to score math query, pass a prepared query subpaths to function below */
void prepare_score_struct(struct subpaths*);
