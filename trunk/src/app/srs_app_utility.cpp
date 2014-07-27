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

#include <srs_app_utility.hpp>

#include <sys/types.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

using namespace std;

#include <srs_kernel_log.hpp>
#include <srs_app_config.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_kernel_error.hpp>
#include <srs_app_kbps.hpp>
#include <srs_app_json.hpp>
#include <srs_kernel_consts.hpp>

int srs_socket_connect(std::string server, int port, int64_t timeout, st_netfd_t* pstfd)
{
    int ret = ERROR_SUCCESS;
    
    *pstfd = NULL;
    st_netfd_t stfd = NULL;
    sockaddr_in addr;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        ret = ERROR_SOCKET_CREATE;
        srs_error("create socket error. ret=%d", ret);
        return ret;
    }
    
    srs_assert(!stfd);
    stfd = st_netfd_open_socket(sock);
    if(stfd == NULL){
        ret = ERROR_ST_OPEN_SOCKET;
        srs_error("st_netfd_open_socket failed. ret=%d", ret);
        return ret;
    }
    
    // connect to server.
    std::string ip = srs_dns_resolve(server);
    if (ip.empty()) {
        ret = ERROR_SYSTEM_IP_INVALID;
        srs_error("dns resolve server error, ip empty. ret=%d", ret);
        goto failed;
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    if (st_connect(stfd, (const struct sockaddr*)&addr, sizeof(sockaddr_in), timeout) == -1){
        ret = ERROR_ST_CONNECT;
        srs_error("connect to server error. ip=%s, port=%d, ret=%d", ip.c_str(), port, ret);
        goto failed;
    }
    srs_info("connect ok. server=%s, ip=%s, port=%d", server.c_str(), ip.c_str(), port);
    
    *pstfd = stfd;
    return ret;
    
failed:
    if (stfd) {
        srs_close_stfd(stfd);
    }
    return ret;
}

int srs_get_log_level(std::string level)
{
    if ("verbose" == _srs_config->get_log_level()) {
        return SrsLogLevel::Verbose;
    } else if ("info" == _srs_config->get_log_level()) {
        return SrsLogLevel::Info;
    } else if ("trace" == _srs_config->get_log_level()) {
        return SrsLogLevel::Trace;
    } else if ("warn" == _srs_config->get_log_level()) {
        return SrsLogLevel::Warn;
    } else if ("error" == _srs_config->get_log_level()) {
        return SrsLogLevel::Error;
    } else {
        return SrsLogLevel::Trace;
    }
}

static SrsRusage _srs_system_rusage;

SrsRusage::SrsRusage()
{
    ok = false;
    sample_time = 0;
    memset(&r, 0, sizeof(rusage));
}

SrsRusage* srs_get_system_rusage()
{
    return &_srs_system_rusage;
}

void srs_update_system_rusage()
{
    if (getrusage(RUSAGE_SELF, &_srs_system_rusage.r) < 0) {
        srs_warn("getrusage failed, ignore");
        return;
    }
    
    _srs_system_rusage.sample_time = srs_get_system_time_ms();
    
    _srs_system_rusage.ok = true;
}

static SrsProcSelfStat _srs_system_cpu_self_stat;
static SrsProcSystemStat _srs_system_cpu_system_stat;

SrsProcSelfStat::SrsProcSelfStat()
{
    ok = false;
    sample_time = 0;
    percent = 0;
    
    pid = 0;
    memset(comm, 0, sizeof(comm));
    state = 0;
    ppid = 0;
    pgrp = 0;
    session = 0;
    tty_nr = 0;
    tpgid = 0;
    flags = 0;
    minflt = 0;
    cminflt = 0;
    majflt = 0;
    cmajflt = 0;
    utime = 0;
    stime = 0;
    cutime = 0;
    cstime = 0;
    priority = 0;
    nice = 0;
    num_threads = 0;
    itrealvalue = 0;
    starttime = 0;
    vsize = 0;
    rss = 0;
    rsslim = 0;
    startcode = 0;
    endcode = 0;
    startstack = 0;
    kstkesp = 0;
    kstkeip = 0;
    signal = 0;
    blocked = 0;
    sigignore = 0;
    sigcatch = 0;
    wchan = 0;
    nswap = 0;
    cnswap = 0;
    exit_signal = 0;
    processor = 0;
    rt_priority = 0;
    policy = 0;
    delayacct_blkio_ticks = 0;
    guest_time = 0;
    cguest_time = 0;
}

SrsProcSystemStat::SrsProcSystemStat()
{
    ok = false;
    sample_time = 0;
    percent = 0;
    total_delta = 0;
    user = 0;
    nice = 0;
    sys = 0;
    idle = 0;
    iowait = 0;
    irq = 0;
    softirq = 0;
    steal = 0;
    guest = 0;
}

int64_t SrsProcSystemStat::total()
{
    return user + nice + sys + idle + iowait + irq + softirq + steal + guest;
}

SrsProcSelfStat* srs_get_self_proc_stat()
{
    return &_srs_system_cpu_self_stat;
}

SrsProcSystemStat* srs_get_system_proc_stat()
{
    return &_srs_system_cpu_system_stat;
}

bool get_proc_system_stat(SrsProcSystemStat& r)
{
    FILE* f = fopen("/proc/stat", "r");
    if (f == NULL) {
        srs_warn("open system cpu stat failed, ignore");
        return false;
    }
    
    r.ok = false;
    
    static char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        if (strncmp(buf, "cpu ", 4) != 0) {
            continue;
        }
        
        int ret = sscanf(buf, "cpu %llu %llu %llu %llu %llu "
            "%llu %llu %llu %llu\n", 
            &r.user, &r.nice, &r.sys, &r.idle, &r.iowait,
            &r.irq, &r.softirq, &r.steal, &r.guest);
        srs_assert(ret == 9);

        // matched ok.
        r.ok = true;

        break;
    }
    
    fclose(f);
    
    return r.ok;
}

bool get_proc_self_stat(SrsProcSelfStat& r)
{
    FILE* f = fopen("/proc/self/stat", "r");
    if (f == NULL) {
        srs_warn("open self cpu stat failed, ignore");
        return false;
    }
    
    int ret = fscanf(f, "%d %32s %c %d %d %d %d "
        "%d %u %lu %lu %lu %lu "
        "%lu %lu %ld %ld %ld %ld "
        "%ld %ld %llu %lu %ld "
        "%lu %lu %lu %lu %lu "
        "%lu %lu %lu %lu %lu "
        "%lu %lu %lu %d %d "
        "%u %u %llu "
        "%lu %ld", 
        &r.pid, r.comm, &r.state, &r.ppid, &r.pgrp, &r.session, &r.tty_nr,
        &r.tpgid, &r.flags, &r.minflt, &r.cminflt, &r.majflt, &r.cmajflt,
        &r.utime, &r.stime, &r.cutime, &r.cstime, &r.priority, &r.nice,
        &r.num_threads, &r.itrealvalue, &r.starttime, &r.vsize, &r.rss,
        &r.rsslim, &r.startcode, &r.endcode, &r.startstack, &r.kstkesp,
        &r.kstkeip, &r.signal, &r.blocked, &r.sigignore, &r.sigcatch,
        &r.wchan, &r.nswap, &r.cnswap, &r.exit_signal, &r.processor,
        &r.rt_priority, &r.policy, &r.delayacct_blkio_ticks, 
        &r.guest_time, &r.cguest_time);
    
    fclose(f);
        
    if (ret >= 0) {
        r.ok = true;
    }
    
    return r.ok;
}

void srs_update_proc_stat()
{
    // always assert the USER_HZ is 1/100ths
    // @see: http://stackoverflow.com/questions/7298646/calculating-user-nice-sys-idle-iowait-irq-and-sirq-from-proc-stat/7298711
    static bool user_hz_assert = false;
    if (!user_hz_assert) {
        user_hz_assert = true;
        
        int USER_HZ = sysconf(_SC_CLK_TCK);
        srs_trace("USER_HZ=%d", USER_HZ);
        srs_assert(USER_HZ == 100);
    }
    
    // system cpu stat
    if (true) {
        SrsProcSystemStat r;
        if (!get_proc_system_stat(r)) {
            return;
        }
        
        r.sample_time = srs_get_system_time_ms();
        
        // calc usage in percent
        SrsProcSystemStat& o = _srs_system_cpu_system_stat;
        
        // @see: http://blog.csdn.net/nineday/article/details/1928847
        // @see: http://stackoverflow.com/questions/16011677/calculating-cpu-usage-using-proc-files
        if (o.total() > 0) {
            r.total_delta = r.total() - o.total();
        }
        if (r.total_delta > 0) {
            int64_t idle = r.idle - o.idle;
            r.percent = (float)(1 - idle / (double)r.total_delta);
        }
        
        // upate cache.
        _srs_system_cpu_system_stat = r;
    }
    
    // self cpu stat
    if (true) {
        SrsProcSelfStat r;
        if (!get_proc_self_stat(r)) {
            return;
        }
        
        r.sample_time = srs_get_system_time_ms();
        
        // calc usage in percent
        SrsProcSelfStat& o = _srs_system_cpu_self_stat;
        
        // @see: http://stackoverflow.com/questions/16011677/calculating-cpu-usage-using-proc-files
        int64_t total = r.sample_time - o.sample_time;
        int64_t usage = (r.utime + r.stime) - (o.utime + o.stime);
        if (total > 0) {
            r.percent = (float)(usage * 1000 / (double)total / 100);
        }
        
        // upate cache.
        _srs_system_cpu_self_stat = r;
    }
}

SrsDiskStat::SrsDiskStat()
{
    ok = false;
    sample_time = 0;
    in_KBps = out_KBps = 0;
    busy = 0;
    
    pgpgin = 0;
    pgpgout = 0;
    
    rd_ios = rd_merges = 0;
    rd_sectors = 0;
    rd_ticks = 0;
    
    wr_ios = wr_merges = 0;
    wr_sectors = 0;
    wr_ticks = nb_current = ticks = aveq = 0;
}

static SrsDiskStat _srs_disk_stat;

SrsDiskStat* srs_get_disk_stat()
{
    return &_srs_disk_stat;
}

bool srs_get_disk_vmstat_stat(SrsDiskStat& r)
{
    FILE* f = fopen("/proc/vmstat", "r");
    if (f == NULL) {
        srs_warn("open vmstat failed, ignore");
        return false;
    }
    
    r.ok = false;
    r.sample_time = srs_get_system_time_ms();
    
    static char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        unsigned long value = 0;
        int ret = sscanf(buf, "%*s %lu\n", &value);
        srs_assert(ret == 1);

        if (strncmp(buf, "pgpgin ", 7) == 0) {
            r.pgpgin = value;
        } else if (strncmp(buf, "pgpgout ", 8) == 0) {
            r.pgpgout = value;
        }
    }
    
    fclose(f);
    
    r.ok = true;
    
    return true;
}

bool srs_get_disk_diskstats_stat(SrsDiskStat& r)
{
    r.ok = false;
    r.sample_time = srs_get_system_time_ms();
    
    // if disabled, ignore all devices.
    SrsConfDirective* conf = _srs_config->get_stats_disk_device();
    if (conf == NULL) {
        r.ok = true;
        return true;
    }
    
    FILE* f = fopen("/proc/diskstats", "r");
    if (f == NULL) {
        srs_warn("open vmstat failed, ignore");
        return false;
    }
    
    static char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        unsigned int major = 0;
        unsigned int minor = 0;
        static char name[32];
        unsigned int rd_ios = 0;
        unsigned int rd_merges = 0;
        unsigned long long rd_sectors = 0;
        unsigned int rd_ticks = 0;
        unsigned int wr_ios = 0;
        unsigned int wr_merges = 0;
        unsigned long long wr_sectors = 0;
        unsigned int wr_ticks = 0;
        unsigned int nb_current = 0;
        unsigned int ticks = 0;
        unsigned int aveq = 0;
        memset(name, sizeof(name), 0);
        int ret = sscanf(buf, 
            "%4d %4d %31s %u %u %llu %u %u %u %llu %u %u %u %u", 
            &major, &minor, name, &rd_ios, &rd_merges,
            &rd_sectors, &rd_ticks, &wr_ios, &wr_merges,
            &wr_sectors, &wr_ticks, &nb_current, &ticks, &aveq);
        srs_assert(ret == 14);

        for (int i = 0; i < (int)conf->args.size(); i++) {
            string name_ok = conf->args.at(i);
            
            if (strcmp(name_ok.c_str(), name) != 0) {
                continue;
            }
            
            r.rd_ios += rd_ios;
            r.rd_merges += rd_merges;
            r.rd_sectors += rd_sectors;
            r.rd_ticks += rd_ticks;
            r.wr_ios += wr_ios;
            r.wr_merges += wr_merges;
            r.wr_sectors += wr_sectors;
            r.wr_ticks += wr_ticks;
            r.nb_current += nb_current;
            r.ticks += ticks;
            r.aveq += aveq;
            
            break;
        }
    }
    
    fclose(f);
    
    r.ok = true;
    
    return true;
}

void srs_update_disk_stat()
{
    SrsDiskStat r;
    if (!srs_get_disk_vmstat_stat(r)) {
        return;
    }
    if (!srs_get_disk_diskstats_stat(r)) {
        return;
    }
    if (!get_proc_system_stat(r.cpu)) {
        return;
    }
    
    SrsDiskStat& o = _srs_disk_stat;
    if (!o.ok) {
        _srs_disk_stat = r;
        return;
    }
    
    // vmstat
    if (true) {
        int64_t duration_ms = r.sample_time - o.sample_time;
        
        if (o.pgpgin > 0 && r.pgpgin > o.pgpgin && duration_ms > 0) {
            // KBps = KB * 1000 / ms = KB/s
            r.in_KBps = (r.pgpgin - o.pgpgin) * 1000 / duration_ms;
        }
        
        if (o.pgpgout > 0 && r.pgpgout > o.pgpgout && duration_ms > 0) {
            // KBps = KB * 1000 / ms = KB/s
            r.out_KBps = (r.pgpgout - o.pgpgout) * 1000 / duration_ms;
        }
    }
    
    // diskstats
    if (r.cpu.ok && o.cpu.ok) {
        SrsCpuInfo* cpuinfo = srs_get_cpuinfo();
        r.cpu.total_delta = r.cpu.total() - o.cpu.total();
        
        if (r.cpu.ok && r.cpu.total_delta > 0
            && cpuinfo->ok && cpuinfo->nb_processors > 0
            && o.ticks < r.ticks
        ) {
            // @see: print_partition_stats() of iostat.c
            double delta_ms = r.cpu.total_delta * 10 / cpuinfo->nb_processors;
            unsigned int ticks = r.ticks - o.ticks;
            
            // busy in [0, 1], where 0.1532 means 15.32%
            r.busy = (float)(ticks / delta_ms);
        }
    }
    
    _srs_disk_stat = r;
}

SrsMemInfo::SrsMemInfo()
{
    ok = false;
    sample_time = 0;
    
    percent_ram = 0;
    percent_swap = 0;
    
    MemActive = 0;
    RealInUse = 0;
    NotInUse = 0;
    MemTotal = 0;
    MemFree = 0;
    Buffers = 0;
    Cached = 0;
    SwapTotal = 0;
    SwapFree = 0;
}

static SrsMemInfo _srs_system_meminfo;

SrsMemInfo* srs_get_meminfo()
{
    return &_srs_system_meminfo;
}

void srs_update_meminfo()
{
    FILE* f = fopen("/proc/meminfo", "r");
    if (f == NULL) {
        srs_warn("open meminfo failed, ignore");
        return;
    }
    
    SrsMemInfo& r = _srs_system_meminfo;
    r.ok = false;
    
    static char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        static unsigned long value;
        int ret = sscanf(buf, "%*s %lu", &value);
        srs_assert(ret == 1);
        
        if (strncmp(buf, "MemTotal:", 9) == 0) {
            r.MemTotal = value;
        } else if (strncmp(buf, "MemFree:", 8) == 0) {
            r.MemFree = value;
        } else if (strncmp(buf, "Buffers:", 8) == 0) {
            r.Buffers = value;
        } else if (strncmp(buf, "Cached:", 7) == 0) {
            r.Cached = value;
        } else if (strncmp(buf, "SwapTotal:", 10) == 0) {
            r.SwapTotal = value;
        } else if (strncmp(buf, "SwapFree:", 9) == 0) {
            r.SwapFree = value;
        }
    }
    
    fclose(f);
    
    r.sample_time = srs_get_system_time_ms();
    r.MemActive = r.MemTotal - r.MemFree;
    r.RealInUse = r.MemActive - r.Buffers - r.Cached;
    r.NotInUse = r.MemTotal - r.RealInUse;
    
    r.ok = true;
    if (r.MemTotal > 0) {
        r.percent_ram = (float)(r.RealInUse / (double)r.MemTotal);
    }
    if (r.SwapTotal > 0) {
        r.percent_swap = (float)((r.SwapTotal - r.SwapFree) / (double)r.SwapTotal);
    }
}

SrsCpuInfo::SrsCpuInfo()
{
    ok = false;
    
    nb_processors = 0;
    nb_processors_online = 0;
}

SrsCpuInfo* srs_get_cpuinfo()
{
    static SrsCpuInfo* cpu = NULL;
    if (cpu != NULL) {
        return cpu;
    }
    
    // initialize cpu info.
    cpu = new SrsCpuInfo();
    cpu->ok = true;
    cpu->nb_processors = sysconf(_SC_NPROCESSORS_CONF);
    cpu->nb_processors_online = sysconf(_SC_NPROCESSORS_ONLN);
    
    return cpu;
}

SrsPlatformInfo::SrsPlatformInfo()
{
    ok = false;
    
    srs_startup_time = 0;
    
    os_uptime = 0;
    os_ilde_time = 0;
    
    load_one_minutes = 0;
    load_five_minutes = 0;
    load_fifteen_minutes = 0;
}

static SrsPlatformInfo _srs_system_platform_info;

SrsPlatformInfo* srs_get_platform_info()
{
    return &_srs_system_platform_info;
}

void srs_update_platform_info()
{
    SrsPlatformInfo& r = _srs_system_platform_info;
    r.ok = true;
    
    r.srs_startup_time = srs_get_system_startup_time_ms();
    
    if (true) {
        FILE* f = fopen("/proc/uptime", "r");
        if (f == NULL) {
            srs_warn("open uptime failed, ignore");
            return;
        }
        
        int ret = fscanf(f, "%lf %lf\n", &r.os_uptime, &r.os_ilde_time);
    
        fclose(f);

        if (ret < 0) {
            r.ok = false;
        }
    }
    
    if (true) {
        FILE* f = fopen("/proc/loadavg", "r");
        if (f == NULL) {
            srs_warn("open loadavg failed, ignore");
            return;
        }
        
        int ret = fscanf(f, "%lf %lf %lf\n", 
            &r.load_one_minutes, &r.load_five_minutes, &r.load_fifteen_minutes);
    
        fclose(f);

        if (ret < 0) {
            r.ok = false;
        }
    }
}

SrsNetworkDevices::SrsNetworkDevices()
{
    ok = false;
    
    memset(name, 0, sizeof(name));
    sample_time = 0;
    
    rbytes = 0;
    rpackets = 0;
    rerrs = 0;
    rdrop = 0;
    rfifo = 0;
    rframe = 0;
    rcompressed = 0;
    rmulticast = 0;
    
    sbytes = 0;
    spackets = 0;
    serrs = 0;
    sdrop = 0;
    sfifo = 0;
    scolls = 0;
    scarrier = 0;
    scompressed = 0;
}

#define MAX_NETWORK_DEVICES_COUNT 16
static SrsNetworkDevices _srs_system_network_devices[MAX_NETWORK_DEVICES_COUNT];
static int _nb_srs_system_network_devices = -1;

SrsNetworkDevices* srs_get_network_devices()
{
    return _srs_system_network_devices;
}

int srs_get_network_devices_count()
{
    return _nb_srs_system_network_devices;
}

void srs_update_network_devices()
{
    if (true) {
        FILE* f = fopen("/proc/net/dev", "r");
        if (f == NULL) {
            srs_warn("open proc network devices failed, ignore");
            return;
        }
        
        // ignore title.
        static char buf[1024];
        fgets(buf, sizeof(buf), f);
        fgets(buf, sizeof(buf), f);
    
        for (int i = 0; i < MAX_NETWORK_DEVICES_COUNT; i++) {
            SrsNetworkDevices& r = _srs_system_network_devices[i];
            r.ok = false;
            r.sample_time = 0;
    
            int ret = fscanf(f, "%6[^:]:%llu %lu %lu %lu %lu %lu %lu %lu %llu %lu %lu %lu %lu %lu %lu %lu\n", 
                r.name, &r.rbytes, &r.rpackets, &r.rerrs, &r.rdrop, &r.rfifo, &r.rframe, &r.rcompressed, &r.rmulticast,
                &r.sbytes, &r.spackets, &r.serrs, &r.sdrop, &r.sfifo, &r.scolls, &r.scarrier, &r.scompressed);
                
            if (ret == 17) {
                r.ok = true;
                r.name[sizeof(r.name) - 1] = 0;
                _nb_srs_system_network_devices = i + 1;
                r.sample_time = srs_get_system_time_ms();
            }
            
            if (ret == EOF) {
                break;
            }
        }
    
        fclose(f);
    }
}

SrsNetworkRtmpServer::SrsNetworkRtmpServer()
{
    ok = false;
    sample_time = rbytes = sbytes = 0;
    nb_conn_sys = nb_conn_srs = 0;
    nb_conn_sys_et = nb_conn_sys_tw = nb_conn_sys_ls = 0;
    nb_conn_sys_udp = 0;
}

static SrsNetworkRtmpServer _srs_network_rtmp_server;

SrsNetworkRtmpServer* srs_get_network_rtmp_server()
{
    return &_srs_network_rtmp_server;
}

// @see: http://stackoverflow.com/questions/5992211/list-of-possible-internal-socket-statuses-from-proc
enum {
    SYS_TCP_ESTABLISHED =      0x01,
    SYS_TCP_SYN_SENT,       // 0x02
    SYS_TCP_SYN_RECV,       // 0x03
    SYS_TCP_FIN_WAIT1,      // 0x04
    SYS_TCP_FIN_WAIT2,      // 0x05
    SYS_TCP_TIME_WAIT,      // 0x06
    SYS_TCP_CLOSE,          // 0x07
    SYS_TCP_CLOSE_WAIT,     // 0x08
    SYS_TCP_LAST_ACK,       // 0x09
    SYS_TCP_LISTEN,         // 0x0A
    SYS_TCP_CLOSING,        // 0x0B /* Now a valid state */

    SYS_TCP_MAX_STATES      // 0x0C /* Leave at the end! */
};

void srs_update_rtmp_server(int nb_conn, SrsKbps* kbps)
{
    SrsNetworkRtmpServer& r = _srs_network_rtmp_server;
    
    // reset total.
    r.nb_conn_sys = 0;
    
    if (true) {
        FILE* f = fopen("/proc/net/tcp", "r");
        if (f == NULL) {
            srs_warn("open proc network tcp failed, ignore");
            return;
        }
        
        // ignore title.
        static char buf[1024];
        fgets(buf, sizeof(buf), f);
    
        int nb_conn_sys_established = 0;
        int nb_conn_sys_time_wait = 0;
        int nb_conn_sys_listen = 0;
        int nb_conn_sys_other = 0;
        
        // @see: http://tester-higkoo.googlecode.com/svn-history/r14/trunk/Tools/iostat/iostat.c
        while (fgets(buf, sizeof(buf), f)) {
            int st = 0;
            int ret = sscanf(buf, "%*s %*s %*s %2x\n", &st);
            
            if (ret == 1) {
                if (st == SYS_TCP_ESTABLISHED) {
                    nb_conn_sys_established++;
                } else if (st == SYS_TCP_TIME_WAIT) {
                    nb_conn_sys_time_wait++;
                } else if (st == SYS_TCP_LISTEN) {
                    nb_conn_sys_listen++;
                } else {
                    nb_conn_sys_other++;
                }
            }
            
            if (ret == EOF) {
                break;
            }
        }
        
        r.nb_conn_sys = nb_conn_sys_established + nb_conn_sys_time_wait + nb_conn_sys_listen + nb_conn_sys_other;
        r.nb_conn_sys_et = nb_conn_sys_established;
        r.nb_conn_sys_tw = nb_conn_sys_time_wait;
        r.nb_conn_sys_ls = nb_conn_sys_listen;
    
        fclose(f);
    }
    
    if (true) {
        FILE* f = fopen("/proc/net/udp", "r");
        if (f == NULL) {
            srs_warn("open proc network udp failed, ignore");
            return;
        }
        
        // ignore title.
        static char buf[1024];
        fgets(buf, sizeof(buf), f);
    
        // all udp is close state.
        int nb_conn_sys_close = 0;
        
        // @see: http://tester-higkoo.googlecode.com/svn-history/r14/trunk/Tools/iostat/iostat.c
        while (fgets(buf, sizeof(buf), f)) {
            int st = 0;
            int ret = sscanf(buf, "%*s %*s %*s %2x\n", &st);
            
            if (ret == EOF) {
                break;
            }
            
            nb_conn_sys_close++;
        }
        
        r.nb_conn_sys += nb_conn_sys_close;
        r.nb_conn_sys_udp = nb_conn_sys_close;
    
        fclose(f);
    }
    
    if (true) {
        r.ok = true;
        
        r.nb_conn_srs = nb_conn;
        r.sample_time = srs_get_system_time_ms();
        
        r.rbytes = kbps->get_recv_bytes();
        r.rkbps = kbps->get_recv_kbps();
        r.rkbps_30s = kbps->get_recv_kbps_30s();
        r.rkbps_5m = kbps->get_recv_kbps_5m();
        
        r.sbytes = kbps->get_send_bytes();
        r.skbps = kbps->get_send_kbps();
        r.skbps_30s = kbps->get_send_kbps_30s();
        r.skbps_5m = kbps->get_send_kbps_5m();
    }
}

vector<string> _srs_system_ipv4_ips;

void retrieve_local_ipv4_ips()
{
    vector<string>& ips = _srs_system_ipv4_ips;
    
    ips.clear();
    
    ifaddrs* ifap;
    if (getifaddrs(&ifap) == -1) {
        srs_warn("retrieve local ips, ini ifaddrs failed.");
        return;
    }
    
    ifaddrs* p = ifap;
    while (p != NULL) {
        sockaddr* addr = p->ifa_addr;
        
        // retrieve ipv4 addr
        if (addr->sa_family == AF_INET) {
            in_addr* inaddr = &((sockaddr_in*)addr)->sin_addr;
            
            char buf[16];
            memset(buf, 0, sizeof(buf));
            
            if ((inet_ntop(addr->sa_family, inaddr, buf, sizeof(buf))) == NULL) {
                srs_warn("convert local ip failed");
                break;
            }
            
            std::string ip = buf;
            if (ip != SRS_CONSTS_LOCALHOST) {
                srs_trace("retrieve local ipv4 ip=%s, index=%d", ip.c_str(), (int)ips.size());
                ips.push_back(ip);
            }
        }
        
        p = p->ifa_next;
    }

    freeifaddrs(ifap);
}

vector<string>& srs_get_local_ipv4_ips()
{
    if (_srs_system_ipv4_ips.empty()) {
        retrieve_local_ipv4_ips();
    }

    return _srs_system_ipv4_ips;
}

string srs_get_local_ip(int fd)
{
    std::string ip;

    // discovery client information
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getsockname(fd, (sockaddr*)&addr, &addrlen) == -1) {
        return ip;
    }
    srs_verbose("get local ip success.");

    // ip v4 or v6
    char buf[INET6_ADDRSTRLEN];
    memset(buf, 0, sizeof(buf));

    if ((inet_ntop(addr.sin_family, &addr.sin_addr, buf, sizeof(buf))) == NULL) {
        return ip;
    }

    ip = buf;

    srs_verbose("get local ip of client ip=%s, fd=%d", buf, fd);

    return ip;
}

string srs_get_peer_ip(int fd)
{
    std::string ip;
    
    // discovery client information
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getpeername(fd, (sockaddr*)&addr, &addrlen) == -1) {
        return ip;
    }
    srs_verbose("get peer name success.");

    // ip v4 or v6
    char buf[INET6_ADDRSTRLEN];
    memset(buf, 0, sizeof(buf));
    
    if ((inet_ntop(addr.sin_family, &addr.sin_addr, buf, sizeof(buf))) == NULL) {
        return ip;
    }
    srs_verbose("get peer ip of client ip=%s, fd=%d", buf, fd);
    
    ip = buf;
    
    srs_verbose("get peer ip success. ip=%s, fd=%d", ip, fd);
    
    return ip;
}

void srs_api_dump_summaries(std::stringstream& ss)
{
    SrsRusage* r = srs_get_system_rusage();
    SrsProcSelfStat* u = srs_get_self_proc_stat();
    SrsProcSystemStat* s = srs_get_system_proc_stat();
    SrsCpuInfo* c = srs_get_cpuinfo();
    SrsMemInfo* m = srs_get_meminfo();
    SrsPlatformInfo* p = srs_get_platform_info();
    SrsNetworkDevices* n = srs_get_network_devices();
    SrsNetworkRtmpServer* nrs = srs_get_network_rtmp_server();
    SrsDiskStat* d = srs_get_disk_stat();
    
    float self_mem_percent = 0;
    if (m->MemTotal > 0) {
        self_mem_percent = (float)(r->r.ru_maxrss / (double)m->MemTotal);
    }
    
    int64_t now = srs_get_system_time_ms();
    double srs_uptime = (now - p->srs_startup_time) / 100 / 10.0;
    
    bool n_ok = false;
    int64_t n_sample_time = 0;
    int64_t nr_bytes = 0;
    int64_t ns_bytes = 0;
    int nb_n = srs_get_network_devices_count();
    for (int i = 0; i < nb_n; i++) {
        SrsNetworkDevices& o = n[i];
        
        // ignore the lo interface.
        std::string inter = o.name;
        if (!o.ok || inter == "lo") {
            continue;
        }
        
        n_ok = true;
        nr_bytes += o.rbytes;
        ns_bytes += o.sbytes;
        n_sample_time = o.sample_time;
    }
    
    ss << __SRS_JOBJECT_START
        << __SRS_JFIELD_ERROR(ERROR_SUCCESS) << __SRS_JFIELD_CONT
        << __SRS_JFIELD_ORG("data", __SRS_JOBJECT_START)
            << __SRS_JFIELD_ORG("rusage_ok", (r->ok? "true":"false")) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("self_cpu_stat_ok", (u->ok? "true":"false")) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("system_cpu_stat_ok", (s->ok? "true":"false")) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("cpuinfo_ok", (c->ok? "true":"false")) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("disk_ok", (d->ok? "true":"false")) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("meminfo_ok", (m->ok? "true":"false")) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("platform_ok", (p->ok? "true":"false")) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("network_ok", (n_ok? "true":"false")) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("network_srs_ok", (nrs->ok? "true":"false")) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("now_ms", now) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("self", __SRS_JOBJECT_START)
                << __SRS_JFIELD_STR("version", RTMP_SIG_SRS_VERSION) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("pid", getpid()) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("ppid", u->ppid) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_STR("argv", _srs_config->argv()) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_STR("cwd", _srs_config->cwd()) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("mem_kbyte", r->r.ru_maxrss) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("mem_percent", self_mem_percent) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("cpu_percent", u->percent) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("srs_uptime", srs_uptime)
            << __SRS_JOBJECT_END << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("system", __SRS_JOBJECT_START)
                << __SRS_JFIELD_ORG("cpu_percent", s->percent) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("disk_read_KBps", d->in_KBps) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("disk_write_KBps", d->out_KBps) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("disk_busy_percent", d->busy) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("mem_ram_kbyte", m->MemTotal) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("mem_ram_percent", m->percent_ram) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("mem_swap_kbyte", m->SwapTotal) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("mem_swap_percent", m->percent_swap) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("cpus", c->nb_processors) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("cpus_online", c->nb_processors_online) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("uptime", p->os_uptime) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("ilde_time", p->os_ilde_time) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("load_1m", p->load_one_minutes) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("load_5m", p->load_five_minutes) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("load_15m", p->load_fifteen_minutes) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("net_sample_time", n_sample_time) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("net_recv_bytes", nr_bytes) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("net_send_bytes", ns_bytes) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("srs_sample_time", nrs->sample_time) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("srs_recv_bytes", nrs->rbytes) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("srs_recv_kbps", nrs->rkbps) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("srs_send_bytes", nrs->sbytes) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("srs_send_kbps", nrs->skbps) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("conn_sys", nrs->nb_conn_sys) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("conn_sys_et", nrs->nb_conn_sys_et) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("conn_sys_tw", nrs->nb_conn_sys_tw) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("conn_sys_ls", nrs->nb_conn_sys_ls) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("conn_sys_udp", nrs->nb_conn_sys_udp) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_ORG("conn_srs", nrs->nb_conn_srs)
            << __SRS_JOBJECT_END
        << __SRS_JOBJECT_END
        << __SRS_JOBJECT_END;
}
