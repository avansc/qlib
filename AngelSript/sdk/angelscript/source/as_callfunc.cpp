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
// as_callfunc.cpp
//
// These functions handle the actual calling of system functions
//



#include "as_config.h"
#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"

BEGIN_AS_NAMESPACE

int DetectCallingConvention(bool isMethod, const asSFuncPtr &ptr, int callConv, asSSystemFunctionInterface *internal)
{
	memset(internal, 0, sizeof(asSSystemFunctionInterface));

	internal->func = (size_t)ptr.ptr.f.func;

	// Was a compatible calling convention specified?
	if( internal->func )
	{
		if( ptr.flag == 1 && callConv != asCALL_GENERIC )
			return asWRONG_CALLING_CONV;
		else if( ptr.flag == 2 && (callConv == asCALL_GENERIC || callConv == asCALL_THISCALL) )
			return asWRONG_CALLING_CONV;
		else if( ptr.flag == 3 && callConv != asCALL_THISCALL )
			return asWRONG_CALLING_CONV;
	}

	asDWORD base = callConv;
	if( !isMethod )
	{
		if( base == asCALL_CDECL )
			internal->callConv = ICC_CDECL;
		else if( base == asCALL_STDCALL )
			internal->callConv = ICC_STDCALL;
		else if( base == asCALL_GENERIC )
			internal->callConv = ICC_GENERIC_FUNC;
		else
			return asNOT_SUPPORTED;
	}
	else
	{
#ifndef AS_NO_CLASS_METHODS
		if( base == asCALL_THISCALL )
		{
			internal->callConv = ICC_THISCALL;
#ifdef GNU_STYLE_VIRTUAL_METHOD
			if( (size_t(ptr.ptr.f.func) & 1) )
				internal->callConv = ICC_VIRTUAL_THISCALL;
#endif
			internal->baseOffset = ( int )MULTI_BASE_OFFSET(ptr);
#if defined(AS_ARM) && defined(__GNUC__)
			// As the least significant bit in func is used to switch to THUMB mode
			// on ARM processors, the LSB in the __delta variable is used instead of
			// the one in __pfn on ARM processors.
			if( (size_t(internal->baseOffset) & 1) )
				internal->callConv = ICC_VIRTUAL_THISCALL;
#endif

#ifdef HAVE_VIRTUAL_BASE_OFFSET
			// We don't support virtual inheritance
			if( VIRTUAL_BASE_OFFSET(ptr) != 0 )
				return asNOT_SUPPORTED;
#endif
		}
		else
#endif
		if( base == asCALL_CDECL_OBJLAST )
			internal->callConv = ICC_CDECL_OBJLAST;
		else if( base == asCALL_CDECL_OBJFIRST )
			internal->callConv = ICC_CDECL_OBJFIRST;
		else if( base == asCALL_GENERIC )
			internal->callConv = ICC_GENERIC_METHOD;
		else
			return asNOT_SUPPORTED;
	}

	return 0;
}

// This function should prepare system functions so that it will be faster to call them
int PrepareSystemFunctionGeneric(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine * /*engine*/)
{
	asASSERT(internal->callConv == ICC_GENERIC_METHOD || internal->callConv == ICC_GENERIC_FUNC);

	// Calculate the size needed for the parameters
	internal->paramSize = func->GetSpaceNeededForArguments();

	return 0;
}

// This function should prepare system functions so that it will be faster to call them
int PrepareSystemFunction(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine *engine)
{
#ifdef AS_MAX_PORTABILITY
	// This should never happen, as when AS_MAX_PORTABILITY is on, all functions 
	// are asCALL_GENERIC, which are prepared by PrepareSystemFunctionGeneric
	asASSERT(false);
#endif

	// References are always returned as primitive data
	if( func->returnType.IsReference() || func->returnType.IsObjectHandle() )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize     = sizeof(void*)/4;
		internal->hostReturnFloat    = false;
	}
	// Registered types have special flags that determine how they are returned
	else if( func->returnType.IsObject() )
	{
		asDWORD objType = func->returnType.GetObjectType()->flags;
	
		// Only value types can be returned by value
		asASSERT( objType & asOBJ_VALUE );

		if( !(objType & (asOBJ_APP_CLASS | asOBJ_APP_PRIMITIVE | asOBJ_APP_FLOAT)) )
		{
			// If the return is by value then we need to know the true type
			engine->WriteMessage("", 0, 0, asMSGTYPE_INFORMATION, func->GetDeclarationStr().AddressOf());

			asCString str;
			str.Format(TXT_CANNOT_RET_TYPE_s_BY_VAL, func->returnType.GetObjectType()->name.AddressOf());
			engine->WriteMessage("", 0, 0, asMSGTYPE_ERROR, str.AddressOf());
			engine->ConfigError(asINVALID_CONFIGURATION);
		}
		else if( objType & asOBJ_APP_CLASS )
		{
			internal->hostReturnFloat = false;
			if( objType & COMPLEX_MASK )
			{
				internal->hostReturnInMemory = true;
				internal->hostReturnSize     = sizeof(void*)/4;
			}
			else
			{
#ifdef HAS_128_BIT_PRIMITIVES
				if( func->returnType.GetSizeInMemoryDWords() > 4 )
#else
				if( func->returnType.GetSizeInMemoryDWords() > 2 )
#endif
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = sizeof(void*)/4;
				}
				else
				{
					internal->hostReturnInMemory = false;
					internal->hostReturnSize     = func->returnType.GetSizeInMemoryDWords();
				}

#ifdef THISCALL_RETURN_SIMPLE_IN_MEMORY
				if((internal->callConv == ICC_THISCALL ||
					internal->callConv == ICC_VIRTUAL_THISCALL) &&
					func->returnType.GetSizeInMemoryDWords() >= THISCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE)
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize     = sizeof(void*)/4;
				}
#endif
#ifdef CDECL_RETURN_SIMPLE_IN_MEMORY
				if((internal->callConv == ICC_CDECL         ||
					internal->callConv == ICC_CDECL_OBJLAST ||
					internal->callConv == ICC_CDECL_OBJFIRST) &&
					func->returnType.GetSizeInMemoryDWords() >= CDECL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE)
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize     = sizeof(void*)/4;
				}
#endif
#ifdef STDCALL_RETURN_SIMPLE_IN_MEMORY
				if( internal->callConv == ICC_STDCALL &&
					func->returnType.GetSizeInMemoryDWords() >= STDCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE)
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize     = sizeof(void*)/4;
				}
#endif
			}
		}
		else if( objType & asOBJ_APP_PRIMITIVE )
		{
			internal->hostReturnInMemory = false;
			internal->hostReturnSize     = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat    = false;
		}
		else if( objType & asOBJ_APP_FLOAT )
		{
			internal->hostReturnInMemory = false;
			internal->hostReturnSize     = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat    = true;
		}
	}
	// Primitive types can easily be determined
#ifdef HAS_128_BIT_PRIMITIVES
	else if( func->returnType.GetSizeInMemoryDWords() > 4 )
	{
		// Shouldn't be possible to get here
		asASSERT(false);
	}
	else if( func->returnType.GetSizeInMemoryDWords() == 4 )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize     = 4;
		internal->hostReturnFloat    = false;
	}
#else
	else if( func->returnType.GetSizeInMemoryDWords() > 2 )
	{
		// Shouldn't be possible to get here
		asASSERT(false);
	}
#endif
	else if( func->returnType.GetSizeInMemoryDWords() == 2 )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize     = 2;
		internal->hostReturnFloat    = func->returnType.IsEqualExceptConst(asCDataType::CreatePrimitive(ttDouble, true));
	}
	else if( func->returnType.GetSizeInMemoryDWords() == 1 )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize     = 1;
		internal->hostReturnFloat    = func->returnType.IsEqualExceptConst(asCDataType::CreatePrimitive(ttFloat, true));
	}
	else
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize     = 0;
		internal->hostReturnFloat    = false;
	}

	// Calculate the size needed for the parameters
	internal->paramSize = func->GetSpaceNeededForArguments();

	// Verify if the function takes any objects by value
	asUINT n;
	internal->takesObjByVal = false;
	for( n = 0; n < func->parameterTypes.GetLength(); n++ )
	{
		if( func->parameterTypes[n].IsObject() && !func->parameterTypes[n].IsObjectHandle() && !func->parameterTypes[n].IsReference() )
		{
			internal->takesObjByVal = true;

			// Can't pass objects by value unless the application type is informed
			if( !(func->parameterTypes[n].GetObjectType()->flags & (asOBJ_APP_CLASS | asOBJ_APP_PRIMITIVE | asOBJ_APP_FLOAT)) )
			{
				engine->WriteMessage("", 0, 0, asMSGTYPE_INFORMATION, func->GetDeclarationStr().AddressOf());
	
				asCString str;
				str.Format(TXT_CANNOT_PASS_TYPE_s_BY_VAL, func->parameterTypes[n].GetObjectType()->name.AddressOf());
				engine->WriteMessage("", 0, 0, asMSGTYPE_ERROR, str.AddressOf());
				engine->ConfigError(asINVALID_CONFIGURATION);
			}


#ifdef SPLIT_OBJS_BY_MEMBER_TYPES
			// It's not safe to pass objects by value because different registers
			// will be used depending on the memory layout of the object
#ifdef COMPLEX_OBJS_PASSED_BY_REF
			if( !(func->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK) )	
#endif
			{
				engine->WriteMessage("", 0, 0, asMSGTYPE_INFORMATION, func->GetDeclarationStr().AddressOf());

				asCString str;
				str.Format(TXT_DONT_SUPPORT_TYPE_s_BY_VAL, func->parameterTypes[n].GetObjectType()->name.AddressOf());
				engine->WriteMessage("", 0, 0, asMSGTYPE_ERROR, str.AddressOf());
				engine->ConfigError(asINVALID_CONFIGURATION);
			}
#endif
			break;
		}
	}

	// Verify if the function has any registered autohandles
	internal->hasAutoHandles = false;
	for( n = 0; n < internal->paramAutoHandles.GetLength(); n++ )
	{
		if( internal->paramAutoHandles[n] )
		{
			internal->hasAutoHandles = true;
			break;
		}
	}

	return 0;
}

#ifdef AS_MAX_PORTABILITY

int CallSystemFunction(int id, asCContext *context, void *objectPointer)
{
	asCScriptEngine *engine = context->engine;
	asSSystemFunctionInterface *sysFunc = engine->scriptFunctions[id]->sysFuncIntf;
	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(id, objectPointer);

	context->SetInternalException(TXT_INVALID_CALLING_CONVENTION);

	return 0;
}

#else

//
// CallSystemFunctionNative
//
// This function is implemented for each platform where the native calling conventions is supported.
// See the various as_callfunc_xxx.cpp files for their implementation. It is responsible for preparing
// the arguments for the function call, calling the function, and then retrieving the return value.
//
// Parameters:
//
// context    - This is the context that can be used to retrieve specific information from the engine
// descr      - This is the script function object that holds the information on how to call the function
// obj        - This is the object pointer, if the call is for a class method, otherwise it is null
// args       - This is the function arguments, which are packed as in AngelScript
// retPointer - This points to a the memory buffer where the return object is to be placed, if the function returns the value in memory rather than in registers
// retQW2     - This output parameter should be used if the function returns a value larger than 64bits in registers
//
// Return value:
//
// The function should return the value that is returned in registers. 
//
asQWORD CallSystemFunctionNative(asCContext *context, asCScriptFunction *descr, void *obj, asDWORD *args, void *retPointer, asQWORD &retQW2);


int CallSystemFunction(int id, asCContext *context, void *objectPointer)
{
	asCScriptEngine            *engine  = context->engine;
	asCScriptFunction          *descr   = engine->scriptFunctions[id];
	asSSystemFunctionInterface *sysFunc = descr->sysFuncIntf;

	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(id, objectPointer);

	asQWORD  retQW             = 0;
	asQWORD  retQW2            = 0;
	asDWORD *args              = context->regs.stackPointer;
	void    *retPointer        = 0;
	void    *obj               = 0;
	int      popSize           = sysFunc->paramSize;

	context->regs.objectType = descr->returnType.GetObjectType();
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() && !descr->returnType.IsObjectHandle() )
	{
		// Allocate the memory for the object
		retPointer = engine->CallAlloc(descr->returnType.GetObjectType());
	}

	if( callConv >= ICC_THISCALL )
	{
		if( objectPointer )
		{
			obj = objectPointer;
		}
		else
		{
			// The object pointer should be popped from the context stack
			popSize += AS_PTR_SIZE;

			// Check for null pointer
			obj = (void*)*(size_t*)(args);
			if( obj == 0 )
			{
				context->SetInternalException(TXT_NULL_POINTER_ACCESS);
				if( retPointer )
					engine->CallFree(retPointer);
				return 0;
			}

			// Add the base offset for multiple inheritance
#if defined(__GNUC__) && defined(AS_ARM)
			// On GNUC + ARM the lsb of the offset is used to indicate a virtual function
			// and the whole offset is thus shifted one bit left to keep the original
			// offset resolution
			obj = (void*)(size_t(obj) + (sysFunc->baseOffset>>1));
#else
			obj = (void*)(size_t(obj) + sysFunc->baseOffset);
#endif

			// Skip the object pointer
			args += AS_PTR_SIZE;
		}
	}

	retQW = CallSystemFunctionNative(context, descr, obj, args, sysFunc->hostReturnInMemory ? retPointer : 0, retQW2);

#if defined(COMPLEX_OBJS_PASSED_BY_REF) || defined(AS_LARGE_OBJS_PASSED_BY_REF)
	if( sysFunc->takesObjByVal )
	{
		// Need to free the complex objects passed by value, but that the 
		// calling convention implicitly passes by reference behind the scene
		args = context->regs.stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer )
			args += AS_PTR_SIZE;

		int spos = 0;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( descr->parameterTypes[n].IsObject() &&
				!descr->parameterTypes[n].IsObjectHandle() && 
				!descr->parameterTypes[n].IsReference() && (
#ifdef COMPLEX_OBJS_PASSED_BY_REF				
				(descr->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK) ||
#endif				
#ifdef AS_LARGE_OBJS_PASSED_BY_REF
				(descr->parameterTypes[n].GetSizeInMemoryDWords() >= AS_LARGE_OBJ_MIN_SIZE) ||
#endif
				0) )
			{
				void *obj = (void*)*(size_t*)&args[spos];
				spos += AS_PTR_SIZE;

#ifndef AS_CALLEE_DESTROY_OBJ_BY_VAL
				// If the called function doesn't destroy objects passed by value we must do so here
				asSTypeBehaviour *beh = &descr->parameterTypes[n].GetObjectType()->beh;
				if( beh->destruct )
					engine->CallObjectMethod(obj, beh->destruct);
#endif

				engine->CallFree(obj);
			}
			else
				spos += descr->parameterTypes[n].GetSizeOnStackDWords();
		}
	}
#endif

	// Store the returned value in our stack
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() )
	{
		if( descr->returnType.IsObjectHandle() )
		{
#if defined(AS_BIG_ENDIAN) && AS_PTR_SIZE == 1
			// Since we're treating the system function as if it is returning a QWORD we are
			// actually receiving the value in the high DWORD of retQW.
			retQW >>= 32;
#endif

			context->regs.objectRegister = (void*)(size_t)retQW;

			if( sysFunc->returnAutoHandle && context->regs.objectRegister )
				engine->CallObjectMethod(context->regs.objectRegister, descr->returnType.GetObjectType()->beh.addref);
		}
		else
		{
			if( !sysFunc->hostReturnInMemory )
			{
				// Copy the returned value to the pointer sent by the script engine
				if( sysFunc->hostReturnSize == 1 )
				{
#if defined(AS_BIG_ENDIAN) && AS_PTR_SIZE == 1
					// Since we're treating the system function as if it is returning a QWORD we are
					// actually receiving the value in the high DWORD of retQW.
					retQW >>= 32;
#endif

					*(asDWORD*)retPointer = (asDWORD)retQW;
				}
				else if( sysFunc->hostReturnSize == 2 )
					*(asQWORD*)retPointer = retQW;
				else if( sysFunc->hostReturnSize == 3 )
				{
					*(asQWORD*)retPointer         = retQW;
					*(((asDWORD*)retPointer) + 2) = (asDWORD)retQW2;
				}
				else // if( sysFunc->hostReturnSize == 4 )
				{
					*(asQWORD*)retPointer         = retQW;
					*(((asQWORD*)retPointer) + 1) = retQW2;
				}
			}

			// Store the object in the register
			context->regs.objectRegister = retPointer;
		}
	}
	else
	{
		// Store value in value register
		if( sysFunc->hostReturnSize == 1 )
		{
#if defined(AS_BIG_ENDIAN)
			// Since we're treating the system function as if it is returning a QWORD we are
			// actually receiving the value in the high DWORD of retQW.
			retQW >>= 32;

			// Due to endian issues we need to handle return values that are
			// less than a DWORD (32 bits) in size specially
			int numBytes = descr->returnType.GetSizeInMemoryBytes();
			if( descr->returnType.IsReference() ) numBytes = 4;
			switch( numBytes )
			{
			case 1:
				{
					// 8 bits
					asBYTE *val = (asBYTE*)&context->regs.valueRegister;
					val[0] = (asBYTE)retQW;
					val[1] = 0;
					val[2] = 0;
					val[3] = 0;
					val[4] = 0;
					val[5] = 0;
					val[6] = 0;
					val[7] = 0;
				}
				break;
			case 2:
				{
					// 16 bits
					asWORD *val = (asWORD*)&context->regs.valueRegister;
					val[0] = (asWORD)retQW;
					val[1] = 0;
					val[2] = 0;
					val[3] = 0;
				}
				break;
			default:
				{
					// 32 bits
					asDWORD *val = (asDWORD*)&context->regs.valueRegister;
					val[0] = (asDWORD)retQW;
					val[1] = 0;
				}
				break;
			}
#else
			*(asDWORD*)&context->regs.valueRegister = (asDWORD)retQW;
#endif
		}
		else
			context->regs.valueRegister = retQW;
	}

	// Release autohandles in the arguments
	if( sysFunc->hasAutoHandles )
	{
		args = context->regs.stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer )
			args += AS_PTR_SIZE;

		int spos = 0;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( sysFunc->paramAutoHandles[n] && *(size_t*)&args[spos] != 0 )
			{
				// Call the release method on the type
				engine->CallObjectMethod((void*)*(size_t*)&args[spos], descr->parameterTypes[n].GetObjectType()->beh.release);
				*(size_t*)&args[spos] = 0;
			}

			if( descr->parameterTypes[n].IsObject() && !descr->parameterTypes[n].IsObjectHandle() && !descr->parameterTypes[n].IsReference() )
				spos += AS_PTR_SIZE;
			else
				spos += descr->parameterTypes[n].GetSizeOnStackDWords();
		}
	}

	return popSize;
}

#endif // AS_MAX_PORTABILITY

END_AS_NAMESPACE

