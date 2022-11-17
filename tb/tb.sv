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
// ========================================================================== //

`include "common_defs.vh"

`include "v_pkg.vh"

module tb (

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
// Testbench State
, output logic [31:0]                             o_tb_cycle
//
, output logic                                    o_tb_wrbk_vld_r
, output v_pkg::id_t                              o_tb_wrbk_prod_id_r
, output v_pkg::state_t                           o_tb_wrbk_state_r

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

int                                     tb_cycle;

// ========================================================================== //
//                                                                            //
//  UUT                                                                       //
//                                                                            //
// ========================================================================== //

v u_v (
  //
    .i_upd_vld                          (i_upd_vld)
  , .i_upd_prod_id                      (i_upd_prod_id)
  , .i_upd_cmd                          (i_upd_cmd)
  , .i_upd_key                          (i_upd_key)
  , .i_upd_size                         (i_upd_size)
  //
  , .i_lut_vld                          (i_lut_vld)
  , .i_lut_prod_id                      (i_lut_prod_id)
  , .i_lut_level                        (i_lut_level)
  , .o_lut_vld_r                        (o_lut_vld_r)
  , .o_lut_key                          (o_lut_key)
  , .o_lut_size                         (o_lut_size)
  , .o_lut_error                        (o_lut_error)
  , .o_lut_listsize                     (o_lut_listsize)
  //
  , .o_lv0_vld_r                        (o_lv0_vld_r)
  , .o_lv0_prod_id_r                    (o_lv0_prod_id_r)
  , .o_lv0_key_r                        (o_lv0_key_r)
  , .o_lv0_size_r                       (o_lv0_size_r)
  //
  , .o_busy_r                           (o_busy_r)
  //
  , .clk                                (clk)
  , .arst_n                             (arst_n)
);

// ========================================================================== //
//                                                                            //
//  TB                                                                        //
//                                                                            //
// ========================================================================== //

// -------------------------------------------------------------------------- //
//
initial tb_cycle = 0;

always_ff @(posedge clk)
  tb_cycle <= tb_cycle + 'b1;

// ========================================================================== //
//                                                                            //
//  Outputs                                                                   //
//                                                                            //
// ========================================================================== //

assign o_tb_cycle = tb_cycle;

// Expose updates to the state table.
assign o_tb_wrbk_vld_r = u_v.u_v_pipe_update.wrbk_vld_r;
assign o_tb_wrbk_prod_id_r = u_v.u_v_pipe_update.wrbk_prod_id_r;
assign o_tb_wrbk_state_r = u_v.u_v_pipe_update.wrbk_state_r;

endmodule // v
