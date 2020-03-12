/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 Winlin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SRS_APP_CONFIG_HPP
#define SRS_APP_CONFIG_HPP

#include <srs_core.hpp>

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>

#include <srs_app_reload.hpp>
#include <srs_app_async_call.hpp>
#include <srs_app_thread.hpp>

class SrsRequest;
class SrsFileWriter;
class SrsJsonObject;
class SrsJsonArray;
class SrsJsonAny;

class SrsConfig;
class SrsRequest;
class SrsJsonArray;
class SrsConfDirective;

/**
 * whether the two vector actual equals, for instance,
 *      srs_vector_actual_equals([0, 1, 2], [0, 1, 2])      ==== true
 *      srs_vector_actual_equals([0, 1, 2], [2, 1, 0])      ==== true
 *      srs_vector_actual_equals([0, 1, 2], [0, 2, 1])      ==== true
 *      srs_vector_actual_equals([0, 1, 2], [0, 1, 2, 3])   ==== false
 *      srs_vector_actual_equals([1, 2, 3], [0, 1, 2])      ==== false
 */
template<typename T>
bool srs_vector_actual_equals(const std::vector<T>& a, const std::vector<T>& b)
{
    // all elements of a in b.
    for (int i = 0; i < (int)a.size(); i++) {
        const T& e = a.at(i);
        if (std::find(b.begin(), b.end(), e) == b.end()) {
            return false;
        }
    }

    // all elements of b in a.
    for (int i = 0; i < (int)b.size(); i++) {
        const T& e = b.at(i);
        if (std::find(a.begin(), a.end(), e) == a.end()) {
            return false;
        }
    }

    return true;
}

namespace _srs_internal
{
    // The buffer of config content.
    class SrsConfigBuffer
    {
    protected:
        // The last available position.
        char* last;
        // The end of buffer.
        char* end;
        // The start of buffer.
        char* start;
    public:
        // Current consumed position.
        char* pos;
        // Current parsed line.
        int line;
    public:
        SrsConfigBuffer();
        virtual ~SrsConfigBuffer();
    public:
        // Fullfill the buffer with content of file specified by filename.
        virtual srs_error_t fullfill(const char* filename);
        // Whether buffer is empty.
        virtual bool empty();
    };
};

// Deep compare directive.
extern bool srs_directive_equals(SrsConfDirective* a, SrsConfDirective* b);
extern bool srs_directive_equals(SrsConfDirective* a, SrsConfDirective* b, std::string except);

// The helper utilities, used for compare the consts values.
extern bool srs_config_hls_is_on_error_ignore(std::string strategy);
extern bool srs_config_hls_is_on_error_continue(std::string strategy);
extern bool srs_config_ingest_is_file(std::string type);
extern bool srs_config_ingest_is_stream(std::string type);
extern bool srs_config_dvr_is_plan_segment(std::string plan);
extern bool srs_config_dvr_is_plan_session(std::string plan);
extern bool srs_stream_caster_is_udp(std::string caster);
extern bool srs_stream_caster_is_rtsp(std::string caster);
extern bool srs_stream_caster_is_flv(std::string caster);
// Whether the dvr_apply active the stream specified by req.
extern bool srs_config_apply_filter(SrsConfDirective* dvr_apply, SrsRequest* req);

// Convert bool in str to on/off
extern std::string srs_config_bool2switch(std::string sbool);

// Parse loaded vhost directives to compatible mode.
// For exmaple, SRS1/2 use the follow refer style:
//          refer   a.domain.com b.domain.com;
// while SRS3 use the following:
//          refer {
//              enabled on;
//              all a.domain.com b.domain.com;
//          }
// so we must transform the vhost directive anytime load the config.
// @param root the root directive to transform, in and out parameter.
extern srs_error_t srs_config_transform_vhost(SrsConfDirective* root);

// @global config object.
extern SrsConfig* _srs_config;

// The config directive.
// The config file is a group of directives,
// all directive has name, args and child-directives.
// For example, the following config text:
//      vhost vhost.ossrs.net {
//       enabled         on;
//       ingest livestream {
//      	 enabled      on;
//      	 ffmpeg       /bin/ffmpeg;
//       }
//      }
// will be parsed to:
//      SrsConfDirective: name="vhost", arg0="vhost.ossrs.net", child-directives=[
//          SrsConfDirective: name="enabled", arg0="on", child-directives=[]
//          SrsConfDirective: name="ingest", arg0="livestream", child-directives=[
//              SrsConfDirective: name="enabled", arg0="on", child-directives=[]
//              SrsConfDirective: name="ffmpeg", arg0="/bin/ffmpeg", child-directives=[]
//          ]
//      ]
// @remark, allow empty directive, for example: "dir0 {}"
// @remark, don't allow empty name, for example: ";" or "{dir0 arg0;}
class SrsConfDirective
{
public:
    // The line of config file in which the directive from
    int conf_line;
    // The name of directive, for example, the following config text:
    //       enabled     on;
    // will be parsed to a directive, its name is "enalbed"
    std::string name;
    // The args of directive, for example, the following config text:
    //       listen      1935 1936;
    // will be parsed to a directive, its args is ["1935", "1936"].
    std::vector<std::string> args;
    // The child directives, for example, the following config text:
    //       vhost vhost.ossrs.net {
    //           enabled         on;
    //       }
    // will be parsed to a directive, its directives is a vector contains
    // a directive, which is:
    //       name:"enalbed", args:["on"], directives:[]
    //
    // @remark, the directives can contains directives.
    std::vector<SrsConfDirective*> directives;
public:
    SrsConfDirective();
    virtual ~SrsConfDirective();
public:
    // Deep copy the directive, for SrsConfig to use it to support reload in upyun cluster,
    // For when reload the upyun dynamic config, the root will be changed,
    // so need to copy it to an old root directive, and use the copy result to do reload.
    virtual SrsConfDirective* copy();
    // @param except the name of sub directive.
    virtual SrsConfDirective* copy(std::string except);
// args
public:
    // Get the args0,1,2, if user want to get more args,
    // directly use the args.at(index).
    virtual std::string arg0();
    virtual std::string arg1();
    virtual std::string arg2();
    virtual std::string arg3();
// directives
public:
    // Get the directive by index.
    // @remark, assert the index<directives.size().
    virtual SrsConfDirective* at(int index);
    // Get the directive by name, return the first match.
    virtual SrsConfDirective* get(std::string _name);
    // Get the directive by name and its arg0, return the first match.
    virtual SrsConfDirective* get(std::string _name, std::string _arg0);
    // RAW 
public:
    virtual SrsConfDirective* get_or_create(std::string n);
    virtual SrsConfDirective* get_or_create(std::string n, std::string a0);
    virtual SrsConfDirective* get_or_create(std::string n, std::string a0, std::string a1);
    virtual SrsConfDirective* set_arg0(std::string a0);
    // Remove the v from sub directives, user must free the v.
    virtual void remove(SrsConfDirective* v);
// help utilities
public:
    // Whether current directive is vhost.
    virtual bool is_vhost();
    // Whether current directive is stream_caster.
    virtual bool is_stream_caster();
// Parse utilities
public:
    // Parse config directive from file buffer.
    virtual srs_error_t parse(_srs_internal::SrsConfigBuffer* buffer);
    // Marshal the directive to writer.
    // @param level, the root is level0, all its directives are level1, and so on.
    virtual srs_error_t persistence(SrsFileWriter* writer, int level);
    // Dumps the args[0-N] to array(string).
    virtual SrsJsonArray* dumps_args();
    // Dumps arg0 to str, number or boolean.
    virtual SrsJsonAny* dumps_arg0_to_str();
    virtual SrsJsonAny* dumps_arg0_to_integer();
    virtual SrsJsonAny* dumps_arg0_to_number();
    virtual SrsJsonAny* dumps_arg0_to_boolean();
// private parse.
private:
    // The directive parsing type.
    enum SrsDirectiveType {
        // The root directives, parsing file.
        parse_file,
        // For each direcitve, parsing text block.
        parse_block
    };
    // Parse the conf from buffer. the work flow:
    // 1. read a token(directive args and a ret flag),
    // 2. initialize the directive by args, args[0] is name, args[1-N] is args of directive,
    // 3. if ret flag indicates there are child-directives, read_conf(directive, block) recursively.
    virtual srs_error_t parse_conf(_srs_internal::SrsConfigBuffer* buffer, SrsDirectiveType type);
    // Read a token from buffer.
    // A token, is the directive args and a flag indicates whether has child-directives.
    // @param args, the output directive args, the first is the directive name, left is the args.
    // @param line_start, the actual start line of directive.
    // @return, an error code indicates error or has child-directives.
    virtual srs_error_t read_token(_srs_internal::SrsConfigBuffer* buffer, std::vector<std::string>& args, int& line_start);
};

// The config service provider.
// For the config supports reload, so never keep the reference cross st-thread,
// that is, never save the SrsConfDirective* get by any api of config,
// For it maybe free in the reload st-thread cycle.
// You could keep it before st-thread switch, or simply never keep it.
class SrsConfig
{
// user command
private:
    // Whether srs is run in dolphin mode.
    // @see https://github.com/ossrs/srs-dolphin
    bool dolphin;
    std::string dolphin_rtmp_port;
    std::string dolphin_http_port;
    // Whether show help and exit.
    bool show_help;
    // Whether test config file and exit.
    bool test_conf;
    // Whether show SRS version and exit.
    bool show_version;
    // Whether show SRS signature and exit.
    bool show_signature;
// global env variables.
private:
    // The user parameters, the argc and argv.
    // The argv is " ".join(argv), where argv is from main(argc, argv).
    std::string _argv;
    // current working directory.
    std::string _cwd;
    // Config section
private:
    // The last parsed config file.
    // If  reload, reload the config file.
    std::string config_file;
protected:
    // The directive root.
    SrsConfDirective* root;
// Reload  section
private:
    // The reload subscribers, when reload, callback all handlers.
    std::vector<ISrsReloadHandler*> subscribes;
public:
    SrsConfig();
    virtual ~SrsConfig();
    // dolphin
public:
    // Whether srs is in dolphin mode.
    virtual bool is_dolphin();
// Reload
public:
    // For reload handler to register itself,
    // when config service do the reload, callback the handler.
    virtual void subscribe(ISrsReloadHandler* handler);
    // For reload handler to unregister itself.
    virtual void unsubscribe(ISrsReloadHandler* handler);
    // Reload  the config file.
    // @remark, user can test the config before reload it.
    virtual srs_error_t reload();
private:
    // Reload  the vhost section of config.
    virtual srs_error_t reload_vhost(SrsConfDirective* old_root);
protected:
    // Reload  from the config.
    // @remark, use protected for the utest to override with mock.
    virtual srs_error_t reload_conf(SrsConfig* conf);
private:
    // Reload  the http_api section of config.
    virtual srs_error_t reload_http_api(SrsConfDirective* old_root);
    // Reload  the http_stream section of config.
    // TODO: FIXME: rename to http_server.
    virtual srs_error_t reload_http_stream(SrsConfDirective* old_root);
    // Reload  the transcode section of vhost of config.
    virtual srs_error_t reload_transcode(SrsConfDirective* new_vhost, SrsConfDirective* old_vhost);
    // Reload  the ingest section of vhost of config.
    virtual srs_error_t reload_ingest(SrsConfDirective* new_vhost, SrsConfDirective* old_vhost);
// Parse options and file
public:
    // Parse the cli, the main(argc,argv) function.
    virtual srs_error_t parse_options(int argc, char** argv);
    // initialize the cwd for server,
    // because we may change the workdir.
    virtual srs_error_t initialize_cwd();
    // Marshal current config to file.
    virtual srs_error_t persistence();
private:
    virtual srs_error_t do_persistence(SrsFileWriter* fw);
public:
    // Dumps the global sections to json.
    virtual srs_error_t global_to_json(SrsJsonObject* obj);
    // Dumps the minimal sections to json.
    virtual srs_error_t minimal_to_json(SrsJsonObject* obj);
    // Dumps the vhost section to json.
    virtual srs_error_t vhost_to_json(SrsConfDirective* vhost, SrsJsonObject* obj);
    // Dumps the http_api sections to json for raw api info.
    virtual srs_error_t raw_to_json(SrsJsonObject* obj);
    // RAW  set the global listen.
    virtual srs_error_t raw_set_listen(const std::vector<std::string>& eps, bool& applied);
    // RAW  set the global pid.
    virtual srs_error_t raw_set_pid(std::string pid, bool& applied);
    // RAW  set the global chunk size.
    virtual srs_error_t raw_set_chunk_size(std::string chunk_size, bool& applied);
    // RAW  set the global ffmpeg log dir.
    virtual srs_error_t raw_set_ff_log_dir(std::string ff_log_dir, bool& applied);
    // RAW  set the global log tank.
    virtual srs_error_t raw_set_srs_log_tank(std::string srs_log_tank, bool& applied);
    // RAW  set the global log level.
    virtual srs_error_t raw_set_srs_log_level(std::string srs_log_level, bool& applied);
    // RAW  set the global log file path for file tank.
    virtual srs_error_t raw_set_srs_log_file(std::string srs_log_file, bool& applied);
    // RAW  set the global max connections of srs.
    virtual srs_error_t raw_set_max_connections(std::string max_connections, bool& applied);
    // RAW  set the global whether use utc time.
    virtual srs_error_t raw_set_utc_time(std::string utc_time, bool& applied);
    // RAW  set the global pithy print interval in ms.
    virtual srs_error_t raw_set_pithy_print_ms(std::string pithy_print_ms, bool& applied);
    // RAW  create the new vhost.
    virtual srs_error_t raw_create_vhost(std::string vhost, bool& applied);
    // RAW  update the disabled vhost name.
    virtual srs_error_t raw_update_vhost(std::string vhost, std::string name, bool& applied);
    // RAW  delete the disabled vhost.
    virtual srs_error_t raw_delete_vhost(std::string vhost, bool& applied);
    // RAW  disable the enabled vhost.
    virtual srs_error_t raw_disable_vhost(std::string vhost, bool& applied);
    // RAW  enable the disabled vhost.
    virtual srs_error_t raw_enable_vhost(std::string vhost, bool& applied);
    // RAW  enable the dvr of stream of vhost.
    virtual srs_error_t raw_enable_dvr(std::string vhost, std::string stream, bool& applied);
    // RAW  disable the dvr of stream of vhost.
    virtual srs_error_t raw_disable_dvr(std::string vhost, std::string stream, bool& applied);
private:
    virtual srs_error_t do_reload_listen();
    virtual srs_error_t do_reload_pid();
    virtual srs_error_t do_reload_srs_log_tank();
    virtual srs_error_t do_reload_srs_log_level();
    virtual srs_error_t do_reload_srs_log_file();
    virtual srs_error_t do_reload_max_connections();
    virtual srs_error_t do_reload_utc_time();
    virtual srs_error_t do_reload_pithy_print_ms();
    virtual srs_error_t do_reload_vhost_added(std::string vhost);
    virtual srs_error_t do_reload_vhost_removed(std::string vhost);
    virtual srs_error_t do_reload_vhost_dvr_apply(std::string vhost);
public:
    // Get the config file path.
    virtual std::string config();
private:
    // Parse each argv.
    virtual srs_error_t parse_argv(int& i, char** argv);
    // Print help and exit.
    virtual void print_help(char** argv);
public:
    // Parse the config file, which is specified by cli.
    virtual srs_error_t parse_file(const char* filename);
    // Check the parsed config.
    virtual srs_error_t check_config();
protected:
    virtual srs_error_t check_normal_config();
    virtual srs_error_t check_number_connections();
protected:
    // Parse config from the buffer.
    // @param buffer, the config buffer, user must delete it.
    // @remark, use protected for the utest to override with mock.
    virtual srs_error_t parse_buffer(_srs_internal::SrsConfigBuffer* buffer);
    // global env
public:
    // Get the current work directory.
    virtual std::string cwd();
    // Get the cli, the main(argc,argv), program start command.
    virtual std::string argv();
// global section
public:
    // Get the directive root, corresponding to the config file.
    // The root directive, no name and args, contains directives.
    // All directive parsed can retrieve from root.
    virtual SrsConfDirective* get_root();
    // Get the daemon config.
    // If  true, SRS will run in daemon mode, fork and fork to reap the
    // grand-child process to init process.
    virtual bool get_daemon();
    // Get the max connections limit of system.
    // If  exceed the max connection, SRS will disconnect the connection.
    // @remark, linux will limit the connections of each process,
    //       for example, when you need SRS to service 10000+ connections,
    //       user must use "ulimit -HSn 10000" and config the max connections
    //       of SRS.
    virtual int get_max_connections();
    // Get the listen port of SRS.
    // user can specifies multiple listen ports,
    // each args of directive is a listen port.
    virtual std::vector<std::string> get_listens();
    // Get the pid file path.
    // The pid file is used to save the pid of SRS,
    // use file lock to prevent multiple SRS starting.
    // @remark, if user need to run multiple SRS instance,
    //       for example, to start multiple SRS for multiple CPUs,
    //       user can use different pid file for each process.
    virtual std::string get_pid_file();
    // Get pithy print pulse in srs_utime_t,
    // For example, all rtmp connections only print one message
    // every this interval in ms.
    virtual srs_utime_t get_pithy_print();
    // Whether use utc-time to format the time.
    virtual bool get_utc_time();
    // Get the configed work dir.
    // ignore if empty string.
    virtual std::string get_work_dir();
    // Whether use asprocess mode.
    virtual bool get_asprocess();
    // Whether empty client IP is ok.
    virtual bool empty_ip_ok();
    // Get the start wait in ms for gracefully quit.
    virtual srs_utime_t get_grace_start_wait();
    // Get the final wait in ms for gracefully quit.
    virtual srs_utime_t get_grace_final_wait();
    // Whether force to gracefully quit, never fast quit.
    virtual bool is_force_grace_quit();
    // Whether disable daemon for docker.
    virtual bool disable_daemon_for_docker();
    // Whether use inotify to auto reload by watching config file changes.
    virtual bool inotify_auto_reload();
    // Whether enable auto reload config for docker.
    virtual bool auto_reload_for_docker();
// stream_caster section
public:
    // Get all stream_caster in config file.
    virtual std::vector<SrsConfDirective*> get_stream_casters();
    // Get whether the specified stream_caster is enabled.
    virtual bool get_stream_caster_enabled(SrsConfDirective* conf);
    // Get the engine of stream_caster, the caster config.
    virtual std::string get_stream_caster_engine(SrsConfDirective* conf);
    // Get the output rtmp url of stream_caster, the output config.
    virtual std::string get_stream_caster_output(SrsConfDirective* conf);
    // Get the listen port of stream caster.
    virtual int get_stream_caster_listen(SrsConfDirective* conf);
    // Get the min udp port for rtp of stream caster rtsp.
    virtual int get_stream_caster_rtp_port_min(SrsConfDirective* conf);
    // Get the max udp port for rtp of stream caster rtsp.
    virtual int get_stream_caster_rtp_port_max(SrsConfDirective* conf);
// vhost specified section
public:
    // Get the vhost directive by vhost name.
    // @param vhost, the name of vhost to get.
    // @param try_default_vhost whether try default when get specified vhost failed.
    virtual SrsConfDirective* get_vhost(std::string vhost, bool try_default_vhost = true);
    // Get all vhosts in config file.
    virtual void get_vhosts(std::vector<SrsConfDirective*>& vhosts);
    // Whether vhost is enabled
    // @param vhost, the vhost name.
    // @return true when vhost is ok; otherwise, false.
    virtual bool get_vhost_enabled(std::string vhost);
    // Whether vhost is enabled
    // @param vhost, the vhost directive.
    // @return true when vhost is ok; otherwise, false.
    virtual bool get_vhost_enabled(SrsConfDirective* conf);
    // Whether gop_cache is enabled of vhost.
    // gop_cache used to cache last gop, for client to fast startup.
    // @return true when gop_cache is ok; otherwise, false.
    // @remark, default true.
    virtual bool get_gop_cache(std::string vhost);
    // Whether debug_srs_upnode is enabled of vhost.
    // debug_srs_upnode is very important feature for tracable log,
    // but some server, for instance, flussonic donot support it.
    // @see https://github.com/ossrs/srs/issues/160
    // @return true when debug_srs_upnode is ok; otherwise, false.
    // @remark, default true.
    virtual bool get_debug_srs_upnode(std::string vhost);
    // Whether atc is enabled of vhost.
    // atc always use encoder timestamp, SRS never adjust the time.
    // @return true when atc is ok; otherwise, false.
    // @remark, default false.
    virtual bool get_atc(std::string vhost);
    // Whether atc_auto is enabled of vhost.
    // atc_auto used to auto enable atc, when metadata specified the bravo_atc.
    // @return true when atc_auto is ok; otherwise, false.
    // @remark, default true.
    virtual bool get_atc_auto(std::string vhost);
    // Get the time_jitter algorithm.
    // @return the time_jitter algorithm, defined in SrsRtmpJitterAlgorithm.
    // @remark, default full.
    virtual int get_time_jitter(std::string vhost);
    // Whether use mix correct algorithm to ensure the timestamp
    // monotonically increase.
    virtual bool get_mix_correct(std::string vhost);
    // Get the cache queue length, in srs_utime_t.
    // when exceed the queue length, drop packet util I frame.
    // @remark, default 10s.
    virtual srs_utime_t get_queue_length(std::string vhost);
    // Whether the refer hotlink-denial enabled.
    virtual bool get_refer_enabled(std::string vhost);
    // Get the refer hotlink-denial for all type.
    // @return the refer, NULL for not configed.
    virtual SrsConfDirective* get_refer_all(std::string vhost);
    // Get the refer hotlink-denial for play.
    // @return the refer, NULL for not configed.
    virtual SrsConfDirective* get_refer_play(std::string vhost);
    // Get the refer hotlink-denial for publish.
    // @return the refer, NULL for not configed.
    virtual SrsConfDirective* get_refer_publish(std::string vhost);
    // Get the input default ack size, which is generally set by message from peer.
    virtual int get_in_ack_size(std::string vhost);
    // Get the output default ack size, to notify the peer to send acknowledge to server.
    virtual int get_out_ack_size(std::string vhost);
    // Get the chunk size of vhost.
    // @param vhost, the vhost to get the chunk size. use global if not specified.
    //       empty string to get the global.
    // @remark, default 60000.
    virtual int get_chunk_size(std::string vhost);
    // Whether parse the sps when publish stream to SRS.
    virtual bool get_parse_sps(std::string vhost);
    // Whether mr is enabled for vhost.
    // @param vhost, the vhost to get the mr.
    virtual bool get_mr_enabled(std::string vhost);
    // Get the mr sleep time in srs_utime_t for vhost.
    // @param vhost, the vhost to get the mr sleep time.
    // TODO: FIXME: add utest for mr config.
    virtual srs_utime_t get_mr_sleep(std::string vhost);
    // Get the mw sleep time in srs_utime_t for vhost.
    // @param vhost, the vhost to get the mw sleep time.
    // TODO: FIXME: add utest for mw config.
    virtual srs_utime_t get_mw_sleep(std::string vhost);
    // Whether min latency mode enabled.
    // @param vhost, the vhost to get the min_latency.
    // TODO: FIXME: add utest for min_latency.
    virtual bool get_realtime_enabled(std::string vhost);
    // Whether enable tcp nodelay for all clients of vhost.
    virtual bool get_tcp_nodelay(std::string vhost);
    // The minimal send interval in srs_utime_t.
    virtual srs_utime_t get_send_min_interval(std::string vhost);
    // Whether reduce the sequence header.
    virtual bool get_reduce_sequence_header(std::string vhost);
    // The 1st packet timeout in srs_utime_t for encoder.
    virtual srs_utime_t get_publish_1stpkt_timeout(std::string vhost);
    // The normal packet timeout in srs_utime_t for encoder.
    virtual srs_utime_t get_publish_normal_timeout(std::string vhost);
private:
    // Get the global chunk size.
    virtual int get_global_chunk_size();
    // forward section
public:
    // Whether the forwarder enabled.
    virtual bool get_forward_enabled(std::string vhost);
    // Get the forward directive of vhost.
    virtual SrsConfDirective* get_forwards(std::string vhost);

public:
    // Whether the srt sevice enabled
    virtual bool get_srt_enabled();
    // Get the srt service listen port
    virtual unsigned short get_srt_listen_port();
    // Get the srt SRTO_MAXBW, max bandwith, default is -1.
    virtual int get_srto_maxbw();
    // Get the srt SRTO_MSS, Maximum Segment Size, default is 1500.
    virtual int get_srto_mss();
    // Get the srt SRTO_LATENCY, latency, default is 0 which means peer/recv latency is 120ms.
    virtual int get_srto_latency();
    // Get the srt SRTO_RCVLATENCY, recv latency, default is 120ms.
    virtual int get_srto_recv_latency();
    // Get the srt SRTO_PEERLATENCY, peer latency, default is 0..
    virtual int get_srto_peer_latency();
    // Get the srt h264 sei filter, default is on, it will drop h264 sei packet.
    virtual bool get_srt_sei_filter();
    // Get the srt SRTO_TLPKDROP, Too-late Packet Drop, default is true.
    virtual bool get_srto_tlpkdrop();
    // Get the srt SRTO_CONNTIMEO, connection timeout, default is 3000ms.
    virtual int get_srto_conntimeout();
    // Get the srt SRTO_SNDBUF, send buffer, default is 8192 × (1500-28).
    virtual int get_srto_sendbuf();
    // Get the srt SRTO_RCVBUF, recv buffer, default is 8192 × (1500-28).
    virtual int get_srto_recvbuf();
    // SRTO_PAYLOADSIZE
    virtual int get_srto_payloadsize();
    // Get the default app.
    virtual std::string get_default_app_name();
    // Get the mix_correct
    virtual bool get_srt_mix_correct();

// http_hooks section
private:
    // Get the http_hooks directive of vhost.
    virtual SrsConfDirective* get_vhost_http_hooks(std::string vhost);
public:
    // Whether vhost http-hooks enabled.
    // @remark, if not enabled, donot callback all http hooks.
    virtual bool get_vhost_http_hooks_enabled(std::string vhost);
    // Get the on_connect callbacks of vhost.
    // @return the on_connect callback directive, the args is the url to callback.
    virtual SrsConfDirective* get_vhost_on_connect(std::string vhost);
    // Get the on_close callbacks of vhost.
    // @return the on_close callback directive, the args is the url to callback.
    virtual SrsConfDirective* get_vhost_on_close(std::string vhost);
    // Get the on_publish callbacks of vhost.
    // @return the on_publish callback directive, the args is the url to callback.
    virtual SrsConfDirective* get_vhost_on_publish(std::string vhost);
    // Get the on_unpublish callbacks of vhost.
    // @return the on_unpublish callback directive, the args is the url to callback.
    virtual SrsConfDirective* get_vhost_on_unpublish(std::string vhost);
    // Get the on_play callbacks of vhost.
    // @return the on_play callback directive, the args is the url to callback.
    virtual SrsConfDirective* get_vhost_on_play(std::string vhost);
    // Get the on_stop callbacks of vhost.
    // @return the on_stop callback directive, the args is the url to callback.
    virtual SrsConfDirective* get_vhost_on_stop(std::string vhost);
    // Get the on_dvr callbacks of vhost.
    // @return the on_dvr callback directive, the args is the url to callback.
    virtual SrsConfDirective* get_vhost_on_dvr(std::string vhost);
    // Get the on_hls callbacks of vhost.
    // @return the on_hls callback directive, the args is the url to callback.
    virtual SrsConfDirective* get_vhost_on_hls(std::string vhost);
    // Get the on_hls_notify callbacks of vhost.
    // @return the on_hls_notify callback directive, the args is the url to callback.
    virtual SrsConfDirective* get_vhost_on_hls_notify(std::string vhost);
// bwct(bandwidth check tool) section
public:
    // Whether bw check enabled for vhost.
    // If  enabled, serve all clients with bandwidth check services.
    // oterwise, serve all cleints with stream.
    virtual bool get_bw_check_enabled(std::string vhost);
    // The key of server, if client key mot match, reject.
    virtual std::string get_bw_check_key(std::string vhost);
    // The check interval, in srs_utime_t.
    // If  the client request check in very short time(in the interval),
    // SRS will reject client.
    // @remark this is used to prevent the bandwidth check attack.
    virtual srs_utime_t get_bw_check_interval(std::string vhost);
    // The max kbps that user can test,
    // If  exceed the kbps, server will slowdown the send-recv.
    // @remark this is used to protect the service bandwidth.
    virtual int get_bw_check_limit_kbps(std::string vhost);
// vhost cluster section
public:
    // Whether vhost is edge mode.
    // For edge, publish client will be proxyed to upnode,
    // For edge, play client will share a connection to get stream from upnode.
    virtual bool get_vhost_is_edge(std::string vhost);
    // Whether vhost is edge mode.
    // For edge, publish client will be proxyed to upnode,
    // For edge, play client will share a connection to get stream from upnode.
    virtual bool get_vhost_is_edge(SrsConfDirective* conf);
    // Get the origin config of edge,
    // specifies the origin ip address, port.
    virtual SrsConfDirective* get_vhost_edge_origin(std::string vhost);
    // Whether edge token tranverse is enabled,
    // If  true, edge will send connect origin to verfy the token of client.
    // For example, we verify all clients on the origin FMS by server-side as,
    // all clients connected to edge must be tranverse to origin to verify.
    virtual bool get_vhost_edge_token_traverse(std::string vhost);
    // Get the transformed vhost for edge,
    // @see https://github.com/ossrs/srs/issues/372
    virtual std::string get_vhost_edge_transform_vhost(std::string vhost);
    // Whether enable the origin cluster.
    // @see https://github.com/ossrs/srs/wiki/v3_EN_OriginCluster
    virtual bool get_vhost_origin_cluster(std::string vhost);
    // Get the co-workers of origin cluster.
    // @see https://github.com/ossrs/srs/wiki/v3_EN_OriginCluster
    virtual std::vector<std::string> get_vhost_coworkers(std::string vhost);
// vhost security section
public:
    // Whether the secrity of vhost enabled.
    virtual bool get_security_enabled(std::string vhost);
    // Get the security rules.
    virtual SrsConfDirective* get_security_rules(std::string vhost);
// vhost transcode section
public:
    // Get the transcode directive of vhost in specified scope.
    // @param vhost, the vhost name to get the transcode directive.
    // @param scope, the scope, empty to get all. for example, user can transcode
    //       the app scope stream, by config with app:
    //                   transcode live {...}
    //       when the scope is "live", this directive is matched.
    //       the scope can be: empty for all, app, app/stream.
    // @remark, please see the samples of full.conf, the app.transcode.srs.com
    //       and stream.transcode.srs.com.
    virtual SrsConfDirective* get_transcode(std::string vhost, std::string scope);
    // Whether the transcode directive is enabled.
    virtual bool get_transcode_enabled(SrsConfDirective* conf);
    // Get the ffmpeg tool path of transcode.
    virtual std::string get_transcode_ffmpeg(SrsConfDirective* conf);
    // Get the engines of transcode.
    virtual std::vector<SrsConfDirective*> get_transcode_engines(SrsConfDirective* conf);
    // Whether the engine is enabled.
    virtual bool get_engine_enabled(SrsConfDirective* conf);
    // Get the perfile of engine
    virtual std::vector<std::string> get_engine_perfile(SrsConfDirective* conf);
    // Get the iformat of engine
    virtual std::string get_engine_iformat(SrsConfDirective* conf);
    // Get the vfilter of engine,
    // The video filter set before the vcodec of FFMPEG.
    virtual std::vector<std::string> get_engine_vfilter(SrsConfDirective* conf);
    // Get the vcodec of engine,
    // The codec of video, can be vn, copy or libx264
    virtual std::string get_engine_vcodec(SrsConfDirective* conf);
    // Get the vbitrate of engine,
    // The bitrate in kbps of video, for example, 800kbps
    virtual int get_engine_vbitrate(SrsConfDirective* conf);
    // Get the vfps of engine.
    // The video fps, for example, 25fps
    virtual double get_engine_vfps(SrsConfDirective* conf);
    // Get the vwidth of engine,
    // The video width, for example, 1024
    virtual int get_engine_vwidth(SrsConfDirective* conf);
    // Get the vheight of engine,
    // The video height, for example, 576
    virtual int get_engine_vheight(SrsConfDirective* conf);
    // Get the vthreads of engine,
    // The video transcode libx264 threads, for instance, 8
    virtual int get_engine_vthreads(SrsConfDirective* conf);
    // Get the vprofile of engine,
    // The libx264 profile, can be high,main,baseline
    virtual std::string get_engine_vprofile(SrsConfDirective* conf);
    // Get the vpreset of engine,
    // The libx264 preset, can be ultrafast,superfast,veryfast,faster,fast,medium,slow,slower,veryslow,placebo
    virtual std::string get_engine_vpreset(SrsConfDirective* conf);
    // Get the additional video params.
    virtual std::vector<std::string> get_engine_vparams(SrsConfDirective* conf);
    // Get the acodec of engine,
    // The audio codec can be an, copy or libfdk_aac
    virtual std::string get_engine_acodec(SrsConfDirective* conf);
    // Get the abitrate of engine,
    // The audio bitrate in kbps, for instance, 64kbps.
    virtual int get_engine_abitrate(SrsConfDirective* conf);
    // Get the asample_rate of engine,
    // The audio sample_rate, for instance, 44100HZ
    virtual int get_engine_asample_rate(SrsConfDirective* conf);
    // Get the achannels of engine,
    // The audio channel, for instance, 1 for mono, 2 for stereo.
    virtual int get_engine_achannels(SrsConfDirective* conf);
    // Get the aparams of engine,
    // The audio additional params.
    virtual std::vector<std::string> get_engine_aparams(SrsConfDirective* conf);
    // Get the oformat of engine
    virtual std::string get_engine_oformat(SrsConfDirective* conf);
    // Get the output of engine, for example, rtmp://localhost/live/livestream,
    // @remark, we will use some variable, for instance, [vhost] to substitude with vhost.
    virtual std::string get_engine_output(SrsConfDirective* conf);
// vhost exec secion
private:
    // Get the exec directive of vhost.
    virtual SrsConfDirective* get_exec(std::string vhost);
public:
    // Whether the exec is enabled of vhost.
    virtual bool get_exec_enabled(std::string vhost);
    // Get all exec publish directives of vhost.
    virtual std::vector<SrsConfDirective*> get_exec_publishs(std::string vhost);
    // vhost ingest section
public:
    // Get the ingest directives of vhost.
    virtual std::vector<SrsConfDirective*> get_ingesters(std::string vhost);
    // Get specified ingest.
    virtual SrsConfDirective* get_ingest_by_id(std::string vhost, std::string ingest_id);
    // Whether ingest is enalbed.
    virtual bool get_ingest_enabled(SrsConfDirective* conf);
    // Get the ingest ffmpeg tool
    virtual std::string get_ingest_ffmpeg(SrsConfDirective* conf);
    // Get the ingest input type, file or stream.
    virtual std::string get_ingest_input_type(SrsConfDirective* conf);
    // Get the ingest input url.
    virtual std::string get_ingest_input_url(SrsConfDirective* conf);
// log section
public:
    // Whether log to file.
    virtual bool get_log_tank_file();
    // Get the log level.
    virtual std::string get_log_level();
    // Get the log file path.
    virtual std::string get_log_file();
    // Whether ffmpeg log enabled
    virtual bool get_ff_log_enabled();
    // The ffmpeg log dir.
    // @remark, /dev/null to disable it.
    virtual std::string get_ff_log_dir();
    // The ffmpeg log level.
    virtual std::string get_ff_log_level();
// The MPEG-DASH section.
private:
    virtual SrsConfDirective* get_dash(std::string vhost);
public:
    // Whether DASH is enabled.
    virtual bool get_dash_enabled(std::string vhost);
    // Get the duration of segment in srs_utime_t.
    virtual srs_utime_t get_dash_fragment(std::string vhost);
    // Get the period to update MPD in srs_utime_t.
    virtual srs_utime_t get_dash_update_period(std::string vhost);
    // Get the depth of timeshift buffer in srs_utime_t.
    virtual srs_utime_t get_dash_timeshift(std::string vhost);
    // Get the base/home dir/path for dash, into which write files.
    virtual std::string get_dash_path(std::string vhost);
    // Get the path for DASH MPD, to generate the MPD file.
    virtual std::string get_dash_mpd_file(std::string vhost);
// hls section
private:
    // Get the hls directive of vhost.
    virtual SrsConfDirective* get_hls(std::string vhost);
public:
    // Whether HLS is enabled.
    virtual bool get_hls_enabled(std::string vhost);
    // Get the HLS m3u8 list ts segment entry prefix info.
    virtual std::string get_hls_entry_prefix(std::string vhost);
    // Get the HLS ts/m3u8 file store path.
    virtual std::string get_hls_path(std::string vhost);
    // Get the HLS m3u8 file path template.
    virtual std::string get_hls_m3u8_file(std::string vhost);
    // Get the HLS ts file path template.
    virtual std::string get_hls_ts_file(std::string vhost);
    // Whether enable the floor(timestamp/hls_fragment) for variable timestamp.
    virtual bool get_hls_ts_floor(std::string vhost);
    // Get the hls fragment time, in srs_utime_t.
    virtual srs_utime_t get_hls_fragment(std::string vhost);
    // Get the hls td(target duration) ratio.
    virtual double get_hls_td_ratio(std::string vhost);
    // Get the hls aof(audio overflow) ratio.
    virtual double get_hls_aof_ratio(std::string vhost);
    // Get the hls window time, in srs_utime_t.
    // a window is a set of ts, the ts collection in m3u8.
    // @remark SRS will delete the ts exceed the window.
    virtual srs_utime_t get_hls_window(std::string vhost);
    // Get the hls hls_on_error config.
    // The ignore will ignore error and disable hls.
    // The disconnect will disconnect publish connection.
    // @see https://github.com/ossrs/srs/issues/264
    virtual std::string get_hls_on_error(std::string vhost);
    // Get the HLS default audio codec.
    virtual std::string get_hls_acodec(std::string vhost);
    // Get the HLS default video codec.
    virtual std::string get_hls_vcodec(std::string vhost);
    // Whether cleanup the old ts files.
    virtual bool get_hls_cleanup(std::string vhost);
    // The timeout in srs_utime_t to dispose the hls.
    virtual srs_utime_t get_hls_dispose(std::string vhost);
    // Whether reap the ts when got keyframe.
    virtual bool get_hls_wait_keyframe(std::string vhost);
    // encrypt ts or not
    virtual bool get_hls_keys(std::string vhost);
    // how many fragments can one key encrypted.
    virtual int get_hls_fragments_per_key(std::string vhost);
    // Get the HLS key file path template.
    virtual std::string get_hls_key_file(std::string vhost);
    // Get the HLS key file store path.
    virtual std::string get_hls_key_file_path(std::string vhost);
    // Get the HLS key file url which will be put in m3u8
    virtual std::string get_hls_key_url(std::string vhost);
    // Get the size of bytes to read from cdn network, for the on_hls_notify callback,
    // that is, to read max bytes of the bytes from the callback, or timeout or error.
    virtual int get_vhost_hls_nb_notify(std::string vhost);
    // Whether turn the FLV timestamp to TS DTS.
    virtual bool get_vhost_hls_dts_directly(std::string vhost);
// hds section
private:
    // Get the hds directive of vhost.
    virtual SrsConfDirective* get_hds(const std::string &vhost);
public:
    // Whether HDS is enabled.
    virtual bool get_hds_enabled(const std::string &vhost);
    // Get the HDS file store path.
    virtual std::string get_hds_path(const std::string &vhost);
    // Get the hds fragment time, in srs_utime_t.
    virtual srs_utime_t get_hds_fragment(const std::string &vhost);
    // Get the hds window time, in srs_utime_t.
    // a window is a set of hds fragments.
    virtual srs_utime_t get_hds_window(const std::string &vhost);
// dvr section
private:
    // Get the dvr directive.
    virtual SrsConfDirective* get_dvr(std::string vhost);
public:
    // Whether dvr is enabled.
    virtual bool get_dvr_enabled(std::string vhost);
    // Get the filter of dvr to apply to.
    // @remark user can use srs_config_apply_filter(conf, req):bool to check it.
    virtual SrsConfDirective* get_dvr_apply(std::string vhost);
    // Get the dvr path, the flv file to save in.
    virtual std::string get_dvr_path(std::string vhost);
    // Get the plan of dvr, how to reap the flv file.
    virtual std::string get_dvr_plan(std::string vhost);
    // Get the duration of dvr flv.
    virtual srs_utime_t get_dvr_duration(std::string vhost);
    // Whether wait keyframe to reap segment.
    virtual bool get_dvr_wait_keyframe(std::string vhost);
    // Get the time_jitter algorithm for dvr.
    virtual int get_dvr_time_jitter(std::string vhost);
// http api section
private:
    // Whether http api enabled
    virtual bool get_http_api_enabled(SrsConfDirective* conf);
public:
    // Whether http api enabled.
    virtual bool get_http_api_enabled();
    // Get the http api listen port.
    virtual std::string get_http_api_listen();
    // Whether enable crossdomain for http api.
    virtual bool get_http_api_crossdomain();
    // Whether enable the HTTP RAW API.
    virtual bool get_raw_api();
    // Whether allow rpc reload.
    virtual bool get_raw_api_allow_reload();
    // Whether allow rpc query.
    virtual bool get_raw_api_allow_query();
    // Whether allow rpc update.
    virtual bool get_raw_api_allow_update();
// http stream section
private:
    // Whether http stream enabled.
    virtual bool get_http_stream_enabled(SrsConfDirective* conf);
public:
    // Whether http stream enabled.
    // TODO: FIXME: rename to http_static.
    virtual bool get_http_stream_enabled();
    // Get the http stream listen port.
    virtual std::string get_http_stream_listen();
    // Get the http stream root dir.
    virtual std::string get_http_stream_dir();
    // Whether enable crossdomain for http static and stream server.
    virtual bool get_http_stream_crossdomain();
public:
    // Get whether vhost enabled http stream
    virtual bool get_vhost_http_enabled(std::string vhost);
    // Get the http mount point for vhost.
    // For example, http://vhost/live/livestream
    virtual std::string get_vhost_http_mount(std::string vhost);
    // Get the http dir for vhost.
    // The path on disk for mount root of http vhost.
    virtual std::string get_vhost_http_dir(std::string vhost);
// flv live streaming section
public:
    // Get whether vhost enabled http flv live stream
    virtual bool get_vhost_http_remux_enabled(std::string vhost);
    // Get the fast cache duration for http audio live stream.
    virtual srs_utime_t get_vhost_http_remux_fast_cache(std::string vhost);
    // Get the http flv live stream mount point for vhost.
    // used to generate the flv stream mount path.
    virtual std::string get_vhost_http_remux_mount(std::string vhost);
// http heartbeart section
private:
    // Get the heartbeat directive.
    virtual SrsConfDirective* get_heartbeart();
public:
    // Whether heartbeat enabled.
    virtual bool get_heartbeat_enabled();
    // Get the heartbeat interval, in srs_utime_t.
    virtual srs_utime_t get_heartbeat_interval();
    // Get the heartbeat report url.
    virtual std::string get_heartbeat_url();
    // Get the device id of heartbeat, to report to server.
    virtual std::string get_heartbeat_device_id();
    // Whether report with summaries of http api: /api/v1/summaries.
    virtual bool get_heartbeat_summaries();
// stats section
private:
    // Get the stats directive.
    virtual SrsConfDirective* get_stats();
public:
    // Get the network device index, used to retrieve the ip of device,
    // For heartbeat to report to server, or to get the local ip.
    // For example, 0 means the eth0 maybe.
    virtual int get_stats_network();
    // Get the disk stat device name list.
    // The device name configed in args of directive.
    // @return the disk device name to stat. NULL if not configed.
    virtual SrsConfDirective* get_stats_disk_device();
};

#endif

