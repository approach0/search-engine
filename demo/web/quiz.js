$(function () {
	$("#quiz-hint").typed({
		strings: [
			"Okay, find all solutions for ...",
			"Wait, what is this? It looks terrible!",
			"Honestly, I hate my math homework.",
			"Hmmm... Can I <a href=\"#\">search it</a> on Internet?"
		],
		contentType: 'html',
		cursorChar: "|",
		typeSpeed: 0,
		startDelay: 5000,
		backDelay: 1200,
		callback: function() {
			$("#quiz-hint").children("a").on("click", function () {
				var raw_qry = "$(1+x+\\frac 1 2 x^2)e^{-x}$, solution";
				type_and_search(raw_qry);
			});
		}
	});
});

$(document).ready(function () {
	tex_render("#quiz-question");
});
