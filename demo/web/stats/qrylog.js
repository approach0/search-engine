function push_query(db, qry_json) {
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
	return db.prepare(`SELECT id,
		time, query.ip, page, city, region, country,
		json_group_array(str) as kw,
		json_group_array(type) as type
		FROM query
		JOIN keyword ON query.id = keyword.qryID
		JOIN ip_info ON query.ip = ip_info.ip
		WHERE date(time) BETWEEN ? AND ?
		GROUP BY id ORDER BY id DESC LIMIT ?`)
		.all(date_begin, date_end, max)
		.map((q) => {
			q.kw = JSON.parse(q.kw);
			q.type = JSON.parse(q.type);
			return q;
		}
	);
}

function pull_query_items_of(db, ip, max, date_range) {
	if (date_range === undefined) date_range = {};
	const date_begin = date_range['begin'] || '0000-01-01';
	const date_end   = date_range['end']   || '9999-12-31';
	console.log(ip);
	return db.prepare(`SELECT id,
		time, query.ip, page, city, region, country,
		json_group_array(str) as kw,
		json_group_array(type) as type
		FROM query
		JOIN keyword ON query.id = keyword.qryID
		JOIN ip_info ON query.ip = ip_info.ip
		WHERE (date(time) BETWEEN ? AND ? AND query.ip = ?)
		GROUP BY id ORDER BY id DESC LIMIT ?`)
		.all(date_begin, date_end, ip, max)
		.map((q) => {
			q.kw = JSON.parse(q.kw);
			q.type = JSON.parse(q.type);
			return q;
		}
	);
}

function pull_query_IPs(db, max, date_range) {
	if (date_range === undefined) date_range = {};
	const date_begin = date_range['begin'] || '0000-01-01';
	const date_end   = date_range['end']   || '9999-12-31';
	return db.prepare(`SELECT max(time) as time, query.ip as ip,
		city, region, country, COUNT(*) as counter
		FROM query
		JOIN ip_info ON query.ip = ip_info.ip
		WHERE date(time) BETWEEN ? AND ?
		GROUP BY query.ip ORDER BY counter DESC LIMIT ?`)
		.all(date_begin, date_end, max);
}

function pull_query_IPs_of(db, ip, max, date_range) {
	if (date_range === undefined) date_range = {};
	const date_begin = date_range['begin'] || '0000-01-01';
	const date_end   = date_range['end']   || '9999-12-31';
	return db.prepare(`SELECT max(time) as time, query.ip as ip,
		city, region, country, COUNT(*) as counter
		FROM query
		JOIN ip_info ON query.ip = ip_info.ip
		WHERE (date(time) BETWEEN ? AND ? AND query.ip = ?)
		GROUP BY query.ip ORDER BY counter DESC LIMIT ?`)
		.all(date_begin, date_end, ip, max);
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

function pull_query_trend(db, date_range) {
	if (date_range === undefined) date_range = {};
	const date_begin = date_range['begin'] || '0000-01-01';
	const date_end   = date_range['end']   || '9999-12-31';
	const date_limit = date_range['limit'] || 32;
	return db.prepare(
		`SELECT COUNT(DISTINCT IP) as n_uniq_ip, date(time) as date
		FROM query WHERE date(time) BETWEEN ? AND ?
		GROUP BY date(time) LIMIT ?`)
		.all(date_begin, date_end, date_limit);
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

function get_ip_info(db, ip) {
	const res = db.prepare(
		`SELECT city, region, country, ip FROM ip_info WHERE ip = ?`
	).all(ip);
	if (res.length < 1) return {};
	else return res[0];
}

function map_ip_info(db, ip, callbk) {
//	db.prepare(
//		'DELETE FROM ip_info'
//	).run();
	const info = get_ip_info(db, ip);
	console.log(info);
	if (info.ip) {
		callbk(info);
	} else {
		var request = require('request');
		const url = `http://api.ipstack.com/${ip}?access_key=7cd94fd6ca2622d0bd7d39d03dbf608a`
		console.log(url);
		request.get(url, (error, response, body) => {
			var response = {};
			if (error) {
				console.log(error);
			} else {
				/* can be error too here, e.g. rate limit. */
				console.log(body);
				response = JSON.parse(body);
			}
			console.log(response);
			db.prepare(
				`INSERT INTO ip_info (city, region, country, ip)
				VALUES (?, ?, ?, ?)`
			).run(
				response['city'] || 'Unknown',
				response['region_name'] || 'Unknown',
				response['country_name'] || 'Unknown',
				response['ip']
			);
			callbk(get_ip_info(db, ip));
		});
	}
}

function initialize(db) {
	db.prepare(`CREATE TABLE IF NOT EXISTS
		ip_info (city STRING, region STRING, country STRING, ip STRING PRIMARY KEY)`).run();
	db.prepare(`CREATE TABLE IF NOT EXISTS
		query (time TEXT, ip STRING, page INT, id INTEGER PRIMARY KEY)`).run();
	db.prepare(`CREATE TABLE IF NOT EXISTS
		keyword (str STRING, type STRING, op STRING, qryID INTEGER,
		FOREIGN KEY(qryID) REFERENCES query(id))`).run();
}

module.exports = {
	initialize,
	push_query,
	map_ip_info,
	get_ip_info,
	pull_query_items,
	pull_query_items_of,
	pull_query_IPs,
	pull_query_IPs_of,
	pull_query_summary,
	pull_query_trend
}
