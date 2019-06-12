<template>
<v-app light>

<v-form id="form">
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
  </v-flex>
</v-layout>

</v-container>
</v-form>

<div class="pa-5" v-show="results.length != 0">
  From {{show_time(from, false)}} to {{show_time(to, false)}},
  there are {{n_queries}} queries ({{n_uniq_ip}} unique IPs).
</div>

<v-container fill-height fluid>
  <v-layout align-center justify-center>
    <v-timeline dense v-show="results.length > 0">
      <v-timeline-item class="mb-3" color="grey lighten-1"
       small v-for="(item, i) in results" v-bind:key="i">
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
var moment = require('moment');

export default {
  watch: {
    'from': function (val) {
      this.refresh();
    },
    'to': function (val) {
      this.refresh();
    },
    'max': function (val) {
      this.refresh();
    },
    'ip': function (val) {
      this.refresh();
    },
    'showQueries': function (val) {
      this.refresh();
    }
  },
  mounted: function () {
    this.refresh();
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
            vm.$set(vm.results[i], 'location',  'Unknown location');
            ((i) => {
              this.get_geo_info(item.ip, (info) => {
                let loc = `${info.city}, ${info.region}, ${info.country}`;
                info.country = info.country || 'Unknown';
                if (info.country == 'Unknown')
                  loc = 'Unknown location';
                vm.$set(vm.results[i], 'location', loc);
              })
            })(i);
          }
          setTimeout(function(){ vm.render(); }, 500);
        },
        error: (req, err) => {
          console.log(err);
        }
      });
    },
    get_geo_info(ip, callbk) {
      $.ajax({
        url: '/stats-api/pull/ip-info/' + ip,
        type: 'GET',
        success: (data) => {
          callbk(data['res']);
        },
        error: (req, err) => {
          console.log(err);
        }
      });
    },
    mask_ip(ip) {
      if (ip.trim() == '') return '*';
      const masked = ip.split('.').slice(0,2).join('.');
      return masked + '.*.*';
    },
    filter_ip(ip) {
      this.ip = ip;
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
      MathJax.Hub.Queue(["Typeset", MathJax.Hub]);
    }
  },
  data: function () {
    return {
      last_index_update: '2019-06-03T19:16:00',
      from: '0000-01-01',
      to: new Date().toISOString().substr(0, 10),
      max: 30,
      ip: '',
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
</style>
