#include <string.h>
#include <unistd.h>
#include <spawn.h>
#include <signal.h>
#include <sys/prctl.h>

#include <iostream>
#include <memory>
#include <vector>

#include "lib/processsupervisor/processsupervisor.h"
#include "lib/signalmonitor/signalmonitor.h"

using namespace std;

namespace {
unique_ptr<SignalMonitor> s_sigmonitor;
pid_t                     s_child;

void signal_setup()
{
    s_sigmonitor.reset(new SignalMonitor);
    s_sigmonitor->setHandler([](int signo){
        if (s_child)
            kill(s_child, signo);
    });
    s_sigmonitor->addSignal<SIGTERM>();
    s_sigmonitor->addSignal<SIGINT>();
    s_sigmonitor->addSignal<SIGHUP>();
}

void supervise_process(int argc, char**argv)
{
    ProcessSupervisor mon;

    vector<char*> args(argc + 1, nullptr);
    for (size_t i = 1; i < size_t(argc); ++i)
    {
        args[i - 1] = ::strdup(argv[i]);
    }

    mon.setForkRoutine([&args](){
        pid_t pid;

#if __linux__
        // Use vfork() instead of posix_spawn() to use linux-specific prctl()
        pid = vfork();
        if (pid == 0)
        {
            // Defaulting signals
            for (int sig = 1; sig < NSIG; ++sig)
            {
                ::signal(sig, SIG_DFL);
            }

            // Unexpected parent exit
            ::prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0);
            if (::execvp(args[0], args.data()) < 0)
            {
                cerr << "Can't exec process: errno=" << errno << ", text=" << strerror(errno) << endl;
                ::exit(255);
            }
        }
#else
        if(posix_spawnp(&pid, args[0], nullptr, nullptr, args.data(), nullptr) < 0)
        {
            pid = -1;
        }
#endif

        if (pid < 0)
        {
            cerr << "Can't spawn process: errno=" << errno << ", text=" << strerror(errno) << endl;
            ::exit(1);
        }

        s_child = pid;

        return pid;
    });

    mon.setLogCallback([](const std::string& text){
        cerr << text << endl;
    });

    mon.setRestartCheckCallback([](int status){
        bool signaled = WIFSIGNALED(status);
        int  signal   = WTERMSIG(status);
        bool exited   = WIFEXITED(status);
        int  exitstat = WEXITSTATUS(status);

        if (signaled)
        {
            if (signal == SIGINT || signal == SIGTERM)
                return false;
            return true;
        }

        if (exited)
        {
            if (exitstat)
            {
                ::sleep(2);
                return true;
            }
        }
        return false;
    });

    int sts = mon.start();

    // we must call free here instead delete, because we duplicate string with ::strdup()
    for (auto & arg : args)
        free(arg);

    ::exit(sts);
}

} // ::<unnamed>


int main(int argc, char**argv)
{
    if (argc < 2)
    {
        cerr << "Use: " << argv[0] << " prog [args]\n";
        ::exit(1);
    }

    signal_setup();
    supervise_process(argc, argv);

    return 0;
}

