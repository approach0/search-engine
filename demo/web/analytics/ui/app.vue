<template>
<v-app light>

<v-form id="form">
<v-container fluid>

<v-layout row wrap justify-space-between>
  <v-flex>
    <v-text-field v-model="from" prepend-icon="date_range"
      :rules="timeRules" :counter="10" label="From">
    </v-text-field>
  </v-flex>

  <v-flex>
    <v-text-field v-model="to" prepend-icon="date_range"
      :rules="timeRules" :counter="10" label="To">
    </v-text-field>
  </v-flex>

  <v-flex>
    <v-text-field v-model="max" prepend-icon="format_list_numbered"
      :rules="numberRules" :counter="3" label="Maximum items">
    </v-text-field>
  </v-flex>

  <v-flex>
    <v-checkbox v-model="showQueries" label="Show query detail"></v-checkbox>
  </v-flex>

  <v-flex>
    <v-btn color="primary" @click="show()" block>show</v-btn>
  </v-flex>
</v-layout>

<v-layout justify-space-around>
  <v-flex xs12 md3>
  </v-flex>
</v-layout>

</v-container>
</v-form>

<v-container fill-height fluid>
  <v-layout align-center justify-center>

    <v-timeline dense clipped v-show="results.length > 0">
      <v-timeline-item class="mb-3" small v-for="(item, i) in results" v-bind:key="i">
        <v-layout justify-space-between wrap>
          <v-flex xs6><b>{{show_desc(item)}}</b></v-flex>
          <v-flex xs6><b>{{item.location}}</b></v-flex>
          <v-flex xs12 text-xs-left>{{show_time(item)}}</v-flex>
        </v-layout>
        <v-layout justify-start wrap>
          <v-flex v-for="(kw, j) in item.kw" v-bind:key="j">
            <v-chip> {{show_keyword(kw, item.type[j])}} </v-chip>
          </v-flex>
        </v-layout>
      </v-timeline-item>
    </v-timeline>

    <div v-show="results.length == 0">
      <img src="./images/logo.png"/>
    </div>

  </v-layout>
</v-container>

<v-footer dark height="auto">
  <v-card class="flex" flat tile>
    <v-card-actions class="grey py-3 justify-center">
      <strong>Approach0 Query log</strong>
    </v-card-actions>
  </v-card>
</v-footer>

</v-app>
</template>

<script>
import $ from 'jquery' /* AJAX request lib */
var moment = require('moment');

export default {
  methods: {
    show() {
      var vm = this;
      $.ajax({
        url: `http://localhost/stats-api/pull/query-items/${vm.max}`,
        type: 'GET',
        success: (data) => {
          console.log(data); /* print */
          vm.results = data['res'];
          for (var i = 0; i < vm.results.length; i++) {
            var item = vm.results[i];
            vm.$set(vm.results[i], 'location',  'Unknown location');
            ((i) => {
              this.get_geo_info(item.ip, (info) => {
                let loc = `${info.city}, ${info.country_name}`;
                if (info.country_name === undefined)
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
        url: `https://ipapi.co/${ip}/json/`,
        type: 'GET',
        success: (data) => {
          callbk(data);
        },
        error: (req, err) => {
          console.log(err);
        }
      });
    },
    show_desc(item) {
      var vm = this;
      return `IP: ${item.ip}`
    },
    show_keyword(kw, type) {
      if (type == 'tex')
        return `$$ ${kw} $$`
      else
        return `${kw}`
    },
    show_time(item) {
      var m = moment(item.time);
      const format = m.format('MMMM Do YYYY, H:mm:ss');
      const fromNow = m.fromNow();
      return `${format} (${fromNow})`;
    },
    render() {
      MathJax.Hub.Queue(["Typeset", MathJax.Hub]);
    }
  },
  data: function () {
    return {
      from: '0000-01-01',
      to: new Date().toISOString().substr(0, 10),
      max: 30,
      showQueries: false,
      timeRules: [
        v => /\d{4}-\d{2}-\d{2}/.test(v) || 'Invalid time format'
      ],
      numberRules: [
        v => /^\d+$/.test(v) || 'Invalid number'
      ],
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
</style>
