const { dependencies } = require('./package.json')

var webpack = require('webpack')

/* HTML template engine */
const HtmlWebpackPlugin = require('html-webpack-plugin')

/* vue-loader 15 requires a webpack plugin to function */
const VueLoaderPlugin = require('vue-loader/lib/plugin')

module.exports = {
	/* default is development mode, run 'yarn run build' for production. */
	mode: 'development',
	entry: { /* required field */
		app: __dirname + '/ui/main.js'
	},
	output: { /* required field */
		filename: 'bundle.js'
	},
	module: {
		rules: [
			{ test: /\.vue$/, use: 'vue-loader' },
			{ test: /\.css$/, use: [
				'style-loader', 'css-loader'
				/* need both here to both inject the CSS and
				 * convert CSS to JavaScript module. */
			]},
			{
				test: /\.(eot|woff|woff2|svg|ttf)([\?]?.*)$/,
				use: [{
					loader: 'file-loader',
					options: {
						outputPath: 'fonts/'
					}
				}]
			},
      {
        test: /\.(jpe?g|png|gif|svg)$/i,
				use: [{
					loader: 'file-loader',
					options: {}
				}]
      }
		]
	},
	resolve: {
		alias: {
			vue: 'vue/dist/vue.js'
		}
	},
	plugins: [
		new HtmlWebpackPlugin({
			favicon: 'assets/favicon.ico',
			hash: true, /* cache busting */
			template: __dirname + '/ui/index-template.html',
			filename: 'index.html' /* default goes to ./dist */
		}),
		new VueLoaderPlugin()
	],
	devServer: {
		contentBase: __dirname + '/dist',
		port: 19985,
		historyApiFallback: true /* make history-mode routing possible */
	}
}
