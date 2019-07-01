<template>
<v-app light>

<v-container>
  <v-layout row>
	  <img style="margin-top: 8px; height: 32px; width: 32px" src="./images/logo32.png"/>
    <div class="headline pa-2" style="color: #828282">
    Shout out to the contributors
    </div>
  </v-layout>
  <div style="margin-top: 3em;"></div>
  <v-container fill-height>
  <v-layout justify-center wrap>
    <v-flex v-for="(s, i) in supporters" xs10 sm6 md4>
      <v-badge left bottom overlap color="cyan">
      <template v-slot:badge>
        <v-icon v-if="s.badges.length == 0" color="white">build</v-icon>
        <v-icon v-else color="white">favorite_border</v-icon>
      </template>
      <a v-bind:href="'https://' + s.site + '/users/' + s.account" target="_blank">
      <img v-bind:src="'https://' + s.site + '/users/flair/' + s.account + '.png'"
        width="208" height="58"/>
      </a>
      </v-badge>
      <div class="badges grey--text" style="text-align: center; max-width: 208px">
      <p v-if="s.badges.length > 0"> {{s.badges.join(', ')}} </p>
      <p v-else> Code contributor</p>
      </div>
    </v-flex>
  </v-layout>
  </v-container>
</v-container>

<v-footer height="auto" color="grey" class="lighten-2">
  <v-layout justify-center style="text-align:center">
    <v-flex xs10 sm6 md3>
      <div style="margin-top: 2em;"></div>
      <v-icon color="red">favorite_border</v-icon>
      <p> Thank you!</p>
      <p><strong>You are the integral part of this site.</strong></p>
    </v-flex>
  </v-layout>
</v-footer>
</v-app>
</template>

<script>
import $ from 'jquery' /* AJAX request lib */

export default {
  mounted: function () {
    this.pulldata();
  },
  methods: {
    pulldata() {
      console.log('test');
    }
  },
  data: function () {
    return {
      supporters: [
        {
          account: 3244601,
          site: 'stackexchange.com',
          badges: []
        },
        {
          account: 290240,
          site: 'math.stackexchange.com',
          badges: []
        },
        {
          account: 267077,
          site: 'stackexchange.com',
          badges: ['First-prime Backer']
        }
      ]
    }
  }
}
</script>
<style>
.badges p {
  text-shadow: -1px -1px 1px #d4d4d4, 1px 1px 1px #fafafa;
  color: #9c8468;
}
</style>
