1. Compile the code, see http://approach0.xyz:8080/docs/src/build.html
2. `cd ./qac`, and `make update`
3. run qad daemon: `./run/qacd.out`
4. index and query:
	* example can be found at `test.py`
	* change `test.py` fname to your NTCIR data file path.
	* uncomment `post_query_logs(x)` to index the first x lines
	* uncomment `test_qac_query('1+')` to issue a query
	* QAC results are returned from `qacd.out` in JSON format.
