#!/bin/sh

LLVM_DIR=../../llvm

if [[ -n "$1" ]]; then
	LLVM_DIR=$1
fi

# Headers
cp ./ObjCRuntime.h ${LLVM_DIR}/tools/clang/include/clang/Basic/ObjCRuntime.h
cp ./Options.td ${LLVM_DIR}/tools/clang/include/clang/Driver/Options.td

# Source files
cp ./ASTContext.cpp ${LLVM_DIR}/tools/clang/lib/AST/ASTContext.cpp
cp ./ObjCRuntime.cpp ${LLVM_DIR}/tools/clang/lib/Basic/ObjCRuntime.cpp
cp ./CGException.cpp ${LLVM_DIR}/tools/clang/lib/CodeGen/CGException.cpp
cp ./CGObjCGNU.cpp ${LLVM_DIR}/tools/clang/lib/CodeGen/CGObjCGNU.cpp
cp ./CGObjCMac.cpp ${LLVM_DIR}/tools/clang/lib/CodeGen/CGObjCMac.cpp
cp ./CodeGenModule.cpp ${LLVM_DIR}/tools/clang/lib/CodeGen/CodeGenModule.cpp
cp ./Tools.cpp ${LLVM_DIR}/tools/clang/lib/Driver/Tools.cpp

