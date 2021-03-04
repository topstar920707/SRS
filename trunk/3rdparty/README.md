http-parser-2.1.zip
* for srs to support http callback.

nginx-1.5.7.zip
* for srs to support hls streaming.

openssl-1.1.1b.tar.gz    
openssl-1.1.0e.zip
openssl-OpenSSL_1_0_2u.tar.gz
* openssl for SRS(with-ssl) RTMP complex handshake to delivery h264+aac stream.
* SRTP depends on openssl 1.0.*, so we use both ssl versions.

CherryPy-3.2.4.zip
* sample api server for srs.

libsrtp-2.3.0.tar.gz
* For WebRTC.

ffmpeg-3.2.4.tar.gz
yasm-1.2.0.tar.gz
lame-3.99.5.tar.gz
speex-1.2rc1.zip
x264-snapshot-20131129-2245-stable.tar.bz2 (core.138)
* for srs to support live stream transcoding.
* remark: we use *.zip for all linux plantform.

fdk-aac-0.1.3.zip
* https://github.com/mstorsjo/fdk-aac/releases

tools/ccache-3.1.9.zip
* to fast build.
    
gtest-1.6.0.zip
* google test framework.
    
gperftools-2.1.zip
* gperf tools for performance benchmark.

st-srs
st-1.9.zip
state-threads
state-threads-1.9.1.tar.gz
* Patched ST from https://github.com/ossrs/state-threads

links:
* nginx:
        http://nginx.org/
* http-parser:
        https://github.com/joyent/http-parser
* state-threads:
        https://github.com/ossrs/state-threads
* ffmpeg: 
        http://ffmpeg.org/ 
        http://ffmpeg.org/releases/ffmpeg-3.2.4.tar.gz
* x264: 
        http://www.videolan.org/ 
        ftp://ftp.videolan.org/pub/videolan/x264/snapshots/x264-snapshot-20131129-2245-stable.tar.bz2
* lame: 
        http://sourceforge.net/projects/lame/ 
        http://nchc.dl.sourceforge.net/project/lame/lame/3.99/lame-3.99.5.tar.gz
* yasm:
        http://yasm.tortall.net/
        http://www.tortall.net/projects/yasm/releases/yasm-1.2.0.tar.gz
* cherrypy:
        http://www.cherrypy.org/
        https://pypi.python.org/pypi/CherryPy/3.2.4
* openssl:
        http://www.openssl.org/
        http://www.openssl.org/source/openssl-1.1.0e.tar.gz
* gtest:
        https://code.google.com/p/googletest
        https://code.google.com/p/googletest/downloads/list
* gperftools:
        https://code.google.com/p/gperftools/
        https://code.google.com/p/gperftools/downloads/list
* speex:
        http://www.speex.org/downloads/
        http://downloads.xiph.org/releases/speex/speex-1.2rc1.tar.gz
* srtp:
        https://github.com/cisco/libsrtp/releases/tag/v2.3.0
