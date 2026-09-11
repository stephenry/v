//========================================================================== //
// Copyright (c) 2022, Stephen Henry
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//========================================================================== //

#include "oob_error.h"

#include "../log.h"
#include "../test.h"
#include "cfg.h"
#include "directed.h"

namespace {

// Prefer an encoding that is representable in id_t / level_t but strictly
// greater than CONTEXT_N / ENTRIES_N when those parameters are not powers of
// two (the default build uses CONTEXT_N=10, ENTRIES_N=10 → 4b indices).
constexpr bool has_oob_context_encodings() {
  return (cfg::CONTEXT_N & (cfg::CONTEXT_N - 1)) != 0;
}

constexpr bool has_oob_level_encodings() {
  return (cfg::ENTRIES_N & (cfg::ENTRIES_N - 1)) != 0;
}

constexpr tb::prod_id_t oob_context_id() {
  return static_cast<tb::prod_id_t>(cfg::CONTEXT_N);
}

constexpr tb::level_t oob_level_id() {
  return static_cast<tb::level_t>(cfg::ENTRIES_N);
}

struct CheckOobContext : public tb::tests::Directed {
  CREATE_TEST_BUILDER(CheckOobContext);

  void program() override {
    V_NOTE("Test begins: OOB Context on Update and Lookup...");

    if (!has_oob_context_encodings()) {
      V_NOTE("CONTEXT_N is a power of two; no OOB Context encodings exist. Skip.");
      return;
    }

    // Populate a valid context so we can later prove OOB updates are NOPs.
    push_back(tb::UpdateCommand{0, tb::Cmd::Add, 42, 7});
    wait_cycles(10);

    // Lookup of Context 0 / level 0 must succeed.
    push_back(tb::QueryCommand{0, 0});
    wait_cycles(5);

    // Update to an OOB Context (e.g. 10 on a 10-Context machine, or 14 when
    // stimulus uses a larger encoding). Expect o_upd_error_r and no state
    // side-effects.
    const tb::prod_id_t oob = oob_context_id();
    V_NOTE("Issuing OOB Update to context ", static_cast<int>(oob));
    push_back(tb::UpdateCommand{oob, tb::Cmd::Add, 99, 1});
    wait_cycles(10);

    // Context 0 must be unchanged.
    push_back(tb::QueryCommand{0, 0});
    wait_cycles(5);

    // Lookup of the OOB Context must return error.
    V_NOTE("Issuing OOB Lookup to context ", static_cast<int>(oob));
    push_back(tb::QueryCommand{oob, 0});
    wait_cycles(5);

    // README example-style encoding: Context 14 when CONTEXT_N <= 14 and the
    // ID width can represent it (true for default CONTEXT_N=10, 4b id_t).
    if (cfg::CONTEXT_N <= 14) {
      constexpr tb::prod_id_t ctx14 = 14;
      V_NOTE("Issuing OOB Update/Lookup to context 14");
      push_back(tb::UpdateCommand{ctx14, tb::Cmd::Clr, 0, 0});
      wait_cycles(10);
      push_back(tb::QueryCommand{ctx14, 0});
      wait_cycles(5);
    }

    // Valid context still intact after OOB traffic.
    push_back(tb::QueryCommand{0, 0});
    wait_cycles(5);

    V_NOTE("Test ends...");
  }
};

struct CheckOobEntry : public tb::tests::Directed {
  CREATE_TEST_BUILDER(CheckOobEntry);

  void program() override {
    V_NOTE("Test begins: invalid Entry / n-th selection...");

    // Empty context: any level is an invalid entry selection.
    push_back(tb::QueryCommand{0, 0});
    wait_cycles(5);

    // Insert a single entry.
    push_back(tb::UpdateCommand{0, tb::Cmd::Add, 10, 3});
    wait_cycles(10);

    // Level 0 is valid.
    push_back(tb::QueryCommand{0, 0});
    wait_cycles(2);

    // Level past occupancy (but within ENTRIES_N) must error.
    if (cfg::ENTRIES_N > 1) {
      push_back(tb::QueryCommand{0, 1});
      wait_cycles(2);
    }

    // Level >= ENTRIES_N (representable when ENTRIES_N is not a power of two)
    // must error.
    if (has_oob_level_encodings()) {
      const tb::level_t oob_lvl = oob_level_id();
      V_NOTE("Issuing OOB level ", static_cast<int>(oob_lvl));
      push_back(tb::QueryCommand{0, oob_lvl});
      wait_cycles(5);
    }

    // Fill the context, then query beyond occupancy / ENTRIES_N again.
    for (tb::key_t k = 1; k < static_cast<tb::key_t>(cfg::ENTRIES_N); ++k) {
      push_back(tb::UpdateCommand{0, tb::Cmd::Add, k + 10, static_cast<tb::volume_t>(k)});
      wait_cycles(1);
    }
    wait_cycles(10);

    for (tb::level_t lvl = 0; lvl < cfg::ENTRIES_N; ++lvl) {
      push_back(tb::QueryCommand{0, lvl});
    }
    if (has_oob_level_encodings()) {
      push_back(tb::QueryCommand{0, oob_level_id()});
    }
    wait_cycles(10);

    V_NOTE("Test ends...");
  }
};

}  // namespace

namespace tb::tests::oob_error {

void init(tb::TestRegistry& r) {
  CheckOobContext::Builder::init(r);
  CheckOobEntry::Builder::init(r);
}

}  // namespace tb::tests::oob_error
