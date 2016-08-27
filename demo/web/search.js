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
		tex_render("p.snippet");
	}, 500);
}

function srch_enc_qry(enc_qry, page, is_pushState) {
	$.ajax({
		url: 'search-relay.php',
		data: 'p=' + page + '&q=' + enc_qry,
		dataType: 'json'
	}).done(function(res) {
		handle_search_res(res, enc_qry, page);
	}).fail(function(res) {
		response.ret_code = 101;
		response.ret_str = "Server is down right now, " +
			"but will be back shortly";
	});

	if (is_pushState) {

		srch_state = {
			"qry": decodeURIComponent(enc_qry),
			"page": page
		};

		srch_state_str = JSON.stringify(srch_state);
		//console.log(srch_state_str);
		history.pushState(srch_state, srch_state_str);
		//console.log('push history state...');
	}
}

function srch_qry(qry, page, is_pushState) {
	srch_enc_qry(encodeURIComponent(qry), page, is_pushState);
}

$(document).ready(function() {
	vm = new Vue({
		el: '#search-vue-app',
		data: response
	});

	/* push a initial history state (clear forward states) */
	history.pushState({"qry": "", "page": -1}, '');
});
