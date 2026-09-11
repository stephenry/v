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

`include "v_pkg.vh"
`include "macros.vh"

module v_pipe_update (

// -------------------------------------------------------------------------- //
// List Update Bus
  input wire logic                                i_upd_vld
, input wire v_pkg::id_t                          i_upd_prod_id
, input wire v_pkg::cmd_t                         i_upd_cmd
, input wire v_pkg::key_t                         i_upd_key
, input wire v_pkg::size_t                        i_upd_size

// -------------------------------------------------------------------------- //
// State Interface
//
, input wire v_pkg::state_t                       i_state_rdata
//
, output wire logic                               o_state_ren
, output wire v_pkg::addr_t                       o_state_raddr
//
, output wire logic                               o_state_wen_r
, output wire v_pkg::addr_t                       o_state_waddr_r
, output wire v_pkg::state_t                      o_state_wdata_r

// -------------------------------------------------------------------------- //
// Notify Bus
, output wire logic                               o_lv0_vld_r
, output wire v_pkg::id_t                         o_lv0_prod_id_r
, output wire v_pkg::key_t                        o_lv0_key_r
, output wire v_pkg::size_t                       o_lv0_size_r

// -------------------------------------------------------------------------- //
// Update Error (OOB Context)
, output wire logic                               o_upd_error_r

// -------------------------------------------------------------------------- //
// Update Pipeline Interface
//
, output wire logic                               o_s1_upd_vld_r
, output wire v_pkg::id_t                         o_s1_upd_prod_id_r
//
, output wire logic                               o_s2_upd_vld_r
, output wire v_pkg::id_t                         o_s2_upd_prod_id_r
//
, output wire logic                               o_s3_upd_vld_r
, output wire v_pkg::id_t                         o_s3_upd_prod_id_r
//
, output wire logic                               o_s4_upd_vld_r
, output wire v_pkg::id_t                         o_s4_upd_prod_id_r
//
, output wire logic                               o_s5_upd_vld_r
, output wire v_pkg::id_t                         o_s5_upd_prod_id_r

// -------------------------------------------------------------------------- //
// Initialization
, input wire logic                                init_r

// -------------------------------------------------------------------------- //
// Clk/Reset
, input wire logic                                clk
);

// ========================================================================== //
//                                                                            //
//  Wires                                                                     //
//                                                                            //
// ========================================================================== //

// S0:
logic                                             s1_upd_en;
logic                                             s0_upd_error_oob;

// S1:
//
logic                                             s1_state_ren;
v_pkg::addr_t                                     s1_state_raddr;
//
logic                                             s2_upd_en;

// S2:
//
logic [1:0]                                       s2_upd_state_fwd;
logic [1:0]                                       s2_upd_state_sel;
logic                                             s2_upd_state_sel_early;
v_pkg::state_t                                    s2_upd_state_early;
//
logic                                             s3_upd_en;

// S3:
//
logic [cfg_pkg::ENTRIES_N - 1:0]                  s3_exe_stcur_vld_r;
v_pkg::key_t [cfg_pkg::ENTRIES_N - 1:0]           s3_exe_stcur_keys_r;
logic                                             s3_upd_match_hit;
logic                                             s3_upd_match_full;
logic [cfg_pkg::ENTRIES_N - 1:0]                  s3_upd_match_sel;
logic [cfg_pkg::ENTRIES_N - 1:0]                  s3_upd_mask_cmp;

logic                                             s4_upd_en;

// S4:
//
logic [cfg_pkg::ENTRIES_N - 1:0]                  s4_exe_stcur_vld_r;
v_pkg::key_t [cfg_pkg::ENTRIES_N - 1:0]           s4_exe_stcur_keys_r;
v_pkg::volume_t [cfg_pkg::ENTRIES_N - 1:0]        s4_exe_stcur_volumes_r;
v_pkg::listsize_t                                 s4_exe_stcur_listsize_r;
//
logic [cfg_pkg::ENTRIES_N - 1:0]                  s4_exe_stnxt_vld;
v_pkg::key_t [cfg_pkg::ENTRIES_N - 1:0]           s4_exe_stnxt_keys;
v_pkg::volume_t [cfg_pkg::ENTRIES_N - 1:0]        s4_exe_stnxt_volumes;
v_pkg::listsize_t                                 s4_exe_stnxt_listsize;


// EXE
logic                                             exe_notify_vld;
v_pkg::key_t                                      exe_notify_key;
v_pkg::volume_t                                   exe_notify_volume;

// Writeback
logic                                             wrbk_en;

// Notify bus:
logic                                             lv0_en;

// ========================================================================== //
//                                                                            //
//  Flops                                                                     //
//                                                                            //
// ========================================================================== //

`V_DFF(logic, s1_upd_vld);
`V_DFFE(v_pkg::id_t, s1_upd_prod_id, s1_upd_en);
`V_DFFE(v_pkg::cmd_t, s1_upd_cmd, s1_upd_en);
`V_DFFE(v_pkg::key_t, s1_upd_key, s1_upd_en);
`V_DFFE(v_pkg::size_t, s1_upd_size, s1_upd_en);

`V_DFF(logic, s2_upd_vld);
`V_DFFE(v_pkg::id_t, s2_upd_prod_id, s2_upd_en);
`V_DFFE(v_pkg::cmd_t, s2_upd_cmd, s2_upd_en);
`V_DFFE(v_pkg::key_t, s2_upd_key, s2_upd_en);
`V_DFFE(v_pkg::size_t, s2_upd_size, s2_upd_en);
`V_DFFE(logic, s2_upd_wrbk_vld, s2_upd_en);
`V_DFFE(v_pkg::state_t, s2_upd_wrbk, s2_upd_en);

`V_DFF(logic, s3_upd_vld);
`V_DFFE(v_pkg::id_t, s3_upd_prod_id, s3_upd_en);
`V_DFFE(v_pkg::cmd_t, s3_upd_cmd, s3_upd_en);
`V_DFFE(v_pkg::key_t, s3_upd_key, s3_upd_en);
`V_DFFE(v_pkg::size_t, s3_upd_size, s3_upd_en);
`V_DFFE(v_pkg::state_t, s3_upd_state, s3_upd_en);

`V_DFF(logic, s4_upd_vld);
`V_DFFE(v_pkg::id_t, s4_upd_prod_id, s4_upd_en);
`V_DFFE(v_pkg::cmd_t, s4_upd_cmd, s4_upd_en);
`V_DFFE(v_pkg::key_t, s4_upd_key, s4_upd_en);
`V_DFFE(v_pkg::size_t, s4_upd_size, s4_upd_en);
`V_DFFE(v_pkg::state_t, s4_upd_state, s4_upd_en);
`V_DFFE(logic, s4_upd_match_hit, s4_upd_en);
`V_DFFE(logic, s4_upd_match_full, s4_upd_en);
`V_DFFE(logic [cfg_pkg::ENTRIES_N - 1:0], s4_upd_mask_cmp, s4_upd_en);
`V_DFFE(logic [cfg_pkg::ENTRIES_N - 1:0], s4_upd_match_sel, s4_upd_en);

`V_DFF(logic, wrbk_vld);
`V_DFFE(v_pkg::id_t, wrbk_prod_id, wrbk_en);
`V_DFFE(v_pkg::state_t, wrbk_state, wrbk_en);

`V_DFF(logic, lv0_vld);
`V_DFFE(v_pkg::id_t, lv0_prod_id, lv0_en);
`V_DFFE(v_pkg::key_t, lv0_key, lv0_en);
`V_DFFE(v_pkg::size_t, lv0_size, lv0_en);

// OOB Context error delay line (aligned with writeback latency).
`V_DFF(logic, s1_upd_err);
`V_DFF(logic, s2_upd_err);
`V_DFF(logic, s3_upd_err);
`V_DFF(logic, s4_upd_err);
`V_DFF(logic, wrbk_err);

// ========================================================================== //
//                                                                            //
// S0 Stage: Input Latch                                                      //
//                                                                            //
// ========================================================================== //

// -------------------------------------------------------------------------- //
// Out-of-bounds Context detection.
//
// Context IDs are sized as $clog2(CONTEXT_N) bits. When CONTEXT_N is not a
// power of two, encodings in [CONTEXT_N, 2**W) are representable but illegal.
// Fail-safe behaviour: do not admit the command into the update datapath
// (no SRAM access / writeback / notify), and pulse o_upd_error_r at writeback
// latency so the error is signalled consistently with the rest of the pipe.
//
assign s0_upd_error_oob =
    ($unsigned(i_upd_prod_id) >= cfg_pkg::CONTEXT_N);

// Pipeline controls:
//
assign s1_upd_vld_w = i_upd_vld & (~init_r) & (~s0_upd_error_oob);
assign s1_upd_err_w = i_upd_vld & (~init_r) &   s0_upd_error_oob;

assign s1_upd_en = s1_upd_vld_w;
assign s1_upd_prod_id_w = i_upd_prod_id;
assign s1_upd_cmd_w = i_upd_cmd;
assign s1_upd_key_w = i_upd_key;
assign s1_upd_size_w = i_upd_size;

// ========================================================================== //
//                                                                            //
// S1 Stage: Table Lookup                                                     //
//                                                                            //
// ========================================================================== //

// Writeback collision: Forward write-back state to S2 read port on collision;
// also kill lookup into state table to prevent possible data corruption where
// memory does not support forwarding.
assign s2_upd_wrbk_vld_w = wrbk_vld_r & (wrbk_prod_id_r == s1_upd_prod_id_r);
assign s2_upd_wrbk_w = wrbk_state_r;

//
assign s1_state_ren = s1_upd_vld_r & (~s2_upd_wrbk_vld_w);
assign s1_state_raddr = s1_upd_prod_id_r;

// Pipeline controls:
//
assign s2_upd_vld_w = s1_upd_vld_r & (~init_r);
assign s2_upd_err_w = s1_upd_err_r & (~init_r);

assign s2_upd_en = s2_upd_vld_w;
assign s2_upd_prod_id_w = s1_upd_prod_id_r;
assign s2_upd_cmd_w = s1_upd_cmd_r;
assign s2_upd_key_w = s1_upd_key_r;
assign s2_upd_size_w = s1_upd_size_r;

// ========================================================================== //
//                                                                            //
// S2 Stage: Lookup Stage                                                     //
//                                                                            //
// ========================================================================== //

// -------------------------------------------------------------------------- //
// S2 Stage: State arrival and EXE-forwarding stage.
//

// -------------------------------------------------------------------------- //
// Attempt hit on current writeback.
assign s2_upd_state_fwd [1] = wrbk_vld_w & (wrbk_prod_id_w == s2_upd_prod_id_r);
// Otherwise, attempt hit on prior writeback
assign s2_upd_state_fwd [0] = s2_upd_wrbk_vld_r;

pri #(.W(2)) u_s2_forwarding_pri (
  //
    .i_x                                (s2_upd_state_fwd)
  //
  , .o_y                                (s2_upd_state_sel)
);

assign s2_upd_state_sel_early = (|s2_upd_state_fwd);

// -------------------------------------------------------------------------- //
// Early state forwarding; the state that is expected to arrival
// relatively early into the current cycle.
//
assign s2_upd_state_early =
   ({v_pkg::STATE_BITS{s2_upd_state_sel[1]}} & wrbk_state_w) |
   ({v_pkg::STATE_BITS{s2_upd_state_sel[0]}} & s2_upd_wrbk_r);

// -------------------------------------------------------------------------- //
// Final State forwarding injecting those signals (from RAM) that is expected to
// arrive relatively late into the cycle. To this so that such state is injected
// late into the overall combinatorial logic cone.
//
assign s3_upd_state_w =
   ({v_pkg::STATE_BITS{ s2_upd_state_sel_early}} & s2_upd_state_early) |
   ({v_pkg::STATE_BITS{~s2_upd_state_sel_early}} & i_state_rdata);


assign s3_upd_vld_w = s2_upd_vld_r & (~init_r);
assign s3_upd_err_w = s2_upd_err_r & (~init_r);

assign s3_upd_en =  s3_upd_vld_w;
assign s3_upd_prod_id_w = s2_upd_prod_id_r;
assign s3_upd_cmd_w = s2_upd_cmd_r;
assign s3_upd_key_w = s2_upd_key_r;
assign s3_upd_size_w = s2_upd_size_r;

// ========================================================================== //
//                                                                            //
// S3 Stage: Compare Stage                                                    //
//                                                                            //
// ========================================================================== //

assign s3_exe_stcur_vld_r = s3_upd_state_r.vld;
assign s3_exe_stcur_keys_r = s3_upd_state_r.key;

v_pipe_update_cmp u_v_pipe_update_cmp (
  //
    .i_pipe_key_r                       (s3_upd_key_r)
  //
  , .i_stcur_vld_r                      (s3_exe_stcur_vld_r)
  , .i_stcur_keys_r                     (s3_exe_stcur_keys_r)
  //
  , .o_match_hit                        (s3_upd_match_hit)
  , .o_match_full                       (s3_upd_match_full)
  , .o_mask_cmp                         (s3_upd_mask_cmp)
  , .o_match_sel                        (s3_upd_match_sel)
);

// -------------------------------------------------------------------------- //
//
assign s4_upd_vld_w = s3_upd_vld_r & (~init_r);
assign s4_upd_err_w = s3_upd_err_r & (~init_r);

assign s4_upd_en =  s4_upd_vld_w;
assign s4_upd_prod_id_w = s3_upd_prod_id_r;
assign s4_upd_cmd_w = s3_upd_cmd_r;
assign s4_upd_key_w = s3_upd_key_r;
assign s4_upd_size_w = s3_upd_size_r;
assign s4_upd_state_w = s3_upd_state_r;
assign s4_upd_mask_cmp_w = s3_upd_mask_cmp;
assign s4_upd_match_hit_w = s3_upd_match_hit;
assign s4_upd_match_full_w = s3_upd_match_full;
assign s4_upd_match_sel_w = s3_upd_match_sel;

// ========================================================================== //
//                                                                            //
// S4 Stage: Execute Stage                                                    //
//                                                                            //
// ========================================================================== //

// -------------------------------------------------------------------------- //
//
assign s4_exe_stcur_vld_r = s4_upd_state_r.vld;
assign s4_exe_stcur_keys_r = s4_upd_state_r.key;
assign s4_exe_stcur_volumes_r = s4_upd_state_r.volume;
assign s4_exe_stcur_listsize_r = s4_upd_state_r.listsize;

// -------------------------------------------------------------------------- //
//
v_pipe_update_exe u_v_pipe_update_exe (
  //
    .i_pipe_cmd_r                       (s4_upd_cmd_r)
  , .i_pipe_key_r                       (s4_upd_key_r)
  , .i_pipe_volume_r                    (s4_upd_size_r)
  , .i_pipe_match_hit_r                 (s4_upd_match_hit_r)
  , .i_pipe_match_full_r                (s4_upd_match_full_r)
  , .i_pipe_match_sel_r                 (s4_upd_match_sel_r)
  , .i_pipe_mask_cmp_r                  (s4_upd_mask_cmp_r)
  //
  , .i_stcur_vld_r                      (s4_exe_stcur_vld_r)
  , .i_stcur_keys_r                     (s4_exe_stcur_keys_r)
  , .i_stcur_volumes_r                  (s4_exe_stcur_volumes_r)
  , .i_stcur_listsize_r                 (s4_exe_stcur_listsize_r)
  //
  , .o_stnxt_vld                        (s4_exe_stnxt_vld)
  , .o_stnxt_keys                       (s4_exe_stnxt_keys)
  , .o_stnxt_volumes                    (s4_exe_stnxt_volumes)
  , .o_stnxt_listsize                   (s4_exe_stnxt_listsize)
  //
  , .o_notify_vld                       (exe_notify_vld)
  , .o_notify_key                       (exe_notify_key)
  , .o_notify_volume                    (exe_notify_volume)
);

// Writeback:
assign wrbk_vld_w = s4_upd_vld_r & (~init_r);
assign wrbk_en = wrbk_vld_w;
assign wrbk_prod_id_w = s4_upd_prod_id_r;
assign wrbk_state_w.vld = s4_exe_stnxt_vld;
assign wrbk_state_w.listsize = s4_exe_stnxt_listsize;
assign wrbk_state_w.key = s4_exe_stnxt_keys;
assign wrbk_state_w.volume = s4_exe_stnxt_volumes;

// OOB error writeback pulse (no state side-effects; command never entered
// the datapath — this flop chain exists only to time the error flag).
assign wrbk_err_w = s4_upd_err_r & (~init_r);

// -------------------------------------------------------------------------- //
// Emit messages

// Notify bus:
assign lv0_vld_w = s4_upd_vld_r & exe_notify_vld & (~init_r);
assign lv0_en = lv0_vld_w;
assign lv0_prod_id_w = s4_upd_prod_id_r;
assign lv0_key_w = exe_notify_key;
assign lv0_size_w = exe_notify_volume;

// ========================================================================== //
//                                                                            //
//  Outputs                                                                   //
//                                                                            //
// ========================================================================== //

assign o_state_ren = s1_state_ren;
assign o_state_raddr = s1_state_raddr;

// State update
assign o_state_wen_r = wrbk_vld_r;
assign o_state_waddr_r = wrbk_prod_id_r;
assign o_state_wdata_r = wrbk_state_r;

// Notify interface
assign o_lv0_vld_r = lv0_vld_r;
assign o_lv0_prod_id_r = lv0_prod_id_r;
assign o_lv0_key_r = lv0_key_r;
assign o_lv0_size_r = lv0_size_r;

// Update OOB error (aligned with writeback / notify latency).
assign o_upd_error_r = wrbk_err_r;

// Update pipeline status.
assign o_s1_upd_vld_r = s1_upd_vld_r;
assign o_s1_upd_prod_id_r = s1_upd_prod_id_r;
assign o_s2_upd_vld_r = s2_upd_vld_r;
assign o_s2_upd_prod_id_r = s2_upd_prod_id_r;
assign o_s3_upd_vld_r = s3_upd_vld_r;
assign o_s3_upd_prod_id_r = s3_upd_prod_id_r;
assign o_s4_upd_vld_r = s4_upd_vld_r;
assign o_s4_upd_prod_id_r = s4_upd_prod_id_r;
assign o_s5_upd_vld_r = wrbk_vld_r;
assign o_s5_upd_prod_id_r = wrbk_prod_id_r;

endmodule // v_pipe_update

`include "unmacros.vh"
