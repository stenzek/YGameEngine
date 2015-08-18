#include "Engine/PrecompiledHeader.h"
#include "Engine/ScriptTypes.h"

Y_Define_NameTable(NameTables::ScriptCallResults)
    Y_NameTable_Entry("Success", ScriptCallResult_Success)
    Y_NameTable_Entry("Yielded", ScriptCallResult_Yielded)
    Y_NameTable_Entry("ParseError", ScriptCallResult_ParseError)
    Y_NameTable_Entry("RuntimeError", ScriptCallResult_RuntimeError)
    Y_NameTable_Entry("NotInThread", ScriptCallResult_NotInThread)
    Y_NameTable_Entry("MethodNotFound", ScriptCallResult_MethodNotFound)
    Y_NameTable_Entry("NoObject", ScriptCallResult_NoObject)
Y_NameTable_End()

Y_Define_NameTable(NameTables::ScriptThreadTimeoutActions)
    Y_NameTable_Entry("Abort", ScriptThreadTimeoutAction_Abort)
    Y_NameTable_Entry("Resume", ScriptThreadTimeoutAction_Resume)
    Y_NameTable_Entry("ReturnNil", ScriptThreadTimeoutAction_ReturnNil)
Y_NameTable_End()

