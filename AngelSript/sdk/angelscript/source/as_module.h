/*
   AngelCode Scripting Library
   Copyright (c) 2003-2011 Andreas Jonsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/



//
// as_module.h
//
// A class that holds a script module
//

#ifndef AS_MODULE_H
#define AS_MODULE_H

#include "as_config.h"
#include "as_atomic.h"
#include "as_string.h"
#include "as_array.h"
#include "as_datatype.h"
#include "as_scriptfunction.h"
#include "as_property.h"

BEGIN_AS_NAMESPACE

// TODO: import: Remove this when the imported functions are removed
const int FUNC_IMPORTED = 0x40000000;

class asCScriptEngine;
class asCCompiler;
class asCBuilder;
class asCContext;
class asCConfigGroup;

struct sBindInfo
{
	asCScriptFunction *importedFunctionSignature;
	asCString		   importFromModule;
	int                boundFunctionId;
};

struct sObjectTypePair
{
	asCObjectType *a;
	asCObjectType *b;
};

// TODO: import: Remove function imports. When I have implemented function 
//               pointers the function imports should be deprecated.

// TODO: Need a separate interface for compiling scripts. The asIScriptCompiler
//       will have a target module, and will allow the compilation of an entire
//       script or just individual functions within the scope of the module
// 
//       With this separation it will be possible to compile the library without
//       the compiler, thus giving a much smaller binary executable.

// TODO: There should be an special compile option that will let the application
//       recompile an already compiled script. The compiler should check if no
//       destructive changes have been made (changing function signatures, etc)
//       then it should simply replace the bytecode within the functions without
//       changing the values of existing global properties, etc.


class asCModule : public asIScriptModule
{
//-------------------------------------------
// Public interface
//--------------------------------------------
public:
	virtual asIScriptEngine *GetEngine() const;
	virtual void             SetName(const char *name);
	virtual const char      *GetName() const;

	// Compilation
	virtual int  AddScriptSection(const char *name, const char *code, size_t codeLength, int lineOffset);
	virtual int  Build();
	virtual int  CompileFunction(const char *sectionName, const char *code, int lineOffset, asDWORD reserved, asIScriptFunction **outFunc);
	virtual int  CompileGlobalVar(const char *sectionName, const char *code, int lineOffset);

	// Script functions
	virtual int                GetFunctionCount() const;
	virtual int                GetFunctionIdByIndex(int index) const;
	virtual int                GetFunctionIdByName(const char *name) const;
	virtual int                GetFunctionIdByDecl(const char *decl) const;
	virtual asIScriptFunction *GetFunctionDescriptorByIndex(int index) const;
	virtual asIScriptFunction *GetFunctionDescriptorById(int funcId) const;
	virtual int                RemoveFunction(int funcId);

	// Script global variables
	virtual int         ResetGlobalVars();
	virtual int         GetGlobalVarCount() const;
	virtual int         GetGlobalVarIndexByName(const char *name) const;
	virtual int         GetGlobalVarIndexByDecl(const char *decl) const;
	virtual const char *GetGlobalVarDeclaration(asUINT index) const;
	virtual int         GetGlobalVar(asUINT index, const char **name, int *typeId, bool *isConst) const;
	virtual void       *GetAddressOfGlobalVar(asUINT index);
	virtual int         RemoveGlobalVar(asUINT index);

	// Type identification
	virtual int            GetObjectTypeCount() const;
	virtual asIObjectType *GetObjectTypeByIndex(asUINT index) const;
	virtual int            GetTypeIdByDecl(const char *decl) const;

	// Enums
	virtual int         GetEnumCount() const;
	virtual const char *GetEnumByIndex(asUINT index, int *enumTypeId) const;
	virtual int         GetEnumValueCount(int enumTypeId) const;
	virtual const char *GetEnumValueByIndex(int enumTypeId, asUINT index, int *outValue) const;

	// Typedefs
	virtual int         GetTypedefCount() const;
	virtual const char *GetTypedefByIndex(asUINT index, int *typeId) const;

	// Dynamic binding between modules
	virtual int         GetImportedFunctionCount() const;
	virtual int         GetImportedFunctionIndexByDecl(const char *decl) const;
	virtual const char *GetImportedFunctionDeclaration(int importIndex) const;
	virtual const char *GetImportedFunctionSourceModule(int importIndex) const;
	virtual int         BindImportedFunction(int index, int sourceID);
	virtual int         UnbindImportedFunction(int importIndex);
	virtual int         BindAllImportedFunctions();
	virtual int         UnbindAllImportedFunctions();

	// Bytecode Saving/Loading
	virtual int SaveByteCode(asIBinaryStream *out) const;
	virtual int LoadByteCode(asIBinaryStream *in);

#ifdef AS_DEPRECATED
	// Since 2.20.0
	virtual const char *GetGlobalVarName(int index) const;
	virtual int         GetGlobalVarTypeId(int index, bool *isConst) const;
#endif

//-----------------------------------------------
// Internal
//-----------------------------------------------
	asCModule(const char *name, asCScriptEngine *engine);
	~asCModule();

//protected:
	friend class asCScriptEngine;
	friend class asCBuilder;
	friend class asCCompiler;
	friend class asCContext;
	friend class asCRestore;

	void InternalReset();

	int  CallInit();
	void CallExit();

	void JITCompile();

	int  AddScriptFunction(int sectionIdx, int id, const char *name, const asCDataType &returnType, asCDataType *params, asETypeModifiers *inOutFlags, asCString **defaultArgs, int paramCount, bool isInterface, asCObjectType *objType = 0, bool isConstMethod = false, bool isGlobalFunction = false, bool isPrivate = false);
	int  AddScriptFunction(asCScriptFunction *func);
	int  AddImportedFunction(int id, const char *name, const asCDataType &returnType, asCDataType *params, asETypeModifiers *inOutFlags, int paramCount, const asCString &moduleName);
	int  AddFuncDef(const char *name);

	int  GetNextImportedFunctionId();

	void ResolveInterfaceIds(asCArray<void*> *substitutions = 0);
	bool AreInterfacesEqual(asCObjectType *a, asCObjectType *b, asCArray<sObjectTypePair> &equals);
	bool AreTypesEqual(const asCDataType &a, const asCDataType &b, asCArray<sObjectTypePair> &equals);

	asCScriptFunction *GetImportedFunction(int funcId) const;

	asCObjectType *GetObjectType(const char *type);

	asCGlobalProperty *AllocateGlobalProperty(const char *name, const asCDataType &dt);


	asCString name;

	asCScriptEngine *engine;
	asCBuilder      *builder;

	// This array holds all functions, class members, factories, etc that were compiled with the module
	asCArray<asCScriptFunction *>  scriptFunctions;
	// This array holds global functions declared in the module
	asCArray<asCScriptFunction *>  globalFunctions;
	// This array holds imported functions in the module
	asCArray<sBindInfo *>          bindInformations;

	// This array holds the global variables declared in the script
	asCArray<asCGlobalProperty *>  scriptGlobals;
	bool                           isGlobalVarInitialized;

	// This array holds class and interface types
	asCArray<asCObjectType*>       classTypes;
	// This array holds enum types
	asCArray<asCObjectType*>       enumTypes;
	// This array holds typedefs
	asCArray<asCObjectType*>       typeDefs;
	// This array holds the funcdefs declared in the module
	asCArray<asCScriptFunction*>   funcDefs;
};

END_AS_NAMESPACE

#endif
