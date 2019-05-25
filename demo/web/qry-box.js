$(document).ready(function() {
	var MQ = MathQuill.getInterface(2);

	var query = {
		"raw_str": "",
		"page": 1,
		"items": [
			{"type": "term-input", "str": ""}
		],
		"pad": math_pad, /* from pad.js */
		'mouse_on_teddy': false
	};

	var tex_charset = "+*/\\!^_%()[]:;{}=<>";

	/* query variable <--> raw query string convert */
	var raw_str_2_query = function() {
		var q = query;
		var dollar_open = false;
		var kw_arr = [];
		var kw = '';

		/* get keyword array splitted by commas */
		for (var i = 0; i < q.raw_str.length; i++) {
			if (q.raw_str[i] == '$') {
				if (dollar_open)
					dollar_open = false;
				else
					dollar_open = true;
			} else if (contains(q.raw_str[i], tex_charset)) {
				/* in case user does not enclose math with '$' */
				dollar_open = true;
			}

			if (!dollar_open && q.raw_str[i] == ',') {
				kw_arr.push(kw);
				kw = '';
			} else {
				kw += q.raw_str[i];
			}
		}
		kw_arr.push(kw);

		/* for each of the keyword, add into query */
		q.items = []; /* clear */
		var correct_raw_str = false;
		$.each(kw_arr, function(i, kw) {
			kw = $.trim(kw);
			if (kw == "")
				return true; /* continue */

			if (kw[0] == "$" || contains(kw, tex_charset)) {
				/* strip dollar sign and push into tex keywords */
				var s = kw.replace(/^\$|\$$/g, "");
				q.items.push({"type":"tex", "str": s});

				/* if user forgets to wrap a '$' */
				if (kw[0] != "$")
					correct_raw_str = true;
			} else {
				var s = kw;
				q.items.push({"type":"term", "str": s});
			}
		});

		q.items.push({"type": "term-input", "str": ""});

		/* help user correct raw string if user does not wrap a '$' */
		if (correct_raw_str)
			query_2_raw_str();
	};

	var query_2_raw_str = function() {
		var raw_str_arr = [];
		$.each(query.items, function(i, item) {
			if (item.type == "tex")
				raw_str_arr.push("$" + item.str + "$");
			else
				raw_str_arr.push(item.str);
		});

		query.raw_str = raw_str_arr.slice(0, -1).join(", ");
	};

	/* keyword push/edit */
	var fix_input = function (type, str, then) {
		query.items.pop();

		query.items.push({
			"type": type,
			"str": str
		});
		query.items.push({
			"type": "term-input",
			"str": ""
		});

		query_2_raw_str();

		Vue.nextTick(function () {
			then();
		});
	};

	var switch_to_mq = function (init_tex) {
		query.items.pop();
		query.items.push({
			"type": "tex-input",
			"str": init_tex
		});

		Vue.nextTick(function () {
			var mq = render_mq_edit();
			mq.latex(init_tex);
		});
	};

	var switch_to_mq_2 = function (usr_type_str) {
		query.items.pop();
		query.items.push({
			"type": "tex-input",
			"str": usr_type_str
		});

		Vue.nextTick(function () {
			var mq = render_mq_edit();
			mq.typedText(usr_type_str);
		});
	};

	var render_mq_edit = function () {
		var ele = document.getElementById("math-input");
		var mq = MQ.MathField(ele, {
			supSubsRequireOperand: true, /* avoid _{_a} */
			handlers: {
				edit: function() {
					var arr = query.items;
					input_box = arr[arr.length - 1];
					var latex = mq.latex();
					/* if user finish with a dollar, it is OKay */
					if (-1 != latex.indexOf("$")) {
						latex = latex.replace(/\\\$/g, ' ');
						mq.latex(latex);
						this.enter();
					}
					input_box.str = latex;
				},
				enter: function() {
					if (mq.latex() == '')
						return;

					fix_input("tex", mq.latex(), function() {
						tex_render_fast("div.qry-div-fix");
						setTimeout(function () {
							$("#qry-input-box").focus();
						}, 500);
					});
				}
			}
		});

		if (mq) mq.focus();
		return mq;
	};

	function contains(str, substr) {
		for (i = 0; i < substr.length; i ++) {
			var c = substr[i];
			if (str.indexOf(c) > -1)
				return 1;
		}

		return 0;
	}

	var input_box_on_finish_math = function () {
		var mq = render_mq_edit();
		fix_input("tex", mq.latex(), function() {
			tex_render_fast("div.qry-div-fix");
			setTimeout(function () {
				$("#qry-input-box").focus();
			}, 500);
		});
	};

	var input_box_on_del = function (ev) {
		/* on backspace or delete key */
		var arr = query.items;
		var input_box = arr[arr.length - 1];

		/* delete the most-recent chip */
		const l = query.items.length;
		if (l > 1 && input_box.str.length == 0) {
			arr.splice(l - 2, 1);
			query_2_raw_str();
		}
	};

	var input_box_on_keyup = function (ev) {
		var arr = query.items;
		var input_box = arr[arr.length - 1];

		if (ev.which == 13 /* enter */) {
			if (input_box.str == '') {
				/* think this as search signal */
				click_search(1, true);
			} else {
				/* replace all commas into space */
				input_box.str = input_box.str.replace(/,/g, " ");

				/* add this "term" keyword */
				fix_input("term", input_box.str,function() {
					$("#qry-input-box").focus();
				});
			}
		} else if (ev.which == 32 /* space */) {
			/* split by this space, assuming it is the last char */
			if (input_box.str.length > 1) {
				var head_str = input_box.str.slice(0, -1);
				fix_input("term", head_str, function() {
					$("#qry-input-box").focus();
				});
			}

		} else if (contains(input_box.str, "$")) {
			/* user input a '$' sign */

			/* split by this '$', assuming it is the last char */
			if (input_box.str.length > 1) {
				var head_str = input_box.str.slice(0, -1);
				fix_input("term", head_str, function() {});
			}

			switch_to_mq("");

		} else if (contains(input_box.str, tex_charset)) {
			switch_to_mq_2(input_box.str);
		}
	};

	var input_box_on_paste = function (ev) {
		var clipboardData, pastedData;
		var arr = query.items;
		var input_box = arr[arr.length - 1];

		if (ev.type == "paste") {
			/* Stop data actually being pasted into <input> */
			ev.stopPropagation();
			ev.preventDefault();

			/* Get pasted data via clipboard API */
			clipboardData = ev.clipboardData || window.clipboardData;
			pastedData = clipboardData.getData('Text');
			//console.log(pastedData);

			if ($.trim(query.raw_str) == "") {
				/* nothing in query box now */
				query.raw_str = pastedData;
			} else {
				/* append to the end of current query */
				query.raw_str += ', ';
				query.raw_str += pastedData;
			}

			/* convert new raw query to UI */
			raw_str_2_query();
			Vue.nextTick(function () {
				tex_render_fast("div.qry-div-fix");
				$("#qry-input-box").focus();
			});
		}
	};

	/* Vue instance */
	var qry_vm = new Vue({
		el: "#qry-input-vue-app",
		data: query,
		computed: { /* computed property */
			enc_uri: function () {
				return encodeURIComponent(this.raw_str);
			},
			url_root: function () {
				return location.origin + location.pathname;
			}
		},
		methods: {
			area_on_click: function (ev) {
				$("#qry-input-box").focus();
			},
			on_input: input_box_on_keyup,
			on_del: input_box_on_del,
			on_finish_math_edit: input_box_on_finish_math,
			on_paste: input_box_on_paste,
			on_rawinput: function (ev) {
				var arr = query.items;
				input_box = arr[arr.length - 1];
				if (ev.which == 13 /* enter */) {
						click_search(1, true);
				} else {
					raw_str_2_query();
					Vue.nextTick(function () {
						tex_render_fast("div.qry-div-fix");
					});
				}
			}
		}
	});

	/* on keyword editing */
	window.dele_kw = function (idx) {
		query.items.splice(idx, 1);
		query_2_raw_str();

		raw_str_2_query();
		Vue.nextTick(function () {
			tex_render_fast("div.qry-div-fix");
		});
	};

	window.edit_kw = function (idx) {
		save_str = query.items[idx].str;
		dele_kw(idx);
		switch_to_mq(save_str);
	};

	/* expander */
	function toggle_expander(obj) {
		head = obj;
		below = head.next();
		below.toggle("fast", function () {
			t = head.text();
			if (below.is(":visible"))
				t = t.replace("+", "-");
			else
				t = t.replace("-", "+");
			head.text(t);

			$('#init-footer').stickToBottom();
			Vue.nextTick(function () {
				tex_render_fast("#handy-pad");
			});
		});
	}
	$("span.collapse").next().hide();
	$("span.collapse").click(function () {
		toggle_expander($(this));
	});

	/* search related */
	function correct_math(str) {
		/* simple rules to correct most common bad-math */
		str = str.replace(/_\{ \}/g, " ");
		str = str.replace(/\^\{ \}/g, " ");
		str = str.replace(/\{_/g, "{");
		//console.log(str);
		return str;
	}

	function click_search(page, is_pushState) {
		query.page = page;
		arr = query.items;
		input_box = arr[arr.length - 1];

		qry = correct_math($("#qry").val());

		if (input_box.str != '') {
			/* push the last query to UI */
			if (input_box.type == 'tex-input') {
				fix_input("tex", input_box.str, function() {
					tex_render_fast("div.qry-div-fix");

					/* perform search */
					srch_qry(qry, page, is_pushState);
				});

			} else if (input_box.type == 'term-input') {
				fix_input("term", input_box.str, function() {

					/* perform search */
					srch_qry(qry, page, is_pushState);
				});
			}
		} else {
			/* perform search */
			srch_qry(qry, page, is_pushState);
		}

		/* hide quiz */
		quiz_hide();

		/* hide "handy pad" */
		if ($("#handy-pad").is(":visible")) {
			toggle_expander($("#handy-pad-expander"));
		}
	}

	$('#search_button').on('click', function() {
		click_search(1, true);
	});

	window.type_and_click_search =
	function (qry, page, is_pushState) {
//		console.log('search: ' + raw_qry_str);
		query.raw_str = qry;
		raw_str_2_query();
		Vue.nextTick(function () {
			tex_render_fast("div.qry-div-fix");
			click_search(page, is_pushState);
		});
	};

	/* go to URI-specified query, if any */
	var q = $("#q").val();
	var p = $("#p").val();
	if (q != "") {
		var page = 1;
		if (p != "")
			page = parseInt(p, 10);

		type_and_click_search(q, page, true);
	}

	/* render math pad */
	Vue.nextTick(function () {
		tex_render_fast("#handy-pad");
		//console.log('rendered');
	});

	/* math pad button behaviors */
	function tex_strokes (mq, tex) {
		for (var i = 0; i < tex.length; i++) {
			var c = tex[i];
			if (c == '\t') {
				mq.keystroke('Tab');
			} else {
				mq.typedText(c);
			}
		}
	}

	var switch_to_mq_3 = function (usr_type_str) {
		query.items.pop();
		query.items.push({
			"type": "tex-input",
			"str": usr_type_str
		});

		Vue.nextTick(function () {
			var mq = render_mq_edit();
			tex_strokes(mq, usr_type_str);
		});
	};

	/* math pad button on clicked */
	window.send_cmd = function (tex_cmd) {
		var mq = render_mq_edit();
		//console.log(tex_cmd);
		if (mq) {
			tex_strokes(mq, tex_cmd + ' ');
		} else {
			switch_to_mq_3(tex_cmd + ' ');
		}
	};
});
