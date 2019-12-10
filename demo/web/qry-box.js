$(document).ready(function() {
	var MQ = MathQuill.getInterface(2);

	var query = {
		"raw_str": "",
		'SE_user': 0,
		'SE_netID': 0,
		'SE_site': 'https://stackexchange.com',
		'en_donation': false,
		"ever_focused": false,
		"chip_max_height": 40,
		"page": 1,
		"items": [
			{"type": "term-input", "str": ""}
		],
		"pad": math_pad, /* from pad.js */
		'mouse_on_teddy': false
	};

	var get_max_chip_height = function () {
		var max_height = 0;
		$('li.qry-li').each(function() {
			var h = $(this).height();
			if (h > max_height)
				max_height = h;
		});
		/* add some offset to ensure nice alignment */
		return max_height + 6;
	};

	var tex_charset = "+*/\\!^_%()[]:;{}=<>";

	/* query variable <--> raw query string convert */
	var raw_str_2_query = function() {
		var q = query;
		var dollar_open = false;
		var seen_text = false;
		var kw_arr = [];
		var kw = '';

		/* get keyword array splitted by commas */
		for (var i = 0; i < q.raw_str.length; i++) {
			if (q.raw_str[i] == '$' && i + 1 < q.raw_str.length &&
			    q.raw_str[i + 1] == '$')
				continue; /* this is a double dollar, eat this $ */

			if (q.raw_str[i] == '$') {
				if (dollar_open)
					dollar_open = false;
				else
					dollar_open = true;

				/* handle "word $tex$", user forgets the comma? */
				if (dollar_open && $.trim(kw) != "") {
					/* wrap up this keyword */
					kw_arr.push(kw);
					kw = '';
				}

				/* $ just closed, looking for the text */
				if (!dollar_open)
					seen_text = false;

			} else if (contains(q.raw_str[i], tex_charset)) {
				/* in case user does not enclose math with '$' */
				dollar_open = true;
			}

			if (!dollar_open && q.raw_str[i] != '$') {
				/* have left math, now it is text space */

				/* handle comma between "any, any" */
				if (q.raw_str[i] == ',') {
					/* wrap up this keyword */
					kw_arr.push(kw);
					kw = '';
					continue; /* skip this comma */

				} else if (!seen_text && q.raw_str[i] != ' ') {
					/* or, "$tex$ word" w/o comma, user forgets it? */
					seen_text = true;

					/* wrap up this keyword */
					kw_arr.push(kw);
					kw = '';
				}
			}

			kw += q.raw_str[i];
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
			query.chip_max_height = get_max_chip_height();
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

	var focus_style = function () {
		$("#qry-input-area").css("border", "1px solid #8590a6");
		$("#qry-input-area").css("background-color", "white");
		$("#qry-input-box").css("background-color", "white");
	};

	var blur_style = function () {
		$("#qry-input-area").css("border", "1px solid #ebebeb");
		$("#qry-input-area").css("background-color", "#f6f6f6");
		$("#qry-input-box").css("background-color", "#f6f6f6");
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

		focus_style();
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
		var l = query.items.length;
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
			if (input_box.str.trim().length > 0) {
				var head_str = input_box.str.slice(0, -1);
				fix_input("term", head_str, function() {
					$("#qry-input-box").focus();
				});
			} else {
				input_box.str = input_box.str.trim();
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

	/* expander */
	function toggle_expander(obj) {
		head = obj;
		below = head.next();
		below.toggle("fast", function () {
			t = head.html();
			if (below.is(":visible"))
				t = t.replace("-down", "-up");
			else
				t = t.replace("-up", "-down");
			head.html(t);

			$('#init-footer').stickToBottom('#quiz');

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
		/* if user does not separate keywords by comma, correct it */
		query_2_raw_str();

		/* set query box to blur style */
		blur_style();

		query.page = page;
		arr = query.items;
		input_box = arr[arr.length - 1];

		qry = correct_math(query.raw_str);

		if (input_box.str != '' /* there are un-fixed keywords */) {
			/* push the last query to UI */
			if (input_box.type == 'tex-input') {
				fix_input("tex", input_box.str, function() {
					tex_render_fast("div.qry-div-fix");

					/* blur to cancel fix_input() focus */
					blur_style();

					/* perform search */
					srch_qry(qry, page, is_pushState);
				});

			} else if (input_box.type == 'term-input') {
				fix_input("term", input_box.str, function() {
					/* blur to cancel fix_input() focus */
					blur_style();

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
			SE_collect_usr: function () {
				var vm = this;
				$.ajax({
					type: 'POST',
					url: '/backers/login',
					contentType: "application/json",
					dataType: 'json',
					data: JSON.stringify({
						'user_id': vm.SE_user,
						'site': vm.SE_site,
						'net_id': vm.SE_netID
					})
				});
			},
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

					window.srch_vm.SE_netID = vm.SE_netID;
					window.srch_vm.SE_user = vm.SE_user;
					window.srch_vm.SE_site = vm.SE_site;

					vm.SE_collect_usr();
				};

				/* for test */
//				setTimeout(function () {
//					onSucc({
//						"accessToken": "foo",
//						"expirationDate": "2019-07-01T06:33:43.878Z",
//						"networkUsers": [{
//							"badge_counts": {
//							"bronze": 15,
//							"silver": 5,
//							"gold": 0
//							},
//							"question_count": 5,
//							"answer_count": 3,
//							"last_access_date": 1561876269,
//							"creation_date": 1378003935,
//							"account_id": 3244601,
//							"reputation": 338,
//							"user_id": 2736576,
//							"site_url": "https://stackoverflow.com",
//							"site_name": "Stack Overflow"
//						}]
//					});
//				}, 2000);

				SE.authenticate({
					success: function(data) { onSucc(data); },
					error: function(data) { console.log('error', data); },
					scope: [],
					networkUsers: true
				});
			},
			area_on_click: function (ev) {
				$("#qry-input-box").focus();
				var vm = this;
				$("#qry-input-area").animate(
					{"min-height": "60px"}, "fast", "swing", function () {
					vm.ever_focused = true;
				});
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
						blur_style();
					});
				}
			},
			register_focus_events: function () {
				$("#qry-input-box").focus(function () {
					focus_style();
				});
				$("#qry-input-box").blur(function () {
					blur_style();
				});
			}
		}
	});

	window.type_and_click_search =
	function (qry, page, is_pushState) {
//		console.log('search: ' + raw_qry_str);
		query.raw_str = qry;
		raw_str_2_query();
		Vue.nextTick(function () {
			tex_render_fast("div.qry-div-fix");
			click_search(page, is_pushState);
			qry_vm.ever_focused = true;
			query.chip_max_height = get_max_chip_height();
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

	/* initial focus/blur style */
	setTimeout(function () {
		qry_vm.register_focus_events();
		blur_style();

		/* raw query input box */
		$("#qry").focus(function () {
			$(this).css("border", "1px solid #8590a6");
			$(this).css("background-color", "white");
		});
		$("#qry").blur(function () {
			$(this).css("border", "1px solid #ebebeb");
			$(this).css("background-color", "#f6f6f6");
		});
		$("#qry").blur();
	}, 500);

	qry_vm.$watch('items', function (newVal, oldVal) {
		qry_vm.register_focus_events();
	})

	qry_vm.$watch('SE_user', function (newVal, oldVal) {
		$('#init-footer').stickToBottom('#quiz');
	})

	window.qry_vm = qry_vm;

	/* check if donation is enabled... */
//	$.ajax({
//		url: '/backers/join_rows',
//		type: 'GET',
//		success: function (data) {
//			var arr = data['res'];
//			if (arr.length > 0) {
//				window.qry_vm.en_donation = true;
//				window.srch_vm.unlock = false;
//				Vue.nextTick(function () {
//					$("span.collapse").next().hide();
//				});
//			} else {
//				console.log('donation disabled.');
//			}
//		},
//		error: function (req, err) {
//			console.log('donation disabled.');
//		}
//	});

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

	$('#search_button').on('click', function() {
		click_search(1, true);
	});

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
