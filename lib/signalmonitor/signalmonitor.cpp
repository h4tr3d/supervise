#include <iostream>
#include <cstdlib>

#include "signalmonitor.h"

using namespace std;

namespace {

using sigaction_t = struct sigaction;

inline int makeDescriptorNonblock(int fd)
{
    int flags = ::fcntl(fd, F_GETFL);
    if (flags == -1)
        return -1;

    flags |= O_NONBLOCK;

    if (::fcntl(fd, F_SETFL, flags) == -1)
        return -1;
    return 0;
}

inline void errorExit(const char *str)
{
    perror(str);
    exit(1);
}

}

SignalMonitor::SignalMonitor()
{
    if (::pipe(m_signalPipe) == -1)
        errorExit("can't create signal pipe");


    // Make read and write ends of pipe nonblocking
    int stat = 0;
    stat += makeDescriptorNonblock(m_signalPipe[0]); // Make read end nonblocking
    stat += makeDescriptorNonblock(m_signalPipe[1]); // Make write end nonblocking

    if (stat < 0)
        errorExit("make descriptor nonblocking");

    m_needStop = false;
    m_thread   = thread(&SignalMonitor::run, this);
}

SignalMonitor::~SignalMonitor()
{
    m_needStop = true;
    if (m_thread.joinable())
    {
        // Remove message handler
        m_handler = MessageHandler();
        // Send dummy signal
        sendMessage(255);
        // Wait thread completion
        m_thread.join();
    }

    // Not thread-safe
    for (auto release : m_catchers2)
    {
        release(this);
    }
    
    ::close(m_signalPipe[0]);
    ::close(m_signalPipe[1]);
}

void SignalMonitor::setHandler(const SignalMonitor::MessageHandler &handler)
{
    m_handler = handler;
}

int SignalMonitor::sendMessage(int signo)
{
    uint8_t ch = (uint8_t)signo;
    return ::write(m_signalPipe[1], &ch, 1);
}

void SignalMonitor::run()
{
    fd_set fds;

    int nfds = m_signalPipe[0] + 1;

    while (!m_needStop)
    {
        struct timeval to = {5, 0};

        FD_ZERO(&fds);
        FD_SET(m_signalPipe[0], &fds);

        int ready = -1;
        do
        {
            ready = select(nfds, &fds, nullptr, nullptr, &to);
        }
        while (ready==-1 && errno == EINTR);

        if (ready < 0)
            errorExit("unknown select error");

        if (ready == 0)
            continue;

        // Signal emited
        if (FD_ISSET(m_signalPipe[0], &fds))
        {
            // Some signal emited. In buffer can be presents more that one signal notification,
            // process all of them.
            for (;;)
            {
                // Note, that only one byte reading/sending is valid.
                uint8_t ch;
                if (read(m_signalPipe[0], &ch, 1) == -1)
                {
                    if (errno == EAGAIN)
                        break;
                    else
                        errorExit("unhandler error on signal pipe read");
                }

                if (m_handler)
                {
                    m_handler(ch);
                }
            }
        }
    }
}

int SignalMonitor::setupSignalCatcher(int signo, SignalMonitor::SignalHandler handler)
{
    auto sa = sigaction_t();
    sa.sa_handler = handler;
    return commonSetupSigAction(signo, sa);
}

int SignalMonitor::setupSignalAction(int signo, SignalMonitor::SignalAction action)
{
    auto sa = sigaction_t();
    sa.sa_sigaction = action;
    sa.sa_flags     = SA_SIGINFO;
    return commonSetupSigAction(signo, sa);
}

int SignalMonitor::setupSignalAction(int signo, struct sigaction *sa, struct sigaction *old)
{
    if (sa)
        sa->sa_flags |= SA_RESTART;
    return ::sigaction(signo, sa, old);
}

int SignalMonitor::commonSetupSigAction(int signo, struct sigaction &sa)
{
    sa.sa_flags |= SA_RESTART;
    ::sigemptyset(&sa.sa_mask);
    return ::sigaction(signo, &sa, nullptr);
}
