/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENCE - 1.0
 *
 * This project is licensed under the AAF Inspectable Licence 1.0.
 * You are allowed to inspect the source of this project(s) free of
 * cost, and also to verify the authenticity of the product.
 *
 * Unless required by applicable law, this project is provided
 * "AS IS", WITHOUT ANY WARRANTIES OR PROMISES OF ANY KIND, either
 * expressed or implied. The author(s) of this project is not
 * liable for any harms, errors or troubles caused by using the
 * source or the product, unless implied by law. By using this
 * project, or part of it, you are acknowledging the complete terms
 * and conditions of licensing of this project as specified in AAF
 * Inspectable Licence 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicence/
 *
 * This project may contain parts that are not licensed under the
 * same licence. If so, the licences of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same licence in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, its
 * contents, the source or the product or the build result of the
 * source under commercial or non-commercial purposes. You are NOT
 * ALLOWED to revamp, rebrand, refactor, modify, the source, product
 * or the contents of this project.
 *
 * You are NOT ALLOWED to use the name, branding and identity of this
 * project to identify or brand any other project. You ARE however
 * allowed to use the name and branding to pinpoint/show the source
 * of the contents/code/logic of any other project. You are not
 * allowed to use the identification of the Authors of this project
 * to associate them to other projects, in a way that is deceiving
 * or misleading or gives out false information.
 */

#include "./string_to_callingconv.hpp"

llvm::CallingConv::ID qat::utils::stringToCallingConv(std::string name) {
  if (name == "C") {
    return llvm::CallingConv::C;
  } else if (name == "Fast") {
    return llvm::CallingConv::Fast;
  } else if (name == "Cold") {
    return llvm::CallingConv::Cold;
  } else if (name == "GHC") {
    return llvm::CallingConv::GHC;
  } else if (name == "HiPE") {
    return llvm::CallingConv::HiPE;
  } else if (name == "WebKitJS") {
    return llvm::CallingConv::WebKit_JS;
  } else if (name == "AnyReg") {
    return llvm::CallingConv::AnyReg;
  } else if (name == "PreserveMost") {
    return llvm::CallingConv::PreserveMost;
  } else if (name == "PreserveAll") {
    return llvm::CallingConv::PreserveAll;
  } else if (name == "Swift") {
    return llvm::CallingConv::Swift;
  } else if (name == "CXX_FAST_TLS") {
    return llvm::CallingConv::CXX_FAST_TLS;
  } else if (name == "Tail") {
    return llvm::CallingConv::Tail;
  } else if (name == "CFGuardCheck") {
    return llvm::CallingConv::CFGuard_Check;
  } else if (name == "SwiftTail") {
    return llvm::CallingConv::SwiftTail;
  } else if (name == "FirstTargetCC") {
    return llvm::CallingConv::FirstTargetCC;
  } else if (name == "X86_StdCall") {
    return llvm::CallingConv::X86_StdCall;
  } else if (name == "X86_FastCall") {
    return llvm::CallingConv::X86_FastCall;
  } else if (name == "ARM_APCS") {
    return llvm::CallingConv::ARM_APCS;
  } else if (name == "ARM_AAPCS") {
    return llvm::CallingConv::ARM_AAPCS;
  } else if (name == "ARM_AAPCS_VFP") {
    return llvm::CallingConv::ARM_AAPCS_VFP;
  } else if (name == "MSP430_INTR") {
    return llvm::CallingConv::MSP430_INTR;
  } else if (name == "X86_ThisCall") {
    return llvm::CallingConv::X86_ThisCall;
  } else if (name == "PTX_Kernel") {
    return llvm::CallingConv::PTX_Kernel;
  } else if (name == "PTX_Device") {
    return llvm::CallingConv::PTX_Device;
  } else if (name == "SPIR_FUNC") {
    return llvm::CallingConv::SPIR_FUNC;
  } else if (name == "SPIR_KERNEL") {
    return llvm::CallingConv::SPIR_KERNEL;
  } else if (name == "Intel_OCL_BI") {
    return llvm::CallingConv::Intel_OCL_BI;
  } else if (name == "X86_64_SysV") {
    return llvm::CallingConv::X86_64_SysV;
  } else if (name == "Win64") {
    return llvm::CallingConv::Win64;
  } else if (name == "X86_VectorCall") {
    return llvm::CallingConv::X86_VectorCall;
  } else if (name == "HHVM") {
    return llvm::CallingConv::HHVM;
  } else if (name == "HHVM_C") {
    return llvm::CallingConv::HHVM_C;
  } else if (name == "X86_INTR") {
    return llvm::CallingConv::X86_INTR;
  } else if (name == "AVR_INTR") {
    return llvm::CallingConv::AVR_INTR;
  } else if (name == "AVR_SIGNAL") {
    return llvm::CallingConv::AVR_SIGNAL;
  } else if (name == "AVR_BUILTIN") {
    return llvm::CallingConv::AVR_BUILTIN;
  } else if (name == "AMDGPU_VS") {
    return llvm::CallingConv::AMDGPU_VS;
  } else if (name == "AMDGPU_GS") {
    return llvm::CallingConv::AMDGPU_GS;
  } else if (name == "AMDGPU_PS") {
    return llvm::CallingConv::AMDGPU_PS;
  } else if (name == "AMDGPU_CS") {
    return llvm::CallingConv::AMDGPU_CS;
  } else if (name == "AMDGPU_KERNEL") {
    return llvm::CallingConv::AMDGPU_KERNEL;
  } else if (name == "X86_RegCall") {
    return llvm::CallingConv::X86_RegCall;
  } else if (name == "AMDGPU_HS") {
    return llvm::CallingConv::AMDGPU_HS;
  } else if (name == "MSP430_BUILTIN") {
    return llvm::CallingConv::MSP430_BUILTIN;
  } else if (name == "AMDGPU_LS") {
    return llvm::CallingConv::AMDGPU_LS;
  } else if (name == "AMDGPU_ES") {
    return llvm::CallingConv::AMDGPU_ES;
  } else if (name == "AArch64VectorCall") {
    return llvm::CallingConv::AArch64_VectorCall;
  } else if (name == "AArch64_SVE_VectorCall") {
    return llvm::CallingConv::AArch64_SVE_VectorCall;
  } else if (name == "WASM_EmscriptenInvoke") {
    return llvm::CallingConv::WASM_EmscriptenInvoke;
  } else if (name == "AMDGPU_Gfx") {
    return llvm::CallingConv::AMDGPU_Gfx;
  } else if (name == "M68k_INTR") {
    return llvm::CallingConv::M68k_INTR;
  } else {
    return 1024;
  }
}