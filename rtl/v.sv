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
`include "cfg_pkg.vh"

module v (

// -------------------------------------------------------------------------- //
// List Update Bus
  input                                           i_upd_vld
, input v_pkg::id_t                               i_upd_prod_id
, input v_pkg::cmd_t                              i_upd_cmd
, input v_pkg::key_t                              i_upd_key
, input v_pkg::size_t                             i_upd_size

// -------------------------------------------------------------------------- //
// List Query Bus
, input                                           i_lut_vld
, input v_pkg::id_t                               i_lut_prod_id
, input v_pkg::level_t                            i_lut_level
//
, output logic                                    o_lut_vld_r
, output v_pkg::key_t                             o_lut_key
, output v_pkg::size_t                            o_lut_size
, output logic                                    o_lut_error
, output v_pkg::listsize_t                        o_lut_listsize

// -------------------------------------------------------------------------- //
// Notify Bus
, output logic                                    o_lv0_vld_r
, output v_pkg::id_t                              o_lv0_prod_id_r
, output v_pkg::key_t                             o_lv0_key_r
, output v_pkg::size_t                            o_lv0_size_r

// -------------------------------------------------------------------------- //
// Status
, output logic                                    o_busy_r

// -------------------------------------------------------------------------- //
// Clk/Reset
, input                                           clk
, input                                           arst_n
);

// ========================================================================== //
//                                                                            //
//  Wires                                                                     //
//                                                                            //
// ========================================================================== //

//
logic                                   update_ren;
v_pkg::addr_t                           update_raddr;
v_pkg::state_t                          update_rdata;
//
logic                                   query_ren;
v_pkg::addr_t                           query_raddr;
v_pkg::state_t                          query_rdata;
//
logic                                   init_wen_r;
v_pkg::addr_t                           init_waddr_r;
v_pkg::state_t                          init_wdata_r;
//
logic                                   state_wen_r;
v_pkg::addr_t                           state_waddr_r;
v_pkg::state_t                          state_wdata_r;
//
logic                                   wen;
v_pkg::addr_t                           waddr;
v_pkg::state_t                          wdata;

//
logic                                   s1_upd_vld_r;
v_pkg::id_t                             s1_upd_prod_id_r;
logic                                   s2_upd_vld_r;
v_pkg::id_t                             s2_upd_prod_id_r;
logic                                   s3_upd_vld_r;
v_pkg::id_t                             s3_upd_prod_id_r;
logic                                   s4_upd_vld_r;
v_pkg::id_t                             s4_upd_prod_id_r;
logic                                   s5_upd_vld_r;
v_pkg::id_t                             s5_upd_prod_id_r;

// ========================================================================== //
//                                                                            //
//  Flops                                                                     //
//                                                                            //
// ========================================================================== //

`V_DFFR(logic, init, 'b1);

// ========================================================================== //
//                                                                            //
//  Combinatorial Logic                                                       //
//                                                                            //
// ========================================================================== //

// -------------------------------------------------------------------------- //
// 'Init' boot flag to initiate initialization process after reset.
//
assign init_w = '0;

// -------------------------------------------------------------------------- //
// Mux. in BRAM initialization channels when active (otherwise, infer BRAM which
// are initialized to zero).
//
assign wen   = o_busy_r ? init_wen_r : state_wen_r;
assign waddr = o_busy_r ? init_waddr_r : state_waddr_r;
assign wdata = o_busy_r ? init_wdata_r : state_wdata_r;

// ========================================================================== //
//                                                                            //
//  Instances                                                                 //
//                                                                            //
// ========================================================================== //

// -------------------------------------------------------------------------- //
//
v_pipe_update u_v_pipe_update (
  //
    .i_upd_vld                          (i_upd_vld)
  , .i_upd_prod_id                      (i_upd_prod_id)
  , .i_upd_cmd                          (i_upd_cmd)
  , .i_upd_key                          (i_upd_key)
  , .i_upd_size                         (i_upd_size)
  //
  , .i_state_rdata                      (update_rdata)
  , .o_state_ren                        (update_ren)
  , .o_state_raddr                      (update_raddr)
  //
  , .o_state_wen_r                      (state_wen_r)
  , .o_state_waddr_r                    (state_waddr_r)
  , .o_state_wdata_r                    (state_wdata_r)
  //
  , .o_lv0_vld_r                        (o_lv0_vld_r)
  , .o_lv0_prod_id_r                    (o_lv0_prod_id_r)
  , .o_lv0_key_r                        (o_lv0_key_r)
  , .o_lv0_size_r                       (o_lv0_size_r)
  //
  , .o_s1_upd_vld_r                     (s1_upd_vld_r)
  , .o_s1_upd_prod_id_r                 (s1_upd_prod_id_r)
  , .o_s2_upd_vld_r                     (s2_upd_vld_r)
  , .o_s2_upd_prod_id_r                 (s2_upd_prod_id_r)
  , .o_s3_upd_vld_r                     (s3_upd_vld_r)
  , .o_s3_upd_prod_id_r                 (s3_upd_prod_id_r)
  , .o_s4_upd_vld_r                     (s4_upd_vld_r)
  , .o_s4_upd_prod_id_r                 (s4_upd_prod_id_r)
  , .o_s5_upd_vld_r                     (s5_upd_vld_r)
  , .o_s5_upd_prod_id_r                 (s5_upd_prod_id_r)
  //
  , .clk                                (clk)
);

// -------------------------------------------------------------------------- //
//
sram1r1w #(.N(cfg_pkg::CONTEXT_N), .W(v_pkg::STATE_BITS)) u_sram1r1w_update (
  //
    .i_ren                              (update_ren)
  , .i_raddr                            (update_raddr)
  , .o_rdata                            (update_rdata)
  //
  , .i_wen                              (wen)
  , .i_waddr                            (waddr)
  , .i_wdata                            (wdata)
  //
  , .clk                                (clk)
);

// -------------------------------------------------------------------------- //
v_pipe_query u_v_pipe_query (
  //
    .i_lut_vld                          (i_lut_vld)
  , .i_lut_prod_id                      (i_lut_prod_id)
  , .i_lut_level                        (i_lut_level)
  //
  , .o_lut_vld_r                        (o_lut_vld_r)
  , .o_lut_key                          (o_lut_key)
  , .o_lut_size                         (o_lut_size)
  , .o_lut_error                        (o_lut_error)
  , .o_lut_listsize                     (o_lut_listsize)
  //
  , .i_state_rdata                      (query_rdata)
  , .o_state_ren                        (query_ren)
  , .o_state_raddr                      (query_raddr)
  //
  , .i_s1_upd_vld_r                     (s1_upd_vld_r)
  , .i_s1_upd_prod_id_r                 (s1_upd_prod_id_r)
  , .i_s2_upd_vld_r                     (s2_upd_vld_r)
  , .i_s2_upd_prod_id_r                 (s2_upd_prod_id_r)
  , .i_s3_upd_vld_r                     (s3_upd_vld_r)
  , .i_s3_upd_prod_id_r                 (s3_upd_prod_id_r)
  , .i_s4_upd_vld_r                     (s4_upd_vld_r)
  , .i_s4_upd_prod_id_r                 (s4_upd_prod_id_r)
  , .i_s5_upd_vld_r                     (s5_upd_vld_r)
  , .i_s5_upd_prod_id_r                 (s5_upd_prod_id_r)
  //
  , .clk                                (clk)
);

// -------------------------------------------------------------------------- //
//
sram1r1w #(.N(cfg_pkg::CONTEXT_N), .W(v_pkg::STATE_BITS)) u_sram1r1w_query (
  //
    .i_ren                              (query_ren)
  , .i_raddr                            (query_raddr)
  , .o_rdata                            (query_rdata)
  //
  , .i_wen                              (wen)
  , .i_waddr                            (waddr)
  , .i_wdata                            (wdata)
  //
  , .clk                                (clk)
);

// -------------------------------------------------------------------------- //
//
v_init #(.N(cfg_pkg::CONTEXT_N), .W(v_pkg::STATE_BITS)) u_init (
  //
    .o_init_wen_r                       (init_wen_r)
  , .o_init_waddr_r                     (init_waddr_r)
  , .o_init_wdata_r                     (init_wdata_r)
  //
  , .i_init                             (init_r)
  //
  , .o_busy_r                           (o_busy_r)
  //
  , .clk                                (clk)
);

endmodule // v
