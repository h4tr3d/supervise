# Supervise

Simple process supervisor: process runs under supervisor controll and restarts when crashed or exits with non-zero status.

Supervisor ignores signals SIGINT and SIGTERM. If child receive it and not process supervisor completes it work.

If supervisor killed, child process also will be killed (Linux only).


## Build

**Depends**
- Gcc/Clang with C++11 support
- Cmake 2.8 or newer

**Build**
- Create directory `build` and make it as work dir:
```
mkdir build
cd build
```
- Run Cmake:
```
cmake ..
```
- Run make (or ninja or...):
```
make -j8
```

Tool name: `supervise`


## Run

Application runs simple:
```
./supervise prog [prog_args]
```

Note, `prog` should not be deamon (detached from terminal) otherwise `supervise` will stop monitor it.
