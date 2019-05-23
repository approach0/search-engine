function katex_tex_render(scope_select) {
	var tex_tag_open  = '<span class="imath-to-render">';
	var tex_tag_close = '</span>';

	var replace_regex = /\[imath\]([\s\S]+?)\[\/imath\]/g;
	var render_select = "span.imath-to-render";
	var remove_class = "imath-to-render";
	var replace_class = "imath-rendered";

	var err_tag_open_0  = '<span class="imath-err" title="';
	var err_tag_open_1  = '">';
	var err_tag_close = '</span>';

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

function mathjax_tex_render(scope_select) {
	var replace_regex = /\[imath\]([\s\S]+?)\[\/imath\]/g;
	var tex_tag_open  = '<span class="imathjax">';
	var tex_tag_close = '</span>';
	var render_select = "span.imathjax";

	var err_tag_open_0  = '<span class="imath-err" title="';
	var err_tag_open_1  = '">';
	var err_tag_close = '</span>';

	$(scope_select).each(function() {
		repl = $(this).html().replace(
			replace_regex,
			function (_, tex) {
				return tex_tag_open + tex + tex_tag_close;
		});
		$(this).html(repl);
	});

	MathJax.texReset();

	$(render_select).each(function() {
		var tex = $(this).text();
		ele = $(this).get(0);
		try {
			const math_ele = MathJax.tex2chtml(tex, {
				display: false
			});
			ele.innerHTML = '';
			ele.appendChild(math_ele);
		} catch(err) {
			$(this).html(
				err_tag_open_0 + err + err_tag_open_1 +
				tex + err_tag_close
			);
		}
	});

	MathJax.startup.document.clear();
	MathJax.startup.document.updateDocument();
}

function tex_render(scope_select) {
	mathjax_tex_render(scope_select);
}

function tex_render_fast(scope_select) {
	katex_tex_render(scope_select);
}
