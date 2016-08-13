$(document).ready(function(){
	var MQ = MathQuill.getInterface(2);

	var query = {
		"raw_str": "",
		"items": [
			{"type": "term-input", "str": ""}
		]
	};

	/* query variable <--> raw query string convert */
	var raw_str_2_query = function() {
		var q = query;
		var raw_str_arr = q.raw_str.split(",");
		q.items = []; /* clear */
		$.each(raw_str_arr, function(i, raw_item) {
			if(raw_str_arr[0] == "")
				return true; /* continue */

			raw_item = $.trim(raw_item);
			if (raw_item[0] == "$") {
				var s = raw_item.replace(/^\$|\$$/g, "");
				q.items.push({"type":"tex", "str": s});
			} else {
				var s = raw_item;
				q.items.push({"type":"term", "str": s});
			}
		});

		q.items.push({"type": "term-input", "str": ""});
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

	var render_mq_edit = function () {
		var ele = document.getElementById("math-input");
		var mq = MQ.MathField(ele, {
			handlers: {
				enter: function() {
					fix_input("tex", mq.latex(), function() {
						tex_render("div.qry-div-fix");
					});
				}
			}
		});

		mq.focus();
		return mq;
	};

	var input_box_on_keyup = function (ev) {
		arr = query.items;
		input_box = arr[arr.length - 1];

		if (ev.which == 13 /* enter */) {
			fix_input("term", input_box.str, function() {});
		} else if ( /* user input a $ as first char */
			ev.which == 52 &&
			input_box.str[0] == "$"
		) {
			switch_to_mq("");
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
			on_rawinput: function () {
				raw_str_2_query();
				Vue.nextTick(function () {
					tex_render("div.qry-div-fix");
				});
			}
		}
	});

	/* on keyword editing */
	window.dele_kw = function (idx) {
		query.items.splice(idx, 1);
		query_2_raw_str();

		raw_str_2_query();
		Vue.nextTick(function () {
			tex_render("div.qry-div-fix");
		});
	};

	window.edit_kw = function (idx) {
		save_str = query.items[idx].str;
		dele_kw(idx);
		switch_to_mq(save_str);
	};

	/* expander */
	$("span.collapse").next().hide();
	$("span.collapse").click(function () {
		head = $(this);
		below = head.next();
		below.toggle("fast", function () {
			t = head.text();
			if (below.is(":visible"))
				t = t.replace("+", "-");
			else
				t = t.replace("-", "+");
			head.text(t);
		});
	});

	/* search button */
	$('#search_button').on('click', function() {
		qry = $("#qry").val();
		srch_qry(encodeURIComponent(qry), 1);
	});
});
