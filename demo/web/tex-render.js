function katex_tex_render(scope_select) {
	tex_tag_open  = '<span class="imath-to-render">';
	tex_tag_close = '</span>';

	replace_regex = /\[imath\]([\s\S]+?)\[\/imath\]/g;
	render_select = "span.imath-to-render";
	remove_class = "imath-to-render";
	replace_class = "imath-rendered";

	err_tag_open_0  = '<span class="imath-err" title="';
	err_tag_open_1  = '">';
	err_tag_close = '</span>';

	$(scope_select).each(function() {
		repl = $(this).html().replace(
			replace_regex,
			function (a, b) {
				return tex_tag_open + b + tex_tag_close;
			});
		$(this).html(repl);
	});

	$(render_select).each(function() {
		var tex = $(this).text();
		ele = $(this).get(0);
		try {
			katex.render(tex, ele, {
				macros: {
					"\\qvar": "\\color{blue}"
				}
			});
		} catch(err) {
			$(this).html(
				err_tag_open_0 + err + err_tag_open_1 +
				tex + err_tag_close
			);
		}

		/* prevent from being rendered again */
		$(this).removeClass(remove_class).addClass(replace_class);
	});
}

var tex_err_map = {};

function mathjax_print_errmsg (ele) {
	/*
	 * after rendering element 'ele', if there is
	 * parse error (with "span.noError" class), then
	 * report error message by setting span title.
	 */
	$(ele).find("span.noError").each(function() {
		var err_tex = $(this).text();

		/*
		 * jQuery text() may convert &nbsp; to ASCII
		 * char (160). Here we replace it to regular
		 * space.
		 */
		err_tex = err_tex.replace(/\s/g, ' ');

//		console.log(err_tex);
//		console.log(tex_err_map[err_tex]);

		var err_msg = tex_err_map[err_tex];
		if (err_msg) {
			$(this).attr('title', err_msg);
			$(this).addClass('imath-err');
		}
	});
}

function mathjax_tex_render(scope_select) {
	$(scope_select).each(function() {
		ele = $(this).get(0);
		MathJax.Hub.Queue(
			["Typeset", MathJax.Hub, ele],
			[mathjax_print_errmsg, ele]
		);
	});
}

function mathjax_init() {
	MathJax.Hub.Config({
		tex2jax: {
			inlineMath: [['[imath]','[/imath]']],
			displayMath: [['[dmath]','[/dmath]']]
		},
		TeX: {
			noErrors: {disabled: false},
			Macros: {
				qvar: ['{\\color{blue}{#1}}',1]
			}
		},
		showMathMenu: true /* false */,
		menuSettings: {CHTMLpreview: false}
	});

	MathJax.Hub.Register.MessageHook("TeX Jax - parse error",function (err) {
		var tex_str = err[2];
		var err_msg = err[1];
//		console.log('TeX string: ' + tex_str);
//		console.log('TeX error: ' + err_msg);
		tex_err_map[tex_str] = err_msg;
	});

	MathJax.Hub.Register.StartupHook("End Jax",function () {
		var jax = "SVG";
		return MathJax.Hub.setRenderer(jax);
	});
}

function tex_render(scope_select) {
	mathjax_tex_render(scope_select);
}

function tex_render_fast(scope_select) {
	katex_tex_render(scope_select);
}
