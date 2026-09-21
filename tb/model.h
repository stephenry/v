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

#ifndef V_TB_MDL_H
#define V_TB_MDL_H

#include "verilated.h"

#include "log.h"

class Vtb;

namespace tb {
class Random;

using prod_id_t = vluint8_t;
enum class Cmd : vluint8_t {
  Clr = 0,
  Add = 1,
  Del = 2,
  Rep = 3,
  Invalid = 0xff
};

// Mirrors v_pkg::err_syndrome_t
enum class ErrSyndrome : vluint8_t {
  Ok            = 0b000,
  OobContext    = 0b001,
  OobLevel      = 0b010,
  Busy          = 0b011,
  InvalidEntry  = 0b100
};

using key_t = vlsint64_t;
using volume_t = vluint32_t;
using level_t = vluint8_t;
using listsize_t = vluint8_t;

class UpdateCommand {
 public:
  explicit UpdateCommand();
  explicit UpdateCommand(prod_id_t prod_id, Cmd cmd, key_t key, volume_t volume);

  bool vld() const { return vld_; }
  prod_id_t prod_id() const { return prod_id_; }
  Cmd cmd() const { return cmd_; }
  key_t key() const { return key_; }
  volume_t volume() const { return volume_; }

 private:
  bool vld_;
  prod_id_t prod_id_;
  Cmd cmd_;
  key_t key_;
  volume_t volume_;
};

bool operator==(const UpdateCommand& lhs, const UpdateCommand& rhs);
bool operator!=(const UpdateCommand& lhs, const UpdateCommand& rhs);

class UpdateResponse {
 public:
  explicit UpdateResponse();
  explicit UpdateResponse(prod_id_t prod_id);

  bool vld() const { return vld_; }
  prod_id_t prod_id() const { return prod_id_; }

 private:
  bool vld_;
  prod_id_t prod_id_;
};

bool operator==(const UpdateResponse& lhs, const UpdateResponse& rhs);
bool operator!=(const UpdateResponse& lhs, const UpdateResponse& rhs);

class QueryCommand {
 public:
  explicit QueryCommand();
  explicit QueryCommand(prod_id_t prod_id, level_t level);

  bool vld() const { return vld_; }
  prod_id_t prod_id() const { return prod_id_; }
  level_t level() const { return level_; }

 private:
  bool vld_;
  prod_id_t prod_id_;
  level_t level_;
};

bool operator==(const QueryCommand& lhs, const QueryCommand& rhs);
bool operator!=(const QueryCommand& lhs, const QueryCommand& rhs);

class QueryResponse {
 public:
  explicit QueryResponse();
  explicit QueryResponse(key_t key, volume_t volume, bool error,
                         listsize_t listsize,
                         ErrSyndrome syndrome = ErrSyndrome::Ok);

  bool vld() const { return vld_; }
  key_t key() const { return key_; }
  volume_t volume() const { return volume_; }
  bool error() const { return error_; }
  listsize_t listsize() const { return listsize_; }
  ErrSyndrome syndrome() const { return syndrome_; }

 private:
  bool vld_;
  key_t key_;
  volume_t volume_;
  bool error_;
  listsize_t listsize_;
  ErrSyndrome syndrome_;
};

bool operator==(const QueryResponse& lhs, const QueryResponse& rhs);
bool operator!=(const QueryResponse& lhs, const QueryResponse& rhs);

class NotifyResponse {
 public:
  explicit NotifyResponse();
  explicit NotifyResponse(prod_id_t prod_id, key_t key, volume_t volume);

  bool vld() const { return vld_; }
  prod_id_t prod_id() const { return prod_id_; }
  key_t key() const { return key_; }
  volume_t volume() const { return volume_; }

 private:
  bool vld_;
  prod_id_t prod_id_;
  key_t key_;
  volume_t volume_;
};

bool operator==(const NotifyResponse& lhs, const NotifyResponse& rhs);
bool operator!=(const NotifyResponse& lhs, const NotifyResponse& rhs);

template<>
struct StreamRenderer<UpdateCommand> {
  static void write(std::ostream& os, const UpdateCommand& uc);
};

template<>
struct StreamRenderer<UpdateResponse> {
  static void write(std::ostream& os, const UpdateResponse& ur);
};

template<>
struct StreamRenderer<QueryCommand> {
  static void write(std::ostream& os, const QueryCommand& qc);
};

template<>
struct StreamRenderer<QueryResponse> {
  static void write(std::ostream& os, const QueryResponse& qr);
};

template<>
struct StreamRenderer<NotifyResponse> {
  static void write(std::ostream& os, const NotifyResponse& qr);
};

template<>
struct StreamRenderer<Cmd> {
  static void write(std::ostream& os, const Cmd& cmd);
};

template<>
struct StreamRenderer<ErrSyndrome> {
  static void write(std::ostream& os, const ErrSyndrome& s);
};

class Model {
  friend class ModelValidation;

  class Impl;
  std::unique_ptr<Impl> impl_;

 public:
  explicit Model(Vtb* tb, Scope* logger);
  ~Model();

  void step();

 private:
  const Impl* impl() const;
};

class ModelValidation {
  class Impl;
  std::unique_ptr<Impl> impl_;

 public:
  explicit ModelValidation();
  ~ModelValidation();

  bool has_active_entries(prod_id_t id) const;

  std::pair<bool, key_t> pick_active_key(prod_id_t id) const;
};

}  // namespace tb

#endif
