//===----------------------------------------------------------------------===//
//
// This provides Objective-C code generation targetting the GNU runtime.  The
// class in this file generates structures used by the GNU Objective-C runtime
// library.  These structures are defined in objc/objc.h and objc/objc-api.h in
// the GNU runtime distribution.
//
//===----------------------------------------------------------------------===//

#import "CGObjCRuntime.h"
#import "CodeGenTypes.h"
#include "LLVMCompat.h"
#include <llvm/Support/Compiler.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#if (LLVM_MAJOR > 3) || (LLVM_MAJOR == 3 && LLVM_MINOR >= 2)
#if (LLVM_MAJOR > 3) || (LLVM_MAJOR == 3 && LLVM_MINOR >= 3)
#include <llvm/IR/DataLayout.h>
#else
#include <llvm/DataLayout.h>
#endif
#define TargetData DataLayout
#else
#include <llvm/Target/TargetData.h>
#endif
#include "objc_pointers.h"
#include <map>
#include <string>
#include <algorithm>
#include <dlfcn.h>

#include "LLVMCompat.h"

extern "C" {
#import <Foundation/Foundation.h>
#import "../LanguageKit.h"
}

using namespace llvm;
using namespace std;
using namespace etoile::languagekit;

// The version of the runtime that this class targets.  Must match the version
// in the runtime.
static int RuntimeVersion = 9;
static int ProtocolVersion = 2;

namespace {
class CGObjCGNU : public CGObjCRuntime {
private:
	llvm::Module &TheModule;
	LLVMStructTy *SelStructTy;
	llvm::IntegerType *SelectorTy;
	LLVMType *PtrToInt8Ty;
	LLVMType *IMPTy;
	LLVMType *IdTy;
	llvm::IntegerType *IntTy;
	LLVMType *PtrTy;
	llvm::IntegerType *LongTy;
	llvm::IntegerType *SizeTy;
	
	llvm::IntegerType *IntegerBit;
	llvm::StructType *FlagsStructTy;
	llvm::StructType *ClassTy;
	LLVMType *PtrToIntTy;
	std::vector<llvm::Constant*> Classes;
	std::vector<llvm::Constant*> Categories;
	std::vector<llvm::Constant*> ConstantStrings;
	object_map<NSString*, llvm::Constant*> ExistingProtocols;
	typedef std::pair<NSString*, llvm::GlobalVariable*> TypedSelector;
	typedef object_map<NSString*, SmallVector<TypedSelector, 2> > SelectorMap;
	SelectorMap SelectorTable;
	// Some zeros used for GEPs in lots of places.
	llvm::Constant *Zeros[2];
	llvm::Constant *NULLPtr;
	bool GC;
	bool JIT;
	llvm::Constant *ObjCIvarOffsetVariable(NSString *className,
		NSString *ivarName, uint64_t Offset);
	llvm::Constant *GenerateIvarList(
		NSString *ClassName,
		StringVector &IvarNames,
		StringVector &IvarTypes,
		const llvm::SmallVectorImpl<int> &IvarOffsets,
		llvm::Constant *&IvarOffsetArray);
	llvm::Constant *GenerateMethodList(NSString *ClassName,
	                                   NSString *CategoryName,
	                                   StringVector &MethodNames, 
	                                   StringVector &MethodTypes, 
	                                   bool isClassMethodList);
	llvm::Constant *GenerateProtocolList(
		StringVector &Protocols);
	llvm::Constant *GenerateClassStructure(
		llvm::Constant *MetaClass,
		llvm::Constant *SuperClass,
		unsigned info,
		NSString *Name,
		llvm::Constant *Version,
		llvm::Constant *InstanceSize,
		llvm::Constant *IVars,
		llvm::Constant *Methods,
		llvm::Constant *Protocols,
		llvm::Constant *IvarOffsets,
		bool isMeta);
	llvm::Constant *GenerateProtocolMethodList(
		const llvm::SmallVectorImpl<llvm::Constant *> &MethodNames,
		const llvm::SmallVectorImpl<llvm::Constant *> &MethodTypes);
	llvm::Constant *MakeConstantString(NSString *Str, const std::string
		&Name="");
	llvm::Constant *MakeGlobal(LLVMStructTy *Ty,
		std::vector<llvm::Constant*> &V, const std::string &Name="",
		bool isPublic=false);
	llvm::Constant *MakeGlobal(LLVMArrayTy *Ty,
		std::vector<llvm::Constant*> &V, const std::string &Name="");
	llvm::Value *GetWeakSymbol(const std::string &Name,
                               LLVMType *Type);
public:
	CGObjCGNU(
		CodeGenTypes *cgTypes,
		llvm::Module &Mp,
		llvm::LLVMContext &C,
		bool enableGC,
		bool isJit);
	virtual void lookupIMPAndTypes(CGBuilder &Builder,
	                               llvm::Value *Sender,
	                               llvm::Value *&Receiver,
	                               NSString *selName,
	                               llvm::Value *&imp,
	                               llvm::Value *&typeEncoding);
	virtual llvm::Constant *GenerateConstantString(NSString *String);
	virtual llvm::Value *GenerateMessageSend(CGBuilder &Builder,
	                                         llvm::Value *Sender,
	                                         llvm::Value *Receiver,
	                                         NSString *selName,
	                                         NSString *selTypes,
	                                         llvm::SmallVectorImpl<llvm::Value*> &ArgV,
	                                         llvm::BasicBlock *CleanupBlock,
	                                         NSString *ReceiverClass,
	                                         bool isClassMessage);
	virtual llvm::Value *GenerateMessageSendSuper(CGBuilder &Builder,
	                                              llvm::Value *Sender,
	                                              NSString *SuperClassName,
	                                              llvm::Value *Receiver,
	                                              NSString *selName,
	                                              NSString *selTypes,
	                                              llvm::SmallVectorImpl<llvm::Value*> &ArgV,
	                                              bool isClassMessage,
	                                              llvm::BasicBlock *CleanupBlock);
	virtual llvm::Constant *LookupClass(NSString *ClassName, bool isMeta);
	virtual llvm::GlobalVariable *GetSelectorByName(NSString *SelName,
													   NSString *TypeEncoding);
	virtual llvm::Value *GetSelector(CGBuilder &Builder,
	                                 llvm::Value *SelName,
	                                 llvm::Value *SelTypes);
	virtual llvm::Value *GetSelector(CGBuilder &Builder,
	                                 NSString *SelName,
	                                 NSString *SelTypes);

	virtual llvm::Function *MethodPreamble(const NSString *ClassName,
	                                       const NSString *CategoryName,
	                                       const NSString *MethodName,
	                                       LLVMType *ReturnTy,
	                                       LLVMType *SelfTy,
	                                       const SmallVectorImpl<LLVMType*> &ArgTy,
	                                       bool isClassMethod,
	                                       bool isSRet,
	                                       bool isVarArg);
	virtual llvm::Value *OffsetOfIvar(CGBuilder &Builder,
	                                  NSString *className,
	                                  NSString *ivarName,
	                                  int offsetGuess);
	
	virtual llvm::Value *callIMP(CGBuilder &Builder,
								 llvm::Value *imp,
								 NSString *typeEncoding,
								 llvm::Value *Receiver,
								 llvm::Value *Selector,
								 llvm::SmallVectorImpl<llvm::Value*> &ArgV,
								 llvm::BasicBlock *CleanupBlock,
								 llvm::MDNode *metadata);
	virtual void GenerateCategory(
		NSString *ClassName, NSString *CategoryName,
		StringVector  &InstanceMethodNames,
		StringVector  &InstanceMethodTypes,
		StringVector  &ClassMethodNames,
		StringVector  &ClassMethodTypes,
		StringVector &Protocols);
	virtual void DefineClassVariables(
		NSString *ClassName,
		StringVector &CvarNames,
		StringVector &CvarTypes);
	virtual void GenerateClass(
		NSString *ClassName,
		NSString *SuperClassName,
		const int instanceSize,
		StringVector &IvarNames,
		StringVector &IvarTypes,
		const llvm::SmallVectorImpl<int> &IvarOffsets,
		StringVector &InstanceMethodNames,
		StringVector &InstanceMethodTypes,
		StringVector &ClassMethodNames,
		StringVector &ClassMethodTypes,
		StringVector &Protocols);
	virtual llvm::Value *GenerateProtocolRef(CGBuilder &Builder,
	                                         NSString *ProtocolName);
	virtual void GenerateProtocol(
		NSString *ProtocolName,
		StringVector &Protocols,
		const llvm::SmallVectorImpl<llvm::Constant *> &InstanceMethodNames,
		const llvm::SmallVectorImpl<llvm::Constant *> &InstanceMethodTypes,
		const llvm::SmallVectorImpl<llvm::Constant *> &ClassMethodNames,
		const llvm::SmallVectorImpl<llvm::Constant *> &ClassMethodTypes);
	virtual llvm::Function *ModuleInitFunction();
	virtual llvm::Value *AddressOfClassVariable(CGBuilder &Builder, 
	                                            NSString *ClassName,
	                                            NSString *CvarName);
	
	llvm::GlobalVariable *MakeGlobalArray(llvm::Type *Ty,
										  ArrayRef<llvm::Constant *> V,
										  StringRef Name="",
										  llvm::GlobalValue::LinkageTypes linkage
										  =llvm::GlobalValue::InternalLinkage) {
		llvm::ArrayType *ArrayTy = llvm::ArrayType::get(Ty, V.size());
		return MakeGlobal(ArrayTy, V, Name, linkage);
	}
	
	llvm::GlobalVariable *MakeGlobal(llvm::ArrayType *Ty,
									 ArrayRef<llvm::Constant *> V,
									 StringRef Name="",
									 llvm::GlobalValue::LinkageTypes linkage
									 =llvm::GlobalValue::InternalLinkage) {
		llvm::Constant *C = llvm::ConstantArray::get(Ty, V);
		return new llvm::GlobalVariable(TheModule, Ty, false,
										linkage, C, Name);
	}
};
} // end anonymous namespace


static std::string SymbolNameForSelector(const NSString *MethodName)
{
	string MethodNameColonStripped([MethodName UTF8String], [MethodName length]);
	std::replace(MethodNameColonStripped.begin(), MethodNameColonStripped.end(), ':', '_');
	return MethodNameColonStripped;
}

static std::string SymbolNameForMethod(const NSString *ClassName, 
	const NSString *CategoryName, const NSString *MethodName, bool isClassMethod)
{
	return std::string(isClassMethod ? "_c_" : "_i_") + [ClassName UTF8String]
		+ "_" + [CategoryName UTF8String]+ "_" +
		SymbolNameForSelector(MethodName);
}

CGObjCGNU::CGObjCGNU(CodeGenTypes *cgTypes,
                     llvm::Module &M,
                     llvm::LLVMContext &C,
                     bool enableGC,
                     bool isJit) :
                      CGObjCRuntime(cgTypes, C),
                      TheModule(M)
{
	IntTy = types->intTy;
	LongTy = types->longTy;
	GC = enableGC;
	JIT = isJit;
	if (GC)
	{
		RuntimeVersion = 10;
	}
	Zeros[0] = ConstantInt::get(LLVMType::getInt32Ty(Context), 0);
	Zeros[1] = Zeros[0];

	msgSendMDKind = Context.getMDKindID("GNUObjCMessageSend");

	NULLPtr = llvm::ConstantPointerNull::get(
		llvm::PointerType::getUnqual(LLVMType::getInt8Ty(Context)));
	// C string type.  Used in lots of places.
	PtrToInt8Ty = 
		llvm::PointerType::getUnqual(LLVMType::getInt8Ty(Context));
	// Get the selector Type.
	SelectorTy = LLVMType::getInt16Ty(Context);
	SelStructTy = GetStructType(
		  Context,
		  PtrToInt8Ty,
		  SelectorTy,
		  (void *)0);
	PtrToIntTy = llvm::PointerType::getUnqual(IntTy);
	PtrTy = PtrToInt8Ty;
 
	// Object type
	IdTy = PtrTy;
	// IMP type
	std::vector<LLVMType*> IMPArgs;
	IMPArgs.push_back(IdTy);
	IMPArgs.push_back(SelectorTy);
	IMPTy = llvm::FunctionType::get(IdTy, IMPArgs, true);
	
	// TODO
	SizeTy = LongTy;
	
	IntegerBit = llvm::Type::getInt1Ty(Context);
	FlagsStructTy = llvm::StructType::get(
															IntegerBit, // meta
															IntegerBit, // resolved
															IntegerBit, // initialized
															IntegerBit, // user_created
															IntegerBit, // has_custom_arr
															IntegerBit, // fake
															NULL
															);
	
	ClassTy = llvm::StructType::get(
											  IdTy,						// class_pointer
											  IdTy,					// super_class
											  PtrToInt8Ty,			// dtable
											  FlagsStructTy,			// flags
											  PtrTy,				// methods
											  PtrToInt8Ty,            // name
											  PtrTy,					// ivar_offsets
											  PtrTy,					// ivars
											  PtrTy,                  // protocols
											  PtrTy,                  // properties (not used by LK yet)
											  PtrTy,                  // subclass_list
											  PtrTy,                  // sibling_class
											  PtrTy,                  // unresolved_previous
											  PtrTy,                  // unresolved_next
											  PtrTy,                  // extra_space
											  PtrTy,                  // kernel_module
											  SizeTy,				 // Instance size
											  IntTy,                 // version
											  NULL);

}

// This has to perform the lookup every time, since posing and related
// techniques can modify the name -> class mapping.
llvm::Constant *CGObjCGNU::LookupClass(NSString *ClassName, bool isMeta)
{
	NSString *symbolName = [(isMeta ? @"_OBJC_METACLASS_": @"_OBJC_CLASS_") stringByAppendingString:ClassName];
	llvm::Constant *Class = TheModule.getGlobalVariable([symbolName UTF8String]);
	if (Class == NULL){
		Class = new llvm::GlobalVariable(TheModule,
										 ClassTy,
										 false,
										 llvm::GlobalValue::ExternalLinkage,
										 0,
										 [symbolName UTF8String]);
	}
	
	return llvm::ConstantExpr::getBitCast(Class, IdTy);
}

llvm::GlobalVariable *CGObjCGNU::GetSelectorByName(NSString *SelName,
															NSString *SelTypes){
	if (SelTypes == nil || [SelTypes isEqualToString:@""]){
		llvm_unreachable("No untyped selectors support in kernel runtime.");
	}
	
	// For static selectors, we return an alias for now then store them all in
	// a list that the runtime will initialise later.
	
	SmallVector<TypedSelector, 2> &Types = SelectorTable[SelName];
	llvm::GlobalVariable *SelValue = 0;
	
	// Check if we've already cached this selector
	for (SmallVectorImpl<TypedSelector>::iterator i = Types.begin(),
	     e = Types.end() ; i!=e ; i++)
	{
		if (i->first == SelTypes || [i->first isEqualToString: SelTypes])
		{
			if (i != Types.begin()){
				// This really means that there are two selectors with different types
				[NSException raise: @"LKCodeGenException"
							format: @"Trying to register selector '%@' for the second time with different type. Initial types: %@ Types now: %@",
				 SelName, Types.begin()->first, i->first];
			}
			return i->second;
		}
	}
	
	// If not, create a new one.
	SelValue = new llvm::GlobalVariable(TheModule,
										SelectorTy,
										false, // Const
										llvm::GlobalValue::LinkerPrivateLinkage, // Link
										llvm::ConstantInt::get(SelectorTy, 0), // Init
										string(".objc_selector_")+[SelName UTF8String],
										NULL,
										llvm::GlobalVariable::NotThreadLocal,
										0,
										true);
	Types.push_back(TypedSelector(SelTypes, SelValue));
	
	return SelValue;
}

/// Statically looks up the selector for the specified name / type pair.
llvm::Value *CGObjCGNU::GetSelector(CGBuilder &Builder,
                                    NSString *SelName,
                                    NSString *SelTypes)
{
	if (SelTypes == nil || [SelTypes isEqualToString:@""]){
		/* If not types, fall back to the RT function. */
		llvm::Value *ConstSelName = MakeConstantString(SelName);
		llvm::Constant *SelectorLookupFn =
		TheModule.getOrInsertFunction("sel_getNamed", SelectorTy, PtrToInt8Ty,
									  (void *)0);
		return Builder.CreateCall(SelectorLookupFn, ConstSelName);
	}

	llvm::GlobalVariable *SelValue = GetSelectorByName(SelName, SelTypes);
	
	// return SelValue;
	return Builder.CreateLoad(Builder.CreateGEP(SelValue, Zeros[0]));
}
/// Dynamically looks up the selector for the specified name / type pair.
llvm::Value *CGObjCGNU::GetSelector(CGBuilder &Builder,
                                    llvm::Value *SelName,
                                    llvm::Value *SelTypes)
{
// Dead code?
assert(0);
	// Dynamically look up selectors from non-constant sources
	llvm::Value *cmd;
	if (SelTypes == 0)
	{
		llvm::Constant *SelFunction =
			TheModule.getOrInsertFunction("sel_register_name",
				SelectorTy,
				PtrToInt8Ty,
				(void *)0);
		cmd = Builder.CreateCall(SelFunction, SelName);
	}
	else
	{
		llvm::Constant *SelFunction =
			TheModule.getOrInsertFunction("sel_register_typed_name",
					SelectorTy,
					PtrToInt8Ty,
					PtrToInt8Ty,
					(void *)0);
		cmd = Builder.CreateCall2(SelFunction, SelName, SelTypes);
	}
	return cmd;
}

llvm::Constant *CGObjCGNU::MakeConstantString(NSString *Str,
                                              const std::string &Name)
{
	std::string str([Str UTF8String], [Str length]);
#if (LLVM_MAJOR > 3) || ((LLVM_MAJOR == 3) && LLVM_MINOR > 0)
	llvm::Constant *ConstStr = llvm::ConstantDataArray::getString(Context,
		str, true);
#else
	llvm::Constant * ConstStr = llvm::ConstantArray::get(Context, str);
#endif
	ConstStr = new llvm::GlobalVariable(TheModule, ConstStr->getType(), true,
		llvm::GlobalValue::InternalLinkage, ConstStr, Name);
	return llvm::ConstantExpr::getGetElementPtr(ConstStr, Zeros, 2);
}

llvm::Constant *CGObjCGNU::MakeGlobal(LLVMStructTy *Ty,
                                      std::vector<llvm::Constant*> &V,
                                      const std::string &Name,
                                      bool isPublic)
{
	llvm::Constant *C = llvm::ConstantStruct::get(Ty, V);
	return new llvm::GlobalVariable(TheModule, Ty, false,
		(isPublic ? llvm::GlobalValue::ExternalLinkage :
		llvm::GlobalValue::InternalLinkage), C, Name);
}

llvm::Constant *CGObjCGNU::MakeGlobal(LLVMArrayTy *Ty,
                                      std::vector<llvm::Constant*> &V,
                                      const std::string &Name) 
{
	llvm::Constant *C = llvm::ConstantArray::get(Ty, V);
	return new llvm::GlobalVariable(TheModule, Ty, false,
		llvm::GlobalValue::InternalLinkage, C, Name);
}
llvm::Value *CGObjCGNU::GetWeakSymbol(const std::string &Name,
                                               LLVMType *Type)
{
	llvm::Constant *value;
	if(0 != (value = TheModule.getNamedGlobal(Name)))
	{
		if (value->getType() != Type)
		{
			value = llvm::ConstantExpr::getBitCast(value, Type);
		}
		return value;
	}
	return new llvm::GlobalVariable(TheModule, Type, false,
			llvm::GlobalValue::WeakAnyLinkage, 0, Name);
}

/// Generate an NSConstantString object.
llvm::Constant *CGObjCGNU::GenerateConstantString(NSString *String)
{
	// In JIT mode, just reuse the string.
	if (JIT)
	{
		// Copy instead of retaining, in case we got passed a mutable string.
		return llvm::ConstantExpr::getIntToPtr(llvm::ConstantInt::get(LongTy, (uintptr_t)(__bridge_retained void*)[String copy]),
			IdTy);
	}

	NSUInteger length = [String length];
	std::vector<llvm::Constant*> Ivars;
	
	llvm::Constant *Class = LookupClass(@"_KKConstString", false);
	llvm::Constant *ConstString = MakeConstantString(String);
	
	Ivars.push_back(Class); // isa
	Ivars.push_back(ConstantInt::get(IntTy, 0)); // retain count
	Ivars.push_back(ConstString); // str
	Ivars.push_back(ConstantInt::get(IntTy, length)); // len
	llvm::Constant *ObjCStr = MakeGlobal(
		GetStructType(Context, IdTy, IntTy, PtrToInt8Ty, IntTy,
		(void *)0), Ivars, ".objc_str");
	printf("Castring string : "); ObjCStr->getType()->dump();
	ObjCStr = llvm::ConstantExpr::getBitCast(ObjCStr, IdTy); // It's an object
	printf(" to "); ObjCStr->getType()->dump(); printf("\n");
	ConstantStrings.push_back(ObjCStr);
	return ObjCStr;
}
llvm::Value *CGObjCGNU::callIMP(
                            CGBuilder &Builder,
                            llvm::Value *imp,
                            NSString *typeEncoding,
                            llvm::Value *Receiver,
                            llvm::Value *Selector,
                            llvm::SmallVectorImpl<llvm::Value*> &ArgV,
                            llvm::BasicBlock *CleanupBlock,
                            llvm::MDNode *metadata)
{
	bool isSRet;
	LLVMType *ReturnTy;
	llvm::FunctionType *fTy = types->functionTypeFromString(typeEncoding, isSRet, ReturnTy);
	llvm::AttrListPtr attributes = types->AI->attributeListForFunctionType(fTy, ReturnTy);
	
	imp = Builder.CreateBitCast(imp, llvm::PointerType::getUnqual(fTy));
	printf("\nIMP Type\n");
	imp->getType()->dump();
	printf("\nDone IMP Type\n");
	

	// Call the method
	llvm::SmallVector<llvm::Value*, 8> callArgs;
	llvm::Value *sret = 0;
	unsigned param = 0;
	if (isSRet)
	{
		sret = Builder.CreateAlloca(ReturnTy);
		callArgs.push_back(sret);
		param++;
	}
	
	printf("Args: \n");
	
	callArgs.push_back(Receiver);
	Receiver->getType()->dump();
	callArgs.push_back(Selector);
	Selector->getType()->dump();
	llvm::Value* callArg;
	for (unsigned int i = 0; i < ArgV.size() ; i++) {
		callArg = ArgV[i];
		callArg->getType()->dump();
		if (types->AI->willPassTypeAsPointer(callArg->getType()))
		{
			llvm::AllocaInst *StructPointer =
				Builder.CreateAlloca(callArg->getType());
			Builder.CreateStore(callArg, StructPointer);
			callArgs.push_back(StructPointer);
		}
		else
		{
			callArgs.push_back(callArg);
		}
	}
	
	printf("\nChecking params vs function params\n");
	for (unsigned int i=0 ; i<fTy->getNumParams() ; i++)
	{
		LLVMType *argTy = fTy->getParamType(i);
		
		callArgs[i]->getType()->dump(); argTy->dump();
		if (callArgs[i]->getType() != argTy)
		{
			callArgs[i] = Builder.CreateBitCast(callArgs[i], argTy);
		}
	}
	printf("\nDone Checking params vs function params\n");
	
	
	/// Setjmp buffer type is an array of this size
	uint64_t SetJmpBufferSize;
	/// Type of integers that are used in the buffer type
	llvm::Type *SetJmpBufferIntTy;
	
	llvm::IntegerType *Int8Ty = llvm::Type::getInt8Ty(Context);
	llvm::PointerType *Int8PtrTy = Int8Ty->getPointerTo();
	
	/// A structure defining the exception data type
	llvm::StructType *ExceptionDataTy = NULL;
	if (llvm::Module::Pointer64){
		SetJmpBufferSize = 12;
		SetJmpBufferIntTy = types->longTy;
	}else if (llvm::Module::Pointer32){
		SetJmpBufferSize = (18);
		SetJmpBufferIntTy = types->intTy;
	}else{
		printf("Unknown target and hence unknown setjmp buffer size.\n");
		abort();
	}
	
	// Exceptions
	llvm::Type *SetJmpType = llvm::ArrayType::get(SetJmpBufferIntTy,
												  SetJmpBufferSize);
	
	ExceptionDataTy =
	llvm::StructType::create("struct._objc_exception_data",
							 SetJmpType,
							 Int8PtrTy, // Next
							 Int8PtrTy, // Exception object
							 Int8PtrTy, // Reserved 1
							 Int8PtrTy, // Reserved 2
							 NULL);
	
	
	// Allocate memory for the setjmp buffer.  This needs to be kept
	// live throughout the try and catch blocks.
	llvm::Value *ExceptionData = Builder.CreateAlloca(ExceptionDataTy, NULL,
													  "exceptiondata.ptr");
	
	llvm::Type *ExceptionDataPointerTy = ExceptionDataTy->getPointerTo();
	Function *ExceptionTryEnterFn = cast<Function>(
												   TheModule.getOrInsertFunction("objc_exception_try_enter",
																				  Type::getVoidTy(Context), ExceptionDataPointerTy, (void *)0));
	
	// Enter a try block:
	//  - Call objc_exception_try_enter to push ExceptionData on top of
	//    the EH stack.
	Builder.CreateCall(ExceptionTryEnterFn, ExceptionData);
	
	llvm::Value *ret = Builder.CreateAlloca(ReturnTy);
	
	
	//  - Call setjmp on the exception data buffer.
	llvm::Constant *Zero = llvm::ConstantInt::get(types->intTy, 0);
	llvm::Value *GEPIndexes[] = { Zero, Zero, Zero };
	llvm::Value *SetJmpBuffer =
	Builder.CreateGEP(ExceptionData, GEPIndexes, "setjmp_buffer");
	
	Function *SetJmpFn = cast<Function>(
										TheModule.getOrInsertFunction("setjmp",
																	   Type::getInt32Ty(Context), SetJmpBufferIntTy->getPointerTo(), (void *)0));
	
	llvm::CallInst *SetJmpResult = Builder.CreateCall(SetJmpFn, SetJmpBuffer, "setjmp_result");
	SetJmpResult->setCanReturnTwice();
	
	// If setjmp returned 0, enter the protected block; otherwise,
	// branch to the handler.
	llvm::BasicBlock *ExcBB = BasicBlock::Create(Context, "exc.handler");
	llvm::BasicBlock *TryBB = BasicBlock::Create(Context, "try.handler");
	llvm::Value *DidCatch =
	Builder.CreateIsNotNull(SetJmpResult, "did_catch_exception");
	Builder.CreateCondBr(DidCatch, ExcBB, TryBB);
	
	
	// Try BB
	CGBuilder TryBuilder(TryBB);
	
	llvm::CallInst *call = TryBuilder.CreateCall(imp, callArgs, "imp.invoke");
	if (0 != metadata){
		call->setMetadata(msgSendMDKind, metadata);
	}
	
	TryBuilder.CreateStore(call, ret);
	
	Function *ExceptionTryExitFn = cast<Function>(
												   TheModule.getOrInsertFunction("objc_exception_try_exit",
																				  Type::getVoidTy(Context), ExceptionDataPointerTy, (void *)0));
	
	TryBuilder.CreateCall(ExceptionTryExitFn, ExceptionData);
	
	// Catch BB
	CGBuilder ExceptionBuilder(ExcBB);
	
	llvm::Constant *Two = llvm::ConstantInt::get(IntTy, 2);
	llvm::Value *ExcGEPIndexes[] = { Zero, Two };
	llvm::Value *ExceptionResult = ExceptionBuilder.CreateGEP(ExceptionData, ExcGEPIndexes, "exc_obj");
	
	
	if (ReturnTy != Type::getVoidTy(Context))
	{
		ExceptionResult = ExceptionBuilder.CreateBitCast(ExceptionBuilder.CreateLoad(ExceptionResult), ReturnTy);
		ExceptionResult->getType()->dump();
		
		ExceptionBuilder.CreateStore(ExceptionResult, ExceptionBuilder.CreateGEP(ret, Zeros[0]));
		
		
		if (isSRet)
		{
			ret = Builder.CreateLoad(sret);
		}
		if (ret->getType() != ReturnTy)
		{
			if (ret->getType()->canLosslesslyBitCastTo(ReturnTy))
			{
				printf("Bitcast1\n");
				ret->getType()->dump(); printf("  "); ReturnTy->dump(); printf("\n");
				ret = Builder.CreateBitCast(ret, ReturnTy);
				printf("After bitcast\n");
			}
			else
			{
				llvm::Value *tmp = Builder.CreateAlloca(ReturnTy);
				printf("Bitcast2");
				llvm::Value *storePtr =
				Builder.CreateBitCast(tmp, llvm::PointerType::getUnqual(ret->getType()));
				Builder.CreateStore(ret, storePtr);
				ret = Builder.CreateLoad(tmp);
			}
		}
	}
	
	return ret;
}

///Generates a message send where the super is the receiver.  This is a message
///send to self with special delivery semantics indicating which class's method
///should be called.
llvm::Value *CGObjCGNU::GenerateMessageSendSuper(CGBuilder &Builder,
                                            llvm::Value *Sender,
                                            NSString *SuperClassName,
                                            llvm::Value *Receiver,
                                            NSString *selName,
                                            NSString *selTypes,
                                            llvm::SmallVectorImpl<llvm::Value*> &ArgV,
                                            bool isClassMessage,
                                            llvm::BasicBlock *CleanupBlock)
{
	llvm::Value *Selector = GetSelector(Builder, selName, selTypes);
	
	llvm::Value *ReceiverClass = LookupClass(SuperClassName, false);
	// If it's a class message, get the metaclass
	if (isClassMessage)
	{
		ReceiverClass = Builder.CreateBitCast(ReceiverClass,
				PointerType::getUnqual(IdTy));
		ReceiverClass = Builder.CreateLoad(ReceiverClass);
	}
	// Construct the structure used to look up the IMP
	llvm::StructType *ObjCSuperTy = GetStructType(Context, 
		Receiver->getType(), IdTy, (void *)0);
	llvm::Value *ObjCSuper = Builder.CreateAlloca(ObjCSuperTy);
	Builder.CreateStore(Receiver, Builder.CreateStructGEP(ObjCSuper, 0));
	Builder.CreateStore(ReceiverClass, Builder.CreateStructGEP(ObjCSuper, 1));

	std::vector<LLVMType*> Params;
	Params.push_back(llvm::PointerType::getUnqual(ObjCSuperTy));
	Params.push_back(SelectorTy);


	// The lookup function returns a slot, which can be safely cached.
	LLVMType *SlotTy = GetStructType(Context, PtrTy, PtrTy, PtrTy, PtrTy,
		IntTy, SelectorTy, (void *)0);

	llvm::Constant *lookupFunction =
	TheModule.getOrInsertFunction("objc_slot_lookup_super",
			llvm::FunctionType::get( llvm::PointerType::getUnqual(SlotTy),
				Params, true));

	llvm::SmallVector<llvm::Value*, 2> lookupArgs;
	lookupArgs.push_back(ObjCSuper);
	lookupArgs.push_back(Selector);
	llvm::CallInst *slot = IRBuilderCreateCall(&Builder, lookupFunction, lookupArgs);
	slot->setOnlyReadsMemory();

	llvm::Value *imp = Builder.CreateLoad(Builder.CreateStructGEP(slot, 2));

	llvm::Value *impMD[] = {
		llvm::MDString::get(Context, [selName UTF8String]), 
		llvm::MDString::get(Context, [SuperClassName UTF8String]),
		llvm::ConstantInt::get(LLVMType::getInt1Ty(Context), isClassMessage)
		};
	llvm::MDNode *node = CreateMDNode(Context, impMD, 3);

	return callIMP(Builder, imp, selTypes, Receiver,
			Selector, ArgV, CleanupBlock, node);
}
void CGObjCGNU::lookupIMPAndTypes(CGBuilder &Builder,
                                  llvm::Value *Sender,
                                  llvm::Value *&Receiver,
                                  NSString *selName,
                                  llvm::Value *&imp,
                                  llvm::Value *&typeEncoding)
{
	llvm::Value *Selector = GetSelector(Builder, selName, 0);
	
	if (0 == Sender)
	{
		Sender = NULLPtr;
	}
	llvm::Value *ReceiverPtr = Builder.CreateAlloca(Receiver->getType());
	Builder.CreateStore(Receiver, ReceiverPtr, true);

	LLVMType *SlotTy = GetStructType(Context, PtrTy, PtrTy, PtrTy, PtrTy,
			IntTy, SelectorTy, (void *)0);

	llvm::Constant *lookupFunction = 
		TheModule.getOrInsertFunction("objc_msg_lookup_sender",
				llvm::PointerType::getUnqual(SlotTy), ReceiverPtr->getType(),
				Selector->getType(), Sender->getType(), (void *)0);
	// Lookup does not capture the receiver pointer
	if (llvm::Function *LookupFn = dyn_cast<llvm::Function>(lookupFunction)) 
	{
		LookupFn->setDoesNotCapture(1);
	}

	llvm::CallInst *slot = 
		Builder.CreateCall3(lookupFunction, ReceiverPtr, Selector, Sender);

	slot->setOnlyReadsMemory();

	imp = Builder.CreateLoad(Builder.CreateStructGEP(slot, 2));
	typeEncoding = Builder.CreateLoad(Builder.CreateStructGEP(slot, 3));
	Receiver = Builder.CreateLoad(ReceiverPtr, true);
}

/// Generate code for a message send expression.
llvm::Value *CGObjCGNU::GenerateMessageSend(CGBuilder &Builder,
                                            llvm::Value *Sender,
                                            llvm::Value *Receiver,
                                            NSString *selName,
                                            NSString *selTypes,
                                            llvm::SmallVectorImpl<llvm::Value*> &ArgV,
                                            llvm::BasicBlock *CleanupBlock,
                                            NSString *ReceiverClass,
                                            bool isClassMessage)
{
if (isClassMessage) { NSLog(@"Sending class message [%@ %@]", ReceiverClass, selName); }
	llvm::Value *Selector = GetSelector(Builder, selName, selTypes);
	
	char ret = [selTypes characterAtIndex: 0];
	const char *msgFuncName = "objc_msgSend";
	switch (ret)
	{
		case 'f': case 'd': case 'D':
			msgFuncName = "objc_msgSend_fpret";
	}
	bool isSRet;
	LLVMType *ReturnTy;
	llvm::FunctionType *fTy = types->functionTypeFromString(selTypes, isSRet, ReturnTy);
	llvm::AttrListPtr attributes = types->AI->attributeListForFunctionType(fTy, ReturnTy);
	if (isSRet)
	{
			msgFuncName = "objc_msgSend_stret";
	}
	llvm::Value *imp = TheModule.getOrInsertFunction(msgFuncName, fTy, attributes);
	llvm::Value *impMD[] = {
		llvm::MDString::get(Context, [selName UTF8String]),
		llvm::MDString::get(Context, [ReceiverClass UTF8String]),
		llvm::ConstantInt::get(LLVMType::getInt1Ty(Context), isClassMessage)
		};
	llvm::MDNode *node = CreateMDNode(Context, impMD, 3);

	// Call the method.
	return callIMP(Builder, imp, selTypes, Receiver, Selector,
				ArgV, CleanupBlock, node);
}

/// Generates a MethodList.  Used in construction of a objc_class and 
/// objc_category structures.
llvm::Constant *CGObjCGNU::GenerateMethodList(
	NSString *ClassName,
	NSString *CategoryName, 
	StringVector  &MethodNames, 
	StringVector  &MethodTypes, 
	bool isClassMethodList) 
{
	// Get the method structure type.  
	llvm::StructType *ObjCMethodTy = GetStructType(
		Context,
		llvm::PointerType::getUnqual(IMPTy), // IMP
		PtrToInt8Ty, // Method Name
		PtrToInt8Ty, // Method types
		SelectorTy, // Selector
		IntTy, // Version
		(void *)0);
	
	std::vector<llvm::Constant*> Methods;
	std::vector<llvm::Constant*> Elements;
	for (unsigned int i = 0, e = MethodTypes.size(); i < e; ++i) 
	{
		Elements.clear();
		
		llvm::Constant *Method =
		TheModule.getFunction(SymbolNameForMethod(ClassName, CategoryName,
												  MethodNames[i], isClassMethodList));
		Method = llvm::ConstantExpr::getBitCast(Method,
												llvm::PointerType::getUnqual(IMPTy));
		Elements.push_back(Method);
		
		Elements.push_back(MakeConstantString(MethodNames[i]));
		Elements.push_back(MakeConstantString(MethodTypes[i]));
		
		Elements.push_back(llvm::ConstantInt::get(SelectorTy, 0));
		Elements.push_back(llvm::ConstantInt::get(IntTy, 0));
		
		Methods.push_back(llvm::ConstantStruct::get(ObjCMethodTy, Elements));
	}

	// Array of method structures
	llvm::ArrayType *ObjCMethodArrayTy = llvm::ArrayType::get(ObjCMethodTy,
			MethodNames.size());
	llvm::Constant *MethodArray = llvm::ConstantArray::get(ObjCMethodArrayTy,
			Methods);

	// Structure containing list pointer, array and array count
	llvm::SmallVector<LLVMType*, 16> ObjCMethodListFields;
	LLVMType *OpaqueNextTy = PtrTy;
	LLVMType *NextPtrTy = llvm::PointerType::getUnqual(OpaqueNextTy);
	llvm::StructType *ObjCMethodListTy = GetStructType(
		Context,
		NextPtrTy,
		IntTy, // Size
	    LLVMType::getInt8Ty(Context), // Bool indicating whether it is dynamically allocated
		ObjCMethodArrayTy, // List of methods
		(void *)0);

	Methods.clear();
	Methods.push_back(llvm::Constant::getNullValue(NextPtrTy));
	Methods.push_back(ConstantInt::get(LLVMType::getInt32Ty(Context),
		MethodTypes.size()));
	Methods.push_back(ConstantInt::get(LLVMType::getInt8Ty(Context), 0));
	Methods.push_back(MethodArray);
	
	// Create an instance of the structure
	return MakeGlobal(ObjCMethodListTy, Methods, ".objc_method_list");
}

/// Generates an IvarList.  Used in construction of a objc_class.
llvm::Constant *CGObjCGNU::GenerateIvarList(
	NSString *ClassName,
	StringVector  &IvarNames, const
	llvm::SmallVectorImpl<NSString*>  &IvarTypes, const
	llvm::SmallVectorImpl<int>  &IvarOffsets, 
	llvm::Constant *&IvarOffsetArray)
{
	if (0 == IvarNames.size())
	{
		return NULLPtr;
	}
	// Get the method structure type.  
	llvm::StructType *ObjCIvarTy = GetStructType(
		Context,
		PtrToInt8Ty, // Name
		PtrToInt8Ty, // Type
		SizeTy, // Size
		IntTy, // Offset
		LLVMType::getInt8Ty(Context), // Align
		(void *)0);
	std::vector<llvm::Constant*> Ivars;
	std::vector<llvm::Constant*> Elements;

	for (unsigned int i = 0, e = IvarNames.size() ; i < e ; i++)
 	{
		Elements.clear();
		Elements.push_back(MakeConstantString(IvarNames[i]));
		Elements.push_back(MakeConstantString(IvarTypes[i]));
		Elements.push_back(ConstantInt::get(SizeTy, 0));
		Elements.push_back(ConstantInt::get(IntTy, IvarOffsets[i]));
		Elements.push_back(ConstantInt::get(LLVMType::getInt8Ty(Context), 0));
		Ivars.push_back(llvm::ConstantStruct::get(ObjCIvarTy, Elements));
	}

	// Array of ivar structures
	llvm::ArrayType *ObjCIvarArrayTy = llvm::ArrayType::get(ObjCIvarTy,
		IvarNames.size());
	
	Elements.clear();
	Elements.push_back(ConstantInt::get(
		 llvm::cast<llvm::IntegerType>(IntTy), (int)IvarNames.size()));
	Elements.push_back(llvm::ConstantArray::get(ObjCIvarArrayTy, Ivars));
	// Structure containing array and array count
	llvm::StructType *ObjCIvarListTy = GetStructType(Context, IntTy,
		ObjCIvarArrayTy,
		(void *)0);

	// Create an instance of the structure
	llvm::Constant *IvarList =
		MakeGlobal(ObjCIvarListTy, Elements, ".objc_ivar_list");

	// Generate the non-fragile ABI offset variables.
	LLVMType *IndexTy = SizeTy;
	llvm::Constant *offsetPointerIndexes[] = {Zeros[0],
		llvm::ConstantInt::get(IndexTy, 1), 0,
		llvm::ConstantInt::get(IndexTy, 2) };

	std::vector<llvm::Constant*> IvarOffsetValues;
	const char *className = [ClassName UTF8String];

	for (unsigned int i = 0, e = IvarNames.size() ; i < e ; i++)
	{
		const std::string Name = std::string("__objc_ivar_offset_") + className
			+ '.' + [IvarNames[i] UTF8String];
		offsetPointerIndexes[2] = llvm::ConstantInt::get(IndexTy, i);
		// Get the correct ivar field
		llvm::Constant *offsetValue = llvm::ConstantExpr::getGetElementPtr(
				IvarList, offsetPointerIndexes, 4);
		// Get the existing alias, if one exists.
		llvm::GlobalVariable *offset = TheModule.getNamedGlobal(Name);
		if (offset)
		{
			offset->setInitializer(offsetValue);
			// If this is the real definition, change its linkage type so that
			// different modules will use this one, rather than their private
			// copy.
			offset->setLinkage(llvm::GlobalValue::ExternalLinkage);
		}
		else 
		{
			// Add a new alias if there isn't one already.
			offset = new llvm::GlobalVariable(TheModule, offsetValue->getType(),
			false, llvm::GlobalValue::ExternalLinkage, offsetValue, Name);
		}

		IvarOffsetValues.push_back(new llvm::GlobalVariable(TheModule, IntTy,
					false, llvm::GlobalValue::ExternalLinkage,
					llvm::ConstantInt::get(SizeTy, IvarOffsets[i]),
					std::string("__objc_ivar_offset_value_") + className +'.' +
					[IvarNames[i] UTF8String]));
	}

	if (IvarOffsetArray)
	{
		llvm::Constant *IvarOffsetArrayInit =
			llvm::ConstantArray::get(llvm::ArrayType::get(PtrToIntTy,
						IvarOffsetValues.size()), IvarOffsetValues);
		IvarOffsetArray = new llvm::GlobalVariable(TheModule,
				IvarOffsetArrayInit->getType(), false,
				llvm::GlobalValue::InternalLinkage, IvarOffsetArrayInit,
				".ivar.offsets");
	}

	return IvarList;
}

/// Generate a class structure
llvm::Constant *CGObjCGNU::GenerateClassStructure(
	llvm::Constant *MetaClass,
	llvm::Constant *SuperClass,
	unsigned info,
	NSString *Name,
	llvm::Constant *Version,
	llvm::Constant *InstanceSize,
	llvm::Constant *IVars,
	llvm::Constant *Methods,
	llvm::Constant *Protocols,
	llvm::Constant *IvarOffsets,
	bool isMeta)
{
	llvm::Constant *Zero = ConstantInt::get(LongTy, 0);
	llvm::Constant *NullP =
		llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(PtrTy));
	// Fill in the structure
	std::vector<llvm::Constant*> Elements;
	Elements.push_back(llvm::ConstantExpr::getBitCast(MetaClass, IdTy));
	Elements.push_back(SuperClass);
	Elements.push_back(NullP); // dtable
	
	// .flags
	std::vector<llvm::Constant*> FlagElements;
	FlagElements.push_back(llvm::ConstantInt::get(IntegerBit, isMeta)); // meta
	FlagElements.push_back(llvm::ConstantInt::get(IntegerBit, 0)); // resolved
	FlagElements.push_back(llvm::ConstantInt::get(IntegerBit, 0)); // initialized
	FlagElements.push_back(llvm::ConstantInt::get(IntegerBit, 0)); // user_created
	FlagElements.push_back(llvm::ConstantInt::get(IntegerBit, 0)); // has_custom_arr
	FlagElements.push_back(llvm::ConstantInt::get(IntegerBit, 0)); // fake
	
	llvm::Constant *Flags = llvm::ConstantStruct::get(FlagsStructTy, FlagElements);
	Elements.push_back(Flags); // flags
	
	Elements.push_back(llvm::ConstantExpr::getBitCast(Methods, PtrTy));
	
	if (NULL == Name)
	{
		// TODO - random string - UUID?
		Name = @"AnonymousClass";
	}
	Elements.push_back(MakeConstantString(Name));
	
	Elements.push_back(llvm::ConstantExpr::getBitCast(IvarOffsets, PtrTy));
	Elements.push_back(llvm::ConstantExpr::getBitCast(IVars, PtrTy)); // ivars
	Elements.push_back(llvm::ConstantExpr::getBitCast(Protocols, PtrTy)); // Proto
	Elements.push_back(NullP); // properties
	Elements.push_back(NullP); // subclass_list
	Elements.push_back(NullP); // sibling_list
	Elements.push_back(NullP); // unresolved_previous
	Elements.push_back(NullP); // unresolved_next
	Elements.push_back(NullP); // extra_space
	Elements.push_back(NullP); // kernel_module
	Elements.push_back(InstanceSize); // instance_size
	Elements.push_back(llvm::ConstantInt::get(IntTy, 0)); // version
	
	// Create an instance of the structure
	return MakeGlobal(ClassTy, Elements, 
			std::string(isMeta ? "_OBJC_METACLASS_": "_OBJC_CLASS_") + [Name UTF8String],
			true);
}

llvm::Constant *CGObjCGNU::GenerateProtocolMethodList(
	const llvm::SmallVectorImpl<llvm::Constant *> &MethodNames,
	const llvm::SmallVectorImpl<llvm::Constant *> &MethodTypes) 
{
	// Get the method structure type.
	llvm::StructType *ObjCMethodDescTy = GetStructType(
		Context,
		PtrToInt8Ty, // Name
		PtrToInt8Ty, // Types
		SelectorTy, // Selector
		(void *)0);
	std::vector<llvm::Constant*> Methods;
	std::vector<llvm::Constant*> Elements;
	for (unsigned int i = 0, e = MethodTypes.size() ; i < e ; i++) {
		Elements.clear();
		Elements.push_back( llvm::ConstantExpr::getGetElementPtr(MethodNames[i],
					Zeros, 2)); 
		Elements.push_back(
					llvm::ConstantExpr::getGetElementPtr(MethodTypes[i], Zeros, 2));
		Elements.push_back(llvm::ConstantInt::get(SelectorTy, 0));
		Methods.push_back(llvm::ConstantStruct::get(ObjCMethodDescTy, Elements));
	}
	
	llvm::ArrayType *ObjCMethodArrayTy = llvm::ArrayType::get(ObjCMethodDescTy,
			MethodNames.size());
	llvm::Constant *Array = llvm::ConstantArray::get(ObjCMethodArrayTy, Methods);
	llvm::StructType *ObjCMethodDescListTy = GetStructType(Context,
														   SizeTy,
														   ObjCMethodArrayTy,
														   (void *)0);
	Methods.clear();
	Methods.push_back(ConstantInt::get(SizeTy, MethodNames.size()));
	Methods.push_back(Array);
	return MakeGlobal(ObjCMethodDescListTy, Methods, ".objc_method_list");
}

// Create the protocol list structure used in classes, categories and so on
llvm::Constant *CGObjCGNU::GenerateProtocolList(
	StringVector &Protocols)
{
	llvm::ArrayType *ProtocolArrayTy = llvm::ArrayType::get(PtrToInt8Ty,
		Protocols.size());
	llvm::StructType *ProtocolListTy = GetStructType(
		Context,
		PtrTy, //Should be a recurisve pointer, but it's always NULL here.
		SizeTy,
		ProtocolArrayTy,
		(void *)0);
	std::vector<llvm::Constant*> Elements; 
	for (StringVector::const_iterator iter=Protocols.begin(), endIter=Protocols.end();
	    iter != endIter ; iter++) 
	{
		llvm::Constant *Ptr = llvm::ConstantExpr::getBitCast(
				ExistingProtocols[*iter], PtrToInt8Ty);
		Elements.push_back(Ptr);
	}
	llvm::Constant * ProtocolArray = llvm::ConstantArray::get(ProtocolArrayTy,
		Elements);
	Elements.clear();
	Elements.push_back(NULLPtr);
	Elements.push_back(ConstantInt::get(SizeTy, Protocols.size()));
	Elements.push_back(ProtocolArray);
	return MakeGlobal(ProtocolListTy, Elements, ".objc_protocol_list");
}

llvm::Value *CGObjCGNU::GenerateProtocolRef(CGBuilder &Builder,
	NSString *ProtocolName) 
{
	return ExistingProtocols[ProtocolName];
}

void CGObjCGNU::GenerateProtocol(
	NSString *ProtocolName,
	StringVector &Protocols,
	const llvm::SmallVectorImpl<llvm::Constant *> &InstanceMethodNames,
	const llvm::SmallVectorImpl<llvm::Constant *> &InstanceMethodTypes,
	const llvm::SmallVectorImpl<llvm::Constant *> &ClassMethodNames,
	const llvm::SmallVectorImpl<llvm::Constant *> &ClassMethodTypes)
{
	llvm::Constant *ProtocolList = GenerateProtocolList(Protocols);
	llvm::Constant *InstanceMethodList =
		GenerateProtocolMethodList(InstanceMethodNames, InstanceMethodTypes);
	llvm::Constant *ClassMethodList =
		GenerateProtocolMethodList(ClassMethodNames, ClassMethodTypes);
	// Protocols are objects containing lists of the methods implemented and
	// protocols adopted.
	llvm::StructType *ProtocolTy = GetStructType(
		Context,
		IdTy, // isa
		IntTy, // retain_count (~ Protocol inherits from KKObject)
		PtrToInt8Ty, // name
		ProtocolList->getType(), // protocols
		InstanceMethodList->getType(), // instance methods
		ClassMethodList->getType(), // class methods
		PtrTy, // optional_instance
		PtrTy, // optional_class
		PtrTy, // properties
		PtrTy, // optional_properties
		PtrTy, // room for flags which we don't need to declare
		(void *)0);
	std::vector<llvm::Constant*> Elements; 
	// The isa pointer must be set to a magic number so the runtime knows it's
	// the correct layout.
	Elements.push_back(llvm::ConstantExpr::getIntToPtr(
		ConstantInt::get(LLVMType::getInt32Ty(Context), ProtocolVersion), IdTy));
	Elements.push_back(llvm::ConstantInt::get(IntTy, 0)); // retain_count
	Elements.push_back(MakeConstantString(ProtocolName, ".objc_protocol_name")); // name
	Elements.push_back(ProtocolList); // protocols
	Elements.push_back(InstanceMethodList); // instance methods
	Elements.push_back(ClassMethodList); // class methods
	
	Elements.push_back(NULLPtr); // optional_instance
	Elements.push_back(NULLPtr); // optional_class
	Elements.push_back(NULLPtr); // properties
	Elements.push_back(NULLPtr); // optional_properties
	Elements.push_back(NULLPtr); // room for flags
	
	ExistingProtocols[ProtocolName] = 
		llvm::ConstantExpr::getBitCast(MakeGlobal(ProtocolTy, Elements,
			".objc_protocol"), IdTy);
}

void CGObjCGNU::GenerateCategory(
	NSString *ClassName,
	NSString *CategoryName,
	StringVector &InstanceMethodNames,
	StringVector &InstanceMethodTypes,
	StringVector &ClassMethodNames,
	StringVector &ClassMethodTypes,
	StringVector &Protocols)
{
	std::vector<llvm::Constant*> Elements;
	Elements.push_back(MakeConstantString(CategoryName)); // Name
	Elements.push_back(MakeConstantString(ClassName)); // Class name
	// Instance method list 
	Elements.push_back(llvm::ConstantExpr::getBitCast(GenerateMethodList(
			ClassName, CategoryName, InstanceMethodNames, InstanceMethodTypes,
			false), PtrTy));
	// Class method list
	Elements.push_back(llvm::ConstantExpr::getBitCast(GenerateMethodList(
			ClassName, CategoryName, ClassMethodNames, ClassMethodTypes, true),
		  PtrTy));
	// Protocol list
	Elements.push_back(llvm::ConstantExpr::getBitCast(
		  GenerateProtocolList(Protocols), PtrTy));
	Categories.push_back(llvm::ConstantExpr::getBitCast(
		  MakeGlobal(GetStructType(Context, PtrToInt8Ty, PtrToInt8Ty, PtrTy,
			  PtrTy, PtrTy, (void *)0), Elements), PtrTy));
}
void CGObjCGNU::GenerateClass(
	NSString *ClassName,
	NSString *SuperClassName,
	const int instanceSize,
	StringVector  &IvarNames,
	StringVector  &IvarTypes,
	const llvm::SmallVectorImpl<int>  &IvarOffsets,
	StringVector  &InstanceMethodNames,
	StringVector  &InstanceMethodTypes,
	StringVector  &ClassMethodNames,
	StringVector  &ClassMethodTypes,
	StringVector &Protocols) 
{
	std::string classSymbolName = "__objc_class_name_";
	classSymbolName += [ClassName UTF8String];
	if (llvm::GlobalVariable *symbol =
	  TheModule.getGlobalVariable(classSymbolName))
	{
		symbol->setInitializer(llvm::ConstantInt::get(LongTy, 0));
	}
	else
	{
		new llvm::GlobalVariable(TheModule, LongTy, false,
			llvm::GlobalValue::ExternalLinkage, llvm::ConstantInt::get(LongTy, 0),
			classSymbolName);
	}

#if (LLVM_MAJOR > 3) || ((LLVM_MAJOR == 3) && LLVM_MINOR > 0)
	llvm::Constant * Name = llvm::ConstantDataArray::getString(Context, [ClassName UTF8String], true);
#else
	llvm::Constant * Name = llvm::ConstantArray::get(Context, [ClassName UTF8String]);
#endif
	Name = new llvm::GlobalVariable(TheModule, Name->getType(), true,
		llvm::GlobalValue::InternalLinkage, Name, ".class_name");
	
	// Empty vector used to construct empty method lists
	llvm::SmallVector<NSString*, 1> empty;
	llvm::SmallVector<int, 1> empty2;
	// Generate the method and instance variable lists
	llvm::Constant *MethodList = GenerateMethodList(ClassName, @"",
		InstanceMethodNames, InstanceMethodTypes, false);
	llvm::Constant *ClassMethodList = GenerateMethodList(ClassName, @"",
		ClassMethodNames, ClassMethodTypes, true);
	llvm::Constant *IvarOffsetValues=NULLPtr, *ignored;
	llvm::Constant *IvarList = GenerateIvarList(ClassName, IvarNames,
			IvarTypes, IvarOffsets, IvarOffsetValues);
	
	llvm::Constant *SuperClass = SuperClassName == nil ? NULLPtr : LookupClass(SuperClassName, false);
	llvm::Constant *SuperClassMeta = SuperClassName == nil ? NULLPtr : LookupClass(SuperClassName, true);
	
	//Generate metaclass for class methods
	llvm::Constant *MetaClassStruct = GenerateClassStructure(SuperClassMeta, SuperClassMeta, 0, ClassName, 0, ConstantInt::get(SizeTy, 0), GenerateIvarList(ClassName, empty, empty, empty2, ignored), ClassMethodList, NULLPtr, NULLPtr, true);
	// Generate the class structure
	llvm::Constant *ClassStruct = GenerateClassStructure(MetaClassStruct,
		SuperClass, 0, ClassName, 0,
		ConstantInt::get(LongTy, 0-instanceSize), IvarList,
		MethodList, GenerateProtocolList(Protocols), IvarOffsetValues, false);
	
	// Add class structure to list to be added to the symtab later
	ClassStruct = llvm::ConstantExpr::getBitCast(ClassStruct, PtrToInt8Ty);
	Classes.push_back(ClassStruct);
}

llvm::Function *CGObjCGNU::ModuleInitFunction()
{
	
	// Only emit an ObjC load function if no Objective-C stuff has been called
	if (Classes.empty() && Categories.empty() && ConstantStrings.empty() &&
		ExistingProtocols.empty() && SelectorTable.empty())
		return NULL;
	
	llvm::IntegerType *Int16Ty = llvm::IntegerType::getInt16Ty(Context);
	
	llvm::StructType *ModuleStructTy;
	llvm::StructType *SymbolTableStructTy;
	llvm::StructType *SelRefStructTy;
	
	llvm::PointerType *PtrToSelectorTy = llvm::PointerType::getUnqual(SelectorTy);
	llvm::PointerType *PtrToSymbolTableStructTy;
	
	SelRefStructTy = llvm::StructType::get(PtrToInt8Ty, // Selector name
										   PtrToInt8Ty, // Selector types
										   PtrToSelectorTy, // Pointer to static SEL
										   NULL);
	SymbolTableStructTy = llvm::StructType::get(IntTy, // Number of selector refs
												PtrToInt8Ty, // Sel refs
												Int16Ty, // Class count
												PtrToInt8Ty, // Class list
												Int16Ty, // Category count
												PtrToInt8Ty, // Categories list
												Int16Ty, // Protocol count
												PtrToInt8Ty, // Protocol list
												NULL
												);
	PtrToSymbolTableStructTy = llvm::PointerType::getUnqual(SymbolTableStructTy);
	ModuleStructTy = llvm::StructType::get(PtrToInt8Ty, // Module Name
										   PtrToSymbolTableStructTy, // Symbol table
										   IntTy, // Version
										   NULL);
	
	std::vector<llvm::Constant*> Elements;
	
	// Selectors.
	std::vector<llvm::Constant*> Selectors;
	for (SelectorMap::iterator iter = SelectorTable.begin(),
		 iterEnd = SelectorTable.end(); iter != iterEnd ; ++iter) {
		
		SmallVectorImpl<TypedSelector> &Types = iter->second;
		for (SmallVectorImpl<TypedSelector>::iterator i = Types.begin(),
			 e = Types.end() ; i!=e ; i++) {
			
			if (i != Types.begin()){
				// We only support one type in kernel
				[NSException raise: @"LKCodeGenException"
							format: @"Trying to register selector '%@' for the second time with different type. Initial types: %@ Types now: %@",
				 iter->first, Types.begin()->first, i->first];
			}
			
			if ([i->first isEqualToString:@""])
				llvm_unreachable("Selector has no type!");
			
			
			NSString *Name = iter->first;
			NSString *Types = i->first;

			Elements.push_back(MakeConstantString(Name));
			Elements.push_back(MakeConstantString(Types));
			
			// Second is the global variable - we supply a pointer to it to the
			// runtime which then fixes it
			Elements.push_back(i->second);
			
			Selectors.push_back(llvm::ConstantStruct::get(SelRefStructTy, Elements));
			
			Elements.clear();
		}
	}
	
	unsigned SelectorCount = Selectors.size();
	
	// Selectors
	Elements.push_back(llvm::ConstantInt::get(IntTy, SelectorCount));
	llvm::Constant *SelectorList = MakeGlobalArray(SelRefStructTy, Selectors,
												   ".objc_selector_list");
	Elements.push_back(llvm::ConstantExpr::getBitCast(SelectorList,
													  PtrToInt8Ty));
	
	Elements.push_back(llvm::ConstantInt::get(Int16Ty, Classes.size())); // Classes
	llvm::Constant *ClassList = MakeGlobalArray(IdTy, Classes,
												".objc_class_list");
	Elements.push_back(llvm::ConstantExpr::getBitCast(ClassList,
													  PtrToInt8Ty));
	
	Elements.push_back(llvm::ConstantInt::get(Int16Ty, Categories.size())); // Categories
	llvm::Constant *CategoriesList = MakeGlobalArray(IdTy, Categories,
													 ".objc_class_list");
	Elements.push_back(llvm::ConstantExpr::getBitCast(CategoriesList,
													  PtrToInt8Ty));
	
	Elements.push_back(llvm::ConstantInt::get(Int16Ty, ExistingProtocols.size()));
	
	std::vector<llvm::Constant*> Protocols;
	for (object_map<NSString*, llvm::Constant*>::iterator i = ExistingProtocols.begin();
		 i != ExistingProtocols.end(); ++i){
		Protocols.push_back(i->second);
	}
	llvm::Constant *ProtocolList = MakeGlobalArray(IdTy, Protocols);
	Elements.push_back(llvm::ConstantExpr::getBitCast(ProtocolList, PtrToInt8Ty));
	
	llvm::Constant *SymbolTable = MakeGlobal(SymbolTableStructTy, Elements);
	Elements.clear();
	
	// Now we need to create the loader module
	Elements.push_back(MakeConstantString([NSString stringWithUTF8String:TheModule.getModuleIdentifier().c_str()])); // Name
	Elements.push_back(SymbolTable);
	Elements.push_back(llvm::ConstantInt::get(IntTy, (int)0x301));
	
	llvm::Constant *ModuleStruct = MakeGlobal(ModuleStructTy,
													Elements,
													".objc_module");
	llvm::Constant *ModuleList = new llvm::GlobalVariable(TheModule,
																PtrTy,
																false,
																llvm::GlobalValue::InternalLinkage,
																llvm::ConstantExpr::getBitCast(ModuleStruct, PtrTy),
																"objc_module_list");
	
	return NULL;
}

llvm::Function *CGObjCGNU::MethodPreamble(
	const NSString *ClassName,
	const NSString *CategoryName,
	const NSString *MethodName,
	LLVMType *ReturnTy,
	LLVMType *SelfTy,
	const SmallVectorImpl<LLVMType*> &ArgTy,
	bool isClassMethod,
	bool isSRet,
	bool isVarArg)
{
	assert(0);
	std::vector<LLVMType*> Args;
	Args.push_back(SelfTy);
	Args.push_back(SelectorTy);
	Args.insert(Args.end(), ArgTy.begin(), ArgTy.end());

	llvm::FunctionType *MethodTy = llvm::FunctionType::get(ReturnTy,
		Args,
		isVarArg);
	llvm::AttrListPtr attributes = types->AI->attributeListForFunctionType(MethodTy, ReturnTy);
	std::string FunctionName = SymbolNameForMethod(ClassName, CategoryName,
		MethodName, isClassMethod);
	
	llvm::Function *Method = llvm::Function::Create(MethodTy,
		llvm::GlobalValue::InternalLinkage,
		FunctionName,
		&TheModule);
	Method->setAttributes(attributes);
	llvm::Function::arg_iterator AI = Method->arg_begin();
	if (isSRet)
	{
		AI->setName("retval");
		++AI;
	}
	AI->setName("self");
	++AI;
	AI->setName("_cmd");
	return Method;
}

static string ClassVariableName(NSString *ClassName, NSString *CvarName)
{
	return string(".class_variable_") + [ClassName UTF8String] + "_" +
		[CvarName UTF8String];
}

void CGObjCGNU::DefineClassVariables(
	NSString *ClassName,
	StringVector &CvarNames,
	StringVector &CvarTypes)
{
	// TODO: Store class variable metadata somewhere.
	// FIXME: Support non-object cvars
	for (unsigned int i = 0, e = CvarNames.size() ; i < e ; i++) 
	{
		string cvarname = ClassVariableName(ClassName, CvarNames[i]);

		new llvm::GlobalVariable(TheModule, IdTy, false,
				llvm::GlobalValue::InternalLinkage,
				ConstantPointerNull::get(cast<PointerType>(IdTy)), cvarname);
	}
}

llvm::Value *CGObjCGNU::AddressOfClassVariable(CGBuilder &Builder,
                                               NSString *ClassName,
                                               NSString *CvarName)
{
	string cvarname = ClassVariableName(ClassName, CvarName);
	GlobalVariable *var = TheModule.getNamedGlobal(cvarname);
	return var;
}


llvm::Constant *CGObjCGNU::ObjCIvarOffsetVariable(NSString *className,
		NSString *ivarName, uint64_t Offset)
{
	const std::string Name = std::string("__objc_ivar_offset_") +
		[className UTF8String] + '.' + [ivarName UTF8String];
	// Emit the variable and initialize it with what we think the correct value
	// is.  This allows code compiled with non-fragile ivars to work correctly
	// when linked against code which isn't (most of the time).
	llvm::GlobalVariable *IvarOffsetPointer = TheModule.getNamedGlobal(Name);
	if (!IvarOffsetPointer)
	{
		if (Offset == 0)
		{
			return TheModule.getOrInsertGlobal(Name, llvm::PointerType::getUnqual(LLVMType::getInt32Ty(Context)));
		}
		llvm::ConstantInt *OffsetGuess =
			llvm::ConstantInt::get(LLVMType::getInt32Ty(Context), Offset,
					"ivar");
		llvm::GlobalVariable *IvarOffsetGV = new
			llvm::GlobalVariable(TheModule, LLVMType::getInt32Ty(Context),
					false, llvm::GlobalValue::PrivateLinkage, OffsetGuess,
					Name+".guess");
		IvarOffsetPointer = new llvm::GlobalVariable(TheModule,
		IvarOffsetGV->getType(), false, llvm::GlobalValue::LinkOnceAnyLinkage,
		IvarOffsetGV, Name);
	}
	return IvarOffsetPointer;
}
llvm::Value *CGObjCGNU::OffsetOfIvar(CGBuilder &Builder,
                                     NSString *className,
                                     NSString *ivarName,
                                     int offsetGuess)
{
	return Builder.CreateLoad(Builder.CreateLoad(
		ObjCIvarOffsetVariable(className, ivarName, offsetGuess), false,
		"ivar"));
}

CGObjCRuntime *etoile::languagekit::CreateObjCRuntime(CodeGenTypes *types,
                                                      llvm::Module &M,
                                                      llvm::LLVMContext &C,
                                                      bool enableGC,
                                                      bool isJit)
{
  return new CGObjCGNU(types, M, C, enableGC, isJit);
}
CGObjCRuntime::~CGObjCRuntime() {}
