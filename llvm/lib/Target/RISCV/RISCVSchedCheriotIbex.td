//===-- RISCVSchedCheriotIbex.td - CHERIoT Ibex Processor --*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

def CheriotIbexModel : SchedMachineModel {
  let MicroOpBufferSize = 0;   // in-order
  let IssueWidth        = 1;   // single issue
  let CompleteModel     = 0;
}

let SchedModel = CheriotIbexModel in {

// Capability loads have a 2-cycle load-to-use penalty.
// Everything else is single cycle.
foreach W = !instances<SchedWrite>() in {
def : WriteRes<W, []> {
  let Latency = !if(!eq(W, WriteLDC), 3, 1);
}
}

foreach R = !instances<SchedRead>() in
def : ReadAdvance<R, 0>;

} // SchedModel = CheriotIbexModel
