var response = {
	'ret_code': -1,
	'ret_str': '',
	'tot_pages': -1,
	'cur_page': -1,
	'prev': '',
	'next': '',
	"hits": []
};

function render_math() {
	console.log('rendering math...');
	MathJax.Hub.Queue(
		["Typeset", MathJax.Hub]
	);
}

function str_fmt() {
	var res_str = arguments[0];
	for (var i = 1; i < arguments.length; i++) {
		var re = new RegExp("\\{" + (i - 1) + "\\}", "gm");
		res_str = res_str.replace(re, arguments[i]);
	}
	return res_str;
}

function handle_search_res(res, qry, page) {
	console.log(res);

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
		render_math();
	}, 1000);
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

	$('#search_button').on('click', function() {
		qry = $("#search_input_box").val();
		srch_qry(encodeURIComponent(qry), 1);
	});

	$("#search_input_box").on("keyup", function(ev) {
		if (ev.which == 13 /* enter */) {
			qry = $(this).val();
			srch_qry(encodeURIComponent(qry), 1);
		}
	});
});
