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

#include "directed.h"

#include "../log.h"
#include "../mdl.h"
#include "../tb.h"
#include "Vobj/Vtb.h"

enum class Opcode { WaitUntilNotBusy, WaitCycles, Emit, EndSimulation };

struct Instruction {
  static std::unique_ptr<Instruction> make_emit(const tb::UpdateCommand& uc,
                                                const tb::QueryCommand& qc);
  static std::unique_ptr<Instruction> make_wait(std::size_t n);

  Opcode op{Opcode::EndSimulation};
  std::size_t n;
  tb::UpdateCommand uc;
  tb::QueryCommand qc;
};

std::unique_ptr<Instruction> Instruction::make_emit(
    const tb::UpdateCommand& uc, const tb::QueryCommand& qc) {
  std::unique_ptr<Instruction> i = std::make_unique<Instruction>();
  i->op = Opcode::Emit;
  i->uc = uc;
  i->qc = qc;
  return std::move(i);
}

std::unique_ptr<Instruction> Instruction::make_wait(std::size_t n) {
  std::unique_ptr<Instruction> i = std::make_unique<Instruction>();
  i->op = Opcode::WaitCycles;
  i->n = n;
  return std::move(i);
}

class tb::tests::Directed::Impl : public tb::VKernelCB {
 public:
  Impl(Directed* parent) : parent_(parent) {
    VKernelOptions opts;
    ls_ = parent->lg();
    k_ = std::make_unique<VKernel>(opts, ls_->create_child("kernel"));
  }
  bool run() {
    k_->run(this);
    return true;
  }

  bool on_negedge_clk(Vtb* tb) {
    std::deque<std::unique_ptr<Instruction> >& d{parent_->d_};

    if (d.empty()) {
      return false;
    }

    // Process further stimulus:
    Instruction* i{d.front().get()};
    switch (i->op) {
      case Opcode::WaitUntilNotBusy: {
        const bool is_busy = VDriver::is_busy(tb);
        if (!is_busy) {
          ls_->log(log::Level::Info, "Initialization complete!");
          d.pop_front();
        }
        return is_busy;
      } break;
      case Opcode::WaitCycles: {
        VDriver::issue(tb, UpdateCommand{});
        VDriver::issue(tb, QueryCommand{});
        if (--i->n == 0) {
          d.pop_front();
        }
      } break;
      case Opcode::Emit: {
        VDriver::issue(tb, i->uc);
        VDriver::issue(tb, i->qc);
        d.pop_front();
      } break;
      case Opcode::EndSimulation: {
        ls_->log(log::Level::Info, "Simulation complete!");
        d.pop_front();
        return false;
      } break;
      default: {
        // Unknown command!
      } break;
    }
    return false;
  }

  bool on_posedge_clk(Vtb* tb) {
    // Sensitive only to the negative clock edge.
    return true;
  }

 private:
  std::unique_ptr<VKernel> k_;
  Directed* parent_;
  log::Scope* ls_;
};

namespace tb::tests {

Directed::Directed() { impl_ = std::make_unique<Impl>(this); }

Directed::~Directed() {}

bool Directed::run() { return impl_->run(); }

void Directed::wait_until_not_busy() {}

void Directed::push_back(const UpdateCommand& uc, const QueryCommand& qc) {
  d_.push_back(Instruction::make_emit(uc, qc));
}

void Directed::wait_cycles(std::size_t n) {
  d_.push_back(Instruction::make_wait(n));
}

};  // namespace tb::tests