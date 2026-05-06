/*
/^-----^\   data: 2026-05-06
V  o o  V  file: src/features/automation/region_selector/region_selector.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef REGION_SELECTOR_HPP
#define REGION_SELECTOR_HPP

#include <array>
#include <cstdint>
#include <string_view>

#include "games/tf2/sdk/interfaces/steam_networking_utils.hpp"

namespace automation::region_selector
{

struct data_center
{
  const char* label;
  const char* code;
  std::uint64_t bit;
};

constexpr std::array<data_center, 47> data_centers{{
  {"amsterdam (ams)", "ams", 1ull << 0},
  {"frankfurt (fra)", "fra", 1ull << 1},
  {"falkenstein (fsn)", "fsn", 1ull << 2},
  {"helsinki (hel)", "hel", 1ull << 3},
  {"london (lhr)", "lhr", 1ull << 4},
  {"madrid (mad)", "mad", 1ull << 5},
  {"paris (par)", "par", 1ull << 6},
  {"stockholm 1 (sto)", "sto", 1ull << 7},
  {"stockholm 2 (sto2)", "sto2", 1ull << 8},
  {"warsaw (waw)", "waw", 1ull << 9},
  {"luxembourg (lux)", "lux", 1ull << 10},
  {"luxembourg 1 (lux1)", "lux1", 1ull << 11},
  {"luxembourg 2 (lux2)", "lux2", 1ull << 12},
  {"vienna (vie)", "vie", 1ull << 13},

  {"atlanta (atl)", "atl", 1ull << 14},
  {"moses lake (eat)", "eat", 1ull << 15},
  {"moses lake 2 (mwh)", "mwh", 1ull << 16},
  {"virginia (iad)", "iad", 1ull << 17},
  {"los angeles (lax)", "lax", 1ull << 18},
  {"oklahoma city (okc)", "okc", 1ull << 19},
  {"chicago (ord)", "ord", 1ull << 20},
  {"seattle (sea)", "sea", 1ull << 21},
  {"dallas (dfw)", "dfw", 1ull << 22},

  {"sao paulo (gru)", "gru", 1ull << 23},
  {"lima (lim)", "lim", 1ull << 24},
  {"santiago (scl)", "scl", 1ull << 25},
  {"buenos aires (eze)", "eze", 1ull << 26},

  {"mumbai (bom)", "bom", 1ull << 27},
  {"dubai (dxb)", "dxb", 1ull << 28},
  {"tokyo narita (gnrt)", "gnrt", 1ull << 29},
  {"hong kong (hkg)", "hkg", 1ull << 30},
  {"chennai/ambattur (maa)", "maa", 1ull << 31},
  {"new delhi (maa2)", "maa2", 1ull << 32},
  {"bangalore (maa3)", "maa3", 1ull << 33},
  {"manila (man)", "man", 1ull << 34},
  {"singapore (sgp)", "sgp", 1ull << 35},
  {"tokyo 1 (tyo)", "tyo", 1ull << 36},
  {"tokyo 3 (tyo2)", "tyo2", 1ull << 37},
  {"tokyo 2 (tyo1)", "tyo1", 1ull << 38},
  {"seoul (seo)", "seo", 1ull << 39},

  {"guangzhou (can)", "can", 1ull << 40},
  {"shanghai (sha)", "sha", 1ull << 41},
  {"tianjin (tsn)", "tsn", 1ull << 42},
  {"wuhan (wuh)", "wuh", 1ull << 43},
  {"chengdu (ctu)", "ctu", 1ull << 44},

  {"sydney (syd)", "syd", 1ull << 45},
  {"johannesburg (jnb)", "jnb", 1ull << 46},
}};

constexpr std::uint64_t all_region_bits = (1ull << 47) - 1ull;
constexpr int blocked_region_ping = 69420;
constexpr int preferred_region_ping = 1;

bool is_region_allowed(std::string_view region);
void set_region_allowed(std::uint64_t bit, bool allowed);
bool is_region_bit_allowed(std::uint64_t bit);
int adjust_ping(int original_ping, steam_networking_pop_id pop_id);
void refresh_ping_data();

} // namespace automation::region_selector

#endif
