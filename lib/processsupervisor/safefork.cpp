#include <unistd.h>

#include <fstream>

#include "safefork.h"

using namespace std;

#ifdef __linux
ssize_t get_process_threads_count() noexcept
{
    string  fileName = "/proc/self/status";
    ssize_t count    = -1;

    ifstream ifs(fileName);
    if (!ifs)
    {
        //cerr << "Can't open process stat file: " << fileName << endl;
        return -1;
    }

    string line;
    while (getline(ifs, line))
    {
        constexpr static char subline[] = "Threads:\0";

        size_t pos = line.find(subline);

        if (pos != string::npos)
        {
            count = strtoul(line.c_str() + pos + sizeof(subline) - 1, nullptr, 10);
            break;
        }
    }

    return count;
}
#else
#  error Unsupported OS
#endif

pid_t safe_fork() throw(SafeForkError)
{
    const auto threads = get_process_threads_count();
    if (threads > 1)
    {
        throw SafeForkError("It is not safe to do fork() with exists threads");
    }
    return fork();
}
