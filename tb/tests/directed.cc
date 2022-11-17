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

#include <deque>

#include "../log.h"
#include "../tb.h"

enum class Opcode {
  ApplyReset,
  WaitUntilNotBusy,
  WaitCycles,
  Emit,
  EndSimulation,
  LogMessage
};

struct Instruction {
  static std::unique_ptr<Instruction> make_emit(const tb::UpdateCommand& uc,
                                                const tb::QueryCommand& qc);
  static std::unique_ptr<Instruction> make_wait(std::size_t n);
  static std::unique_ptr<Instruction> make_note(const tb::log::Msg& msg);
  static std::unique_ptr<Instruction> make_wait_until_not_busy();
  static std::unique_ptr<Instruction> make_apply_reset();
  static std::unique_ptr<Instruction> make_end_simulation();
  static std::unique_ptr<Instruction> make_log_message(const tb::log::Msg& msg);

  Opcode op{Opcode::EndSimulation};
  std::size_t n;
  tb::UpdateCommand uc;
  tb::QueryCommand qc;
  tb::log::Msg msg;
};

std::unique_ptr<Instruction> Instruction::make_emit(
    const tb::UpdateCommand& uc, const tb::QueryCommand& qc) {
  std::unique_ptr<Instruction> i = std::make_unique<Instruction>();
  i->op = Opcode::Emit;
  i->uc = uc;
  i->qc = qc;
  return i;
}

std::unique_ptr<Instruction> Instruction::make_wait(std::size_t n) {
  std::unique_ptr<Instruction> i = std::make_unique<Instruction>();
  i->op = Opcode::WaitCycles;
  i->n = n;
  return i;
}

std::unique_ptr<Instruction> Instruction::make_note(const tb::log::Msg& msg) {
  std::unique_ptr<Instruction> i = std::make_unique<Instruction>();
  i->op = Opcode::LogMessage;
  i->msg = msg;
  return i;
}

std::unique_ptr<Instruction> Instruction::make_wait_until_not_busy() {
  std::unique_ptr<Instruction> i = std::make_unique<Instruction>();
  i->op = Opcode::WaitUntilNotBusy;
  return i;
}

std::unique_ptr<Instruction> Instruction::make_apply_reset() {
  std::unique_ptr<Instruction> i = std::make_unique<Instruction>();
  i->op = Opcode::ApplyReset;
  i->n = 0;
  return i;
}

std::unique_ptr<Instruction> Instruction::make_end_simulation() {
  std::unique_ptr<Instruction> i = std::make_unique<Instruction>();
  i->op = Opcode::EndSimulation;
  return i;
}

std::unique_ptr<Instruction> Instruction::make_log_message(
    const tb::log::Msg& msg) {
  std::unique_ptr<Instruction> i = std::make_unique<Instruction>();
  i->op = Opcode::LogMessage;
  i->msg = msg;
  return i;
}

class tb::tests::Directed::Impl : public tb::VKernelCB {
 public:
  Impl(Directed* parent) : parent_(parent) {}
  bool run() {
    program_prologue();
    parent_->prologue();
    parent_->program();
    parent_->epilogue();
    program_epilogue();

    VKernel* k{parent_->k()};
    k->run(this);
    k->end();
    return true;
  }

  void push_back(std::unique_ptr<Instruction>&& i) {
    d_.push_back(std::move(i));
  }

  void program_prologue() {
    push_back(Instruction::make_apply_reset());
    push_back(Instruction::make_wait_until_not_busy());
  }

  void program_epilogue() { push_back(Instruction::make_end_simulation()); }

  bool on_negedge_clk(Vtb* tb) {
    // Process further stimulus:
    bool do_next_command;
    do {
      if (d_.empty()) {
        // No further stimulus
        return false;
      }

      Instruction* i{d_.front().get()};
      bool consume_instruction = true;
      do_next_command = false;
      switch (i->op) {
        case Opcode::ApplyReset: {
          VKernel* k{parent_->k()};
          if (i->n == 0) {
            V_LOG(parent_->lg(), Info, "Resetting UUT.");
          }
          const bool do_apply_reset = (i->n++ < 10);
          VDriver::reset(tb, !do_apply_reset);
          consume_instruction = !do_apply_reset;
          if (consume_instruction) {
            V_LOG(parent_->lg(), Info, "Reset complete!");
          }
        } break;
        case Opcode::WaitUntilNotBusy: {
          const bool is_busy = VDriver::is_busy(tb);
          consume_instruction = !is_busy;
          if (consume_instruction) {
            V_LOG(parent_->lg(), Info, "Initialization complete!");
          }
        } break;
        case Opcode::WaitCycles: {
          VDriver::issue(tb, UpdateCommand{});
          VDriver::issue(tb, QueryCommand{});
          consume_instruction = (--i->n == 0);
        } break;
        case Opcode::Emit: {
          VDriver::issue(tb, i->uc);
          VDriver::issue(tb, i->qc);
        } break;
        case Opcode::EndSimulation: {
          V_LOG(parent_->lg(), Info, "Simulation complete!");
          return false;
        } break;
        case Opcode::LogMessage: {
          V_LOG_MSG(parent_->lg(), i->msg);
          do_next_command = true;
        } break;
        default: {
          // Unknown command!
          V_LOG(parent_->lg(), Info, "Unknown command!");
          return false;
        } break;
      }

      if (consume_instruction) d_.pop_front();
    } while (do_next_command);
    return true;
  }

  bool on_posedge_clk(Vtb* tb) {
    // Sensitive only to the negative clock edge.
    return true;
  }

 private:
  Directed* parent_;
  std::deque<std::unique_ptr<Instruction> > d_;
};

namespace tb::tests {

Directed::Directed() { impl_ = std::make_unique<Impl>(this); }

Directed::~Directed() {}

bool Directed::run() { return impl_->run(); }

void Directed::wait_until_not_busy() {
  impl_->push_back(Instruction::make_wait_until_not_busy());
}

void Directed::push_back(const UpdateCommand& uc, const QueryCommand& qc) {
  impl_->push_back(Instruction::make_emit(uc, qc));
}

void Directed::wait_cycles(std::size_t n) {
  impl_->push_back(Instruction::make_wait(n));
}

void Directed::apply_reset() {
  impl_->push_back(Instruction::make_apply_reset());
}

void Directed::note(const log::Msg& msg) {
  impl_->push_back(Instruction::make_log_message(msg));
}

};  // namespace tb::tests
