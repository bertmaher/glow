#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <vector>

static void dumpAsm(
  llvm::raw_ostream &os, llvm::TargetMachine *mach, llvm::Module *m) {
    llvm::SmallVector<char, 0> asmBuffer;
    llvm::raw_svector_ostream asmStream(asmBuffer);
    llvm::legacy::PassManager PM;
    mach->addPassesToEmitFile(
      PM, asmStream, nullptr,
      llvm::TargetMachine::CodeGenFileType::CGFT_AssemblyFile);
    PM.run(*m);
    os << asmStream.str();
}

static llvm::SmallVector<std::string, 0> getAttrs() {
  llvm::SmallVector<std::string, 0> res;
  llvm::StringMap<bool> features;
  if (llvm::sys::getHostCPUFeatures(features)) {
    for (auto const &feature : features) {
      if (feature.second) {
        res.push_back(feature.first());
      }
    }
  }
  return res;
}

static void optimize(llvm::TargetMachine *TM, llvm::Module *M) {
  llvm::legacy::FunctionPassManager FPM(M);
  llvm::legacy::PassManager PM;

  // Add internal analysis passes from the target machine.
  PM.add(llvm::createTargetTransformInfoWrapperPass(TM->getTargetIRAnalysis()));
  FPM.add(llvm::createTargetTransformInfoWrapperPass(TM->getTargetIRAnalysis()));

  llvm::PassManagerBuilder PMB;
  PMB.OptLevel = 3;
  PMB.LoopVectorize = true;
  PMB.SLPVectorize = true;
  TM->adjustPassManager(PMB);

  PMB.populateFunctionPassManager(FPM);
  PMB.populateModulePassManager(PM);
  FPM.doInitialization();
  PM.run(*M);
  for (auto &FF : *M) {
    FPM.run(FF);
  }
  FPM.doFinalization();
  PM.run(*M);
}

int main() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  llvm::LLVMContext ctx;
  llvm::IRBuilder<> irb(ctx);
  auto module = std::make_unique<llvm::Module>("basic", ctx);

  // Types.
  auto i32 = llvm::Type::getInt32Ty(ctx);
  auto i8 = llvm::Type::getInt8Ty(ctx);
  auto int8PtrType = llvm::Type::getInt8PtrTy(ctx);
  auto voidType = llvm::Type::getVoidTy(ctx);

  // Function type: void(*)(int8_t *)
  auto fnType = llvm::FunctionType::get(voidType, {int8PtrType}, false);
  
  // Create function.
  auto fn = llvm::Function::Create(
    fnType, llvm::Function::ExternalLinkage, "inner", module.get());
  fn->setCallingConv(llvm::CallingConv::C);
  fn->addParamAttr(0, llvm::Attribute::NoAlias);

  // Set up preheader and induction variable.
  auto preheader = llvm::BasicBlock::Create(ctx, "preheader", fn);
  irb.SetInsertPoint(preheader);
  auto loop = llvm::BasicBlock::Create(ctx, "loop", fn);
  irb.CreateBr(loop);
  irb.SetInsertPoint(loop);
  auto idx = irb.CreatePHI(i32, 2);
  idx->addIncoming(
    llvm::Constant::getIntegerValue(i32, llvm::APInt(32, 0)),
    preheader);

  // Store 0 into the dest.
  auto dst = fn->arg_begin();
  irb.CreateStore(
    llvm::Constant::getIntegerValue(i8, llvm::APInt(8, 0)),
    irb.CreateGEP(dst, idx));

  // Increment induction variable and exit.
  auto inc = irb.CreateAdd(
    idx,
    llvm::Constant::getIntegerValue(i32, llvm::APInt(32, 1)));
  auto cond = irb.CreateICmpSLT(
    inc,
    llvm::Constant::getIntegerValue(i32, llvm::APInt(32, 1024)));
  auto exit = llvm::BasicBlock::Create(ctx, "exit", fn);
  irb.CreateCondBr(cond, loop, exit);
  irb.SetInsertPoint(exit);
  idx->addIncoming(inc, loop);

  // Return.
  irb.CreateRetVoid();

  // Create target machine.
  auto triple = llvm::sys::getDefaultTargetTriple();
  auto cpu = llvm::sys::getHostCPUName();
  auto attrs = getAttrs();
  auto mach = llvm::EngineBuilder().selectTarget(llvm::Triple(), "", cpu, attrs);
   
  module->setDataLayout(mach->createDataLayout());
  module->setTargetTriple(mach->getTargetTriple().normalize());

  // Optimize, verify, and dump assembly.
  llvm::errs() << *module;
  optimize(mach, module.get());
  llvm::errs() << *module;

  assert(!llvm::verifyFunction(*fn, &llvm::errs()));
  dumpAsm(llvm::errs(), mach, module.get());
  return 0;
}
