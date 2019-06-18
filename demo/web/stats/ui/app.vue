<template>
<v-app light>

<v-form id="form" v-model="form_valid">
<v-container fluid>

<v-layout row wrap justify-space-between>
  <v-flex xs4 md2>
    <v-text-field v-model="from" prepend-icon="date_range"
      :rules="timeRules" :counter="10" label="From">
    </v-text-field>
  </v-flex>

  <v-flex xs4 md2>
    <v-text-field v-model="to" prepend-icon="date_range"
      :rules="timeRules" :counter="10" label="To">
    </v-text-field>
  </v-flex>

  <v-flex xs4 md2>
    <v-text-field v-bind:value="mask_ip(ip)" @click:clear="clear_ip_filter()"
      prepend-icon="filter_list" :counter="15" label="Specified IP"
      readonly clearable>
    </v-text-field>
  </v-flex>

  <v-flex xs4 md2>
    <v-text-field v-model="max" prepend-icon="format_list_numbered"
      :rules="numberRules" :counter="3" label="Maximum items">
    </v-text-field>
  </v-flex>

  <v-flex xs4 md2>
    <v-checkbox v-model="showQueries" label="Show query detail"></v-checkbox>
  </v-flex>

  <v-flex xs4 md2>
    <v-btn color="primary" @click="refresh()" block>refresh</v-btn>
    <!-- <v-btn color="primary" @click="render()" block>render</v-btn> -->
  </v-flex>
</v-layout>

</v-container>
</v-form>

<div class="pa-5" v-if="results.length != 0">
  Unique IPs of past {{trend_days}} days (back from {{show_time(to, false)}}):
</div>
<v-layout align-center justify-center>
  <!-- <div v-for="(e, i) in trend"> {{e}} </div> -->
  <svg class="chart" width="500">
    <g v-for="(val, i) in trend">
      <title>{{trend_label[i] + ': val=' + val}}</title>
      <rect width="19" v-bind:x="i * 20"
        v-bind:height="trend_h(val, trend, 100)"
        v-bind:y="100 - trend_h(val, trend, 100)">
      </rect>
    </g>
  </svg>
</v-layout>

<div class="pa-5" v-if="results.length != 0">
  From {{show_time(from, false)}} to {{show_time(to, false)}},
  there are {{n_queries}} queries ({{n_uniq_ip}} unique IPs).
</div>

<v-container fill-height fluid>
  <v-layout align-center justify-center>
    <v-timeline dense v-show="results.length > 0">
      <v-timeline-item class="mb-3"
       small v-for="(item, i) in results" v-bind:key="item.ip + item.time">
        <v-layout justify-space-between wrap>
          <v-flex xs6>
            <b>IP:</b>
            <v-btn outline small @click="filter_ip(item.ip)">
              {{mask_ip(item.ip)}}
            </v-btn>
          </v-flex>
          <v-flex xs6><b>{{item.location}}</b></v-flex>
          <v-flex xs12 text-xs-left>
            <span v-if="!showQueries">
              Queries: {{item.counter}}, most recent:
            </span>
            {{show_time(item.time, true)}}
          </v-flex>
        </v-layout>
        <div v-if="showQueries">
          <v-flex v-for="(kw, j) in item.kw" v-bind:key="j">
            <v-chip class="limitw">
              {{show_keyword(kw, item.type[j])}}
            </v-chip>
          </v-flex>
          <v-flex md4>
            <v-btn flat round @click="search(i)">search again</v-btn>
          </v-flex>
        </div>
      </v-timeline-item>
    </v-timeline>

    <div v-show="results.length == 0">
      <img src="./images/logo.png"/>
    </div>

  </v-layout>
</v-container>

<v-footer dark height="auto">
  <v-card class="flex" flat tile>
    &nbsp; Last update of index: {{show_time(last_index_update, true)}}.
    <v-divider></v-divider>
    <v-card-actions class="grey py-3 justify-center">
      <strong>Approach0</strong> &nbsp; {{new Date().getFullYear()}}
    </v-card-actions>
  </v-card>
</v-footer>

</v-app>
</template>

<script>
import $ from 'jquery' /* AJAX request lib */
var urlpar = require('url');
var Base64 = require('Base64');
var moment = require('moment');

export default {
  watch: {
    'from': function () {
      if (this.form_valid) this.refresh();
    },
    'to': function () {
      if (this.form_valid) this.refresh();
    },
    'ip': function () {
      if (this.form_valid) this.refresh();
    },
    'max': function () {
      if (this.form_valid) this.refresh();
    },
    'showQueries': function () {
      if (this.form_valid) this.refresh();
    },
    'form_valid': function(val) {
      if (val) this.refresh();
    },
    'results': function (val) {
      var vm = this;
      vm.$nextTick(function () {
        vm.render();
      });
    },
  },
  mounted: function () {
    this.refresh();
    var vm = this;
    vm.$nextTick(function () {
      vm.render();
    });
  },
  methods: {
    api_compose(from, to, max, ip, detail) {
      var from_ip = `from-${ip}/`;
      if (ip.trim() == '') from_ip = '';
      if (detail)
        return `query-items/${from_ip}${max}/${from}.${to}`;
      else
        return `query-IPs/${from_ip}${max}/${from}.${to}`;
    },
    subname(str, sep) {
      str = str || '';
      if (str == 'Unknown')
        str = '';
      if (str != '')
        str = str + sep;
      return str;
    },
    refresh() {
      var vm = this;
      $.ajax({
        url: `/stats-api/pull/query-summary/${vm.from}.${vm.to}/`,
        type: 'GET',
        success: (data) => {
          const summary = data['res'][0];
          //console.log(summary);
          vm.n_uniq_ip = summary['n_uniq_ip'];
          vm.n_queries = summary['n_queries'];
        },
        error: (req, err) => {
          console.log(err);
        }
      });
      const api_uri = vm.api_compose(
        vm.from, vm.to, vm.max, vm.ip, vm.showQueries
      );
      $.ajax({
        url: '/stats-api/pull/' + api_uri,
        type: 'GET',
        success: (data) => {
          // console.log(data); /* print */
          vm.results = data['res'];
          for (var i = 0; i < vm.results.length; i++) {
            var item = vm.results[i];
            const loc = this.subname(item.city, ', ') +
            this.subname(item.region, ', ') + this.subname(item.country, '.');
            vm.$set(vm.results[i], 'location',  (loc == '') ? 'Unknown' : loc);
          }
        },
        error: (req, err) => {
          console.log(err);
        }
      });
      this.get_trend();
    },
    mask_ip(ip) {
      if (ip.trim() == '') return '*';
      const masked = ip.split('.').slice(0,2).join('.');
      return masked + '.*.*';
    },
    filter_ip(ip) {
      const url = urlpar.parse(window.location.href);
      window.open(url['pathname'] + '?loc=' + Base64.btoa(ip), '_blank');
    },
    clear_ip_filter() {
      this.ip = '';
    },
    show_keyword(kw, type) {
      if (type == 'tex')
        return `$$ ${kw} $$`;
      else
        return `${kw}`;
    },
    show_time(time, detail) {
      if (true != this.timeRules[0](time))
        return '?';
      var m = moment(time);
      if (detail) {
        const format = m.format('MMMM Do YYYY, H:mm:ss');
        const fromNow = m.fromNow();
        return `${format} (${fromNow})`;
      } else {
        const format = m.format('MMMM Do YYYY');
        return `${format}`;
      }
    },
    search(index) {
      var item = this.results[index];
      const prefix = 'https://approach0.xyz/search/?q=';
      var uri = '';
      for (var i = 0; i < item.kw.length; i++) {
        const kw   = item.kw[i];
        const type = item.type[i];
        if (type == 'tex')
          uri += `$${kw}$`;
        else
          uri += `${kw}`;
        if (i + 1 < item.kw.length)
          uri += ', ';
      }
      window.open(prefix + encodeURIComponent(uri), '_blank');
    },
    render() {
      $('.limitw span').each(function() {
        var ele = $(this).get(0);
        var prefix = $(this).html().trim().slice(0, 1);
        if (prefix == '$') {
          // console.log($(this).html());
          MathJax.Hub.Queue(
            ["Typeset", MathJax.Hub, ele]
          );
        }
      });
    },
    get_trend() {
      this.trend = [];
      const now = this.to;
      const tot = this.trend_days;
      var vm = this;
      for (var i = 0; i < tot; i ++) {
        this.trend.push(0);
        ((i) => {
          const m = moment(now).subtract(tot - i - 1, 'days');
          const day = m.format("YYYY-MM-DD");
          // console.log(tot - i - 1, day);
          $.ajax({
            url: `/stats-api/pull/query-summary/${day}.${day}/`,
            type: 'GET',
            success: (data) => {
              const summary = data['res'][0];
              vm.$set(vm.trend_label, i, day);
              vm.$set(vm.trend, i, summary['n_uniq_ip']);
              /* below is for test purpose */
              // vm.$set(vm.trend, i, Math.floor(Math.random() * 10));
              // vm.$set(vm.trend, i, i);
            },
            error: (req, err) => {
              console.log(err);
            }
          });
        })(i);
      }
    },
    trend_h(val, trend, height) {
      const max = Math.max(1, ...trend);
      const h = Math.ceil(height * (val / max));
      return h + 1;
    },
    uri_IP() {
      const uri_params = urlpar.parse(window.location.href, true)['query'];
      const ip_base64 = uri_params['loc'] || '';
      return Base64.atob(ip_base64);
    }
  },
  data: function () {
    return {
      form_valid: false,
      last_index_update: '2019-06-03T19:16:00',
      from: new Date().toISOString().substr(0, 10),
      to: new Date().toISOString().substr(0, 10),
      max: 30,
      ip: this.uri_IP(),
      showQueries: true,
      n_uniq_ip: 0,
      n_queries: 0,
      timeRules: [
        v => /\d{4}-\d{2}-\d{2}/.test(v) || 'Invalid time format'
      ],
      numberRules: [
        v => /^\d+$/.test(v) || 'Invalid number'
      ],
//      ipRules: [
//        v => /\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/.test(v) || 'Invalid IP format'
//      ],
      trend_days: 16,
      trend: [],
      trend_label: [],
      results: []
    }
  }
}
</script>
<style>
img {max-width:100%;}

#form {
  box-shadow: 0 3px 1px -2px rgba(0,0,0,.2), 0 2px 2px 0 rgba(0,0,0,.14), 0 1px 5px 0 rgba(0,0,0,.12);
  background-color: #e4e4e4;
}

.limitw {
  max-width: 60vw !important;
  overflow-x: auto;
  height: 100%;
}

.v-chip__content {
  height: auto !important;
  padding: 4px 4px 4px 12px !important;
}

.chart rect {
  fill: grey;
}

.chart rect:hover {
  fill: #1976d2;
}
</style>
