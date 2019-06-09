var sqlite3 = require('better-sqlite3');
var qrylog = require("./qrylog.js");

var express = require('express');
var bodyParser = require('body-parser');

app = express();
app.use(bodyParser.json());

var db = new sqlite3('qrylog.sqlite3', {verbose: console.log});
qrylog.initialize(db);

app.get('/', function (req, res) {
	res.send('Hello World!');

}).post('/push/query', (req, res) => {
	qrylog.push_query(db, req.body);
	res.json({'res': 'succussful'});

}).get('/pull/query-items/:max', (req, res) => {
	const arr = qrylog.pull_query_items(db, req.params.max);
	res.json({'res': arr});

}).get('/pull/query-items/:max/:from.:to', (req, res) => {
	const arr = qrylog.pull_query_items(db, req.params.max, {
		begin: req.params.from,
		end: req.params.to
	});
	res.json({'res': arr});

}).get('/pull/query-IPs/:max', (req, res) => {
	const arr = qrylog.pull_query_IPs(db, req.params.max);
	res.json({'res': arr});

}).get('/pull/query-IPs/:max/:from.:to', (req, res) => {
	const arr = qrylog.pull_query_IPs(db, req.params.max, {
		begin: req.params.from,
		end: req.params.to
	});
	res.json({'res': arr});

}).get('/pull/query-summary', (req, res) => {
	const arr = qrylog.pull_query_summary(db);
	res.json({'res': arr});

}).get('/pull/query-summary/:from.:to', (req, res) => {
	const arr = qrylog.pull_query_summary(db, {
		begin: req.params.from,
		end: req.params.to
	});
	res.json({'res': arr});

});

const port = 3207;
console.log('listening on ' + port);
app.listen(port);

process.on('SIGINT', function() {
	console.log('')
	console.log('closing...')
	db.close();
	process.exit();
})
