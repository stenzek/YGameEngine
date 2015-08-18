#pragma once
#include "Engine/Common.h"

// Reference type
typedef int ScriptReferenceType;
const ScriptReferenceType INVALID_SCRIPT_REFERENCE = (ScriptReferenceType)-1;

// Userdata tag type
typedef unsigned int ScriptUserDataTagType;

// Other defines
const float DEFAULT_SCRIPT_TIMEOUT = 60.0f;

// Native function type
typedef int(*ScriptNativeFunctionType)(struct lua_State *);

// Call results
enum ScriptCallResult
{
    ScriptCallResult_Success,               // Normal success error code
    ScriptCallResult_Yielded,               // Script has yielded execution and can be resumed later
    ScriptCallResult_ParseError,            // Parse error encountered when compiling the script
    ScriptCallResult_RuntimeError,          // Generic error encountered during execution
    ScriptCallResult_NotInThread,           // Script attempted to yield, but the call was not invoked in a coroutine thread
    ScriptCallResult_MethodNotFound,        // Table call was issued, but the method was not found in the method table
    ScriptCallResult_NoObject,              // A method call was attempted on a non-object
};

// Script thread timeout actions
enum ScriptThreadTimeoutAction
{
    ScriptThreadTimeoutAction_Abort,        // Abort script execution of the thread when the timeout is reached.
    ScriptThreadTimeoutAction_Resume,       // Resume script execution of the thread when the timeout is reached.
    ScriptThreadTimeoutAction_ReturnNil,    // Resume script execution, returning nil when the timeout is reached.
    ScriptThreadTimeoutAction_Count,
};

// call result -> string
namespace NameTables {
    Y_Declare_NameTable(ScriptCallResults);
    Y_Declare_NameTable(ScriptThreadTimeoutActions);
};

// mapping entity functions -> proxy functions
// todo: cached list of available entity methods?
struct SCRIPT_FUNCTION_TABLE_ENTRY
{
    const char *FunctionName;
    ScriptNativeFunctionType FunctionPointer;
};

// script userdata tags
enum ScriptUserDataTag
{
    ScriptUserDataTag_None,
    ScriptUserDataTag_ScriptObject,
    ScriptUserDataTag_Count,
};

// Forward declarations of script classes.
class ScriptManager; 
class ScriptThread;
class ScriptObject;
