#!/bin/sh

# To install uglify-js: 
# npm install uglify-js -g

cat search.css \
	qry-box.css \
	font.css \
	quiz.css \
	> all.css


uglifyjs --compress --mangle -o bundle.min.js -- \
	tex-render.js \
	vendor/typed/typed.js \
	search.js \
	quiz-list.js \
	quiz.js \
	pad.js \
	qry-box.js \
	footer.js

new_hash=$(tr -dc 'a-f0-9' < /dev/urandom | head -c16)
sed -i -E "s/hash=([a-z0-9]+)/hash=${new_hash}/" index.php
