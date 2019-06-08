function push_query(db, qry_json) {
	db.prepare(`CREATE TABLE IF NOT EXISTS
		query (time TEXT, ip STRING, page INT, id INTEGER PRIMARY KEY)`).run();
	db.prepare(`CREATE TABLE IF NOT EXISTS
		keyword (str STRING, type STRING, op STRING, qryID INTEGER,
		FOREIGN KEY(qryID) REFERENCES query(id))`).run();

	const now = new Date();

	const info = db.prepare(
		"insert into query (time, ip, page) values (?, ?, ?)"
	).run(now.toISOString(), qry_json['ip'], qry_json['page']);

	for (var i = 0; i < qry_json.kw.length; i++) {
		const kw = qry_json.kw[i];
		const stmt = db.prepare(`INSERT INTO keyword (str, type, op, qryID)
			VALUES (?, ?, ?, ?)`);

		stmt.run(kw['str'], kw['type'], 'OR', info.lastInsertRowid);
	}
}

function pull_query_items(db, max, date_range) {
	if (date_range === undefined) date_range = {};
	const date_begin = date_range['begin'] || '0000-01-01';
	const date_end   = date_range['end']   || '9999-12-31';
	return db.prepare(
		`SELECT id, time, ip, page, json_group_array(str) as kw
		FROM query INNER JOIN keyword ON query.id = keyword.qryID
		WHERE date(time) BETWEEN ? AND ?
		GROUP BY id ORDER BY id DESC LIMIT ?`)
		.all(date_begin, date_end, max)
		.map((q) => {
			q.kw = JSON.parse(q.kw);
			return q;
		}
	);
}

function pull_query_IPs(db, max, date_range) {
	if (date_range === undefined) date_range = {};
	const date_begin = date_range['begin'] || '0000-01-01';
	const date_end   = date_range['end']   || '9999-12-31';
	return db.prepare(
		`SELECT max(time) as most_recent, ip, COUNT(*) as counter
		FROM query
		WHERE date(time) BETWEEN ? AND ?
		GROUP BY ip ORDER BY counter DESC LIMIT ?`)
		.all(date_begin, date_end, max);
}

function pull_query_summary(db, date_range) {
	if (date_range === undefined) date_range = {};
	const date_begin = date_range['begin'] || '0000-01-01';
	const date_end   = date_range['end']   || '9999-12-31';
	return db.prepare(
		`SELECT COUNT(*) as n_queries, COUNT(DISTINCT IP) as n_uniq_ip
		FROM query
		WHERE date(time) BETWEEN ? AND ?`)
		.all(date_begin, date_end);
}

function test() {
	var sqlite3 = require('better-sqlite3');
	var db = new sqlite3('test.sqlite3', {verbose__: console.log});

	test_json = {
		"ip":"121.32.196.150",
		"page":1,
		"kw":[
			{"type":"tex","str":"\\sqrt{\\frac{a}{a+8}}"},
			{"type":"term","str":"inequality"}
		]
	};
	push_query(db, test_json);

	test_json = {"ip":"87.92.156.50","page":2,"kw":[{"type":"tex","str":"n^2+n+1"},{"type":"tex","str":"4^n+2^n+1"}]}
	push_query(db, test_json);

	test_json = {"ip":"121.32.196.150","page":1,"kw":[{"type":"tex","str":"(x + y + z)^3 + 9xyz \\ge 4(x + y + z)(xy + yz + zx)"}]}
	push_query(db, test_json);

	console.log(pull_query_items(db, 3, {end: '2019-06-09'}));
	console.log(pull_query_IPs(db, 3));
	console.log(pull_query_summary(db));

	db.close();
}

module.exports = {
	push_query,
	pull_query_items,
	pull_query_IPs,
	pull_query_summary
}
