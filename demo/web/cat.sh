#!/bin/sh
cat search.css \
	qry-box.css \
	font.css \
	quiz.css \
	> all.css

cat tex-render.js \
	vendor/typed/typed.js \
	search.js \
	quiz-list.js \
	quiz.js \
	pad.js \
	qry-box.js \
	footer.js \
	> bundle.js

new_hash=$(tr -dc 'a-f0-9' < /dev/urandom | head -c16)
sed -E "s/hash=([a-z0-9]+)/hash=${new_hash}/" index.php
