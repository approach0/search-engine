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
        <v-icon v-if="s.badges.join().includes('Sponsor')" color="white">monetization_on</v-icon>
        <v-icon v-else-if="s.badges.join().includes('Backer')" color="white">favorite_border</v-icon>
        <v-icon v-else color="white">build</v-icon>
      </template>
      <a v-bind:href="s.site + '/users/' + s.account" target="_blank">
      <img v-bind:src="s.site + '/users/flair/' + s.account + '.png'"
        width="208" height="58"/>
      </a>
      </v-badge>
      <div class="badges grey--text" style="text-align: center; max-width: 208px">
      <p> {{s.badges.join(', ')}} </p>
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
      const vm = this;
      $.ajax({
        url: `/backers/query`,
        type: 'GET',
        success: (data) => {
          const arr = data['res'];
          vm.supporters = arr;
        },
        error: (req, err) => {
          console.log(err);
        }
      });
    }
  },
  data: function () {
    return {
      supporters: []
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
