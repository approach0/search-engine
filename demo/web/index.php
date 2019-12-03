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
<title>Approach0: A Math-aware search engine</title>
<meta charset="utf-8"/>
<meta name="description" content="Approach Zero: A math-aware search engine. Search millions of math formulas.">
<meta name="keywords" content="Approach0, Approach Zero, ApproachO, mathematics, math, formula, equation, math search, equation search, formula search, search engine" />
<link rel="shortcut icon" href="images/favicon.ico">

<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/font-awesome@4.7.0/css/font-awesome.min.css" type="text/css"/>
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/mathquill@0.10.1-a/build/mathquill.css" type="text/css"/>
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/katex@0.10.2/dist/katex.min.css" type="text/css"/>
<link rel="stylesheet" href="all.css?hash=3085682ee277e029" type="text/css"/>
<!-- -->
<script src="https://cdn.jsdelivr.net/npm/polyfill@0.1.0/index.min.js"></script>
<script src="https://cdn.jsdelivr.net/gh/approach0/mathjax-v3@cdn/components/dist/tex-chtml.js"></script>

<script type='text/javascript' src='vendor/stackexchange/oauth.js'></script>

<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/jquery@1.12.4/dist/jquery.min.js"></script>
<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/vue@1.0.26/dist/vue.min.js"></script>
<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/mathquill@0.10.1-a/build/mathquill.min.js"></script>
<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/katex@0.10.2/dist/katex.min.js"></script>
<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/typed.js@2.0.10/lib/typed.min.js"></script>
<script type="text/javascript" src="bundle.min.js?hash=3085682ee277e029"></script>

<style>
img.social {
	height: 16px;
}
div.center-v-pad {
	height: 200px;
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
</style>
</head>

<body style="margin: 0" class="mainfont">
<div id="progress" style="position: fixed; border-top: 2px solid #46ece5; z-index:9999;"></div>

<!-- Query Box App -->
<div id="qry-input-vue-app" style="background: white;
	padding: 8px 8px 10px 8px; box-shadow: 0 0 4px rgba(0,0,0,0.25);">

<!-- Query input area -->
<div class="toleft" style="display: flex">
<div v-show="ever_focused" style="line-height: 60px;">
	<a href="." title="Approach0 (Version 3)">
	<img src="images/logo42-v3.png" style="vertical-align: middle;"/>
	</a>
</div>
<div id="qry-input-area" style="flex: 1; margin-left: 15px; border-radius: 6px;" v-on:click="area_on_click">
<ul class="qry-li-wrap"><template v-for="i in items">
		<li v-if="i.type == 'term'" class="qry-li">
			<div class="qry-div-fix">
				<span style="padding-right: 12px;">{{{i.str}}}</span>
				<span title="delete" class="dele" v-bind:onclick="'dele_kw('+$index+')'">
					<i class="fa fa-times" aria-hidden="true"></i>
				</span>
			</div>
		</li>
		<li v-if="i.type == 'tex'" class="qry-li">
			<div class="qry-div-fix">
				<span style="padding-right: 12px;">[imath]{{i.str}}[/imath]</span>
				<span title="edit" class="edit" v-bind:onclick="'edit_kw('+$index+')'">
					<i class="fa fa-pencil" aria-hidden="true"></i>
				</span>
				<span title="delete" class="dele" v-bind:onclick="'dele_kw('+$index+')'">
					<i class="fa fa-times" aria-hidden="true"></i>
				</span>
			</div>
		</li>
		<li v-if="i.type == 'term-input'" class="qry-li">
			<input v-on:keyup="on_input" v-on:keydown.delete="on_del" v-on:paste="on_paste" v-model="i.str"
			type="text" id="qry-input-box" class="pl_holder"
			placeholder="Enter query keywords here, type $ for math formula."/>
		</li>
		<li v-if="i.type == 'tex-input'" class="qry-li">
			<span id="math-input"></span>
			<span class="pl_holder">
			<b>You are editing a math formula</b>.
			When you finish, press enter or click <a @click="on_finish_math_edit">here</a>.</span>
		</li>
</template></ul>
</div>
</div>
<!-- Query input area END -->

<!-- Search button and options -->
<div style="padding-top: 18px; padding-bottom: 5px; position: relative" class="toleft">
	<div style="display: inline-block; width: 15px;">
	</div>

	<a style="text-decoration: none; font-size: 14px;"
	href="/guide" target="_blank">
		<i class="fa fa-external-link" aria-hidden="true"></i> Guide
	</a>

	<span class="collapse" title="Lookup TeX commands" id="handy-pad-expander">
		<i class="fa fa-angle-down" aria-hidden="true"></i> Symbols
	</span>
	<div id="handy-pad">
		<p>Unfamiliar with entering math symbols?
		Check out a helpful hand written math recognizer
		<a target="_blank" href="https://webdemo.myscript.com/views/math/index.html">
		here</a>.</p>
		<p> Alternatively, you can click button to insert symbols:</p>
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

		<p>Above only lists most frequently used math snippets. Refer to <a target="_blank"
		href="http://www.onemathematicalcat.org/MathJaxDocumentation/TeXSyntax.htm">
		this link</a> for a complete math-related TeX commands and paste what you need here.
		</p>
		<p>Willing to learn some Tex? <a target="_blank" href=
		"https://en.wikibooks.org/wiki/LaTeX">Here</a> is a helpful tutorial.
		</p>
		<p>Want more? Make a request and add more buttons
		<a target="_blank" href="https://github.com/approach0/search-engine/blob/master/demo/web/pad.js">here</a>.
		</p>
		<hr class="vsep"/>
	</div>

	<span class="collapse" title="Raw query and API">
		<i class="fa fa-angle-down" aria-hidden="true"></i> Raw query
	</span>
	<div>
		<p>Know TeX? Great! You may want to edit raw query below directly (separate keywords by commas).</p>
		<input id="qry" style="padding-left: 6px; margin-bottom: 6px; width:100%;" type="text" v-model="raw_str" v-on:keyup="on_rawinput"
		placeholder="Enter raw query ..."/>

		<!-- hidden URI parameters -->
		<input id="q" type="hidden" value=
		"<?php if (isset($_GET['q']) && is_scalar($_GET['q'])) echo htmlentities($_GET['q'], ENT_QUOTES,'UTF-8'); ?>"/>
		<input id="p" type="hidden" value=
		"<?php if (isset($_GET['p']) && is_scalar($_GET['p'])) echo htmlentities($_GET['p'], ENT_QUOTES,'UTF-8'); ?>"/>
	</div>

<!--
	<span v-show="en_donation" class="collapse" title="Donate" id="donate-expander">(+) donations </span>
	<div v-show="en_donation">
		<h3>Please consider to donate</h3>
		<div v-show="SE_user == 0">
			<p>If this project has ever helped you in anyway, please consider to
			sponsor me to keep maintaining and pushing forward.</p>

			<p>Interested to donate?
			<a class="btn" v-on:click="SE_auth()" href="javascript: void(0)">
			Get authenticated</a> as StackExchange user to proceed to donation options
			(<a class="btn" href="/backers/letter" target="_blank">why?</a>).

		</div>

		<div id="donation" v-else>

		<p>Here are some donation options:</p>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site" v-bind:netid="SE_netID"
		 stripeid="plan_FHtUQUawetBrC8">
		<b><span style="color: red;">♡ </span> Fibonacci Sponsor</b>
		<p>($ 3.36 / month)</p>
		<p>
			Subscription support at around Reciprocal Fibonacci constant cost per month.
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site" v-bind:netid="SE_netID"
		 >
		<b><span style="color: red;">♡ </span> Feigenbaum Sponsor</b>
		<p>($ 4.67 / month)</p>
		<p>
			Subscription support at around Feigenbaum constant cost per month.
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site" v-bind:netid="SE_netID"
		 stripeid="sku_FHszC4bhsTUNQb">
		<b><span style="color: red;">♡ </span> Epsilon Backer</b>
		<p>($ 10 one time)</p>
		<p>
			Single-time donation to encourage Approach0 to stay on non-imaginary axis.
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site" v-bind:netid="SE_netID"
		 >
		<b><span style="color: red;">♡ </span> Euler Infinity Sponsor</b>
		<p>($ 50 / month)</p>
		<p>
			Cover the hosting cost of this site regularly to help Approach0 exist online for arbitrary large number of sponsored time.
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site" v-bind:netid="SE_netID"
		 >
		<b><span style="color: red;">♡ </span> Unity Backer</b>
		<p>($ 50 one time)</p>
		<p>
			Cover the hosting cost of this site for another one month (one time donation).
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site" v-bind:netid="SE_netID"
		 >
		<b><span style="color: red;">♡ </span>Ludwig Schläfli Backer</b>
		<p>($ 80 one time)</p>
		<p>
			Cover the domain name cost of approach0.xyz for another year (one time donation).
		</p>
		</button>

		<button class="sponsor pad" v-bind:account="SE_user" v-bind:site="SE_site" v-bind:netid="SE_netID"
		 >
		<b><span style="color: red;">♡ </span> First-prime Backer</b>
		<p>($ 100 one time)</p>
		<p>
			Cover the hosting cost of this site for another two months (one time donation).
		</p>
		</button>

<p>
	Your payment details will be processed by <a href="https://stripe.com" target="_blank">Stripe</a> (for credit/debit cards) and a record of your donation will be stored in the database of this site. A (subscribed) sponsor will be billed at the beginning of each month, until <a target="_blank" href="https://github.com/approach0/search-engine/issues/new">a request</a> is received or when this site no longer operates. Refunds can be provided up to 1 month.
</p>
<p>
You can also donate to this project via bitcoin, Paypal, Alipay or WeChat. If you choose these methods, please leave a message afterwards (for example, send a notice <a href="https://github.com/approach0/search-engine/issues/new" target="_blank">here</a>) about your donation time and amount, in order to update your sponsor/backer membership.
</p>

	<h3>Benefits of being a Sponsor or Backer</h3>
	<ul>
		<li>Complete search results are provided to all sponsors or backers.</li>
		<li>Sponsor can <a target="_blank" href="https://github.com/approach0/search-engine/issues/new">request</a> to place a logo image on this site, in the name of a private entity or company.
		<li>Within the budget limit, this site will try to index more data sources upon any <a target="_blank" href="https://github.com/approach0/search-engine/issues/new">request</a> from sponsor or backer.</li>
		<li>Your StackExchange flair will be shown in our <a href="/backers" target="_blank">list of sponsors or backers</a> and your support will always be appreciated!</li>
	</ul>

	</div>

		<script src="https://js.stripe.com/v3"></script>
		<div id="stripe-error-message" style="color: red"></div>
<script>
var stripe = Stripe('pk_test_2TIdFYWCiMtR1Dt8Qg7pGczn00YUkb2ROx');
$('#donation button').each(function() {
	$(this)[0].addEventListener('click', function () {
		var stripe_id = $(this).attr('stripeid') || 'none_empty';
		var SE_account = $(this).attr('account') || 0;
		var SE_netID = $(this).attr('netid') || 0;
		var SE_site = $(this).attr('site') || 'https://stackexchange.com';
		var type = stripe_id.split('_')[0];
		var order = {quantity: 1};
		order[type] = stripe_id;

		var options = {
			items: [order],
			successUrl: 'https://approach0.xyz/backers/?id={CHECKOUT_SESSION_ID}',
			cancelUrl: 'https://approach0.xyz/backers/?id=0',
			clientReferenceId: '' + SE_netID + '@' + SE_site + '@' + SE_account
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

function expand_donation_options() {
	/* expand */
	setTimeout(function(){
		head = $("#donate-expander");
		below = head.next();
		below.show(function () {
			t = head.text();
			t = t.replace("+", "-");
			head.text(t);
			$('#init-footer').stickToBottom(aaaaaaa);
		});
	}, 500);

	/* scroll to the top */
	$("html, body").animate({ scrollTop: 0 }, "fast");
}

$(document).ready(function() {
	/* show this donate expander on #donate URI */
	var anchor_name = window.location.href.split('#')[1] || 'none';
	if (anchor_name == 'donate') {
		expand_donation_options();
	}
});

/* StackExchange OAuth2 initialization */
SE.init({
	clientId: 15681,
	key: 'tEI8QC487ZIN5Pu9I1nY4A((',
	channelUrl: 'https://approach0.xyz/backers/letter',
	//channelUrl: 'http://localhost/a0/backers/letter', /* for test */
	complete: function (data) {
		// console.log('[SE OAuth2]', data);
	}
});
</script>
	</div>
-->

	<button style="position: absolute; right: 10px; top: 15px;"
	type="button" id="search_button">
		<i class="fa fa-search" style="margin-right: 10px;"></i>
		Search
	</button>

</div>
<!-- Search button and options END -->

</div>
<!-- Query Box App END -->

<!-- Quiz App -->
<div id="quiz-vue-app" v-show="!hide">
	<div id="quiz" class="toleft" style="padding-bottom: 100px;">
		<div class="center-v-pad"></div>
		<div class="center-horiz">
			<p id="quiz-question">
			<b>Question</b>: &nbsp; {{Q}}
			</p>
		</div>
		<div class="center-horiz" style="padding-top:20px;">
			<span id="quiz-hint"></span>
		</div>
	</div>

	<!-- Initial Footer -->
	<div v-show="!hide" id="init-footer"
	style="font-size: small; margin-top: 40px; width: 100%;
	bottom: 0px; position: absolute; background: #f4f6f8;
	padding-bottom: 15px; padding-top: 15px;
	box-shadow: 0 0 4px rgba(0,0,0,0.25);">
		<div class="toleft" style="text-align: center;">
			<a target="_blank" href="https://twitter.com/approach0"
			title="Approach0">
			<img src="images/logo32.png" style="vertical-align:middle;"/></a>
			=
			<a target="_blank" href="https://math.stackexchange.com/"
			title="Math StackExchange">
			<img src="images/math-stackexchange.png" style="vertical-align:middle;"/></a>
			+
			<a target="_blank" href="https://artofproblemsolving.com/community"
			title="Art of Problem solving (community)">
			<img src="images/aops.png" style="vertical-align:middle;"/></a>
			+
			<span style="color: red; font-size:18px; font-weight:bold;
			vertical-align:middle;">
				<i class="fa fa-heart-o" aria-hidden="true"></i>
			</span>

			<p>Approach0: A math-aware search engine.</p>
		[<a style="text-decoration: none;" href="/stats" target="_blank">
		query log
		</a>]
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
	<i class="fa fa-cog fa-spin fa-fw"></i>
	{{ret_str}}
	<span class="dots"></span></p>
	</template>
	<p v-else>
		<i class="fa fa-exclamation-circle" aria-hidden="true"></i>
		{{ret_str}}. (return code #{{ret_code}})
	</p>
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
				style="text-decoration: none; font-size: 1.5em;">
				{{mess_up(hit.title)}}</a><br/>
				<span style="color:#006d21">{{mess_up(hit.url)}}</span>
				<div style="overflow-x: hidden;">
					<p class="snippet">{{{ mess_up(hit.snippet) }}}</p>
				</div>
			</div>
			<div style="position: absolute; top: 10px; width: 100%; top: 50%;
			text-align: center; text-shadow: 0.5px 0.5px white;">
			<p v-if="SE_user == 0">
				Please <a class="btn" v-on:click="SE_auth()" href="javascript: void(0)">
				get authenticated</a> as StackExchange user to see blurred search result.
				(<a class="btn" href="/backers/letter" target="_blank">why?</a>)
			</p>
			<p v-if="SE_user != 0 && !unlock">
				View complete search results by
				<a class="btn" href="javascript: void(0)"
				onclick="expand_donation_options()"> supporting</a>
				this project.
				(<a class="btn" v-on:click="SE_verify()" href="javascript: void(0)">refresh</a>)
				<p v-if="verifying">verifying ...</p>
			</p>
			</div>
		</div>
		<div v-else>
			<span class="docid">{{hit.docid}}</span>
			<span class="score">{{hit.score}}</span>
			<a class="title" target="_blank" v-bind:href="hit.url"
			style="text-decoration: none; font-size: 1.5em;">
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
<div id="navigator" v-show="ret_code == 0" class="toleft"
	style="text-align: center; margin-bottom: 50px; margin-top: 30px;">

	<span class="pagination">
		<a v-if="cur_page - 1 > 0" title="Previous page"
			v-bind:onclick="'goto_page(' + (cur_page - 1) + ')'">
			<i class="fa fa-angle-left" aria-hidden="true"></i>
		</a>
		<template v-if="-1 == pages.indexOf(1)">
			<a title="First page"
				v-bind:onclick="'goto_page(1)'">
				1
			</a>
			<span v-if="-1 == pages.indexOf(2)">
			...
			</span>
		</template>
		<a v-for="p in pages" v-bind:onclick="'goto_page(' + p + ')'"
		v-bind:class="{active: p == cur_page}">
			{{p}}
		</a>
		<template v-if="-1 == pages.indexOf(tot_pages)">
			<span v-if="-1 == pages.indexOf(tot_pages - 1)">
			...
			</span>
			<a title="Last page"
				v-bind:onclick="'goto_page(' + tot_pages + ')'">
				{{tot_pages}}
			</a>
		</template>
		<a v-if="cur_page + 1 <= tot_pages" title="Next page"
			v-bind:onclick="'goto_page(' + (cur_page + 1) + ')'">
			<i class="fa fa-angle-right" aria-hidden="true"></i>
		</a>
	</span>

</div>

<!-- Footer -->
<div id="search-footer" v-show="ret_code == 0"
	style="padding-top: 15px; background: #f4f6f8; width: 100%;
	box-shadow: 0 0 4px rgba(0,0,0,0.25);">

	<div style="text-align: right; letter-spacing: 5px;">
		<a href="https://twitter.com/intent/tweet?text=Check%20this%20out%3A%20%40Approach0%2C%20A%20math-aware%20search%20engine%3A%20http%3A%2F%2Fwww.approach0.xyz"
		target="_blank" title="Share on Twitter" class="twitter-share-button">
			<i class="fa fa-twitter fa-lg" aria-hidden="true"></i></a>

		<a href="http://www.reddit.com/submit?url=https%3A%2F%2Fwww.approach0.xyz&title=Check%20out%20this%20math-aware%20search%20engine!"
		target="_blank" title="Submit to Reddit">
			<i class="fa fa-reddit-alien fa-lg" aria-hidden="true"></i></a>

		<a href="https://github.com/approach0/search-engine"
		target="_blank" title="Star on Github">
			<i class="fa fa-github fa-lg" aria-hidden="true"></i></a>
	</div>

</div>
<!-- Footer END -->

</div>
<!-- Search App END -->

</body>
</html>
