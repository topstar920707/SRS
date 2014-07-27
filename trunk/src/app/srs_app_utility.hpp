/*
The MIT License (MIT)

Copyright (c) 2013-2014 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef SRS_APP_UTILITY_HPP
#define SRS_APP_UTILITY_HPP

/*
#include <srs_app_utility.hpp>
*/

#include <srs_core.hpp>

#include <vector>
#include <string>
#include <sstream>

#include <sys/resource.h>

#include <srs_app_st.hpp>

class SrsKbps;

// client open socket and connect to server.
extern int srs_socket_connect(std::string server, int port, int64_t timeout, st_netfd_t* pstfd);

/**
* convert level in string to log level in int.
* @return the log level defined in SrsLogLevel.
*/
extern int srs_get_log_level(std::string level);

// current process resouce usage.
// @see: man getrusage
class SrsRusage
{
public:
    // whether the data is ok.
    bool ok;
    // the time in ms when sample.
    int64_t sample_time;
    
    rusage r;
    
    SrsRusage();
};

// get system rusage, use cache to avoid performance problem.
extern SrsRusage* srs_get_system_rusage();
// the deamon st-thread will update it.
extern void srs_update_system_rusage();

// to stat the process info.
// @see: man 5 proc, /proc/[pid]/stat
class SrsProcSelfStat
{
public:
    // whether the data is ok.
    bool ok;
    // the time in ms when sample.
    int64_t sample_time;
    // the percent of usage. 0.153 is 15.3%.
    float percent;
    
    // pid %d      The process ID.
    int pid;
    // comm %s     The  filename  of  the  executable,  in parentheses. This is visible whether or not the executable is
    //             swapped out.
    char comm[32];
    // state %c    One character from the string "RSDZTW" where R is running, S is sleeping in an interruptible  wait,  D
    //             is  waiting in uninterruptible disk sleep, Z is zombie, T is traced or stopped (on a signal), and W is
    //             paging.
    char state;
    // ppid %d     The PID of the parent.
    int ppid;
    // pgrp %d     The process group ID of the process.
    int pgrp;
    // session %d  The session ID of the process.
    int session;
    // tty_nr %d   The controlling terminal of the process.  (The minor device number is contained in the combination  of
    //             bits 31 to 20 and 7 to 0; the major device number is in bits 15 t0 8.)
    int tty_nr;
    // tpgid %d    The ID of the foreground process group of the controlling terminal of the process.
    int tpgid;
    // flags %u (%lu before Linux 2.6.22)
    //             The  kernel  flags  word  of  the process.  For bit meanings, see the PF_* defines in <linux/sched.h>.
    //             Details depend on the kernel version.
    unsigned int flags;
    // minflt %lu  The number of minor faults the process has made which have not required loading  a  memory  page  from
    //             disk.
    unsigned long minflt;
    // cminflt %lu The number of minor faults that the process’s waited-for children have made.
    unsigned long cminflt;
    // majflt %lu  The number of major faults the process has made which have required loading a memory page from disk.
    unsigned long majflt;
    // cmajflt %lu The number of major faults that the process’s waited-for children have made.
    unsigned long cmajflt;
    // utime %lu   Amount  of  time that this process has been scheduled in user mode, measured in clock ticks (divide by
    //             sysconf(_SC_CLK_TCK).  This includes guest time, guest_time (time spent running  a  virtual  CPU,  see
    //             below),  so  that  applications  that are not aware of the guest time field do not lose that time from
    //             their calculations.
    unsigned long utime;
    // stime %lu   Amount of time that this process has been scheduled in kernel mode, measured in clock ticks (divide by
    //             sysconf(_SC_CLK_TCK).
    unsigned long stime;
    // cutime %ld  Amount  of  time that this process’s waited-for children have been scheduled in user mode, measured in
    //             clock ticks (divide  by  sysconf(_SC_CLK_TCK).   (See  also  times(2).)   This  includes  guest  time,
    //             cguest_time (time spent running a virtual CPU, see below).
    long cutime;
    // cstime %ld  Amount of time that this process’s waited-for children have been scheduled in kernel mode, measured in
    //             clock ticks (divide by sysconf(_SC_CLK_TCK).
    long cstime;
    // priority %ld
    //          (Explanation for Linux 2.6) For processes running a real-time scheduling  policy  (policy  below;  see
    //          sched_setscheduler(2)),  this  is the negated scheduling priority, minus one; that is, a number in the
    //          range -2 to -100, corresponding to real-time priorities 1 to 99.  For processes running under  a  non-
    //          real-time scheduling policy, this is the raw nice value (setpriority(2)) as represented in the kernel.
    //          The kernel stores nice values as numbers in the range 0 (high) to 39 (low), corresponding to the user-
    //          visible nice range of -20 to 19.
    //
    //          Before Linux 2.6, this was a scaled value based on the scheduler weighting given to this process.
    long priority;
    // nice %ld    The nice value (see setpriority(2)), a value in the range 19 (low priority) to -20 (high priority).
    long nice;
    // num_threads %ld
    //          Number  of threads in this process (since Linux 2.6).  Before kernel 2.6, this field was hard coded to
    //          0 as a placeholder for an earlier removed field.
    long num_threads;
    // itrealvalue %ld
    //          The time in jiffies before the next SIGALRM is sent to the process due to an  interval  timer.   Since
    //          kernel 2.6.17, this field is no longer maintained, and is hard coded as 0.    
    long itrealvalue;
    // starttime %llu (was %lu before Linux 2.6)
    //          The time in jiffies the process started after system boot.
    long long starttime;
    // vsize %lu   Virtual memory size in bytes.
    unsigned long vsize;
    // rss %ld     Resident Set Size: number of pages the process has in real memory.  This is just the pages which count
    //             towards text, data, or stack space.  This does not include pages which have not been demand-loaded in,
    //             or which are swapped out.
    long rss;
    // rsslim %lu  Current  soft limit in bytes on the rss of the process; see the description of RLIMIT_RSS in getprior-
    //             ity(2).
    unsigned long rsslim;
    // startcode %lu
    //             The address above which program text can run.
    unsigned long startcode;
    // endcode %lu The address below which program text can run.
    unsigned long endcode;
    // startstack %lu
    //             The address of the start (i.e., bottom) of the stack.
    unsigned long startstack;
    // kstkesp %lu The current value of ESP (stack pointer), as found in the kernel stack page for the process.
    unsigned long kstkesp;
    // kstkeip %lu The current EIP (instruction pointer).
    unsigned long kstkeip;
    // signal %lu  The bitmap of pending signals, displayed as a decimal number.  Obsolete, because it does  not  provide
    //             information on real-time signals; use /proc/[pid]/status instead.
    unsigned long signal;
    // blocked %lu The  bitmap  of blocked signals, displayed as a decimal number.  Obsolete, because it does not provide
    //             information on real-time signals; use /proc/[pid]/status instead.
    unsigned long blocked;
    // sigignore %lu
    //             The bitmap of ignored signals, displayed as a decimal number.  Obsolete, because it does  not  provide
    //             information on real-time signals; use /proc/[pid]/status instead.
    unsigned long sigignore;
    // sigcatch %lu
    //             The  bitmap  of  caught signals, displayed as a decimal number.  Obsolete, because it does not provide
    //             information on real-time signals; use /proc/[pid]/status instead.
    unsigned long sigcatch;
    // wchan %lu   This is the "channel" in which the process is waiting.  It is the address of a system call, and can be
    //             looked  up in a namelist if you need a textual name.  (If you have an up-to-date /etc/psdatabase, then
    //             try ps -l to see the WCHAN field in action.)
    unsigned long wchan;
    // nswap %lu   Number of pages swapped (not maintained).
    unsigned long nswap;
    // cnswap %lu  Cumulative nswap for child processes (not maintained).
    unsigned long cnswap;
    // exit_signal %d (since Linux 2.1.22)
    //             Signal to be sent to parent when we die.
    int exit_signal;
    // processor %d (since Linux 2.2.8)
    //             CPU number last executed on.
    int processor;
    // rt_priority %u (since Linux 2.5.19; was %lu before Linux 2.6.22)
    //             Real-time scheduling priority, a number in the range 1 to 99 for processes scheduled under a real-time
    //             policy, or 0, for non-real-time processes (see sched_setscheduler(2)).
    unsigned int rt_priority;
    // policy %u (since Linux 2.5.19; was %lu before Linux 2.6.22)
    //             Scheduling policy (see sched_setscheduler(2)).  Decode using the SCHED_* constants in linux/sched.h.
    unsigned int policy;
    // delayacct_blkio_ticks %llu (since Linux 2.6.18)
    //             Aggregated block I/O delays, measured in clock ticks (centiseconds).
    unsigned long long delayacct_blkio_ticks;
    // guest_time %lu (since Linux 2.6.24)
    //             Guest time of the process (time spent running a virtual CPU for a guest operating system), measured in
    //             clock ticks (divide by sysconf(_SC_CLK_TCK).
    unsigned long guest_time;
    // cguest_time %ld (since Linux 2.6.24)
    //             Guest time of the process’s children, measured in clock ticks (divide by sysconf(_SC_CLK_TCK).
    long cguest_time;
    
    SrsProcSelfStat();
};

// to stat the cpu time.
// @see: man 5 proc, /proc/stat
/**
* about the cpu time, @see: http://stackoverflow.com/questions/16011677/calculating-cpu-usage-using-proc-files
* for example, for ossrs.net, a single cpu machine:
*       [winlin@SRS ~]$ cat /proc/uptime && cat /proc/stat
*           5275153.01 4699624.99
*           cpu  43506750 973 8545744 466133337 4149365 190852 804666 0 0
* where the uptime is 5275153.01s
* generally, USER_HZ sysconf(_SC_CLK_TCK)=100, which means the unit of /proc/stat is "1/100ths seconds"
*       that is, USER_HZ=1/100 seconds
* cpu total = 43506750+973+8545744+466133337+4149365+190852+804666+0+0 (USER_HZ)
*           = 523331687 (USER_HZ)
*           = 523331687 * 1/100 (seconds)
*           = 5233316.87 seconds
* the cpu total seconds almost the uptime, the delta is more precise.
*/
class SrsProcSystemStat
{
public:
    // whether the data is ok.
    bool ok;
    // the time in ms when sample.
    int64_t sample_time;
    // the percent of usage. 0.153 is 15.3%.
    float percent;
    
    // always be cpu
    char label[32];
    
    // The amount of time, measured in units  of  USER_HZ  
    // (1/100ths  of  a  second  on  most  architectures,  use
    // sysconf(_SC_CLK_TCK)  to  obtain  the  right value)
    //
    // the system spent in user mode, 
    unsigned long user;
    // user mode with low priority (nice), 
    unsigned long nice;
    // system mode, 
    unsigned long sys;
    // and the idle task, respectively.
    unsigned long idle;

    // In  Linux 2.6 this line includes three additional columns:
    //
    // iowait - time waiting for I/O to complete (since 2.5.41);
    unsigned long iowait;
    // irq - time servicing interrupts (since 2.6.0-test4); 
    unsigned long irq;
    // softirq  -  time  servicing  softirqs  (since 2.6.0-test4).
    unsigned long softirq;
    
    // Since  Linux 2.6.11, there is an eighth column,
    // steal - stolen time, which is the time spent in other oper-
    // ating systems when running in a virtualized environment
    unsigned long steal;
    
    // Since Linux 2.6.24, there is a ninth column, 
    // guest, which is the time spent running a virtual CPU for guest
    // operating systems under the control of the Linux kernel.
    unsigned long guest;

    SrsProcSystemStat();
};

// get system cpu stat, use cache to avoid performance problem.
extern SrsProcSelfStat* srs_get_self_proc_stat();
// get system cpu stat, use cache to avoid performance problem.
extern SrsProcSystemStat* srs_get_system_proc_stat();
// the deamon st-thread will update it.
extern void srs_update_proc_stat();

// stat system memory info
// @see: cat /proc/meminfo 
class SrsMemInfo
{
public:
    // whether the data is ok.
    bool ok;
    // the time in ms when sample.
    int64_t sample_time;
    // the percent of usage. 0.153 is 15.3%.
    float percent_ram;
    float percent_swap;
    
    // MemActive = MemTotal - MemFree
    int64_t MemActive;
    // RealInUse = MemActive - Buffers - Cached
    int64_t RealInUse;
    // NotInUse = MemTotal - RealInUse
    //          = MemTotal - MemActive + Buffers + Cached
    //          = MemTotal - MemTotal + MemFree + Buffers + Cached
    //          = MemFree + Buffers + Cached
    int64_t NotInUse;
    
    int64_t MemTotal;
    int64_t MemFree;
    int64_t Buffers;
    int64_t Cached;
    int64_t SwapTotal;
    int64_t SwapFree;
    
    SrsMemInfo();
};

// get system meminfo, use cache to avoid performance problem.
extern SrsMemInfo* srs_get_meminfo();
// the deamon st-thread will update it.
extern void srs_update_meminfo();

// system cpu hardware info.
// @see: cat /proc/cpuinfo 
class SrsCpuInfo
{
public:
    // whether the data is ok.
    bool ok;
    
    // The number of processors configured.
    int nb_processors;
    // The number of processors currently online (available).
    int nb_processors_online;
    
    SrsCpuInfo();
};

// get system cpu info, use cache to avoid performance problem.
extern SrsCpuInfo* srs_get_cpuinfo();

// platform(os, srs) uptime/load summary
class SrsPlatformInfo
{
public:
    // whether the data is ok.
    bool ok;
    
    // srs startup time, in ms.
    int64_t srs_startup_time;
    
    // @see: cat /proc/uptime
    // system startup time in seconds.
    double os_uptime;
    // system all cpu idle time in seconds.
    // @remark to cal the cpu ustime percent:
    //      os_ilde_time % (os_uptime * SrsCpuInfo.nb_processors_online)
    double os_ilde_time;
    
    // @see: cat /proc/loadavg
    double load_one_minutes;
    double load_five_minutes;
    double load_fifteen_minutes;
    
    SrsPlatformInfo();
};

// get platform info, use cache to avoid performance problem.
extern SrsPlatformInfo* srs_get_platform_info();
// the deamon st-thread will update it.
extern void srs_update_platform_info();

// network device summary for each network device,
// for example, eth0, eth1, ethN
class SrsNetworkDevices
{
public:
    // whether the network device is ok.
    bool ok;
    
    // 6-chars interfaces name
    char name[7];
    // the sample time in ms.
    int64_t sample_time;
    
    // data for receive.
    unsigned long long rbytes;
    unsigned long rpackets;
    unsigned long rerrs;
    unsigned long rdrop;
    unsigned long rfifo;
    unsigned long rframe;
    unsigned long rcompressed;
    unsigned long rmulticast;
    
    // data for transmit
    unsigned long long sbytes;
    unsigned long spackets;
    unsigned long serrs;
    unsigned long sdrop;
    unsigned long sfifo;
    unsigned long scolls;
    unsigned long scarrier;
    unsigned long scompressed;
    
    SrsNetworkDevices();
};

// get network devices info, use cache to avoid performance problem.
extern SrsNetworkDevices* srs_get_network_devices();
extern int srs_get_network_devices_count();
// the deamon st-thread will update it.
extern void srs_update_network_devices();

// system connections, and srs rtmp network summary
class SrsNetworkRtmpServer
{
public:
    // whether the network device is ok.
    bool ok;
    
    // the sample time in ms.
    int64_t sample_time;
    
    // data for receive.
    int64_t rbytes;
    int rkbps;
    int rkbps_30s;
    int rkbps_5m;
    
    // data for transmit
    int64_t sbytes;
    int skbps;
    int skbps_30s;
    int skbps_5m;
    
    // connections
    int nb_conn_sys;
    int nb_conn_sys_et; // established
    int nb_conn_sys_tw; // time wait
    int nb_conn_sys_ls; // listen
    int nb_conn_srs;
    
    SrsNetworkRtmpServer();
};
    
// get network devices info, use cache to avoid performance problem.
extern SrsNetworkRtmpServer* srs_get_network_rtmp_server();
// the deamon st-thread will update it.
extern void srs_update_rtmp_server(int nb_conn, SrsKbps* kbps);

// get local ip, fill to @param ips
extern void srs_retrieve_local_ipv4_ips();
extern std::vector<std::string>& srs_get_local_ipv4_ips();

// get local or peer ip.
// where local ip is the server ip which client connected.
std::string srs_get_local_ip(int fd);
// where peer ip is the client public ip which connected to server.
std::string srs_get_peer_ip(int fd);

// dump summaries for /api/v1/summaries.
void srs_api_dump_summaries(std::stringstream& ss);

#endif
