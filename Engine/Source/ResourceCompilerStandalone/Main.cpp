#include "ResourceCompilerInterface/ResourceCompilerInterfaceRemote.h"
#include "YBaseLib/Subprocess.h"

int main(int argc, char *argv[])
{
    Log::GetInstance().SetConsoleOutputParams(true);

    // Handle subprocess stuff
    if (Subprocess::IsSubprocess())
    {
        ResourceCompilerInterfaceRemote::RemoteProcessLoop();
        return 0;
    }

    //UNREFERENCED_PARAMETER(argc);
    //UNREFERENCED_PARAMETER(argv);
    return 0;
}

