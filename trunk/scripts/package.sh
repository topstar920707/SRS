#!/bin/bash

# Usage:
#       bash package.sh [arm]
#           option arm, whether build for arm, requires ubuntu12.

# user can config the following configs, then package.
INSTALL=/usr/local/srs
# whether build for arm, only for ubuntu12.
ARM=NO

##################################################################################
##################################################################################
##################################################################################
# parse options.
if [[ $1 == "arm" ]]; then ARM=YES; fi

# discover the current work dir, the log and access.
echo "argv[0]=$0"
if [[ ! -f $0 ]]; then 
    echo "directly execute the scripts on shell.";
    work_dir=`pwd`
else 
    echo "execute scripts in file: $0";
    work_dir=`dirname $0`; work_dir=`(cd ${work_dir} && pwd)`
fi
work_dir=`(cd ${work_dir}/.. && pwd)`
product_dir=$work_dir
build_objs=${work_dir}/objs
package_dir=${build_objs}/package

log="${build_objs}/logs/package.`date +%s`.log" && . ${product_dir}/scripts/_log.sh && check_log
ret=$?; if [[ $ret -ne 0 ]]; then exit $ret; fi

# check os version
os_name=`lsb_release --id|awk '{print $3}'` &&
os_release=`lsb_release --release|awk '{print $2}'` &&
os_major_version=`echo $os_release|awk -F '.' '{print $1}'` &&
os_machine=`uname -i`
ret=$?; if [[ $ret -ne 0 ]]; then failed_msg "lsb_release get os info failed."; exit $ret; fi
ok_msg "target os is ${os_name}-${os_major_version} ${os_release} ${os_machine}"

# build srs
# @see https://github.com/winlinvip/simple-rtmp-server/wiki/Build
ok_msg "start build srs"
if [ $ARM = YES ]; then
    (
        cd $work_dir && 
        ./configure --with-ssl --with-arm-ubuntu12 --prefix=$INSTALL &&
        make && rm -rf $package_dir && make DESTDIR=$package_dir install
    ) >> $log 2>&1
else
    (
        cd $work_dir && 
        ./configure --with-ssl --with-hls --with-nginx --with-ffmpeg --with-http-callback --prefix=$INSTALL &&
        make && rm -rf $package_dir && make DESTDIR=$package_dir install
    ) >> $log 2>&1
fi
ret=$?; if [[ 0 -ne ${ret} ]]; then failed_msg "build srs failed"; exit $ret; fi
ok_msg "build srs success"

# copy extra files to package.
ok_msg "start copy extra files to package"
(
    cp $work_dir/scripts/install.sh $package_dir/INSTALL &&
    sed -i "s|^INSTALL=.*|INSTALL=${INSTALL}|g" $package_dir/INSTALL &&
    mkdir -p $package_dir/scripts &&
    cp $work_dir/scripts/_log.sh $package_dir/scripts/_log.sh &&
    chmod +x $package_dir/INSTALL
) >> $log 2>&1
ret=$?; if [[ 0 -ne ${ret} ]]; then failed_msg "copy extra files failed"; exit $ret; fi
ok_msg "copy extra files success"

# detect for arm.
if [ $ARM = YES ]; then
    arm_cpu=`arm-linux-gnueabi-readelf --arch-specific ${build_objs}/srs|grep Tag_CPU_arch:|awk '{print $2}'`
    os_machine=arm${arm_cpu}cpu
fi
ok_msg "machine: $os_machine"

# generate zip dir and zip filename
if [ $ARM = YES ]; then
    srs_version_major=`cat $work_dir/src/core/srs_core.hpp| grep '#define VERSION_MAJOR'| awk '{print $3}'|xargs echo` &&
    srs_version_minor=`cat $work_dir/src/core/srs_core.hpp| grep '#define VERSION_MINOR'| awk '{print $3}'|xargs echo` &&
    srs_version_revision=`cat $work_dir/src/core/srs_core.hpp| grep '#define VERSION_REVISION'| awk '{print $3}'|xargs echo` &&
    srs_version=$srs_version_major.$srs_version_minor.$srs_version_revision
else
    srs_version=`${build_objs}/srs -v 2>/dev/stdout 1>/dev/null`
fi
ret=$?; if [[ 0 -ne ${ret} ]]; then failed_msg "get srs version failed"; exit $ret; fi
ok_msg "get srs version $srs_version"

zip_dir="SRS-${os_name}${os_major_version}-${os_machine}-${srs_version}"
ret=$?; if [[ 0 -ne ${ret} ]]; then failed_msg "generate zip filename failed"; exit $ret; fi
ok_msg "target zip filename $zip_dir"

# zip package.
ok_msg "start zip package"
(
    mv $package_dir ${build_objs}/${zip_dir} &&
    cd ${build_objs} && rm -rf ${zip_dir}.zip && zip -q -r ${zip_dir}.zip ${zip_dir} &&
    mv ${build_objs}/${zip_dir} $package_dir
) >> $log 2>&1
ret=$?; if [[ 0 -ne ${ret} ]]; then failed_msg "zip package failed"; exit $ret; fi
ok_msg "zip package success"

ok_msg "srs package success"
echo ""
echo "package: ${build_objs}/${zip_dir}.zip"
echo "install:"
echo "      unzip -q ${zip_dir}.zip &&"
echo "      cd ${zip_dir} &&"
echo "      sudo bash INSTALL"

exit 0
