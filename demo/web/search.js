var response = {
	'ret_code': -1,
	'ret_str': '',
	'tot_pages': 0,
	'cur_page': 0,
	'prev': '',
	'next': '',
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

	setTimeout(function(){
		tex_render_fast("a.title");
		tex_render("p.snippet");
	}, 500);

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

	/* push a initial history state (clear forward states) */
	history.pushState({"qry": "", "page": -1}, '');
});
