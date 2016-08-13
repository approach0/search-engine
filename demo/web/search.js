var response = {
	'ret_code': 0,
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

function handle_search_res(res, qry, page) {
	response.ret_code = res.ret_code;
	response.ret_str = res.ret_str;
	response.tot_pages = res.tot_pages;
	response.cur_page = page;

	response.hits = res.hits;

	if (page - 1 > 0)
		response.prev = str_fmt(
			'srch_qry("{0}", {1})',
			qry, page - 1
		);
	else
		response.prev = '';

	if (page + 1 <= res.tot_pages)
		response.next = str_fmt(
			'srch_qry("{0}", {1})',
			qry, page + 1
		);
	else
		response.next = '';

	setTimeout(function(){
		tex_render("p.snippet");
	}, 500);
}

function srch_qry(qry, page) {
	$.ajax({
		url: 'search-relay.php',
		data: 'p=' + page + '&q=' + qry,
		dataType: 'json',
	}).done(function(response) {
		handle_search_res(response, qry, page);
	}).fail(function(response) {
		alert('Ajax fails, dude!');
	});
}

$(document).ready(function() {
	vm = new Vue({
		el: '#search-vue-app',
		data: response
	});
});
