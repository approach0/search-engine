var sqlite3 = require('better-sqlite3');
var db = new sqlite3('qrylog.sqlite3', {verbose_: console.log});
var qrylog = require("./qrylog.js");

var express = require('express');
var bodyParser = require('body-parser');

app = express();
app.use(bodyParser.json());

app.get('/', function (req, res) {
	res.send('Hello World!');
});

const port = 3207;
console.log('listening on ' + port);
app.listen(port);
//qrylog.push_query(db, test_json);
