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

#include <string_view>
#include <vector>

#include "../log.h"
#include "../mdl.h"
#include "../rnd.h"
#include "../tb.h"
#include "../test.h"
#include "Vobj/Vtb.h"
#include "cfg.h"
#include "reset.h"

namespace {

std::vector<std::string_view> split(const std::string_view args,
                                    const char sep = ',') {
  std::vector<std::string_view> vs;
  std::string_view::size_type i = 0, j = 0;
  do {
    j = args.find(sep, i);
    if (j == std::string_view::npos) {
      vs.push_back(args.substr(i));
    } else {
      vs.push_back(args.substr(i, j - i));
    }
    i = (j + 1);
  } while (j != std::string_view::npos);
  return vs;
}

std::pair<std::string_view, std::vector<std::string_view> > split_kv(
    std::string_view sv) {
  std::string_view::size_type i = sv.find('=');
  const std::string_view k = sv.substr(0, i);
  const std::vector<std::string_view> vv{split(sv.substr(++i), ';')};
  return {k, vv};
}

struct Options {
  static Options construct(const tb::TestOptions& opts, const tb::Mdl* mdl);

  float clr_weight = 0.01f;
  float add_weight = 1.0f;
  float del_weight = 1.0f;
  float rep_weight = 1.0f;
  float inv_weight = 1.0f;

  int context_n = cfg::CONTEXT_N;

  int n = 100000;

  tb::Rnd* rnd = nullptr;

  const tb::Mdl* mdl = nullptr;
};

Options Options::construct(const tb::TestOptions& topts, const tb::Mdl* mdl) {
  Options opts;
  if (!topts.args.empty()) {
    // Argument list has been populated; process.
    std::vector<std::string_view> argv{split(topts.args)};
    for (const std::string_view vs : argv) {
      const auto arg{split_kv(vs)};
      if (arg.first == "n") {
        std::size_t pos;
        opts.n = std::stoi(std::string{*arg.second.begin()}, &pos);
      } else if (arg.first == "p") {
        // Parse 'probability (p)' argument as:
        //
        //    p=a;b;c;d;e
        //
        // where (command weight):
        //
        //   a - clear weight
        //   b - add weight
        //   c - del. weight
        //   d - rep. weight
        //   e - inv. weight  (bubble)
        //
        std::vector<std::string_view> vs{arg.second};
        for (int i = 0; i < 5; i++) {
          if (vs.empty()) {
            break;
          }
          std::size_t pos;
          const std::string weight_str{vs.back()};
          switch (i) {
            case 0: {
              opts.clr_weight = std::stof(weight_str, &pos);
            } break;
            case 1: {
              opts.add_weight = std::stof(weight_str, &pos);
            } break;
            case 2: {
              opts.del_weight = std::stof(weight_str, &pos);
            } break;
            case 3: {
              opts.rep_weight = std::stof(weight_str, &pos);
            } break;
            case 4: {
              opts.inv_weight = std::stof(weight_str, &pos);
            } break;
          }
          vs.pop_back();
        }
      }
    }
  }
  // The number of contexts to exercise.
  opts.context_n = cfg::CONTEXT_N;
  opts.rnd = topts.rnd;
  opts.mdl = mdl;
  return opts;
}

enum class State { Random, FinalCheck, WindDown };

const char* to_string(State st) {
  switch (st) {
    case State::Random:     return "Random";
    case State::FinalCheck: return "FinalCheck";
    case State::WindDown:   return "WindDown";
    default:                return "Invalid";
  }
}

class Stimulus {
 public:
  Stimulus(const Options& opts) : opts_(opts), val_(opts.mdl) {
    bag_.push_back(tb::Cmd::Clr, opts_.clr_weight);
    bag_.push_back(tb::Cmd::Add, opts_.add_weight);
    bag_.push_back(tb::Cmd::Del, opts_.del_weight);
    bag_.push_back(tb::Cmd::Rep, opts_.rep_weight);
    bag_.push_back(tb::Cmd::Invalid, opts_.inv_weight);
    state(State::Random);
  }

  bool get(tb::UpdateCommand& uc, tb::QueryCommand& qc) {
    bool ret = false;
    switch (st_) {
      case State::Random: {
        ret = get_random(uc, qc);
      } break;
      case State::FinalCheck: {
        ret = get_final_check(uc, qc);
      } break;
      case State::WindDown: {
        ret = (--opts_.n > 0);
      } break;
    }
    return ret;
  }

 private:
  bool get_random(tb::UpdateCommand& uc, tb::QueryCommand& qc) {
    if (opts_.n > 0) {
      int issue_count = handle(uc);
      if (opts_.n > 0) {
        issue_count += handle(qc);
      }
      opts_.n -= issue_count;
    } else {
      opts_.n = cfg::ENTRIES_N * cfg::CONTEXT_N - 1;
      state(State::FinalCheck);
    }
    return true;
  }

  bool get_final_check(tb::UpdateCommand& uc, tb::QueryCommand& qc) {
    const tb::prod_id_t id = (opts_.n / cfg::ENTRIES_N);
    const tb::level_t level = (opts_.n % cfg::ENTRIES_N);
    qc = tb::QueryCommand{id, level};
    if (--opts_.n < 0) {
      opts_.n = 10;
      state(State::WindDown);
    }
    return true;
  }

  int handle(tb::UpdateCommand& uc) {
    b = !b;
    if (b) return 0;

    // Constrain such that keys are unique

    generate(uc);
    return 1;
  }

  int handle(tb::QueryCommand& qc) {
    generate(qc);
    return 1;
  }

  void generate(tb::UpdateCommand& uc) {
    const tb::Cmd cmd = bag_.pick(opts_.rnd);
    switch (cmd) {
      case tb::Cmd::Clr: {
        // No further updates required.
        const tb::prod_id_t prod_id =
            opts_.rnd->uniform(opts_.context_n - 1, 0);
        uc = tb::UpdateCommand{prod_id, cmd, 0, 0};
      } break;
      case tb::Cmd::Add: {
        const tb::prod_id_t prod_id =
            opts_.rnd->uniform(opts_.context_n - 1, 0);
        const tb::key_t key = opts_.rnd->uniform<tb::key_t>();
        const tb::volume_t volume = opts_.rnd->uniform<tb::volume_t>();
        uc = tb::UpdateCommand{prod_id, cmd, key, volume};
      } break;
      case tb::Cmd::Rep:
      case tb::Cmd::Del: {
        const tb::prod_id_t prod_id = 0;
        auto [success, key] = val_.pick_active_key(opts_.rnd, prod_id);
        uc = tb::UpdateCommand{prod_id, tb::Cmd::Del, key, 0};
      } break;
      case tb::Cmd::Invalid: {
        // Insert bubble.
        uc = tb::UpdateCommand{};
      } break;
      default:;
    }
  }

  void generate(tb::QueryCommand& qc) {
    const tb::prod_id_t prod_id = opts_.rnd->uniform(opts_.context_n - 1, 0);
    const tb::level_t level = opts_.rnd->uniform(cfg::ENTRIES_N - 1);
    qc = tb::QueryCommand{prod_id, level};
  }

  void state(State st) { st_ = st; }

  bool b = true;
  tb::Bag<tb::Cmd> bag_;
  Options opts_;
  tb::MdlValidation val_;
  State st_;
};

struct RegressCB : public tb::VKernelCB {
  RegressCB(tb::Test* parent, Stimulus* s)
      : parent_(parent), s_(s), rstt_(parent->lg(), true) {}

  bool on_negedge_clk(Vtb* tb) {
    // Issue reset process.
    if (!rstt_.is_done()) {
      rstt_.check_reset(tb);
      return !rstt_.is_failed();
    }

    tb::UpdateCommand uc{};
    tb::QueryCommand qc{};
    if (!s_->get(uc, qc)) {
      // No further stimulus.
      return false;
    }

    // Issue commands to UUT
    tb::VDriver::issue(tb, uc);
    tb::VDriver::issue(tb, qc);

    return true;
  }

 private:
  Stimulus* s_;
  tb::Test* parent_;
  tb::ResetTracker rstt_;
};

struct Regress : public tb::Test {
  CREATE_TEST_BUILDER(Regress);

  bool run() override {
    Stimulus s{Options::construct(this->opts(), k()->mdl())};
    RegressCB cb{this, std::addressof(s)};
    return k()->run(std::addressof(cb));
  }
};

}  // namespace

namespace tb::tests::regress {

void init(tb::TestRegistry* r) { Regress::Builder::init(r); }

}  // namespace tb::tests::regress
