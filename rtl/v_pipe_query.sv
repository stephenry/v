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

`include "common_defs.vh"
`include "macros.vh"

`include "v_pkg.vh"

module v_pipe_query (

// -------------------------------------------------------------------------- //
// List Query Bus
  input wire logic                                i_lut_vld
, input wire v_pkg::id_t                          i_lut_prod_id
, input wire v_pkg::level_t                       i_lut_level
//
, output wire logic                               o_lut_vld_r
, output wire v_pkg::key_t                        o_lut_key
, output wire v_pkg::volume_t                     o_lut_size
, output wire logic                               o_lut_error
, output wire v_pkg::err_syndrome_t               o_lut_error_syndrome
, output wire v_pkg::listsize_t                   o_lut_listsize

// -------------------------------------------------------------------------- //
// State Interface
//
, input wire v_pkg::state_t                       i_state_rdata
//
, output wire logic                               o_state_ren
, output wire v_pkg::addr_t                       o_state_raddr

// -------------------------------------------------------------------------- //
// Update Pipeline Interface
//
, input wire logic                                i_s1_upd_vld_r
, input wire v_pkg::id_t                          i_s1_upd_prod_id_r
//
, input wire logic                                i_s2_upd_vld_r
, input wire v_pkg::id_t                          i_s2_upd_prod_id_r
//
, input wire logic                                i_s3_upd_vld_r
, input wire v_pkg::id_t                          i_s3_upd_prod_id_r
//
, input wire logic                                i_s4_upd_vld_r
, input wire v_pkg::id_t                          i_s4_upd_prod_id_r
//
, input wire logic                                i_s5_upd_vld_r
, input wire v_pkg::id_t                          i_s5_upd_prod_id_r

// -------------------------------------------------------------------------- //
// Initialization
, input wire logic                                init_r

// -------------------------------------------------------------------------- //
// Clk
, input wire logic                                clk
);

// ========================================================================== //
//                                                                            //
//  Wires                                                                     //
//                                                                            //
// ========================================================================== //

// S0
logic                                   s0_state_ren;
v_pkg::id_t                             s0_state_raddr;
logic                                   s1_lut_en;
logic                                   s0_lut_error_is_busy;
logic                                   s0_lut_error_oob_ctx;
logic                                   s0_lut_error_oob_level;

// S1

v_pkg::listsize_t                       s1_lut_listsize;
v_pkg::key_t                            s1_lut_key;
v_pkg::volume_t                         s1_lut_volume;
logic                                   s1_lut_error_invalid_entry;
logic                                   s1_lut_error_was_busy;
logic                                   s1_lut_error;
v_pkg::err_syndrome_t                   s0_lut_error_syndrome;
v_pkg::err_syndrome_t                   s1_lut_error_syndrome;

// ========================================================================== //
//                                                                            //
//  Flops                                                                     //
//                                                                            //
// ========================================================================== //

`V_DFF(logic, s1_lut_vld);
`V_DFFE(v_pkg::id_t, s1_lut_prod_id, s1_lut_en);
`V_DFFE(logic, s1_lut_error, s1_lut_en);
`V_DFFE(v_pkg::err_syndrome_t, s1_lut_error_syndrome, s1_lut_en);
`V_DFFE(logic [cfg_pkg::ENTRIES_N - 1:0], s1_lut_level_dec, s1_lut_en);

// ========================================================================== //
//                                                                            //
//  Stage 0                                                                   //
//                                                                            //
// ========================================================================== //

// -------------------------------------------------------------------------- //
// Out-of-bounds Context / Entry selection.
//
// Context IDs are sized as $clog2(CONTEXT_N) bits. When CONTEXT_N is not a
// power of two, encodings in [CONTEXT_N, 2**W) are representable but do not
// address a valid Context. Similarly, level encodings may exceed ENTRIES_N.
// Detect these cases up-front and suppress the state-table read so that an
// illegal address is never presented to the BRAM.
//
assign s0_lut_error_oob_ctx =
    (32'(i_lut_prod_id) >= cfg_pkg::CONTEXT_N);
assign s0_lut_error_oob_level =
    (32'(i_lut_level) >= cfg_pkg::ENTRIES_N);

// -------------------------------------------------------------------------- //
// State table lookup
assign s0_state_ren     = i_lut_vld & (~s0_lut_error_oob_ctx);
assign s0_state_raddr   = i_lut_prod_id;

assign s1_lut_vld_w     = i_lut_vld & (~init_r);
assign s1_lut_en        = s1_lut_vld_w;
assign s1_lut_prod_id_w = i_lut_prod_id;

// -------------------------------------------------------------------------- //
// Flag indicating "list busy or not valid entry".
//
// We consider the "list busy" whenever there is a in-flight operation to the
// currently addressed ID in the update pipeline. Whenever an update is
// in-flight, we simply error-out. Otherwise, it would be possible to use some
// more sophisticated forwarding, but this is probably overkill in this
// context and is not required by the specification.
//
assign s0_lut_error_is_busy   =
    (i_s1_upd_vld_r & (i_s1_upd_prod_id_r == i_lut_prod_id)) |
    (i_s2_upd_vld_r & (i_s2_upd_prod_id_r == i_lut_prod_id)) |
    (i_s3_upd_vld_r & (i_s3_upd_prod_id_r == i_lut_prod_id)) |
    (i_s4_upd_vld_r & (i_s4_upd_prod_id_r == i_lut_prod_id)) |
    (i_s5_upd_vld_r & (i_s5_upd_prod_id_r == i_lut_prod_id));

assign s1_lut_error_w =
    (s0_lut_error_is_busy | s0_lut_error_oob_ctx | s0_lut_error_oob_level);

// -------------------------------------------------------------------------- //
// Encode early (S0) error syndrome. Priority (highest first):
//   OOB_CONTEXT > OOB_LEVEL > BUSY.
// INVALID_ENTRY is resolved in S1 once state arrives from the BRAM.
//
assign s0_lut_error_syndrome =
    s0_lut_error_oob_ctx   ? v_pkg::ERR_OOB_CONTEXT :
    s0_lut_error_oob_level ? v_pkg::ERR_OOB_LEVEL   :
    s0_lut_error_is_busy   ? v_pkg::ERR_BUSY        :
                             v_pkg::ERR_OK;

assign s1_lut_error_syndrome_w = s0_lut_error_syndrome;

// -------------------------------------------------------------------------- //
//
dec #(.N(cfg_pkg::ENTRIES_N)) u_s0_id_dec (
//
  .i_x                                  (i_lut_level)
//
, .o_y                                  (s1_lut_level_dec_w)
);

// ========================================================================== //
//                                                                            //
//  Stage 1                                                                   //
//                                                                            //
// ========================================================================== //

// -------------------------------------------------------------------------- //
// S1

// -------------------------------------------------------------------------- //
// The validity of each entry in the state table is retained as a bit-vector. To
// compute the list occupancy (size) from this require would require a
// population count operation. Although not too difficult to implement using a
// CSA structure, this is quite a lot of combinatorial logic to attached to the
// late-arriving data from the state table RAM. Due to the fact that this is
// latency constrained path, it's not possible to pipeline this operation over
// multiple cycles. To overcome this problem, we retain a current list count in
// the table itself which is updated each time an entry is modified. The query
// pipeline simply retains this pre-computed state.
//
assign s1_lut_listsize = i_state_rdata.listsize;

// -------------------------------------------------------------------------- //
// A 'level' is invalid if its associated valid bit is 'b0. As above, we retain
// a bit-vector containing the valid entries within the state. To compute
// whether we are querying a valid entry, we simply consider the valid bit
// associated with its entry. We can perform this calculation trivially using
// the decoded version of the level, which we otherwise require to mux out the
// state from the RAM.
//
assign s1_lut_error_invalid_entry =
    ((s1_lut_level_dec_r & i_state_rdata.vld) == '0);

// -------------------------------------------------------------------------- //
// Update and Query commands which are co-incident must be checked at the input
// to the machine. In S0, for this to happen, we would need to consider the
// input to the update pipeline. For reasons of timing, we've simply pushed to
// the next stage so we can get this state from flops.
//
assign s1_lut_error_was_busy =
    (i_s1_upd_vld_r & (i_s1_upd_prod_id_r == s1_lut_prod_id_r));

// -------------------------------------------------------------------------- //
// Form final error state
assign s1_lut_error =
    (s1_lut_error_r | s1_lut_error_invalid_entry | s1_lut_error_was_busy);

// -------------------------------------------------------------------------- //
// Final error syndrome. Retain any S0 syndrome (higher priority). Otherwise
// prefer BUSY (was_busy) over INVALID_ENTRY.
//
assign s1_lut_error_syndrome =
    (s1_lut_error_syndrome_r != v_pkg::ERR_OK) ? s1_lut_error_syndrome_r :
    s1_lut_error_was_busy                      ? v_pkg::ERR_BUSY          :
    s1_lut_error_invalid_entry                 ? v_pkg::ERR_INVALID_ENTRY :
                                                 v_pkg::ERR_OK;

// -------------------------------------------------------------------------- //
//
mux #(.N(cfg_pkg::ENTRIES_N), .W(v_pkg::KEY_BITS)) u_s1_key_mux (
//
  .i_x                                  (i_state_rdata.key)
, .i_sel                                (s1_lut_level_dec_r)
//
, .o_y                                  (s1_lut_key)
);

// -------------------------------------------------------------------------- //
//
mux #(.N(cfg_pkg::ENTRIES_N), .W(v_pkg::VOLUME_BITS)) u_s1_volume_mux (
//
  .i_x                                  (i_state_rdata.volume)
, .i_sel                                (s1_lut_level_dec_r)
//
, .o_y                                  (s1_lut_volume)
);

// ========================================================================== //
//                                                                            //
//  Outputs                                                                   //
//                                                                            //
// ========================================================================== //

assign o_lut_vld_r = s1_lut_vld_r;
assign o_lut_key = s1_lut_key;
assign o_lut_size = s1_lut_volume;
assign o_lut_error = s1_lut_error;
assign o_lut_error_syndrome = s1_lut_error_syndrome;
assign o_lut_listsize = s1_lut_listsize;

assign o_state_ren = s0_state_ren;
assign o_state_raddr = s0_state_raddr;

endmodule // v_pipe_query

`include "unmacros.vh"
