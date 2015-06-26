#ifndef PROCESSSUPERVISOR_H
#define PROCESSSUPERVISOR_H

#include <signal.h>

#include <functional>
#include <stdexcept>

/**
 * @brief The BadChildRoutine exception class
 *
 * This exception thrown when ProcessSupervisor starts but child rountine does not point
 */
class BadChildRoutine : public std::logic_error
{
public:
    BadChildRoutine(const std::string &what)
        : logic_error(what) {}
};

class ProcessSupervisor
{
public:
    typedef std::function<void()>                   PreforkCallback;
    typedef std::function<void(int)>                PostforkCallback;
    typedef std::function<bool(int)>                RestartCheckCallback;
    typedef std::function<void()>                   PrerestartCallback;
    typedef std::function<void(const std::string&)> LogCallback;
    typedef std::function<pid_t()>                  ForkRoutine;
    typedef std::function<int()>                    Routine;

    ProcessSupervisor() = default;
    explicit ProcessSupervisor(Routine childRoutine);

    void setPreforkCallback(PreforkCallback cb);
    void setPostforkCallback(PostforkCallback cb);
    void setRestartCheckCallback(RestartCheckCallback cb);
    void setPrerestartCallback(PrerestartCallback cb);
    void setForkRoutine(ForkRoutine cb);
    void setChildRoutine(Routine cb);

    void setLogCallback(LogCallback cb);  

    void setChildSignal(int signo);
    int  childSignal() const;

    int start();

private:
    pid_t defaultForkRoutine();

private:
    PreforkCallback  m_prefork;
    PostforkCallback m_postfork;
    RestartCheckCallback m_restartCheck;
    PrerestartCallback m_prerestart;
    LogCallback      m_log;
    ForkRoutine      m_fork;
    Routine          m_child;
    int              m_childSignal = SIGTERM;
};

#endif // PROCESSSUPERVISOR_H
