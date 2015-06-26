#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <functional>
#include <thread>
#include <exception>
#include <cassert>

#include "processsupervisor.h"
#include "safefork.h"

ProcessSupervisor::ProcessSupervisor(ProcessSupervisor::Routine childRoutine)
    : m_child(childRoutine)
{}

void ProcessSupervisor::setPreforkCallback(ProcessSupervisor::PreforkCallback cb)
{
    m_prefork = cb;
}

void ProcessSupervisor::setPostforkCallback(ProcessSupervisor::PostforkCallback cb)
{
    m_postfork = cb;
}

void ProcessSupervisor::setRestartCheckCallback(ProcessSupervisor::RestartCheckCallback cb)
{
    m_restartCheck = cb;
}

void ProcessSupervisor::setPrerestartCallback(ProcessSupervisor::PrerestartCallback cb)
{
    m_prerestart = cb;
}

void ProcessSupervisor::setForkRoutine(ProcessSupervisor::ForkRoutine cb)
{
    m_fork = cb;
}

void ProcessSupervisor::setLogCallback(ProcessSupervisor::LogCallback cb)
{
    m_log = cb;
}

void ProcessSupervisor::setChildSignal(int signo)
{
    m_childSignal = signo;
}

void ProcessSupervisor::setChildRoutine(ProcessSupervisor::Routine cb)
{
    m_child = cb;
}

int ProcessSupervisor::start()
{
    int  status  = 0;
    bool restart = true;
    while (restart)
    {
        if (m_prefork)
            m_prefork();

        pid_t pid;
        if (m_fork)
            pid = m_fork();
        else
            pid = defaultForkRoutine();

        if (m_postfork)
            m_postfork(pid);

        while (true)
        {
            pid_t child = wait(&status);

            if (m_log)
            {
                std::stringstream ss;
                ss << std::boolalpha
                   << "child exits: st=" << status
                   << ", signaled=" << bool(WIFSIGNALED(status))
                   << ", signal="   << WTERMSIG(status)
                   << ", exited="   << bool(WIFEXITED(status))
                   << ", status="   << WEXITSTATUS(status);
                m_log(ss.str());
            }

            if (child == pid)
            {
                restart = WIFSIGNALED(status);
                if (m_restartCheck)
                    restart = m_restartCheck(status);
                status  = WIFEXITED(status) ? WEXITSTATUS(status) : 0;

                if (restart && m_prerestart)
                    m_prerestart();

                break;
            }
        }
    }

    return status;
}

pid_t ProcessSupervisor::defaultForkRoutine()
{
    pid_t pid = safe_fork();
    switch (pid)
    {
        case -1:
        {
            std::cerr << "Can't fork process\n";
            exit(1);
            break;
        }

        case 0: // child
        {
            // set signal that will be sent to the child when parent died.
#ifdef __linux
            prctl(PR_SET_PDEATHSIG, m_childSignal);
#else
#  error Unsupported OS
#endif
            if (!m_child)
                throw BadChildRoutine("Undefined child rotuine");

            exit(m_child());
            break;
        }

        default: // parent
            break;
    }

    return pid;
}
