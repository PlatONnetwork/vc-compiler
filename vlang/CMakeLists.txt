cmake_minimum_required(VERSION 2.8)
project(vlang)

if(NOT DEFINED LLVM_CONFIG)
  find_program(LLVM_CONFIG NAMES llvm-config-6.0)

  if(NOT LLVM_CONFIG)
    find_program(LLVM_CONFIG NAMES llvm-config)
  endif()

  if(NOT LLVM_CONFIG)
    message(FATAL_ERROR "Can not find llvm-config, Please specify by -DLLVM_CONFIG=")
  endif()

endif()


execute_process(COMMAND ${LLVM_CONFIG} --includedir
  OUTPUT_VARIABLE LLVM_INCLUDEDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND ${LLVM_CONFIG} --libdir
  OUTPUT_VARIABLE LLVM_LIBDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fPIC -fvisibility-inlines-hidden -Werror=date-time -std=c++11 -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcast-qual -Wno-missing-field-initializers -pedantic -Wno-long-long -Wno-maybe-uninitialized -Wdelete-non-virtual-dtor -Wno-comment -g  -fno-exceptions -fno-rtti -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")

include_directories(${LLVM_INCLUDEDIR})

link_directories(${LLVM_LIBDIR})


add_subdirectory(Transforms)

add_executable(vlang
  GenKey.cpp  
  GenVCC.cpp  
  parseArgs.cpp
  vlang.cpp
  )


target_link_libraries(vlang
  clangCodeGen 
  clangTooling 
  clangFrontend 
  clangASTMatchers 
  clangParse 
  clangSerialization 
  clangSema 
  clangEdit  
  clangAnalysis 
  clangDriver 
  clangFormat 
  clangToolingCore 
  clangAST 
  clangRewrite 
  clangLex 
  clangBasic
  )

target_link_libraries(vlang Unroll)

target_link_libraries(vlang
  LLVMLTO 
  LLVMPasses 
  LLVMObjCARCOpts 
  LLVMMIRParser 
  LLVMSymbolize 
  LLVMDebugInfoPDB 
  LLVMDebugInfoDWARF 
  LLVMCoverage 
  LLVMTableGen 
  LLVMDlltoolDriver 
  LLVMOrcJIT 

  LLVMOption

  LLVMGlobalISel 
  LLVMSelectionDAG 
  LLVMAsmPrinter 
  LLVMDebugInfoCodeView 
  LLVMDebugInfoMSF 
  LLVMX86Desc 
  LLVMMCDisassembler 
  LLVMX86Info 
  LLVMX86AsmPrinter 
  LLVMX86Utils 
  LLVMMCJIT 
  LLVMLineEditor 
  LLVMInterpreter 
  LLVMExecutionEngine 
  LLVMRuntimeDyld 
  LLVMCodeGen 
  LLVMTarget 
  LLVMCoroutines 
  LLVMipo 
  LLVMInstrumentation 
  LLVMVectorize 
  LLVMScalarOpts 
  LLVMLinker 
  LLVMIRReader 
  LLVMAsmParser 
  LLVMInstCombine 
  LLVMTransformUtils 
  LLVMBitWriter 
  LLVMAnalysis 
  LLVMProfileData 
  LLVMObject 
  LLVMMCParser 
  LLVMMC 
  LLVMBitReader 
  LLVMCore 
  LLVMBinaryFormat 
  LLVMSupport 
  LLVMDemangle
  

)

target_link_libraries(vlang


  csnark
  snark
  ff

  gmpxx
  gmp
  procps
  )

target_link_libraries(vlang
  z 
  rt 
  dl 
  tinfo 
  pthread 
  m
)
