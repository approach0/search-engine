MathJax.Hub.Config({
	tex2jax: {
		inlineMath: [['[imath]','[/imath]']],
		displayMath: [['[dmath]','[/dmath]']]
	},

	extensions: ["tex2jax.js","MathEvents.js","toMathML.js","TeX/noErrors.js","TeX/noUndefined.js","TeX/AMSmath.js","TeX/AMSsymbols.js"],

	jax: ["input/TeX","output/HTML-CSS","output/CommonHTML"],
	
	showMathMenu: false, /* does not show menu */

	/* all I want is to disable fast-preview */
	// CHTMLpreview: false,
	// 'fast-preview': {disabled: true},
	menuSettings: {CHTMLpreview: false}
});

MathJax.Ajax.loadComplete("[MathJax]/config/tkblog-math-config.js");
