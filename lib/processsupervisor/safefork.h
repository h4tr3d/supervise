#ifndef SAFEFORK_H
#define SAFEFORK_H

#include <signal.h>

#include <stdexcept>
//#include <cstddef>
#include <cstdint>

/**
 * @brief The SafeForkError exception
 *
 * Thrown when safe_fork() called and count of threads greater then 1
  */
class SafeForkError : public std::logic_error
{
public:
    explicit SafeForkError(const std::string &what)
        : logic_error(what)
    {}
};

/**
 * @brief Returns count of the threads of the current process
 * @return count of threads (at least 1) or -1 on error
 */
ssize_t get_process_threads_count() noexcept;

/**
 * @brief Simple fork() wrapper that checks count of thread before runs
 * @return -1 on error (see errno status), 0 - child, pid number - parent
 */
pid_t safe_fork() throw(SafeForkError);


#endif // SAFEFORK_H
