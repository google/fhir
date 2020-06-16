// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package jsonpbhelper

import (
	"testing"
)

func TestSnakeToCamel(t *testing.T) {
	testCases := []struct {
		in  string
		out string
	}{
		{"snake_case", "SnakeCase"},
		{"word", "Word"},
		{"Crazy_MIxeD_CASE", "CrazyMixedCase"},
		{"ACTION_AD", "ActionAd"},
		{"GPLUS_SHOPPING_JACKPOT", "GplusShoppingJackpot"},
		{"US_MID_TERM_ELECTION_MAP", "UsMidTermElectionMap"},
		{"XML", "Xml"},
		{"snake_2_2_case", "Snake22Case"},
		{"snake_22_case", "Snake22Case"},
		{"snake_case_2", "SnakeCase2"},
	}

	for i, tc := range testCases {
		got := SnakeToCamel(tc.in)
		if got != tc.out {
			t.Errorf("%d. SnakeToCamel(%v) => %v, want %v", i, tc.in, got, tc.out)
		}
	}
}

func TestCamelToSnake(t *testing.T) {
	testCases := []struct {
		in  string
		out string
	}{
		{"SnakeCase", "snake_case"},
		{"Word", "word"},
		{"lowerUpperUpper", "lower_upper_upper"},
		{"ActionAd", "action_ad"},

		// non-letters
		{"SnakeCase2", "snake_case_2"},
		{"Snake22Case", "snake_2_2_case"},

		// single upper-case letter
		{"GPlusShoppingJackpot", "g_plus_shopping_jackpot"},

		// two-letter acronyms
		{"USMidTermElectionMap", "u_s_mid_term_election_map"},
		{"LocalKPHangoutLinks", "local_k_p_hangout_links"},
		{"VideoResultGroupFullUI", "video_result_group_full_u_i"},
	}

	for i, tc := range testCases {
		got := CamelToSnake(tc.in)
		if got != tc.out {
			t.Errorf("%d. CamelToSnake(%v) => %v, want %v", i, tc.in, got, tc.out)
		}
	}
}

func TestSnakeToLowerCamel(t *testing.T) {
	tests := []struct {
		input, want string
	}{
		{
			"",
			"",
		},
		{
			"a",
			"a",
		},
		{
			"snake_name",
			"snakeName",
		},
		{
			"snake_name_super_long",
			"snakeNameSuperLong",
		},
	}
	for _, test := range tests {
		if got := SnakeToLowerCamel(test.input); got != test.want {
			t.Errorf("snakeToLowerCamel(\"%v\"): got %v, want %v", test.input, got, test.want)
		}
	}
}
