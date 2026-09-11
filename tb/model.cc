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

#include "model.h"

#include <array>
#include <sstream>
#include <vector>

#include "Vobj/Vtb.h"
#include "cfg.h"
#include "log.h"
#include "rnd.h"
#include "tb.h"

namespace tb {

UpdateCommand::UpdateCommand() : vld_(false) {}

UpdateCommand::UpdateCommand(prod_id_t prod_id, Cmd cmd, key_t key,
                             volume_t volume)
    : vld_(true), prod_id_(prod_id), cmd_(cmd), key_(key), volume_(volume) {}

bool operator==(const UpdateCommand& lhs, const UpdateCommand& rhs) {
  if (lhs.vld() != rhs.vld()) return false;

  // If invalid, payload is don't care.
  if (!lhs.vld()) return true;

  if (lhs.prod_id() != rhs.prod_id()) return false;
  if (lhs.cmd() != rhs.cmd()) return false;
  if (lhs.key() != rhs.key()) return false;
  if (lhs.volume() != rhs.volume()) return false;

  return true;
}

bool operator!=(const UpdateCommand& lhs, const UpdateCommand& rhs) {
  return !operator==(lhs, rhs);
}

UpdateResponse::UpdateResponse() : vld_(false) {}

UpdateResponse::UpdateResponse(prod_id_t prod_id)
    : vld_(true), prod_id_(prod_id) {}

bool operator==(const UpdateResponse& lhs, const UpdateResponse& rhs) {
  if (lhs.vld() != rhs.vld()) return false;

  // If invalid, payload is don't care.
  if (!lhs.vld()) return true;

  if (lhs.prod_id() != rhs.prod_id()) return false;

  return true;
}

bool operator!=(const UpdateResponse& lhs, const UpdateResponse& rhs) {
  return !operator==(lhs, rhs);
}

QueryCommand::QueryCommand() : vld_(false) {}

QueryCommand::QueryCommand(prod_id_t prod_id, level_t level)
    : vld_(true), prod_id_(prod_id), level_(level) {}

bool operator==(const QueryCommand& lhs, const QueryCommand& rhs) {
  if (lhs.vld() != rhs.vld()) return false;

  // If invalid, payload is don't care.
  if (!lhs.vld()) return true;

  if (lhs.prod_id() != rhs.prod_id()) return false;
  if (lhs.level() != rhs.level()) return false;

  return true;
}

bool operator!=(const QueryCommand& lhs, const QueryCommand& rhs) {
  return !operator==(lhs, rhs);
}

QueryResponse::QueryResponse() : vld_(false) {}

QueryResponse::QueryResponse(key_t key, volume_t volume, bool error,
                             listsize_t listsize) {
  vld_ = true;
  key_ = key;
  volume_ = volume;
  error_ = error;
  listsize_ = listsize;
}

bool operator==(const QueryResponse& lhs, const QueryResponse& rhs) {
  if (lhs.vld() != rhs.vld()) return false;
  // If invalid, payload is don't care.
  if (!lhs.vld()) return true;

  if (lhs.error() != rhs.error()) return false;

  // If error, disregard further contents (unreliable).
  if (lhs.error()) return true;

  if (lhs.key() != rhs.key()) return false;
  if (lhs.volume() != rhs.volume()) return false;
  if (lhs.listsize() != rhs.listsize()) return false;

  return true;
}

bool operator!=(const QueryResponse& lhs, const QueryResponse& rhs) {
  return !operator==(lhs, rhs);
}

NotifyResponse::NotifyResponse() : vld_(false) {}

NotifyResponse::NotifyResponse(prod_id_t prod_id, key_t key, volume_t volume) {
  vld_ = true;
  prod_id_ = prod_id;
  key_ = key;
  volume_ = volume;
}

bool operator==(const NotifyResponse& lhs, const NotifyResponse& rhs) {
  if (lhs.vld() != rhs.vld()) return false;
  // If invalid, payload is don't care.
  if (!lhs.vld()) return true;
  if (lhs.prod_id() != rhs.prod_id()) return false;
  if (lhs.key() != rhs.key()) return false;
  if (lhs.volume() != rhs.volume()) return false;

  return true;
}

bool operator!=(const NotifyResponse& lhs, const NotifyResponse& rhs) {
  return !operator==(lhs, rhs);
}

void StreamRenderer<Cmd>::write(std::ostream& os, const Cmd& cmd) {
  switch (cmd) {
    case Cmd::Clr: os << "Clr"; break;
    case Cmd::Add: os << "Add"; break;
    case Cmd::Del: os << "Del"; break;
    case Cmd::Rep: os << "Rep"; break;
    default:       os << "Invalid"; break;
  }
}

void StreamRenderer<UpdateCommand>::write(std::ostream& os,
                                          const UpdateCommand& uc) {
  RecordRenderer rr{os, "uc"};
  rr.add("vld", uc.vld());
  if (uc.vld()) {
    rr.add("prod_id", AsDec{uc.prod_id()});
    rr.add("cmd", uc.cmd());
    rr.add("key", AsHex{uc.key()});
    rr.add("volume", AsDec{uc.volume()});
  } else {
    rr.add("prod_id", "x");
    rr.add("cmd", Cmd::Invalid);
    rr.add("key", "x");
    rr.add("volume", "x");
  }
}

void StreamRenderer<UpdateResponse>::write(std::ostream& os,
                                           const UpdateResponse& ur) {
  RecordRenderer rr{os, "ur"};
  rr.add("vld", ur.vld());
  if (ur.vld()) {
    rr.add("prod_id", AsDec{ur.prod_id()});
  } else {
    rr.add("prod_id", "x");
  }
}

void StreamRenderer<QueryCommand>::write(std::ostream& os,
                                         const QueryCommand& qc) {
  RecordRenderer rr{os, "qc"};
  rr.add("vld", qc.vld());
  if (qc.vld()) {
    rr.add("prod_id", AsDec{qc.prod_id()});
    rr.add("level", AsDec{qc.level()});
  } else {
    rr.add("prod_id", "x");
    rr.add("level", "x");
  }
}

void StreamRenderer<QueryResponse>::write(std::ostream& os,
                                          const QueryResponse& qr) {
  RecordRenderer rr{os, "qr"};
  rr.add("vld", qr.vld());
  if (qr.vld()) {
    rr.add("key", AsHex{qr.key()});
    rr.add("volume", AsDec{qr.volume()});
    rr.add("error", AsDec{qr.error()});
    rr.add("listsize", AsDec{qr.listsize()});
  } else {
    rr.add("key", "x");
    rr.add("volume", "x");
    rr.add("error", "x");
    rr.add("listsize", "x");
  }
}

void StreamRenderer<NotifyResponse>::write(std::ostream& os,
                                           const NotifyResponse& nr) {
  RecordRenderer rr{os, "nr"};
  rr.add("vld", nr.vld());
  if (nr.vld()) {
    rr.add("prod_id", AsDec{nr.prod_id()});
    rr.add("key", AsHex{nr.key()});
    rr.add("volume", AsDec{nr.volume()});
  } else {
    rr.add("prod_id", "x");
    rr.add("key", "x");
    rr.add("volume", "x");
  }
}

struct VSampler {
  static UpdateCommand uc(Vtb* tb) {
    if (to_bool(tb->i_upd_vld)) {
      return UpdateCommand{tb->i_upd_prod_id, to_cmd(tb->i_upd_cmd),
                           static_cast<key_t>(tb->i_upd_key), tb->i_upd_size};
    } else {
      return UpdateCommand{};
    }
  }

  static QueryCommand qc(Vtb* tb) {
    if (to_bool(tb->i_lut_vld)) {
      return QueryCommand{tb->i_lut_prod_id, tb->i_lut_level};
    } else {
      return QueryCommand{};
    }
  }

  // Sample Notify Reponse Interface:
  static NotifyResponse nr(Vtb* tb) {
    if (to_bool(tb->o_lv0_vld_r)) {
      return NotifyResponse{tb->o_lv0_prod_id_r,
                            static_cast<key_t>(tb->o_lv0_key_r),
                            tb->o_lv0_size_r};
    } else {
      return NotifyResponse{};
    }
  }

  // Sample Query Response Interface:
  static QueryResponse qr(Vtb* tb) {
    if (to_bool(tb->o_lut_vld_r)) {
      return QueryResponse{static_cast<key_t>(tb->o_lut_key), tb->o_lut_size,
                           to_bool(tb->o_lut_error), tb->o_lut_listsize};
    } else {
      return QueryResponse{};
    }
  }

  // Sample Update Error Interface (writeback-aligned):
  static bool ue(Vtb* tb) { return to_bool(tb->o_upd_error_r); }

 private:
  static bool to_bool(vluint8_t v) { return (v != 0); }

  static Cmd to_cmd(vluint8_t c) { return Cmd{c}; }
};

template <typename T, std::size_t N>
class DelayPipeBase {
 public:
  explicit DelayPipeBase() { clear(); }

  std::string to_string() const {
    std::stringstream ss;
    for (std::size_t i = 0; i < p_.size(); i++) {
      const T& t{p_[(rd_ptr_ + i) % p_.size()]};
      ss << t.to_string() << "\n";
    }
    return ss.str();
  }

  void push_back(const T& t) { p_[wr_ptr_] = t; }

  const T& head() const { return p_[rd_ptr_]; }

  void step() {
    wr_ptr_ = (wr_ptr_ + 1) % p_.size();
    rd_ptr_ = (rd_ptr_ + 1) % p_.size();
  }

  void clear() {
    for (T& t : p_) t = T{};

    wr_ptr_ = N;
    rd_ptr_ = 0;
  }

 protected:
  std::size_t wr_ptr_, rd_ptr_;
  std::array<T, N + 1> p_;
};

template <typename T, std::size_t N>
class DelayPipe : public DelayPipeBase<T, N> {
  using base_class_type = DelayPipeBase<T, N>;
};

template <std::size_t N>
class DelayPipe<UpdateResponse, N> : public DelayPipeBase<UpdateResponse, N> {
  using base_class_type = DelayPipeBase<UpdateResponse, N>;

  using base_class_type::p_;
  using base_class_type::wr_ptr_;

 public:
  bool has_prod_id(prod_id_t prod_id) const {
    for (std::size_t i = 0; i < N; i++) {
      const UpdateResponse& ur{p_[(wr_ptr_ - i) % p_.size()]};
      if (ur.vld() && (ur.prod_id() == prod_id)) return true;
    }
    return false;
  }
};

struct Entry {
  key_t key;
  volume_t volume;
};

template<>
struct StreamRenderer<Entry> {
  static void write(std::ostream& os, const Entry& e) {
    RecordRenderer rr{os, "e"};
    rr.add("key", AsHex{e.key});
    rr.add("volume", AsDec{e.volume});
  }
};

bool compare_keys(key_t rhs, key_t lhs) {
  return cfg::is_bid_table ? (rhs > lhs) : (rhs < lhs);
}

bool compare_entries(const Entry& lhs, const Entry& rhs) {
  return compare_keys(lhs.key, rhs.key);
}

class Model::Impl {
  friend class ModelValidation;

  static constexpr const std::size_t QUERY_PIPE_DELAY = 1;
  static constexpr const std::size_t UPDATE_PIPE_DELAY = 5;

 public:
  explicit Impl(Vtb* tb, Scope* logger) : tb_(tb), logger_(logger) {}

  void step() {
    const UpdateCommand uc{VSampler::uc(tb_)};
    const QueryCommand qc{VSampler::qc(tb_)};

    if (logger_ && (uc.vld() || qc.vld())) {
      logger_->Info("Issue: ", uc, " | ", qc);
    }

    handle(uc);
    handle(qc);

    const NotifyResponse nr{VSampler::nr(tb_)};
    const QueryResponse qr{VSampler::qr(tb_)};
    const bool ue{VSampler::ue(tb_)};

    if (logger_ && (nr.vld() || qr.vld() || ue)) {
      logger_->Info("Response: ", nr, " | ", qr, " | ue=", ue);
    }

    handle(nr);
    handle(qr);
    handle_upd_error(ue);

    // Advance predicted state.
    ur_pipe_.step();
    nr_pipe_.step();
    qr_pipe_.step();
    ue_pipe_.step();
  }

 private:
  void handle(const UpdateCommand& uc) {
    if (!uc.vld()) {
      // No command is present at the interface on this cycle, we do not
      // therefore expect a notification.
      nr_pipe_.push_back(NotifyResponse{});
      ur_pipe_.push_back(UpdateResponse{});
      ue_pipe_.push_back(false);
      return;
    };

    // OOB Context: fail-safe NOP. RTL does not admit the command into the
    // update datapath; expect o_upd_error_r at writeback latency.
    if (uc.prod_id() >= cfg::CONTEXT_N) {
      nr_pipe_.push_back(NotifyResponse{});
      ur_pipe_.push_back(UpdateResponse{});
      ue_pipe_.push_back(true);
      return;
    }

    UpdateResponse ur{};
    NotifyResponse nr{};
    std::vector<Entry>& ctxt{tbl_[uc.prod_id()]};
    switch (uc.cmd()) {
      case Cmd::Clr: {
        ur = UpdateResponse{uc.prod_id()};
        if (!ctxt.empty()) {
          nr = NotifyResponse{uc.prod_id(), 0, 0};
        }
        ctxt.clear();
      } break;
      case Cmd::Add: {
        ur = UpdateResponse{uc.prod_id()};
        if (ctxt.empty() || compare_keys(uc.key(), ctxt.begin()->key)) {
          nr = NotifyResponse{uc.prod_id(), uc.key(), uc.volume()};
        }
        ctxt.push_back(Entry{uc.key(), uc.volume()});
        std::stable_sort(ctxt.begin(), ctxt.end(), compare_entries);
        if (ctxt.size() > cfg::ENTRIES_N) {
          // Entry has been spilled on this Add.
          if (logger_)
            logger_->Warning("Context overflow! Rejected entry: ", ctxt.back());
          ctxt.pop_back();
        }
      } break;
      case Cmd::Rep:
      case Cmd::Del: {
        ur = UpdateResponse{uc.prod_id()};
        auto find_key = [&](const Entry& e) { return (e.key == uc.key()); };
        auto it = std::find_if(ctxt.begin(), ctxt.end(), find_key);

        // The context was either empty or the key was not found. The current
        // command becomes a NOP.
        if (it == ctxt.end()) break;

        if (it == ctxt.begin()) {
          // Item to be replaced is first, therefore raise notification of
          // current first item in context.
          nr = NotifyResponse{uc.prod_id(), it->key, it->volume};
        }

        if (uc.cmd() == Cmd::Rep) {
          // Perform final replacement of 'volume'.
          it->volume = uc.volume();
        } else {
          // Delete: Remove entry from context.
          ctxt.erase(it);
        }
      } break;
      case Cmd::Invalid:
      default: {
          ++tb::Sim::errors;
          logger_->Error("Invalid command received: ", uc.cmd());
      } break;
    }

    // Update predicted notify responses based upon outcome of prior command.
    ur_pipe_.push_back(ur);
    nr_pipe_.push_back(nr);
    ue_pipe_.push_back(false);
  }

  void handle(const NotifyResponse& nr) {
    const NotifyResponse& predicted = nr_pipe_.head();
    const NotifyResponse& actual = nr;
    const char* fail_message = nullptr;
    if (predicted.vld() == actual.vld()) {
      if (predicted.vld() && (predicted != actual))
        fail_message = "Payload mismatch";
    } else {
      fail_message = "Unexpected Notify Response";
    }
    if (fail_message) report_fail(fail_message, predicted, actual);
  }

  void handle(const QueryCommand& qc) {
    QueryResponse qr;
    if (qc.vld()) {
      // OOB Context, OOB level (>= ENTRIES_N), level past occupancy, or
      // query colliding with an in-flight update to the same Context all
      // produce an error response (payload otherwise don't-care).
      if (qc.prod_id() >= cfg::CONTEXT_N) {
        qr = QueryResponse{0, 0, true, 0};
      } else {
        const std::vector<Entry>& ctxt{tbl_[qc.prod_id()]};

        if ((qc.level() >= cfg::ENTRIES_N) || (qc.level() >= ctxt.size()) ||
            ur_pipe_.has_prod_id(qc.prod_id())) {
          // Query is errored, other fields are invalid.
          qr = QueryResponse{0, 0, true, 0};
        } else {
          // Query is valid, populate as necessary.
          const Entry& e{ctxt[qc.level()]};
          const listsize_t listsize = static_cast<listsize_t>(ctxt.size());
          qr = QueryResponse{e.key, e.volume, false, listsize};
        }
      }
    }
    qr_pipe_.push_back(qr);
  }

  void handle(const QueryResponse& qr) {
    const QueryResponse& predicted = qr_pipe_.head();
    const QueryResponse& actual = qr;
    const char* fail_message = nullptr;
    if (predicted.vld() == actual.vld()) {
      if (predicted.vld() && (predicted != actual))
        fail_message = "Payload mismatch";
    } else {
      fail_message = "Unexpected Query Response";
    }
    if (fail_message) {
      report_fail(fail_message, predicted, actual);
    }
  }

  void handle_upd_error(bool actual) {
    const bool predicted = ue_pipe_.head();
    if (predicted != actual) {
      ++tb::Sim::errors;
      if (logger_)
        logger_->Error("Update error mismatch predicted: ", predicted,
                       " actual: ", actual);
    }
  }

  template <typename T>
  void report_fail(const char* reason, const T& predicted, const T& actual) const {
    ++tb::Sim::errors;
    if (logger_)
      logger_->Error(reason, " predicted: ", predicted, " actual:", actual);
  }

  std::array<std::vector<Entry>, cfg::CONTEXT_N> tbl_;
  DelayPipe<NotifyResponse, UPDATE_PIPE_DELAY> nr_pipe_;
  DelayPipe<UpdateResponse, UPDATE_PIPE_DELAY> ur_pipe_;
  DelayPipe<QueryResponse, QUERY_PIPE_DELAY> qr_pipe_;
  DelayPipeBase<bool, UPDATE_PIPE_DELAY> ue_pipe_;

  Vtb* tb_;
  Scope* logger_{nullptr};
};

Model::Model(Vtb* tb, Scope* logger) {
  impl_ = std::make_unique<Impl>(tb, logger);
}

Model::~Model() {}

void Model::step() { impl_->step(); }

const Model::Impl* Model::impl() const { return impl_.get(); }

class ModelValidation::Impl {
 public:
  explicit Impl() = default;

  bool has_active_entries(prod_id_t id) const {
    const Model::Impl* impl{Sim::model->impl()};
    if (impl == nullptr) return false;

    return (id < impl->tbl_.size()) && !impl->tbl_[id].empty();
  }

  std::pair<bool, key_t> pick_active_key(prod_id_t id) const {
    const Model::Impl* impl{Sim::model->impl()};
    const std::vector<Entry>& es{impl->tbl_[id]};
    if (es.empty()) {
      return {false, key_t{}};
    }

    return {true, es[Sim::random->uniform(es.size() - 1)].key};
  }
};

ModelValidation::ModelValidation() { impl_ = std::make_unique<Impl>(); }

ModelValidation::~ModelValidation() {}

bool ModelValidation::has_active_entries(prod_id_t id) const {
  return impl_->has_active_entries(id);
}

std::pair<bool, key_t> ModelValidation::pick_active_key(prod_id_t id) const {
  return impl_->pick_active_key(id);
}

}  // namespace tb
