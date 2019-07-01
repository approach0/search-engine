const stripe_apikey = 'pk_test_2TIdFYWCiMtR1Dt8Qg7pGczn00YUkb2ROx';
const stripe_secret = 'sk_test_MVy8dD4kjCgYcVY4ThZJ8myI001jEJZMsv';

var stripe = require('stripe')(stripe_apikey);
var sqlite3 = require('better-sqlite3');
var express = require('express');
var bodyParser = require('body-parser');

app = express();

app.use(express.static('./dist'))
app.use(bodyParser.json());

var db = new sqlite3('backers.sqlite3', {verbose: console.log});
db.prepare(`CREATE TABLE IF NOT EXISTS
	flair (user_id INTEGER, site STRING, net_id INTEGER PRIMARY KEY)`).run();
db.prepare(`CREATE TABLE IF NOT EXISTS
	donate (net_id INTEGER, badge STRING, donate_id STRING, id INTEGER PRIMARY KEY)`).run();

function save_donate(donate) {
	const reference_id = donate.client_reference_id;
	const user_id = reference_id.split('@')[0];
	const site    = reference_id.split('@')[1];
	const net_id  = reference_id.split('@')[2];
	const dispi = donate.display_items[0];
	let badge;
	if (dispi.type == 'plan') {
		if (dispi.amount == 336)
			badge = 'Fibonacci Sponsor';
		else if (dispi.amount == 467)
			badge = 'Feigenbaum Sponsor';
		else if (dispi.amount == 5000)
			badge = 'Euler Infinity Sponsor';
		else
			badge = 'Sponsor';
	} else {
		badge = dispi[dispi.type]['attributes']['name'];
	}
	const flair = db.prepare(`INSERT OR REPLACE INTO flair (user_id, site, net_id)
		VALUES (?, ?, ?)`);
	const donate = db.prepare(`INSERT INTO donate (net_id, badge, donate_id)
		VALUES (?, ?, ?)`);
	flair.run(user_id, site, net_id);
	donate.run(net_id, badge, donate.customer);
}

function reset_db() {
	db.prepare(`DELETE FROM flair`).run();
	db.prepare(`DELETE FROM donate`).run();
	const flair = db.prepare(`INSERT OR REPLACE INTO flair (user_id, site, net_id)
		VALUES (?, ?, ?)`);
	const donate = db.prepare(`INSERT INTO donate (net_id, badge, donate_id)
		VALUES (?, ?, ?)`);
	/* me */
	flair.run(487199, 'https://superuser.com', 3244601);
	donate.run(3244601, 'Code contributor', 'cus_tk');
	donate.run(3244601, 'Creator', 'cus_tk');

	/* Sil */
	flair.run(290240, 'https://math.stackexchange.com', 1032304);
	donate.run(1032304, 'Code contributor', 'cus_Sil');


	/* Martin */
	flair.run(8297, 'https://math.stackexchange.com', 267077);
	donate.run(267077, 'First-prime Backer', 'cus_MartinSleziak');
}
// reset_db();

function get_all_records() {
	const flair = db.prepare(
		`SELECT * FROM flair`).all();
	const donate = db.prepare(
		`SELECT * FROM donate`).all();
	return {'flair': flair, 'donate': donate};
}

function get_merged_records() {
	const arr = db.prepare(
		`SELECT user_id as account, site,
		json_group_array(badge) as badges, flair.net_id
		FROM donate JOIN flair
		ON donate.net_id = flair.net_id
		GROUP BY user_id`)
	.all().map((q) => {
		q.badges = JSON.parse(q.badges);
		return q;
	});
	return {'res': arr};
}

function set_flair(req) {
	const flair = db.prepare(`INSERT OR REPLACE INTO flair (user_id, site, net_id)
		VALUES (?, ?, ?)`);
	flair.run(req.user_id, req.site, req.net_id);
}

app.post('/webhook', function (request, response) {
	const sig = request.headers['stripe-signature'];
	let ev;
	try {
		ev = stripe.webhooks.constructEvent(request.body, sig, stripe_secret);
	} catch (err) {
		return response.status(400).send(`Webhook Error: ${err.message}`);
	}
	if (event.type === 'checkout.session.completed') {
		const donate = event.data.object;
		save_donate(donate);
	}
	response.send('Webhook OK. Hooray!');

}).get('/list', (req, res) => {
	res.contentType('text/plain');
	res.send(JSON.stringify(get_all_records(), null, 2));

}).get('/join_rows', (req, res) => {
	res.json(get_merged_records());

}).post('/update_flair', (req, res) => {
	set_flair(req.body)
	res.json('{"update_flair": "done"}');

}).post('/new_donate', (req, res) => {
	set_flair(req.body)
	new_donate(req.body)
	res.json('{"new_donate": "done"}');

}).get('/verify/:net_id', (req, res) => {
	res.json(get_merged_records(req.params.net_id));

});

const port = 3212;
console.log('listening on ' + port);
app.listen(port);

process.on('SIGINT', function() {
	console.log('')
	console.log('closing...')
	db.close();
	process.exit();
})
