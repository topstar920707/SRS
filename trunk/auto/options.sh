#!/bin/bash

help=no

SRS_HLS=RESERVED
SRS_SSL=RESERVED
SRS_FFMPEG=RESERVED

# TODO: remove the default to yes.
SRS_HLS=YES
SRS_SSL=YES
SRS_FFMPEG=YES

opt=

for option
do
    opt="$opt `echo $option | sed -e \"s/\(--[^=]*=\)\(.* .*\)/\1'\2'/\"`"

    case "$option" in
        -*=*) value=`echo "$option" | sed -e 's/[-_a-zA-Z0-9]*=//'` ;;
           *) value="" ;;
    esac

    case "$option" in
        --help)                         help=yes                  ;;
        
        --with-ssl)                     SRS_SSL=YES               ;;
        --with-hls)                     SRS_HLS=YES               ;;
        --with-ffmpeg)                  SRS_FFMPEG=YES            ;;
        
        --without-ssl)                  SRS_SSL=NO                ;;
        --without-hls)                  SRS_HLS=NO                ;;
        --without-ffmpeg)               SRS_FFMPEG=NO             ;;

        *)
            echo "$0: error: invalid option \"$option\""
            exit 1
        ;;
    esac
done

# save all config options to macro.
SRS_CONFIGURE="$opt"

if [ $help = yes ]; then
cat << END

  --help                   print this message

  --with-ssl               enable rtmp complex handshake, requires openssl-devel installed.
                           to delivery h264 video and aac audio to flash player.
  --with-hls               enable hls streaming, build nginx as http server for hls.

  --without-ssl            disable rtmp complex handshake.
  --without-hls            disable hls, rtmp streaming only.

END

    exit 1
fi

__check_ok=YES
if [ $SRS_SSL = RESERVED ]; then
    echo "you must specifies the ssl, see: ./configure --help";
    __check_ok=NO
fi
if [ $SRS_HLS = RESERVED ]; then
    echo "you must specifies the hls, see: ./configure --help";
    __check_ok=NO
fi
if [ $SRS_FFMPEG = RESERVED ]; then
    echo "you must specifies the ffmpeg, see: ./configure --help";
    __check_ok=NO
fi
if [ $__check_ok = NO ]; then
    exit 1;
fi