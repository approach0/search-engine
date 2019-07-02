var response = {
	'ret_code': -1,
	'ret_str': '',
	'tot_pages': 0,
	'cur_page': 0,
	'prev': '',
	'next': '',
	'SE_user': 0,
	'SE_netID': 0,
	'SE_site': 'https://stackexchange.com',
	"hits": []
};

function str_fmt() {
	var res_str = arguments[0];
	for (var i = 1; i < arguments.length; i++) {
		var re = new RegExp("\\{" + (i - 1) + "\\}", "gm");
		res_str = res_str.replace(re, arguments[i]);
	}
	return res_str;
}

function render_search_results() {
	setTimeout(function(){
		tex_render("a.title");
		tex_render("p.snippet", function (a, b) {
			var percent = Math.ceil((a * 100) / b);
			if (percent > 90) {
				percent = 100;
			}
			if (percent % 10 == 0) {
				var percent_str = "" + percent + "%";
				// console.log(percent_str);
				$("#progress").css("width", percent_str);
			}
		});
	}, 500);
}

function handle_search_res(res, enc_qry, page) {
	response.ret_code = res.ret_code;
	response.ret_str = res.ret_str;
	response.tot_pages = res.tot_pages;
	response.cur_page = page;

	response.hits = res.hits;

	if (page - 1 > 0)
		response.prev = str_fmt(
			'srch_enc_qry("{0}", {1}, true)',
			enc_qry, page - 1
		);
	else
		response.prev = '';

	if (page + 1 <= res.tot_pages)
		response.next = str_fmt(
			'srch_enc_qry("{0}", {1}, true)',
			enc_qry, page + 1
		);
	else
		response.next = '';

	render_search_results();

//	setTimeout(function(){
//		$("p.snippet > em.hl").each(function() {
//			var score = parseInt($(this).find('span.score').text());
//			var alpha = 255; /* lightest */
//			if (score >= 48)
//				alpha = 0;
//			else if (score > 45)
//				alpha = 20;
//			else if (score > 40)
//				alpha = 100;
//			else
//				alpha = 200;
//			$(this).css('background-color', 'rgba(255,255,' + alpha + ')');
//			/* debug print */
//			//console.log('' + parseInt(score) + ',' + alpha);
//			//$(this).html('' + parseInt(score) + ',' + alpha);
//		});
//	}, 200);
}

function srch_enc_qry(enc_qry, page, is_pushState) {
	/* a dot dot dot animation */
	var dots_timer = window.setInterval(function() {
		$("span.dots").each(function () {
		var cur_dots = $(this).text();

		if (cur_dots.length >= 3)
			$(this).text("");
		else
			$(this).text(cur_dots + ".");
		});
	}, 100);

	/* send AJAX request */
	$.ajax({
		url: 'search-relay.php',
		data: 'p=' + page + '&q=' + enc_qry,
		dataType: 'json'
	}).done(function(res) {
		handle_search_res(res, enc_qry, page);
		clearInterval(dots_timer);
	}).fail(function(res, ajax_err_str) {
		console.log("AJAX error: " + ajax_err_str);
		response.ret_code = 101;
		response.ret_str = "Server is down right now, " +
			"but will be back shortly";
		clearInterval(dots_timer);
	});

	/* let user know we are loading */
	response.ret_code = 102;
	response.ret_str = "Query is running hard";

	/* push browser history if needed */
	if (is_pushState) {

		srch_state = {
			"qry": decodeURIComponent(enc_qry),
			"page": page
		};

		srch_state_str = JSON.stringify(srch_state);
		//console.log(srch_state_str);
		history.pushState(srch_state, srch_state_str,
		                  "?q=" + enc_qry + "&p=" + page);
		//console.log('push history state...');
	}
}

function srch_qry(qry, page, is_pushState) {
	srch_enc_qry(encodeURIComponent(qry), page, is_pushState);
}

$(document).ready(function() {
	vm = new Vue({
		el: '#search-vue-app',
		data: response,
		methods: {
			SE_auth: function() {
				// console.log('Reqest for SE OAuth2 ...');
				var vm = this;
				onSucc = function (data) {
					var accounts = data['networkUsers'];
					var net_id = data['networkUsers'][0]['account_id'];
					var max_rep = 0;
					var save_idx = 0;
					for (var i = 0; i < accounts.length; i++) {
						var reputation = accounts[i]['reputation'];
						if (reputation > max_rep) {
							max_rep = reputation;
							save_idx = i;
						}
					}
					vm.SE_netID = net_id;
					vm.SE_user = accounts[save_idx]['user_id'];
					vm.SE_site = accounts[save_idx]['site_url'];

					window.qry_vm.SE_netID = vm.SE_netID;
					window.qry_vm.SE_user = vm.SE_user;
					window.qry_vm.SE_site = vm.SE_site;
				};

				/* for test */
				setTimeout(function () {
					onSucc({
						"accessToken": "foo",
						"expirationDate": "2019-07-01T06:33:43.878Z",
						"networkUsers": [{
							"badge_counts": {
							"bronze": 15,
							"silver": 5,
							"gold": 0
							},
							"question_count": 5,
							"answer_count": 3,
							"last_access_date": 1561876269,
							"creation_date": 1378003935,
							"account_id": 3244601,
							"reputation": 338,
							"user_id": 2736576,
							"site_url": "https://stackoverflow.com",
							"site_name": "Stack Overflow"
						}]
					});
				}, 2000);

				SE.authenticate({
					success: function(data) { onSucc(data); },
					error: function(data) { console.log('error', data); },
					scope: [],
					networkUsers: true
				});
			},
			blur_this: function(idx) {
				if (vm.SE_user > 0) return false;
				var l = this.hits.length;
				if (idx == Math.min(2, l - 1))
					return true;
				else if (idx == Math.min(6, l - 1))
					return true;
				else
					return false;
			},
			mess_up: function(text) {
				var list = text.split('em');
				var out_li = [];
				for (var i = 0; i < list.length; i++) {
					var word = list[i];
					if (word[word.length - 1] != '/') {
						word = word.replace(/[a-z]/g, 'b');
					}
					out_li.push(word);
				};
				return out_li.join('em');
			},
			surround_special_html: function(text) {
				/*
				 * This function makes sure $a<b$ is converted into $a < b$, otherwise
				 * the search snippet can be rendered incorrectly by browser.
				 */
				var replace_regex = /\[imath\]([\s\S]+?)\[\/imath\]/g;
				return text.replace(replace_regex, function (a, b) {
					return '[imath]' + b.split('<').join(' < ') + '[/imath]';
				});
			}
		}
	});

	vm.$watch('SE_user', function (newVal, oldVal) {
		render_search_results();
	})

	window.srch_vm = vm;

	/* push a initial history state (clear forward states) */
	history.pushState({"qry": "", "page": -1}, '');
});
