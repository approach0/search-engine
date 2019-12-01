$.fn.stickToBottom = function (ceil_selector) {
	var ceil_ele = $(ceil_selector);
	if (ceil_ele.offset() === undefined)
		return;

	var ceil_pos = ceil_ele.offset().top + ceil_ele.outerHeight();
	var wind_height = window.innerHeight;
	var this_margin_top = parseInt($(this).css('margin-top'), 10);
	var this_height = $(this).outerHeight() + this_margin_top;
	var space = wind_height - ceil_pos;

	if (space > this_height) {
		$(this).css({
			position: 'absolute',
			bottom: 0
		});
	} else {
		$(this).css({
			position: 'static'
		});
	}
};

$(window).resize(function() {
	$('#init-footer').stickToBottom('#quiz');
	$('#search-footer').stickToBottom('#navigator');
});

$(document).ready(function() {
	$('#init-footer').stickToBottom('#quiz');
	$('#search-footer').stickToBottom('#navigator');

	/* handle state update (e.g. back/forward clicked) */
	window.addEventListener('popstate', function(event) {
		var state = event.state;
		if (state && state.qry != "") {
			type_and_click_search(state.qry, state.page, false);
		}
	});
});
