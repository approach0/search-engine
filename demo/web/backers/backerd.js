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
	backer (user_id STRING, site STRING, badge STRING, order_id STRING,
	id INTEGER PRIMARY KEY)`).run();

function save_order(order) {
	const stmt = db.prepare(`INSERT INTO backer (user_id, site, badge, order_id)
		VALUES (?, ?, ?, ?)`);
	const reference_id = order.client_reference_id;
	const user_id = reference_id.split('@')[0];
	const site = reference_id.split('@')[0];
	const dispi = order.display_items[0];
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
	stmt.run(user_id, site, badge, order.customer);
}

function reset_db() {
	db.prepare(`DELETE FROM backer`).run();
	const stmt = db.prepare(`INSERT INTO backer (user_id, site, badge, order_id)
		VALUES (?, ?, ?, ?)`);
	stmt.run('3244601', 'https://stackexchange.com', 'Code contributor', 'cus_tk');
	stmt.run('290240', 'https://math.stackexchange.com', 'Code contributor', 'cus_Sil');
	stmt.run('267077', 'https://stackexchange.com', 'First-prime Backer', 'cus_MartinSleziak');
	stmt.run('3244601', 'https://stackexchange.com', 'Creator', 'cus_tk');
}
//reset_db();

function get_all_records() {
	const arr = db.prepare(
		`SELECT * FROM backer`).all();
	return {'res': arr};
}

function get_merged_records() {
	const arr = db.prepare(
		`SELECT user_id as account, site, json_group_array(badge) as badges
		FROM backer GROUP BY user_id`)
	.all().map((q) => {
		q.badges = JSON.parse(q.badges);
		return q;
	});
	return {'res': arr};
}

app.post('/webhook/', function (request, response) {
	const sig = request.headers['stripe-signature'];
	let ev;
	try {
		ev = stripe.webhooks.constructEvent(request.body, sig, stripe_secret);
	} catch (err) {
		return response.status(400).send(`Webhook Error: ${err.message}`);
	}
	if (event.type === 'checkout.session.completed') {
		const order = event.data.object;
		save_order(order);
	}
	response.send('Webhook OK. Hooray!');

}).get('/list', (req, res) => {
	res.contentType('text/plain');
	res.send(JSON.stringify(get_all_records(), null, 2));

}).get('/query', (req, res) => {
	res.json(get_merged_records());
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
