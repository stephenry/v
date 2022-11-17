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

#include "reset.h"

#include "../log.h"
#include "../tb.h"
#include "../test.h"
#include "directed.h"

namespace {

enum class State { PreReset, AssertReset, InReset, PostReset, PostInit, Done };

const char* to_string(State s) {
  switch (s) {
    case State::PreReset:    return "PreReset";
    case State::AssertReset: return "AssertReset";
    case State::InReset:     return "InReset";
    case State::PostReset:   return "PostReset";
    case State::PostInit:    return "PostInit";
    case State::Done:        return "Done";
  }
  return "Invalid";
}

std::uint32_t expected_init_cycles() {
  // Total initialization time (in cycles) is computed as:
  //
  //    Total number of contexts in the machine
  //
  //  + FSM transition IDLE -> BUSY (1 cycle)
  //
  //  + FSM transition BUSY -> DONE (1 cycle)
  //
  return cfg::CONTEXT_N + 2;
}

struct CheckResetCB : tb::VKernelCB {
  CheckResetCB(tb::log::Scope* lg) : r_(lg, true) {}
  bool on_negedge_clk(Vtb* tb) override {
    r_.check_reset(tb);
    return !r_.is_done();
  }

 private:
  tb::ResetTracker r_;
};

struct CheckReset : public tb::Test {
  CREATE_TEST_BUILDER(CheckReset);

  bool run() override {
    CheckResetCB cb{lg()};
    return k()->run(std::addressof(cb));
  };
};

}  // namespace

namespace tb {

ResetTracker::ResetTracker(tb::log::Scope* ls, bool is_active_low)
 : ls_(ls), is_active_low_(is_active_low) {}

void ResetTracker::check_reset(Vtb* tb) {
  switch (st_) {
    case State::PreReset: {
      V_LOG(ls_, Info, "In pre-reset...");
      st_ = State::AssertReset;
    } break;
    case State::AssertReset: {
      V_LOG(ls_, Info, "In setting reset...");
      tb::VDriver::reset(tb, !is_active_low());
      st_ = State::InReset;
    } break;
    case State::InReset: {
      tb::VDriver::reset(tb, is_active_low());
      st_ = State::PostReset;
    } break;
    case State::PostReset: {
      const bool is_busy = tb::VDriver::is_busy(tb);
      is_failed_ = !is_busy;
      V_EXPECT_TRUE(ls_, is_busy);
      if (is_busy) {
        V_LOG(ls_, Info, "Machine becomes busy...");
        cnt_ = expected_init_cycles();
        st_ = is_failed_ ? State::Done : State::PostInit;
      }
    } break;
    case State::PostInit: {
      const bool timeout = (--cnt_ == 0);
      const bool is_busy = tb::VDriver::is_busy(tb);
      if (timeout) {
        is_failed_ |= is_busy;
        V_EXPECT_TRUE(ls_, !is_busy);
      } else if (!timeout) {
        is_failed_ |= !is_busy;
        V_EXPECT_TRUE(ls_, is_busy);
      }
      if (timeout || !is_busy) {
        V_LOG(ls_, Info, "Machine becomes idle...");
        // Winddown interval for tracing.
        cnt_ = 10;
        st_ = State::Done;
      } else if (is_failed_) {
        cnt_ = 0;
        st_ = State::Done;
      }
    } break;
    case State::Done: {
      is_done_ = (--cnt_ > 0);
      if (is_done_) {
        V_LOG(ls_, Info, "Reset test complete...");
      }
    } break;
  }
}

}  // namespace tb

namespace tb::tests::reset {

void init(TestRegistry* r) { CheckReset::Builder::init(r); }

}  // namespace tb::tests::reset
