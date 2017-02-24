//===- X86InstructionSelector.cpp ----------------------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the targeting of the InstructionSelector class for
/// X86.
/// \todo This should be generated by TableGen.
//===----------------------------------------------------------------------===//

#include "X86InstructionSelector.h"
#include "X86InstrInfo.h"
#include "X86RegisterBankInfo.h"
#include "X86RegisterInfo.h"
#include "X86Subtarget.h"
#include "X86TargetMachine.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "X86-isel"

using namespace llvm;

#ifndef LLVM_BUILD_GLOBAL_ISEL
#error "You shouldn't build this"
#endif

#include "X86GenGlobalISel.inc"

X86InstructionSelector::X86InstructionSelector(const X86Subtarget &STI,
                                               const X86RegisterBankInfo &RBI)
    : InstructionSelector(), TII(*STI.getInstrInfo()),
      TRI(*STI.getRegisterInfo()), RBI(RBI) {}

// FIXME: This should be target-independent, inferred from the types declared
// for each class in the bank.
static const TargetRegisterClass *
getRegClassForTypeOnBank(LLT Ty, const RegisterBank &RB) {
  if (RB.getID() == X86::GPRRegBankID) {
    if (Ty.getSizeInBits() <= 32)
      return &X86::GR32RegClass;
    if (Ty.getSizeInBits() == 64)
      return &X86::GR64RegClass;
  }

  llvm_unreachable("Unknown RegBank!");
}

// Set X86 Opcode and constrain DestReg.
static bool selectCopy(MachineInstr &I, const TargetInstrInfo &TII,
                       MachineRegisterInfo &MRI, const TargetRegisterInfo &TRI,
                       const RegisterBankInfo &RBI) {

  unsigned DstReg = I.getOperand(0).getReg();
  if (TargetRegisterInfo::isPhysicalRegister(DstReg)) {
    assert(I.isCopy() && "Generic operators do not allow physical registers");
    return true;
  }

  const RegisterBank &RegBank = *RBI.getRegBank(DstReg, MRI, TRI);
  const unsigned DstSize = MRI.getType(DstReg).getSizeInBits();
  (void)DstSize;
  unsigned SrcReg = I.getOperand(1).getReg();
  const unsigned SrcSize = RBI.getSizeInBits(SrcReg, MRI, TRI);
  (void)SrcSize;
  assert((!TargetRegisterInfo::isPhysicalRegister(SrcReg) || I.isCopy()) &&
         "No phys reg on generic operators");
  assert((DstSize == SrcSize ||
          // Copies are a mean to setup initial types, the number of
          // bits may not exactly match.
          (TargetRegisterInfo::isPhysicalRegister(SrcReg) &&
           DstSize <= RBI.getSizeInBits(SrcReg, MRI, TRI))) &&
         "Copy with different width?!");

  const TargetRegisterClass *RC = nullptr;

  switch (RegBank.getID()) {
  case X86::GPRRegBankID:
    assert((DstSize <= 64) && "GPRs cannot get more than 64-bit width values.");
    RC = getRegClassForTypeOnBank(MRI.getType(DstReg), RegBank);
    break;
  default:
    llvm_unreachable("Unknown RegBank!");
  }

  // No need to constrain SrcReg. It will get constrained when
  // we hit another of its use or its defs.
  // Copies do not have constraints.
  if (!RBI.constrainGenericRegister(DstReg, *RC, MRI)) {
    DEBUG(dbgs() << "Failed to constrain " << TII.getName(I.getOpcode())
                 << " operand\n");
    return false;
  }
  I.setDesc(TII.get(X86::COPY));
  return true;
}

bool X86InstructionSelector::select(MachineInstr &I) const {
  assert(I.getParent() && "Instruction should be in a basic block!");
  assert(I.getParent()->getParent() && "Instruction should be in a function!");

  MachineBasicBlock &MBB = *I.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  unsigned Opcode = I.getOpcode();
  if (!isPreISelGenericOpcode(Opcode)) {
    // Certain non-generic instructions also need some special handling.

    if (I.isCopy())
      return selectCopy(I, TII, MRI, TRI, RBI);

    // TODO: handle more cases - LOAD_STACK_GUARD, PHI
    return true;
  }

  assert(I.getNumOperands() == I.getNumExplicitOperands() &&
         "Generic instruction has unexpected implicit operands\n");

  return selectImpl(I);
}
