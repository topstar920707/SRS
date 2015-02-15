/*
The MIT License (MIT)

Copyright (c) 2013-2015 winlin

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

#ifndef SRS_KERNEL_CODEC_HPP
#define SRS_KERNEL_CODEC_HPP

/*
#include <srs_kernel_codec.hpp>
*/

#include <srs_core.hpp>

#include <string>

class SrsStream;

// AACPacketType IF SoundFormat == 10 UI8
// The following values are defined:
//     0 = AAC sequence header
//     1 = AAC raw
enum SrsCodecAudioType
{
    // set to the max value to reserved, for array map.
    SrsCodecAudioTypeReserved                        = 2,
    
    SrsCodecAudioTypeSequenceHeader                 = 0,
    SrsCodecAudioTypeRawData                         = 1,
};

// E.4.3.1 VIDEODATA
// Frame Type UB [4]
// Type of video frame. The following values are defined:
//     1 = key frame (for AVC, a seekable frame)
//     2 = inter frame (for AVC, a non-seekable frame)
//     3 = disposable inter frame (H.263 only)
//     4 = generated key frame (reserved for server use only)
//     5 = video info/command frame
enum SrsCodecVideoAVCFrame
{
    // set to the zero to reserved, for array map.
    SrsCodecVideoAVCFrameReserved                    = 0,
    SrsCodecVideoAVCFrameReserved1                    = 6,
    
    SrsCodecVideoAVCFrameKeyFrame                     = 1,
    SrsCodecVideoAVCFrameInterFrame                 = 2,
    SrsCodecVideoAVCFrameDisposableInterFrame         = 3,
    SrsCodecVideoAVCFrameGeneratedKeyFrame            = 4,
    SrsCodecVideoAVCFrameVideoInfoFrame                = 5,
};

// AVCPacketType IF CodecID == 7 UI8
// The following values are defined:
//     0 = AVC sequence header
//     1 = AVC NALU
//     2 = AVC end of sequence (lower level NALU sequence ender is
//         not required or supported)
enum SrsCodecVideoAVCType
{
    // set to the max value to reserved, for array map.
    SrsCodecVideoAVCTypeReserved                    = 3,
    
    SrsCodecVideoAVCTypeSequenceHeader                 = 0,
    SrsCodecVideoAVCTypeNALU                         = 1,
    SrsCodecVideoAVCTypeSequenceHeaderEOF             = 2,
};

// E.4.3.1 VIDEODATA
// CodecID UB [4]
// Codec Identifier. The following values are defined:
//     2 = Sorenson H.263
//     3 = Screen video
//     4 = On2 VP6
//     5 = On2 VP6 with alpha channel
//     6 = Screen video version 2
//     7 = AVC
enum SrsCodecVideo
{
    // set to the zero to reserved, for array map.
    SrsCodecVideoReserved                = 0,
    SrsCodecVideoReserved1                = 1,
    SrsCodecVideoReserved2                = 9,
    
    // for user to disable video, for example, use pure audio hls.
    SrsCodecVideoDisabled                = 8,
    
    SrsCodecVideoSorensonH263             = 2,
    SrsCodecVideoScreenVideo             = 3,
    SrsCodecVideoOn2VP6                 = 4,
    SrsCodecVideoOn2VP6WithAlphaChannel = 5,
    SrsCodecVideoScreenVideoVersion2     = 6,
    SrsCodecVideoAVC                     = 7,
};

// SoundFormat UB [4] 
// Format of SoundData. The following values are defined:
//     0 = Linear PCM, platform endian
//     1 = ADPCM
//     2 = MP3
//     3 = Linear PCM, little endian
//     4 = Nellymoser 16 kHz mono
//     5 = Nellymoser 8 kHz mono
//     6 = Nellymoser
//     7 = G.711 A-law logarithmic PCM
//     8 = G.711 mu-law logarithmic PCM
//     9 = reserved
//     10 = AAC
//     11 = Speex
//     14 = MP3 8 kHz
//     15 = Device-specific sound
// Formats 7, 8, 14, and 15 are reserved.
// AAC is supported in Flash Player 9,0,115,0 and higher.
// Speex is supported in Flash Player 10 and higher.
enum SrsCodecAudio
{
    // set to the max value to reserved, for array map.
    SrsCodecAudioReserved1                = 16,
    
    SrsCodecAudioLinearPCMPlatformEndian             = 0,
    SrsCodecAudioADPCM                                 = 1,
    SrsCodecAudioMP3                                 = 2,
    SrsCodecAudioLinearPCMLittleEndian                 = 3,
    SrsCodecAudioNellymoser16kHzMono                 = 4,
    SrsCodecAudioNellymoser8kHzMono                 = 5,
    SrsCodecAudioNellymoser                         = 6,
    SrsCodecAudioReservedG711AlawLogarithmicPCM        = 7,
    SrsCodecAudioReservedG711MuLawLogarithmicPCM    = 8,
    SrsCodecAudioReserved                             = 9,
    SrsCodecAudioAAC                                 = 10,
    SrsCodecAudioSpeex                                 = 11,
    SrsCodecAudioReservedMP3_8kHz                     = 14,
    SrsCodecAudioReservedDeviceSpecificSound         = 15,
};

/**
* the FLV/RTMP supported audio sample rate.
* Sampling rate. The following values are defined:
* 0 = 5.5 kHz = 5512 Hz
* 1 = 11 kHz = 11025 Hz
* 2 = 22 kHz = 22050 Hz
* 3 = 44 kHz = 44100 Hz
*/
enum SrsCodecAudioSampleRate
{
    // set to the max value to reserved, for array map.
    SrsCodecAudioSampleRateReserved                 = 4,
    
    SrsCodecAudioSampleRate5512                     = 0,
    SrsCodecAudioSampleRate11025                    = 1,
    SrsCodecAudioSampleRate22050                    = 2,
    SrsCodecAudioSampleRate44100                    = 3,
};

/**
* E.4.1 FLV Tag, page 75
*/
enum SrsCodecFlvTag
{
    // set to the zero to reserved, for array map.
    SrsCodecFlvTagReserved = 0,

    // 8 = audio
    SrsCodecFlvTagAudio = 8,
    // 9 = video
    SrsCodecFlvTagVideo = 9,
    // 18 = script data
    SrsCodecFlvTagScript = 18,
};

/**
* Annex E. The FLV File Format
* @see SrsAvcAacCodec for the media stream codec.
*/
class SrsFlvCodec
{
public:
    SrsFlvCodec();
    virtual ~SrsFlvCodec();
// the following function used to finger out the flv/rtmp packet detail.
public:
    /**
    * only check the frame_type, not check the codec type.
    */
    static bool video_is_keyframe(char* data, int size);
    /**
    * check codec h264, keyframe, sequence header
    */
    static bool video_is_sequence_header(char* data, int size);
    /**
    * check codec aac, sequence header
    */
    static bool audio_is_sequence_header(char* data, int size);
    /**
    * check codec h264.
    */
    static bool video_is_h264(char* data, int size);
    /**
    * check codec aac.
    */
    static bool audio_is_aac(char* data, int size);
};

/**
* the public data, event HLS disable, others can use it.
*/
/**
* the flv sample rate map
*/
extern int flv_sample_rates[];

/**
* the aac sample rate map
*/
extern int aac_sample_rates[];

#define __SRS_SRS_MAX_CODEC_SAMPLE 128
#define __SRS_AAC_SAMPLE_RATE_UNSET 15

// in ms, for HLS aac flush the audio
#define SRS_CONF_DEFAULT_AAC_DELAY 60

// max PES packets size to flush the video.
#define SRS_AUTO_HLS_AUDIO_CACHE_SIZE 128 * 1024

/**
* the FLV/RTMP supported audio sample size.
* Size of each audio sample. This parameter only pertains to
* uncompressed formats. Compressed formats always decode
* to 16 bits internally.
* 0 = 8-bit samples
* 1 = 16-bit samples
*/
enum SrsCodecAudioSampleSize
{
    // set to the max value to reserved, for array map.
    SrsCodecAudioSampleSizeReserved                 = 2,
    
    SrsCodecAudioSampleSize8bit                     = 0,
    SrsCodecAudioSampleSize16bit                    = 1,
};

/**
* the FLV/RTMP supported audio sound type/channel.
* Mono or stereo sound
* 0 = Mono sound
* 1 = Stereo sound
*/
enum SrsCodecAudioSoundType
{
    // set to the max value to reserved, for array map.
    SrsCodecAudioSoundTypeReserved                  = 2, 
    
    SrsCodecAudioSoundTypeMono                      = 0,
    SrsCodecAudioSoundTypeStereo                    = 1,
};

/**
* the codec sample unit.
* for h.264 video packet, a NALU is a sample unit.
* for aac raw audio packet, a NALU is the entire aac raw data.
* for sequence header, it's not a sample unit.
*/
class SrsCodecSampleUnit
{
public:
    /**
    * the sample bytes is directly ptr to packet bytes,
    * user should never use it when packet destroyed.
    */
    int size;
    char* bytes;
public:
    SrsCodecSampleUnit();
    virtual ~SrsCodecSampleUnit();
};

/**
* the samples in the flv audio/video packet.
* the sample used to analysis a video/audio packet,
* split the h.264 NALUs to buffers, or aac raw data to a buffer,
* and decode the video/audio specified infos.
* 
* the sample unit:
*       a video packet codec in h.264 contains many NALUs, each is a sample unit.
*       a audio packet codec in aac is a sample unit.
* @remark, the video/audio sequence header is not sample unit,
*       all sequence header stores as extra data, 
*       @see SrsAvcAacCodec.avc_extra_data and SrsAvcAacCodec.aac_extra_data
* @remark, user must clear all samples before decode a new video/audio packet.
*/
class SrsCodecSample
{
public:
    /**
    * each audio/video raw data packet will dumps to one or multiple buffers,
    * the buffers will write to hls and clear to reset.
    * generally, aac audio packet corresponding to one buffer,
    * where avc/h264 video packet may contains multiple buffer.
    */
    int nb_sample_units;
    SrsCodecSampleUnit sample_units[__SRS_SRS_MAX_CODEC_SAMPLE];
public:
    /**
    * whether the sample is video sample which demux from video packet.
    */
    bool is_video;
    /**
    * CompositionTime, video_file_format_spec_v10_1.pdf, page 78.
    * cts = pts - dts, where dts = flvheader->timestamp.
    */
    int32_t cts;
public:
    // video specified
    SrsCodecVideoAVCFrame frame_type;
    SrsCodecVideoAVCType avc_packet_type;
public:
    // audio specified
    SrsCodecAudio acodec;
    // audio aac specified.
    SrsCodecAudioSampleRate sound_rate;
    SrsCodecAudioSampleSize sound_size;
    SrsCodecAudioSoundType sound_type;
    SrsCodecAudioType aac_packet_type;
public:
    SrsCodecSample();
    virtual ~SrsCodecSample();
public:
    /**
    * clear all samples.
    * the sample units never copy the bytes, it directly use the ptr,
    * so when video/audio packet is destroyed, the sample must be clear.
    * in a word, user must clear sample before demux it.
    * @remark demux sample use SrsAvcAacCodec.audio_aac_demux or video_avc_demux.
    */
    void clear();
    /**
    * add the a sample unit, it's a h.264 NALU or aac raw data.
    * the sample unit directly use the ptr of packet bytes,
    * so user must never use sample unit when packet is destroyed.
    * in a word, user must clear sample before demux it.
    */
    int add_sample_unit(char* bytes, int size);
};

/**
* the avc payload format, must be ibmf or annexb format.
* we guess by annexb first, then ibmf for the first time,
* and we always use the guessed format for the next time.
*/
enum SrsAvcPayloadFormat
{
    SrsAvcPayloadFormatGuess = 0,
    SrsAvcPayloadFormatAnnexb,
    SrsAvcPayloadFormatIbmf,
};

/**
* the h264/avc and aac codec, for media stream.
*
* to demux the FLV/RTMP video/audio packet to sample,
* add each NALUs of h.264 as a sample unit to sample,
* while the entire aac raw data as a sample unit.
*
* for sequence header,
* demux it and save it in the avc_extra_data and aac_extra_data,
* 
* for the codec info, such as audio sample rate,
* decode from FLV/RTMP header, then use codec info in sequence 
* header to override it.
*/
class SrsAvcAacCodec
{
private:
    SrsStream* stream;
public:
    /**
    * metadata specified
    */
    int             duration;
    int             width;
    int             height;
    int             frame_rate;
    // @see: SrsCodecVideo
    int             video_codec_id;
    int             video_data_rate; // in bps
    // @see: SrsCod ecAudioType
    int             audio_codec_id;
    int             audio_data_rate; // in bps
public:
    /**
    * video specified
    */
    // profile_idc, H.264-AVC-ISO_IEC_14496-10.pdf, page 45.
    u_int8_t        avc_profile; 
    // level_idc, H.264-AVC-ISO_IEC_14496-10.pdf, page 45.
    u_int8_t        avc_level; 
    // lengthSizeMinusOne, H.264-AVC-ISO_IEC_14496-15.pdf, page 16
    int8_t          NAL_unit_length;
    u_int16_t       sequenceParameterSetLength;
    char*           sequenceParameterSetNALUnit;
    u_int16_t       pictureParameterSetLength;
    char*           pictureParameterSetNALUnit;
private:
    // the avc payload format.
    SrsAvcPayloadFormat payload_format;
public:
    /**
    * audio specified
    * audioObjectType, in 1.6.2.1 AudioSpecificConfig, page 33,
    * 1.5.1.1 Audio object type definition, page 23,
    *           in aac-mp4a-format-ISO_IEC_14496-3+2001.pdf.
    */
    u_int8_t        aac_profile; 
    /**
    * samplingFrequencyIndex
    */
    u_int8_t        aac_sample_rate;
    /**
    * channelConfiguration
    */
    u_int8_t        aac_channels;
public:
    /**
    * the avc extra data, the AVC sequence header,
    * without the flv codec header,
    * @see: ffmpeg, AVCodecContext::extradata
    */
    int             avc_extra_size;
    char*           avc_extra_data;
    /**
    * the aac extra data, the AAC sequence header,
    * without the flv codec header,
    * @see: ffmpeg, AVCodecContext::extradata
    */
    int             aac_extra_size;
    char*           aac_extra_data;
public:
    SrsAvcAacCodec();
    virtual ~SrsAvcAacCodec();
// the following function used for hls to build the sample and codec.
public:
    /**
    * demux the audio packet in aac codec.
    * the packet mux in FLV/RTMP format defined in flv specification.
    * demux the audio speicified data(sound_format, sound_size, ...) to sample.
    * demux the aac specified data(aac_profile, ...) to codec from sequence header.
    * demux the aac raw to sample units.
    */
    virtual int audio_aac_demux(char* data, int size, SrsCodecSample* sample);
    virtual int audio_mp3_demux(char* data, int size, SrsCodecSample* sample);
    /**
    * demux the video packet in h.264 codec.
    * the packet mux in FLV/RTMP format defined in flv specification.
    * demux the video specified data(frame_type, codec_id, ...) to sample.
    * demux the h.264 sepcified data(avc_profile, ...) to codec from sequence header.
    * demux the h.264 NALUs to sampe units.
    */
    virtual int video_avc_demux(char* data, int size, SrsCodecSample* sample);
private:
    /**
    * when avc packet type is SrsCodecVideoAVCTypeSequenceHeader,
    * decode the sps and pps.
    */
    virtual int avc_demux_sps_pps(SrsStream* stream);
    /**
    * demux the avc NALU in "AnnexB" 
    * from H.264-AVC-ISO_IEC_14496-10.pdf, page 211.
    */
    virtual int avc_demux_annexb_format(SrsStream* stream, SrsCodecSample* sample);
    /**
    * demux the avc NALU in "ISO Base Media File Format" 
    * from H.264-AVC-ISO_IEC_14496-15.pdf, page 20
    */
    virtual int avc_demux_ibmf_format(SrsStream* stream, SrsCodecSample* sample);
};

#endif
