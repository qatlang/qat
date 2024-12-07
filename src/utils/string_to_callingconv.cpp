#include "./string_to_callingconv.hpp"

namespace qat::utils {

llvm::CallingConv::ID stringToCallingConv(const String& name) {
	namespace CallingConv = llvm::CallingConv;

	// TODO - Check WebKitJS
	if (name == "C") {
		return CallingConv::C;
	} else if (name == "Fast") {
		return CallingConv::Fast;
	} else if (name == "Cold") {
		return CallingConv::Cold;
	} else if (name == "GHC") {
		return CallingConv::GHC;
	} else if (name == "HiPE") {
		return CallingConv::HiPE;
	} else if (name == "AnyReg") {
		return CallingConv::AnyReg;
	} else if (name == "PreserveMost") {
		return CallingConv::PreserveMost;
	} else if (name == "PreserveAll") {
		return CallingConv::PreserveAll;
	} else if (name == "Swift") {
		return CallingConv::Swift;
	} else if (name == "CXX_FAST_TLS") {
		return CallingConv::CXX_FAST_TLS;
	} else if (name == "Tail") {
		return CallingConv::Tail;
	} else if (name == "CFGuardCheck") {
		return CallingConv::CFGuard_Check;
	} else if (name == "SwiftTail") {
		return CallingConv::SwiftTail;
	} else if (name == "FirstTargetCC") {
		return CallingConv::FirstTargetCC;
	} else if (name == "X86_StdCall") {
		return CallingConv::X86_StdCall;
	} else if (name == "X86_FastCall") {
		return CallingConv::X86_FastCall;
	} else if (name == "ARM_APCS") {
		return CallingConv::ARM_APCS;
	} else if (name == "ARM_AAPCS") {
		return CallingConv::ARM_AAPCS;
	} else if (name == "ARM_AAPCS_VFP") {
		return CallingConv::ARM_AAPCS_VFP;
	} else if (name == "MSP430_INTR") {
		return CallingConv::MSP430_INTR;
	} else if (name == "X86_ThisCall") {
		return CallingConv::X86_ThisCall;
	} else if (name == "PTX_Kernel") {
		return CallingConv::PTX_Kernel;
	} else if (name == "PTX_Device") {
		return CallingConv::PTX_Device;
	} else if (name == "SPIR_FUNC") {
		return CallingConv::SPIR_FUNC;
	} else if (name == "SPIR_KERNEL") {
		return CallingConv::SPIR_KERNEL;
	} else if (name == "Intel_OCL_BI") {
		return CallingConv::Intel_OCL_BI;
	} else if (name == "X86_64_SysV") {
		return CallingConv::X86_64_SysV;
	} else if (name == "Win64") {
		return CallingConv::Win64;
	} else if (name == "X86_VectorCall") {
		return CallingConv::X86_VectorCall;
	} else if (name == "X86_INTR") {
		return CallingConv::X86_INTR;
	} else if (name == "AVR_INTR") {
		return CallingConv::AVR_INTR;
	} else if (name == "AVR_SIGNAL") {
		return CallingConv::AVR_SIGNAL;
	} else if (name == "AVR_BUILTIN") {
		return CallingConv::AVR_BUILTIN;
	} else if (name == "AMDGPU_VS") {
		return CallingConv::AMDGPU_VS;
	} else if (name == "AMDGPU_GS") {
		return CallingConv::AMDGPU_GS;
	} else if (name == "AMDGPU_PS") {
		return CallingConv::AMDGPU_PS;
	} else if (name == "AMDGPU_CS") {
		return CallingConv::AMDGPU_CS;
	} else if (name == "AMDGPU_KERNEL") {
		return CallingConv::AMDGPU_KERNEL;
	} else if (name == "X86_RegCall") {
		return CallingConv::X86_RegCall;
	} else if (name == "AMDGPU_HS") {
		return CallingConv::AMDGPU_HS;
	} else if (name == "MSP430_BUILTIN") {
		return CallingConv::MSP430_BUILTIN;
	} else if (name == "AMDGPU_LS") {
		return CallingConv::AMDGPU_LS;
	} else if (name == "AMDGPU_ES") {
		return CallingConv::AMDGPU_ES;
	} else if (name == "AArch64VectorCall") {
		return CallingConv::AArch64_VectorCall;
	} else if (name == "AArch64_SVE_VectorCall") {
		return CallingConv::AArch64_SVE_VectorCall;
	} else if (name == "WASM_EmscriptenInvoke") {
		return CallingConv::WASM_EmscriptenInvoke;
	} else if (name == "AMDGPU_Gfx") {
		return CallingConv::AMDGPU_Gfx;
	} else if (name == "M68k_INTR") {
		return CallingConv::M68k_INTR;
	} else {
		return 1024;
	}
}

} // namespace qat::utils
