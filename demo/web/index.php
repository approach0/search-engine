<!DOCTYPE html>
<html>

<?php
/*
 * Redirect mobile visitors to PC/tablet,
 * because mathquill does not support mobile.
 */
require_once 'vendor/mobile-detect/Mobile_Detect.php';
$detect = new Mobile_Detect;
if ($detect->isMobile()) {
	header("Location: mobile.php");
	exit;
}
?>

<head>
<title>Approach0 - Math-aware search engine</title>
<meta charset="utf-8"/>
<meta name="description" content="Approach Zero: A math-aware search engine. Search millions of math formulas.">
<meta name="keywords" content="Approach0, Approach Zero, ApproachO, mathematics, math, formula, equation, math search, equation search, formula search, search engine" />
<link rel="shortcut icon" href="images/favicon.ico">

<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/mathquill@0.10.1-a/build/mathquill.css" type="text/css"/>
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/katex@0.10.2/dist/katex.min.css" type="text/css"/>
<link rel="stylesheet" href="all.css?hash=f9194751a2a647c0" type="text/css"/>

<script src="https://cdn.polyfill.io/v2/polyfill.min.js"></script>
<script src="https://cdn.jsdelivr.net/gh/approach0/mathjax-v3@cdn/components/dist/tex-chtml.js"></script>

<script type='text/javascript' src='vendor/stackexchange/oauth.js'></script>

<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/jquery@1.12.4/dist/jquery.min.js"></script>
<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/vue@1.0.26/dist/vue.min.js"></script>
<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/mathquill@0.10.1-a/build/mathquill.min.js"></script>
<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/katex@0.10.2/dist/katex.min.js"></script>
<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/typed.js@2.0.10/lib/typed.min.js"></script>
<script type="text/javascript" src="bundle.min.js?hash=f9194751a2a647c0"></script>
<style>
img.social {
	height: 16px;
}
div.center-v-pad {
	height: 200px
}
div.center-horiz {
	margin: 0 auto;
	text-align: center;
}
.toleft {
	width: 932px;
	margin: auto;
}
div.stick-bottom {
	position: absolute;
	bottom: 0;
	width: 100%;
}
button.sponsor {
	text-align: left;
	font-size: 0.8em;
	min-width: 200px;
}
div.blur {
	-webkit-filter: blur(4px);
	-moz-filter: blur(4px);
	-o-filter: blur(4px);
	-ms-filter: blur(4px);
	filter: blur(4px);
	pointer-events: none;
	-webkit-user-select: none;
	-moz-user-select: none;
	-ms-user-select: none;
	user-select: none;
}
a.btn, a.btn:visited{
	color: blue;
}
</style>
</head>

<body style="margin: 0">
<div id="progress" style="position: fixed; border-top: 2px solid #46ece5"></div>

<!-- Query Box App -->
<div id="qry-input-vue-app" style="background: white;
	padding: 8px 8px 10px 8px; box-shadow: 0 0 4px rgba(0,0,0,0.25);">

<!-- Query input area -->
<div class="toleft" style="display: flex">
<div style="flex: 0;">
	<img v-if="!ever_focused && raw_str.trim().length == 0" src="images/logo32.png"/>
	<img v-else style="margin-top: 0px" src="images/logo64.png"/>
</div>
<div id="qry-input-area" style="flex: 1; margin-left: 15px; border-radius: 16px;" v-on:click="area_on_click">
<ul class="qry-li-wrap"><template v-for="i in items">
		<li v-if="i.type == 'term'" class="qry-li">
			<div class="qry-div-fix">
				<span>{{{i.str}}}</span>
				<span title="delete" class="dele" v-bind:onclick="'dele_kw('+$index+')'">×</span>
			</div>
		</li>
		<li v-if="i.type == 'tex'" class="qry-li">
			<div class="qry-div-fix">
				<span>[imath]{{i.str}}[/imath]</span>
				<span title="delete" class="dele" v-bind:onclick="'dele_kw('+$index+')'">×</span>
				<span title="edit" class="edit" v-bind:onclick="'edit_kw('+$index+')'">✐</span>
			</div>
		</li>
		<li v-if="i.type == 'term-input'" class="qry-li">
			<input v-on:keyup="on_input" v-on:keydown.delete="on_del" v-on:paste="on_paste" v-model="i.str"
			type="text" id="qry-input-box" class="pl_holder"
			placeholder="Enter keywords here, type $ for math keyword."/>
		</li>
		<li v-if="i.type == 'tex-input'" class="qry-li">
			<span id="math-input"></span>
			<span class="pl_holder"><b>You are editing a math keyword</b>.
			When you finish, press enter or click <a @click="on_finish_math_edit" href="#">here</a>.</span>
		</li>
</template></ul>
</div>
</div>
<!-- Query input area END -->

<!-- Search button and options -->
<div style="padding-top: 18px; padding-bottom: 5px; position: relative" class="toleft">
	<div style="display: inline-block; width: 5px;">
	</div>

	<span class="collapse" title="Lookup TeX commands" id="handy-pad-expander">(+) handy pad</span>
	<div id="handy-pad">
		<p>If you are not familiar with TeX and math symbol names, we would suggest to use
		<a target="_blank" href="https://webdemo.myscript.com/views/math/index.html">
		a hand written math recognizer</a> to assist your math keyword input.</p>
		<p> Alternatively, we provide some commonly used math symbols below for you to pick. </p>
		<div v-for="p in pad">
		<h3>{{p.tab_name}}</h3>
		<ul class="pad-li-warp"><template v-for="b in p.buttons">
		<li class="pad-li">
			<div v-bind:onclick="'send_cmd(\'' + b.cmd + '\')'">
			<button class="pad" v-bind:title="b.desc">
			[imath]{{b.disp}}[/imath]
			</button>
			</div>
		</li>
		</template></ul>
		</div>

		<h3> Want More?</h3>
		<p>Above only lists most frequently used math snippets. Refer to <a target="_blank"
		href="http://www.onemathematicalcat.org/MathJaxDocumentation/TeXSyntax.htm">
		this link</a> for a complete math-related TeX commands and use "raw query" to
		input unlisted TeX commands.
		</p>
		<p>Willing to learn some Tex? <a target="_blank" href=
		"https://en.wikibooks.org/wiki/LaTeX">Here</a> is a helpful tutorial.
		</p>
		<p>If you want to add more buttons on this "handy pad" or help us imporve in any way,
		send us an issue (for suggestions) or a pull request (for proposed modifications) to our
		<a target="_blank" href="https://github.com/approach0/search-engine/blob/master/demo/web/pad.js">
		Github repository</a>.
		</p>
		<hr class="vsep"/>
	</div>

	<span class="collapse" title="Raw query and API">(+) raw query</span>
	<div>
		<p>Know TeX? You are an expert! Try to edit directly the raw query below (separate keywords by commas).</p>
		<input id="qry" style="padding-left: 6px; width:100%;" type="text" v-model="raw_str" v-on:keyup="on_rawinput"
		placeholder="empty"/>

		<!-- hidden URI parameters -->
		<input id="q" type="hidden" value=
		"<?php if (isset($_GET['q']) && is_scalar($_GET['q'])) echo htmlentities($_GET['q'], ENT_QUOTES,'UTF-8'); ?>"/>
		<input id="p" type="hidden" value=
		"<?php if (isset($_GET['p']) && is_scalar($_GET['p'])) echo htmlentities($_GET['p'], ENT_QUOTES,'UTF-8'); ?>"/>

		<p>You can also make a query in command line. Copy and issue the command below:</p>
		<p style="background-color: black; color: #bbb; padding: 3px 0 3px 6px; overflow-x: auto; white-space: nowrap;">
		curl -v '{{url_root}}search-relay.php?p={{page}}&amp;q={{enc_uri}}'
		<p>

	</div>

	<span class="collapse" title="Donate" id="donate-expander">(+) sponsors / backers</span>
	<div>
		<h3>Please consider to back this project</h3>
		<div v-show="SE_user == 0">
			<p>If this project has ever helped you in anyway, please consider to sponsor me to keep maintaining and pushing forward.</p>
			<p><a class="btn" v-on:click="SE_auth()" href="javascript: void(0)">
			Get authenticated</a> as StackExchange user to proceed to donation options
			(<a class="btn" href="blank.html" target="_blank">why?</a>).
		</div>

		<div id="donation" v-else>

		<p>Here are some donation options:</p>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site"
		 stripeid="plan_FHtUQUawetBrC8">
		<b><span style="color: red;">♡ </span> Fibonacci Sponsor</b>
		<p>(3.36 $ / month)</p>
		<p>
			Subscription support at around Reciprocal Fibonacci constant cost per month.
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site"
		 >
		<b><span style="color: red;">♡ </span> Feigenbaum Sponsor</b>
		<p>(4.67 $ / month)</p>
		<p>
			Subscription support at around Feigenbaum constant cost per month.
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site"
		 stripeid="sku_FHszC4bhsTUNQb">
		<b><span style="color: red;">♡ </span> Epsilon Backer</b>
		<p>(10 $ one time)</p>
		<p>
			Single-time donation to encourage Approach0 to stay on non-imaginary axis.
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site"
		 >
		<b><span style="color: red;">♡ </span> Euler Infinity Sponsor</b>
		<p>(50 $ / month)</p>
		<p>
			Cover the hosting cost of this site regularly to help Approach0 exist online for arbitrary large number of sponsored time.
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site"
		 >
		<b><span style="color: red;">♡ </span> Unity Backer</b>
		<p>(50 $ one time)</p>
		<p>
			Cover the hosting cost of this site for another one month (one time donation).
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site"
		 >
		<b><span style="color: red;">♡ </span>Ludwig Schläfli Backer</b>
		<p>(80 $ one time)</p>
		<p>
			Cover the domain name cost of approach0.xyz for another year (one time donation).
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site"
		 >
		<b><span style="color: red;">♡ </span> First-prime Backer</b>
		<p>(100 $ one time)</p>
		<p>
			Cover the hosting cost of this site for another two months (one time donation).
		</p>
		</button>

<p>
	Your payment details will be processed by Stripe (for credit/debit cards) and a record of your donation will be stored in the database of this site. A (subscribed) sponsor will be billed at the beginning of each month, until <a target="_blank" href="https://github.com/approach0/search-engine/issues/new">a request</a> is received or when this site no longer operates. Refunds can be provided up to 1 month.
</p>
<p>
You can also donate to this project via bitcoin, Paypal, Alipay or WeChat. If you choose these methods, please also send a notice <a href="https://github.com/approach0/search-engine/issues/new" target="_blank">here</a> about your donation time and amount afterwards, in order to update our list of sponsor/backers.
</p>
	<h3>In return</h3>
	<ul>
		<li>Complete search results are provided to all sponsors or backers.</li>
		<li>Sponsor can <a target="_blank" href="https://github.com/approach0/search-engine/issues/new">request</a> to place a logo image on this site, in the name of a private entity or company.
		<li>Within the budget limit, this site will try to index more data sources upon <a target="_blank" href="https://github.com/approach0/search-engine/issues/new">the request</a> of any sponsor or backer.</li>
		<li>Your StackExchange flair will be shown in our <a href="/backers" target="_blank">list of sponsors or backers</a> and your support will always be appreciated!</li>
	</ul>
	</div>  <!-- END v-else -->
		<script src="https://js.stripe.com/v3"></script>
		<div id="stripe-error-message" style="color: red"></div>
<script>
var stripe = Stripe('pk_test_2TIdFYWCiMtR1Dt8Qg7pGczn00YUkb2ROx');
$('#donation button').each(function() {
	$(this)[0].addEventListener('click', function () {
		var stripe_id = $(this).attr('stripeid') || 'none_empty';
		var SE_account = $(this).attr('account') || 0;
		var SE_site = $(this).attr('site') || 'https://stackexchange.com';
		var type = stripe_id.split('_')[0];
		var order = {quantity: 1};
		order[type] = stripe_id;

		var options = {
			items: [order],
			successUrl: 'https://approach0.xyz/backers/?id={CHECKOUT_SESSION_ID}',
			cancelUrl: 'https://approach0.xyz/backers/?id=0',
			clientReferenceId: '' + SE_account + '@' + SE_site
		};
		if (type !== 'plan') options['submitType'] = 'donate';
		// console.log(options);

		stripe.redirectToCheckout(options).then(function (result) {
			if (result.error) {
			var displayError = document.getElementById('stripe-error-message');
			displayError.textContent = result.error.message;
			}
		});
	});
});

$(document).ready(function() {
	/* show this donate expander on #donate URI */
	var anchor_name = window.location.href.split('#')[1] || 'none';
	if (anchor_name == 'donate') {
		setTimeout(function(){
			head = $("#donate-expander");
			below = head.next();
			below.show(function () {
				t = head.text();
				t = t.replace("+", "-");
				head.text(t);
				$('#init-footer').stickToBottom();
			});
		}, 500);
	}
});

/* StackExchange OAuth2 initialization */
SE.init({
	clientId: 15681,
	key: 'tEI8QC487ZIN5Pu9I1nY4A((',
	//channelUrl: 'https://approach0.xyz/search/blank.html',
	channelUrl: 'http://localhost/a0/blank.html',
	complete: function (data) {
		// console.log('[SE OAuth2]', data);
	}
});
</script>
	</div>


	<a style="text-decoration: none; color: blue; font-size: 14px;"
	href="/stats" target="_blank">
	<img src="images/link.png" style="vertical-align: middle;"/>
	query logs
	</a>

	<a style="text-decoration: none; color: blue; font-size: 14px; padding-left: 6px;"
	href="/guide" target="_blank">
	<img src="images/link.png" style="vertical-align: middle;"/>
	user guide
	</a>

	<button style="position: absolute; right: 5px; top: 15px;"
	type="button" id="search_button">Search</button>

</div>
<!-- Search button and options END -->

</div>
<!-- Query Box App END -->

<!-- Quiz App -->
<div id="quiz-vue-app" v-show="!hide">
	<div id="quiz" class="toleft">
		<div class="center-v-pad"></div>
		<div class="center-horiz">
			<p id="quiz-question">
			<b>Question</b>: &nbsp; {{Q}}
			</p>
		</div>
		<div class="center-horiz" style="padding-top:20px;">
			<span id="quiz-hint" class="mainfont"></span>
		</div>
	</div>

	<!-- Initial Footer -->
	<div v-show="!hide" id="init-footer"
	style="font-size: small; margin-top: 40px; width: 100%;
	bottom: 0px; position: absolute; background: #f4f6f8;
	padding-bottom: 15px; padding-top: 15px;
	box-shadow: 0 0 4px rgba(0,0,0,0.25);">
		<div class="toleft" style="text-align: center;">
			<a target="_blank" href="https://twitter.com/approach0">
			<img style="vertical-align:middle"
			src="images/logo32.png"/></a>
			+
			<a target="_blank" href="http://math.stackexchange.com/">
			<img style="vertical-align:middle"
			src="images/math-stackexchange.png"/></a>
			+
			<span style="color: red;">♡ </span>
			=
			<p>A math-aware search engine for Mathematics Stack Exchange.</p>
		</div>
	</div>
	<!-- Initial Footer END -->

</div>
<!-- Quiz App END -->

<!-- Search App -->
<div id="search-vue-app">

<!-- Error code -->
<div v-if="ret_code > 0">
<div class="center-v-pad"></div>
<div class="center-horiz">
	<template v-if="ret_code == 102">
	<p><span class="dots" style="color:white"></span>
	{{ret_str}}
	<span class="dots"></span></p>
	</template>
	<p v-else>Opps! {{ret_str}}. (return code #{{ret_code}})</p>
</div>
</div>
<!-- Error code END -->

<!-- Search Results -->
<div v-if="ret_code == 0" style="margin-top: 30px;">
	<ol>
	<li v-for="(idx, hit) in hits" class="hit toleft">
		<div v-if="blur_this(idx)" style="position: relative;">
			<div class="blur">
				<a class="title" target="_blank" v-bind:href="mess_up(hit.url)"
				style="text-decoration: none; font-size: 120%;">
				{{mess_up(hit.title)}}</a><br/>
				<span style="color:#006d21">{{mess_up(hit.url)}}</span>
				<div style="overflow-x: hidden;">
					<p class="snippet">{{{ mess_up(hit.snippet) }}}</p>
				</div>
			</div>
			<div style="position: absolute; top: 10px; width: 100%; top: 50%;
			text-align: center; text-shadow: 0.5px 0.5px white;">
			<p>Please <a class="btn" v-on:click="SE_auth()" href="javascript: void(0)">
			get authenticated</a> as StackExchange user to see blurred search result.
			(<a class="btn" href="blank.html" target="_blank">why?</a>)
			</p>
			</div>
		</div>
		<div v-else>
			<span class="docid">{{hit.docid}}</span>
			<span class="score">{{hit.score}}</span>
			<a class="title" target="_blank" v-bind:href="hit.url"
			style="text-decoration: none; font-size: 120%;">
			{{hit.title}}</a><br/>
			<span style="color:#006d21">{{hit.url}}</span>
			<div style="overflow-x: hidden;">
				<p class="snippet">{{{ surround_special_html(hit.snippet) }}}</p>
			</div>
		</div>
	</li>
	</ol>
</div>
<!-- Search Results END -->

<!-- Navigator -->
<div v-show="ret_code == 0"
	style="text-align: center; margin-bottom: 26px;" class="toleft">
	<span v-if="prev != ''">
		<b style="font-size:1.5em">←</b>
		<a class="page-navi" v-bind:onclick="prev" href="#">prev</a>
	</span>
	<span class="mainfont">Page {{cur_page}}/{{tot_pages}}</span>
	<span v-if="next != ''">
		<a class="page-navi" v-bind:onclick="next" href="#">next</a>
		<b style="font-size:1.5em">→</b>
	</span>
</div>

<!-- Footer -->
<div v-show="ret_code == 0"
	style="padding-top: 15px; background: #f4f6f8;
	box-shadow: 0 0 4px rgba(0,0,0,0.25);">

	<div style="text-align: right;" class="toleft">
		<a href="https://twitter.com/intent/tweet?text=Check%20this%20out%3A%20%40Approach0%2C%20A%20math-aware%20search%20engine%3A%20http%3A%2F%2Fwww.approach0.xyz"
		target="_blank" title="Tweet" class="twitter-share-button">
		<img class="social" src="images/social/Twitter.svg"></a>

		<a href="http://www.reddit.com/submit?url=https%3A%2F%2Fwww.approach0.xyz&title=Check%20out%20this%20math-aware%20search%20engine!"
		target="_blank" title="Submit to Reddit">
		<img class="social" src="images/social/Reddit.svg"></a>

		<script async defer src="https://buttons.github.io/buttons.js"></script>
		<a class="github-button" href="https://github.com/approach0/search-engine"
		data-count-href="/approach0/search-engine/stargazers" data-count-api="/repos/approach0/search-engine#stargazers_count"
		data-count-aria-label="# stargazers on GitHub" aria-label="Star approach0/search-engine on GitHub">Star</a>
	</div>

</div>
<!-- Footer END -->

</div>
<!-- Search App END -->

</body>
</html>
