<template>
<v-app light>

<v-toolbar>
  <v-text-field prepend-icon="search" v-model="data">
  </v-text-field>
  <v-spacer></v-spacer>
  <v-btn color="primary" @click="foo()">show</v-btn>
</v-toolbar>

{{data}}

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
      "data": ""
    }
  }
}
</script>
<style>
</style>
