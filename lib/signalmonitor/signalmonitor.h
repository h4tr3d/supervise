#ifndef SIGNALMONITOR_H
#define SIGNALMONITOR_H

#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <unordered_set>

#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

/**
 * @brief The SignalMonitor struct
 * Self-pipe signal handling implementation.
 *
 * Every instance of signal monitor creates thread that whait data on one pipe end and run handler
 * function in normal process execution flow (not in signal flow), so you can do some heavy operations.
 *
 * Short terminology:
 * - Signal catcher - system (low-level) signal handler. This handler sets, for example with signal() method.
 *   Runs in application interruption context. Must be small and quickly as soon as possible. Must not
 *   calls any non signal-safe functions (@see `man 7 signal`). Writing to pipe is a signal-safe operation
 *   (but it can be interrupted by other non-blocked signal).
 * - Signal handler - application-side signal handler. Runs in normal application flow context.
 *
 *
 * Example:
 * @code
 * SignalMonitor g_monitor;
 *
 * void signal_catcher(int sig)
 * {
 *    // Simple retransmit signal to application-space
 *    g_monitor.sendMessage(sig);
 * }
 *
 * void signal_handler(int sig)
 * {
 *   if (sig == SIGTERM || sig == SIGINT)
 *   {
 *     // do some heavy work for correct application finish: write logs, complete database
 *     // transactions and so on
 *   }
 * }
 *
 * int main()
 * {
 *    // Setup monitor first to avoid reces
 *    g_monitor.setHandler(std::bind(signal_handler, std::placeholders::_1));
 *
 *    // Setup signal handling
 *    // handle simple `kill PID` command
 *    SignalMonitor::setupSignalCatcher(SIGTERM, signal_catcher);
 *    // handle `Ctrl-C` from terminal
 *    SignalMonitor::setupSignalCatcher(SIGINT, signal_catcher);
 * }
 * @endcode
 *
 */
class SignalMonitor
{
public:
    template<int signo>
    struct SignalCatcher
    {
        static void setup(SignalMonitor *mon)
        {
            sigmon = mon;
            SignalMonitor::setupSignalCatcher(signo, catcher);
        }

        static void release(SignalMonitor *mon)
        {
            if (sigmon == mon)
            {
                reset();
            }
        }

        static void reset()
        {
            SignalMonitor::setupSignalCatcher(signo, SIG_DFL);
            sigmon = nullptr;
        }

        static void catcher(int sig)
        {
            if (sigmon)
                sigmon->sendMessage(sig);
        }

        // TODO: rework
        static SignalMonitor* sigmon;
    };

public:

    typedef std::function<void(int)> MessageHandler;

    SignalMonitor();

    ~SignalMonitor();

    /**
     * @brief setHandler
     * Set handler to process signals notifications.
     *
     * @note
     * This function not thread and signal-safe. Must be called at program start before any signal
     * handling via self-pipe settings.
     *
     * @param handler  functor to handle system signals.
     */
    void setHandler(const MessageHandler &handler);

    /**
     * @brief sendMessage
     * Send signal number from signal catcher to monitor.
     *
     * Also this method can be used from signal handler to avoid heavy cyclic operations.
     *
     * For example we can handle SIGCHLD signal and make waitpid() in handler. Note, that more than
     * one childs can die simultaneously. So you must do waitpid() in infinity loop and exit only on
     * zero return status (if you use WNOHANG option). Theoretically if a lot of childs die in
     * cyclic or quickly this loop can take a lot of time to execute. So we can't process other
     * unblocked signals and pipe queue may be filled and some signals dropped.
     *
     * To avoid this situation you can run loop in finite counts (for example) and if last exit
     * status of waitpid() is not zero you can send message SIGCHLD back to signal monitor and it
     * will be processed on next iteraction.
     *
     * @note
     * We have some guaranties for sending only one byte via pipe, so int value truncates to 8 bits.
     * It is not major problem for classic signals - its values less than 256. But if in future
     * signals with numbers greater 256 will be happens it will be a problem. But in general way
     * you handle only limited set of signals you can use you own signal and sendMessage() argument
     * mapping. In this way you can divide signals into separate sets and use different SignalMonitor
     * instances to handle this sets separately.
     *
     * @param signo  signal number
     * @return zero on succes, -1 on error and errno will be sets (you must restore errno value in
     *         signal handler)
     */
    int sendMessage(int signo);


    template<int signo>
    void addSignal()
    {
        // Setup
        SignalCatcher<signo>::setup(this);

        // Not thread-safe
        SignalCatcher<signo>::sigmon = this;

        // Register catcher to free
        m_catchers2.insert(SignalCatcher<signo>::release);
    }

    std::unordered_set<void(*)(SignalMonitor*)> m_catchers2;

private:
    void run();

public:
    /**
     * @name Static API
     * {
     */

    typedef void (*SignalHandler)(int);
    typedef void (*SignalAction)(int, siginfo_t *, void *);

    /**
     * @brief setupSignalCatcher
     * Install signal handler for given signal
     *
     * @note
     * Note that SA_RESTART flag will be enabled.
     *
     * @param signo   signal to handle
     * @param handler signal catcher
     * @return 0 on success, -1 on error (errno will be set)
     */
    static int setupSignalCatcher(int signo, SignalHandler handler);

    /**
     * @brief setupSignalAction
     * Install extended signal catcher for given signal. @see `man 2 sigaction`.
     *
     * @note
     * Note that SA_RESTART flag will be enabled.
     *
     * @param signo   signal to catch
     * @param action  extended signal catcher
     * @return 0 on success, -1 on error (errno will se set)
     */
    static int setupSignalAction(int signo, SignalAction action);

    /**
     * @brief setupSignalAction
     * Install user-configured signal actions.
     *
     * @note
     * Note that SA_RESTART flag will be enabled.
     *
     * @see `man 2 sigaction`
     *
     * @param signo
     * @param sa
     * @param old
     * @return
     */
    static int setupSignalAction(int signo, struct sigaction *sa, struct sigaction *old);
    /// @}

private:
    static int commonSetupSigAction(int signo, struct sigaction &sa);

private:
    int                 m_signalPipe[2] = {-1};
    MessageHandler      m_handler;
    std::atomic_bool    m_needStop;
    std::thread         m_thread;
};

template<int signo>
SignalMonitor* SignalMonitor::SignalCatcher<signo>::sigmon = nullptr;

#endif // SIGNALMONITOR_H
