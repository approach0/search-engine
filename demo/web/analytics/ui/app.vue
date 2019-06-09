<template>
<v-app light>

<v-form>
<v-container fluid>

<v-layout row wrap justify-space-between>
  <v-flex xs1>
      <img src="./images/logo.png"/>
  </v-flex>
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
    <v-btn color="primary" @click="foo()" block>show</v-btn>
  </v-flex>
</v-layout>

<v-layout justify-space-around>
  <v-flex xs12 md3>
  </v-flex>
</v-layout>

</v-container>
</v-form>

<v-container fill-height>
    <v-layout align-center justify-center>
      <v-icon>watch_later</v-icon>
    </v-layout>
</v-container>

<v-footer dark height="auto">
  <v-card class="flex" flat tile>
    <v-card-actions class="primary py-3 justify-center">
      <strong>Approach0 query log</strong>
    </v-card-actions>
  </v-card>
</v-footer>

</v-app>
</template>

<script>
import axios from 'jquery' /* AJAX request lib */

export default {
  methods: {
    foo() {
      alert(this.data);
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
      ]
    }
  }
}
</script>
<style>
img {max-width:100%;}
</style>
