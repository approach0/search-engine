var quiz = {
	hide: false
};

(function () {
/* choose a random item from quiz list */
	var r = Math.random(); // in [0, 1]
	var r_idx = parseInt(r * quiz_list.length)
	if (r_idx == quiz_list.length) {
		/* in case r equals 1 */
		r_idx = 0;
	}

	//r_idx = 5; /* debug */

	quiz.Q = quiz_list[r_idx].Q;
	quiz.hints = quiz_list[r_idx].hints;
	quiz.search = quiz_list[r_idx].search;
})();

$(function () {
	$("#quiz-hint").typed({
		strings: quiz.hints,
		contentType: 'html',
		cursorChar: "|",
		typeSpeed: 10,
		startDelay: 3000,
		backDelay: 1200,
		callback: function() {
			$("#quiz-hint").children("a").on("click", function () {
				type_and_click_search(quiz.search, 1, true);
			});
		}
	});
});

$(document).ready(function () {

	quiz_vm = new Vue({
		el: '#quiz-vue-app',
		data: quiz
	});

	Vue.nextTick(function () {
		tex_render_fast("#quiz-question");
	});

	window.quiz_hide = function () {
		quiz.hide = true;
	};
});
