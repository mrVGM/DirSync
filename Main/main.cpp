#include "BaseFramework.h"

#include "JSONParser.h"
#include "Jobs.h"
#include "Job.h"
#include "Lock.h"
#include "PipeServer.h"

#include "JobSystemMeta.h"
#include "JobSystem.h"

#include "UDP.h"
#include <filesystem>

int main()
{
	lock::LockObject* lock = new lock::LockObject(lock::MainLockMeta::GetInstance());
	lock->Lock();

	json_parser::Boot();
	jobs::Boot();

    jobs::JobSystem* frontnedProcessJS = new jobs::JobSystem(jobs::JobSystemMeta::GetInstance(), 1);

	pipe_server::ServerObject* server = nullptr;
	jobs::RunSync(jobs::Job::CreateFromLambda([&]() {
		server = new pipe_server::ServerObject();
		std::string inPipe, outPipe;
		server->Start(inPipe, outPipe);


        frontnedProcessJS->ScheduleJob(jobs::Job::CreateFromLambda([=]() {

            char moduleName[1024] = {};
            DWORD length = GetModuleFileName(NULL, moduleName, 1024);
            std::string exe(moduleName);

            size_t dirSep = exe.find_last_of('\\', exe.size());
            exe = exe.substr(0, dirSep + 1);
            exe += "..\\..\\..\\..\\Electron\\electron.exe";


            std::wstring tmp = std::filesystem::absolute(exe).c_str();
            exe = std::string(tmp.begin(), tmp.end());

            std::string pipeNames = inPipe + " " + outPipe;
            exe += " " + pipeNames;

            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));


            LPSTR cl = const_cast<LPSTR>(exe.c_str());
            // Start the child process. 
            if (!CreateProcess(NULL,   // No module name (use command line)
                cl,        // Command line
                NULL,           // Process handle not inheritable
                NULL,           // Thread handle not inheritable
                FALSE,          // Set handle inheritance to FALSE
                0,              // No creation flags
                NULL,           // Use parent's environment block
                NULL,           // Use parent's starting directory 
                &si,            // Pointer to STARTUPINFO structure
                &pi)           // Pointer to PROCESS_INFORMATION structure
                )
            {
                printf("CreateProcess failed (%d).\n", GetLastError());
                return;
            }

            // Wait until child process exits.
            WaitForSingleObject(pi.hProcess, INFINITE);

            // Close process and thread handles. 
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            lock->UnLock();
        }));

	}));

	lock->Lock();

	server->End();

	return 0;
}
