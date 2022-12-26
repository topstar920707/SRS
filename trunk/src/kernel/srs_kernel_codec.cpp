//
// Copyright (c) 2013-2022 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//

#include <srs_kernel_codec.hpp>

#include <string.h>
#include <stdlib.h>
using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_kernel_buffer.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_core_autofree.hpp>
#include <srs_kernel_rtc_rtp.hpp>

string srs_video_codec_id2str(SrsVideoCodecId codec)
{
    switch (codec) {
        case SrsVideoCodecIdAVC:
            return "H264";
        case SrsVideoCodecIdOn2VP6:
        case SrsVideoCodecIdOn2VP6WithAlphaChannel:
            return "VP6";
        case SrsVideoCodecIdHEVC:
            return "HEVC";
        case SrsVideoCodecIdAV1:
            return "AV1";
        case SrsVideoCodecIdReserved:
        case SrsVideoCodecIdReserved1:
        case SrsVideoCodecIdReserved2:
        case SrsVideoCodecIdDisabled:
        case SrsVideoCodecIdSorensonH263:
        case SrsVideoCodecIdScreenVideo:
        case SrsVideoCodecIdScreenVideoVersion2:
        default:
            return "Other";
    }
}

string srs_audio_codec_id2str(SrsAudioCodecId codec)
{
    switch (codec) {
        case SrsAudioCodecIdAAC:
            return "AAC";
        case SrsAudioCodecIdMP3:
            return "MP3";
        case SrsAudioCodecIdOpus:
            return "Opus";
        case SrsAudioCodecIdReserved1:
        case SrsAudioCodecIdLinearPCMPlatformEndian:
        case SrsAudioCodecIdADPCM:
        case SrsAudioCodecIdLinearPCMLittleEndian:
        case SrsAudioCodecIdNellymoser16kHzMono:
        case SrsAudioCodecIdNellymoser8kHzMono:
        case SrsAudioCodecIdNellymoser:
        case SrsAudioCodecIdReservedG711AlawLogarithmicPCM:
        case SrsAudioCodecIdReservedG711MuLawLogarithmicPCM:
        case SrsAudioCodecIdReserved:
        case SrsAudioCodecIdSpeex:
        case SrsAudioCodecIdReservedMP3_8kHz:
        case SrsAudioCodecIdReservedDeviceSpecificSound:
        default:
            return "Other";
    }
}

SrsAudioSampleRate srs_audio_sample_rate_from_number(uint32_t v)
{
    if (v == 5512) return SrsAudioSampleRate5512;
    if (v == 11025) return SrsAudioSampleRate11025;
    if (v == 22050) return SrsAudioSampleRate22050;
    if (v == 44100) return SrsAudioSampleRate44100;

    if (v == 12000) return SrsAudioSampleRate12000;
    if (v == 24000) return SrsAudioSampleRate24000;
    if (v == 48000) return SrsAudioSampleRate48000;

    if (v == 8000) return SrsAudioSampleRateNB8kHz;
    if (v == 12000) return SrsAudioSampleRateMB12kHz;
    if (v == 16000) return SrsAudioSampleRateWB16kHz;
    if (v == 24000) return SrsAudioSampleRateSWB24kHz;
    if (v == 48000) return SrsAudioSampleRateFB48kHz;

    return SrsAudioSampleRateForbidden;
}

SrsAudioSampleRate srs_audio_sample_rate_guess_number(uint32_t v)
{
    if (v >= 48000) return SrsAudioSampleRate48000;
    if (v >= 44100) return SrsAudioSampleRate44100;
    if (v >= 24000) return SrsAudioSampleRate24000;
    if (v >= 24000) return SrsAudioSampleRate24000;
    if (v >= 22050) return SrsAudioSampleRate22050;
    if (v >= 16000) return SrsAudioSampleRateWB16kHz;
    if (v >= 12000) return SrsAudioSampleRate12000;
    if (v >= 8000) return SrsAudioSampleRateNB8kHz;
    if (v >= 5512) return SrsAudioSampleRate5512;

    return SrsAudioSampleRateForbidden;
}

uint32_t srs_audio_sample_rate2number(SrsAudioSampleRate v)
{
    if (v == SrsAudioSampleRate5512) return 5512;
    if (v == SrsAudioSampleRate11025) return 11025;
    if (v == SrsAudioSampleRate22050) return 22050;
    if (v == SrsAudioSampleRate44100) return 44100;

    if (v == SrsAudioSampleRate12000) return 12000;
    if (v == SrsAudioSampleRate24000) return 24000;
    if (v == SrsAudioSampleRate48000) return 48000;

    if (v == SrsAudioSampleRateNB8kHz) return 8000;
    if (v == SrsAudioSampleRateMB12kHz) return 12000;
    if (v == SrsAudioSampleRateWB16kHz) return 16000;
    if (v == SrsAudioSampleRateSWB24kHz) return 24000;
    if (v == SrsAudioSampleRateFB48kHz) return 48000;

    return 0;
}

string srs_audio_sample_rate2str(SrsAudioSampleRate v)
{
    switch (v) {
        case SrsAudioSampleRate5512: return "5512";
        case SrsAudioSampleRate11025: return "11025";
        case SrsAudioSampleRate22050: return "22050";
        case SrsAudioSampleRate44100: return "44100";
        case SrsAudioSampleRateNB8kHz: return "NB8kHz";
        case SrsAudioSampleRateMB12kHz: return "MB12kHz";
        case SrsAudioSampleRateWB16kHz: return "WB16kHz";
        case SrsAudioSampleRateSWB24kHz: return "SWB24kHz";
        case SrsAudioSampleRateFB48kHz: return "FB48kHz";
        default: return "Other";
    }
}

SrsFlvVideo::SrsFlvVideo()
{
}

SrsFlvVideo::~SrsFlvVideo()
{
}

bool SrsFlvVideo::keyframe(char* data, int size)
{
    // 2bytes required.
    if (size < 1) {
        return false;
    }
    
    char frame_type = data[0];
    frame_type = (frame_type >> 4) & 0x0F;
    
    return frame_type == SrsVideoAvcFrameTypeKeyFrame;
}

bool SrsFlvVideo::sh(char* data, int size)
{
    // Check sequence header only for H.264 or H.265
    bool codec_ok = h264(data, size);
#ifdef SRS_H265
    codec_ok = codec_ok? true : hevc(data, size);
#endif
    if (!codec_ok) return false;

    // 2bytes required.
    if (size < 2) {
        return false;
    }
    
    char frame_type = data[0];
    frame_type = (frame_type >> 4) & 0x0F;
    
    char avc_packet_type = data[1];
    
    return frame_type == SrsVideoAvcFrameTypeKeyFrame
    && avc_packet_type == SrsVideoAvcFrameTraitSequenceHeader;
}

bool SrsFlvVideo::h264(char* data, int size)
{
    // 1bytes required.
    if (size < 1) {
        return false;
    }
    
    char codec_id = data[0];
    codec_id = codec_id & 0x0F;
    
    return codec_id == SrsVideoCodecIdAVC;
}

#ifdef SRS_H265
bool SrsFlvVideo::hevc(char* data, int size)
{
    // 1bytes required.
    if (size < 1) {
        return false;
    }

    char codec_id = data[0];
    codec_id = codec_id & 0x0F;

    return codec_id == SrsVideoCodecIdHEVC;
}
#endif

bool SrsFlvVideo::acceptable(char* data, int size)
{
    // 1bytes required.
    if (size < 1) {
        return false;
    }
    
    char frame_type = data[0];
    SrsVideoCodecId codec_id = (SrsVideoCodecId)(uint8_t)(frame_type & 0x0f);
    frame_type = (frame_type >> 4) & 0x0f;
    
    if (frame_type < 1 || frame_type > 5) {
        return false;
    }
    
    if (codec_id != SrsVideoCodecIdAVC && codec_id != SrsVideoCodecIdAV1 && codec_id != SrsVideoCodecIdHEVC) {
        return false;
    }
    
    return true;
}

SrsFlvAudio::SrsFlvAudio()
{
}

SrsFlvAudio::~SrsFlvAudio()
{
}

bool SrsFlvAudio::sh(char* data, int size)
{
    // sequence header only for aac
    if (!aac(data, size)) {
        return false;
    }
    
    // 2bytes required.
    if (size < 2) {
        return false;
    }
    
    char aac_packet_type = data[1];
    
    return aac_packet_type == SrsAudioAacFrameTraitSequenceHeader;
}

bool SrsFlvAudio::aac(char* data, int size)
{
    // 1bytes required.
    if (size < 1) {
        return false;
    }
    
    char sound_format = data[0];
    sound_format = (sound_format >> 4) & 0x0F;
    
    return sound_format == SrsAudioCodecIdAAC;
}

/**
 * the public data, event HLS disable, others can use it.
 */
// 0 = 5.5 kHz = 5512 Hz
// 1 = 11 kHz = 11025 Hz
// 2 = 22 kHz = 22050 Hz
// 3 = 44 kHz = 44100 Hz
int srs_flv_srates[] = {5512, 11025, 22050, 44100, 0};

// the sample rates in the codec,
// in the sequence header.
int srs_aac_srates[] =
{
    96000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025,  8000,
    7350,     0,     0,    0
};

string srs_audio_sample_bits2str(SrsAudioSampleBits v)
{
    switch (v) {
        case SrsAudioSampleBits16bit: return "16bits";
        case SrsAudioSampleBits8bit: return "8bits";
        default: return "Other";
    }
}

string srs_audio_channels2str(SrsAudioChannels v)
{
    switch (v) {
        case SrsAudioChannelsStereo: return "Stereo";
        case SrsAudioChannelsMono: return "Mono";
        default: return "Other";
    }
}

string srs_avc_nalu2str(SrsAvcNaluType nalu_type)
{
    switch (nalu_type) {
        case SrsAvcNaluTypeNonIDR: return "NonIDR";
        case SrsAvcNaluTypeDataPartitionA: return "DataPartitionA";
        case SrsAvcNaluTypeDataPartitionB: return "DataPartitionB";
        case SrsAvcNaluTypeDataPartitionC: return "DataPartitionC";
        case SrsAvcNaluTypeIDR: return "IDR";
        case SrsAvcNaluTypeSEI: return "SEI";
        case SrsAvcNaluTypeSPS: return "SPS";
        case SrsAvcNaluTypePPS: return "PPS";
        case SrsAvcNaluTypeAccessUnitDelimiter: return "AccessUnitDelimiter";
        case SrsAvcNaluTypeEOSequence: return "EOSequence";
        case SrsAvcNaluTypeEOStream: return "EOStream";
        case SrsAvcNaluTypeFilterData: return "FilterData";
        case SrsAvcNaluTypeSPSExt: return "SPSExt";
        case SrsAvcNaluTypePrefixNALU: return "PrefixNALU";
        case SrsAvcNaluTypeSubsetSPS: return "SubsetSPS";
        case SrsAvcNaluTypeLayerWithoutPartition: return "LayerWithoutPartition";
        case SrsAvcNaluTypeCodedSliceExt: return "CodedSliceExt";
        case SrsAvcNaluTypeReserved: default: return "Other";
    }
}

string srs_aac_profile2str(SrsAacProfile aac_profile)
{
    switch (aac_profile) {
        case SrsAacProfileMain: return "Main";
        case SrsAacProfileLC: return "LC";
        case SrsAacProfileSSR: return "SSR";
        default: return "Other";
    }
}

string srs_aac_object2str(SrsAacObjectType aac_object)
{
    switch (aac_object) {
        case SrsAacObjectTypeAacMain: return "Main";
        case SrsAacObjectTypeAacHE: return "HE";
        case SrsAacObjectTypeAacHEV2: return "HEv2";
        case SrsAacObjectTypeAacLC: return "LC";
        case SrsAacObjectTypeAacSSR: return "SSR";
        default: return "Other";
    }
}

SrsAacObjectType srs_aac_ts2rtmp(SrsAacProfile profile)
{
    switch (profile) {
        case SrsAacProfileMain: return SrsAacObjectTypeAacMain;
        case SrsAacProfileLC: return SrsAacObjectTypeAacLC;
        case SrsAacProfileSSR: return SrsAacObjectTypeAacSSR;
        default: return SrsAacObjectTypeReserved;
    }
}

SrsAacProfile srs_aac_rtmp2ts(SrsAacObjectType object_type)
{
    switch (object_type) {
        case SrsAacObjectTypeAacMain: return SrsAacProfileMain;
        case SrsAacObjectTypeAacHE:
        case SrsAacObjectTypeAacHEV2:
        case SrsAacObjectTypeAacLC: return SrsAacProfileLC;
        case SrsAacObjectTypeAacSSR: return SrsAacProfileSSR;
        default: return SrsAacProfileReserved;
    }
}

string srs_avc_profile2str(SrsAvcProfile profile)
{
    switch (profile) {
        case SrsAvcProfileBaseline: return "Baseline";
        case SrsAvcProfileConstrainedBaseline: return "Baseline(Constrained)";
        case SrsAvcProfileMain: return "Main";
        case SrsAvcProfileExtended: return "Extended";
        case SrsAvcProfileHigh: return "High";
        case SrsAvcProfileHigh10: return "High(10)";
        case SrsAvcProfileHigh10Intra: return "High(10+Intra)";
        case SrsAvcProfileHigh422: return "High(422)";
        case SrsAvcProfileHigh422Intra: return "High(422+Intra)";
        case SrsAvcProfileHigh444: return "High(444)";
        case SrsAvcProfileHigh444Predictive: return "High(444+Predictive)";
        case SrsAvcProfileHigh444Intra: return "High(444+Intra)";
        default: return "Other";
    }
}

string srs_avc_level2str(SrsAvcLevel level)
{
    switch (level) {
        case SrsAvcLevel_1: return "1";
        case SrsAvcLevel_11: return "1.1";
        case SrsAvcLevel_12: return "1.2";
        case SrsAvcLevel_13: return "1.3";
        case SrsAvcLevel_2: return "2";
        case SrsAvcLevel_21: return "2.1";
        case SrsAvcLevel_22: return "2.2";
        case SrsAvcLevel_3: return "3";
        case SrsAvcLevel_31: return "3.1";
        case SrsAvcLevel_32: return "3.2";
        case SrsAvcLevel_4: return "4";
        case SrsAvcLevel_41: return "4.1";
        case SrsAvcLevel_5: return "5";
        case SrsAvcLevel_51: return "5.1";
        default: return "Other";
    }
}

#ifdef SRS_H265

string srs_hevc_profile2str(SrsHevcProfile profile)
{
    switch (profile) {
        case SrsHevcProfileMain: return "Main";
        case SrsHevcProfileMain10: return "Main10";
        case SrsHevcProfileMainStillPicture: return "Main Still Picture";
        case SrsHevcProfileRext: return "Rext";
        default: return "Other";
    }
}

string srs_hevc_level2str(SrsHevcLevel level)
{
    switch (level) {
        case SrsHevcLevel_1: return "1";
        case SrsHevcLevel_2: return "2";
        case SrsHevcLevel_21: return "2.1";
        case SrsHevcLevel_3: return "3";
        case SrsHevcLevel_31: return "3.1";
        case SrsHevcLevel_4: return "4";
        case SrsHevcLevel_41: return "4.1";
        case SrsHevcLevel_5: return "5";
        case SrsHevcLevel_51: return "5.1";
        case SrsHevcLevel_52: return "5.2";
        case SrsHevcLevel_6: return "6";
        case SrsHevcLevel_61: return "6.1";
        case SrsHevcLevel_62: return "6.2";
        default: return "Other";
    }
}

#endif

SrsSample::SrsSample()
{
    size = 0;
    bytes = NULL;
    bframe = false;
}

SrsSample::SrsSample(char* b, int s)
{
    size = s;
    bytes = b;
    bframe = false;
}

SrsSample::~SrsSample()
{
}

srs_error_t SrsSample::parse_bframe()
{
    srs_error_t err = srs_success;

    uint8_t header = bytes[0];
    SrsAvcNaluType nal_type = (SrsAvcNaluType)(header & kNalTypeMask);

    if (nal_type != SrsAvcNaluTypeNonIDR && nal_type != SrsAvcNaluTypeDataPartitionA && nal_type != SrsAvcNaluTypeIDR) {
        return err;
    }

    SrsBuffer* stream = new SrsBuffer(bytes, size);
    SrsAutoFree(SrsBuffer, stream);

    // Skip nalu header.
    stream->skip(1);

    SrsBitBuffer bitstream(stream);
    int32_t first_mb_in_slice = 0;
    if ((err = srs_avc_nalu_read_uev(&bitstream, first_mb_in_slice)) != srs_success) {
        return srs_error_wrap(err, "nalu read uev");
    }

    int32_t slice_type_v = 0;
    if ((err = srs_avc_nalu_read_uev(&bitstream, slice_type_v)) != srs_success) {
        return srs_error_wrap(err, "nalu read uev");
    }
    SrsAvcSliceType slice_type = (SrsAvcSliceType)slice_type_v;

    if (slice_type == SrsAvcSliceTypeB || slice_type == SrsAvcSliceTypeB1) {
        bframe = true;
        srs_verbose("nal_type=%d, slice type=%d", nal_type, slice_type);
    }

    return err;
}

SrsSample* SrsSample::copy()
{
    SrsSample* p = new SrsSample();
    p->bytes = bytes;
    p->size = size;
    p->bframe = bframe;
    return p;
}

SrsCodecConfig::SrsCodecConfig()
{
}

SrsCodecConfig::~SrsCodecConfig()
{
}

SrsAudioCodecConfig::SrsAudioCodecConfig()
{
    id = SrsAudioCodecIdForbidden;
    sound_rate = SrsAudioSampleRateForbidden;
    sound_size = SrsAudioSampleBitsForbidden;
    sound_type = SrsAudioChannelsForbidden;
    
    audio_data_rate = 0;
    
    aac_object = SrsAacObjectTypeForbidden;
    aac_sample_rate = SrsAacSampleRateUnset; // sample rate ignored
    aac_channels = 0;
}

SrsAudioCodecConfig::~SrsAudioCodecConfig()
{
}

bool SrsAudioCodecConfig::is_aac_codec_ok()
{
    return !aac_extra_data.empty();
}

SrsVideoCodecConfig::SrsVideoCodecConfig()
{
    id = SrsVideoCodecIdForbidden;
    video_data_rate = 0;
    frame_rate = duration = 0;
    
    width = 0;
    height = 0;
    
    NAL_unit_length = 0;
    avc_profile = SrsAvcProfileReserved;
    avc_level = SrsAvcLevelReserved;
    
    payload_format = SrsAvcPayloadFormatGuess;
}

SrsVideoCodecConfig::~SrsVideoCodecConfig()
{
}

bool SrsVideoCodecConfig::is_avc_codec_ok()
{
    return !avc_extra_data.empty();
}

SrsFrame::SrsFrame()
{
    codec = NULL;
    nb_samples = 0;
    dts = 0;
    cts = 0;
}

SrsFrame::~SrsFrame()
{
}

srs_error_t SrsFrame::initialize(SrsCodecConfig* c)
{
    codec = c;
    nb_samples = 0;
    dts = 0;
    cts = 0;
    return srs_success;
}

srs_error_t SrsFrame::add_sample(char* bytes, int size)
{
    srs_error_t err = srs_success;

    // Ignore empty sample.
    if (!bytes || size <= 0) return err;
    
    if (nb_samples >= SrsMaxNbSamples) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "Frame samples overflow");
    }
    
    SrsSample* sample = &samples[nb_samples++];
    sample->bytes = bytes;
    sample->size = size;
    sample->bframe = false;
    
    return err;
}

SrsAudioFrame::SrsAudioFrame()
{
    aac_packet_type = SrsAudioAacFrameTraitForbidden;
}

SrsAudioFrame::~SrsAudioFrame()
{
}

SrsAudioCodecConfig* SrsAudioFrame::acodec()
{
    return (SrsAudioCodecConfig*)codec;
}

SrsVideoFrame::SrsVideoFrame()
{
    frame_type = SrsVideoAvcFrameTypeForbidden;
    avc_packet_type = SrsVideoAvcFrameTraitForbidden;
    has_idr = has_aud = has_sps_pps = false;
    first_nalu_type = SrsAvcNaluTypeForbidden;
}

SrsVideoFrame::~SrsVideoFrame()
{
}

srs_error_t SrsVideoFrame::initialize(SrsCodecConfig* c)
{
    first_nalu_type = SrsAvcNaluTypeForbidden;
    has_idr = has_sps_pps = has_aud = false;
    return SrsFrame::initialize(c);
}

srs_error_t SrsVideoFrame::add_sample(char* bytes, int size)
{
    srs_error_t err = srs_success;
    
    if ((err = SrsFrame::add_sample(bytes, size)) != srs_success) {
        return srs_error_wrap(err, "add frame");
    }

    SrsVideoCodecConfig* c = vcodec();
    if (size <= 0) return err;

    // For HEVC(H.265), try to parse the IDR from NALUs.
    if (c && c->id == SrsVideoCodecIdHEVC) {
#ifdef SRS_H265
        SrsHevcNaluType nalu_type = SrsHevcNaluTypeParse(bytes[0]);
        has_idr = (SrsHevcNaluType_CODED_SLICE_BLA <= nalu_type) && (nalu_type <= SrsHevcNaluType_RESERVED_23);
        return err;
#else
        return srs_error_new(ERROR_HEVC_DISABLED, "H.265 is disabled");
#endif
    }

    // By default, use AVC(H.264) to parse NALU.
    // For video, parse the nalu type, set the IDR flag.
    SrsAvcNaluType nal_unit_type = (SrsAvcNaluType)(bytes[0] & 0x1f);
    
    if (nal_unit_type == SrsAvcNaluTypeIDR) {
        has_idr = true;
    } else if (nal_unit_type == SrsAvcNaluTypeSPS || nal_unit_type == SrsAvcNaluTypePPS) {
        has_sps_pps = true;
    } else if (nal_unit_type == SrsAvcNaluTypeAccessUnitDelimiter) {
        has_aud = true;
    }
    
    if (first_nalu_type == SrsAvcNaluTypeReserved) {
        first_nalu_type = nal_unit_type;
    }
    
    return err;
}

SrsVideoCodecConfig* SrsVideoFrame::vcodec()
{
    return (SrsVideoCodecConfig*)codec;
}

SrsFormat::SrsFormat()
{
    acodec = NULL;
    vcodec = NULL;
    audio = NULL;
    video = NULL;
    avc_parse_sps = true;
    try_annexb_first = true;
    raw = NULL;
    nb_raw = 0;
}

SrsFormat::~SrsFormat()
{
    srs_freep(audio);
    srs_freep(video);
    srs_freep(acodec);
    srs_freep(vcodec);
}

srs_error_t SrsFormat::initialize()
{
    return srs_success;
}

srs_error_t SrsFormat::on_audio(int64_t timestamp, char* data, int size)
{
    srs_error_t err = srs_success;
    
    if (!data || size <= 0) {
        srs_info("no audio present, ignore it.");
        return err;
    }
    
    SrsBuffer* buffer = new SrsBuffer(data, size);
    SrsAutoFree(SrsBuffer, buffer);
    
    // We already checked the size is positive and data is not NULL.
    srs_assert(buffer->require(1));
    
    // @see: E.4.2 Audio Tags, video_file_format_spec_v10_1.pdf, page 76
    uint8_t v = buffer->read_1bytes();
    SrsAudioCodecId codec = (SrsAudioCodecId)((v >> 4) & 0x0f);
    
    if (codec != SrsAudioCodecIdMP3 && codec != SrsAudioCodecIdAAC) {
        return err;
    }

    bool fresh = !acodec;
    if (!acodec) {
        acodec = new SrsAudioCodecConfig();
    }
    if (!audio) {
        audio = new SrsAudioFrame();
    }
    
    if ((err = audio->initialize(acodec)) != srs_success) {
        return srs_error_wrap(err, "init audio");
    }
    
    // Parse by specified codec.
    buffer->skip(-1 * buffer->pos());
    
    if (codec == SrsAudioCodecIdMP3) {
        return audio_mp3_demux(buffer, timestamp, fresh);
    }
    
    return audio_aac_demux(buffer, timestamp);
}

srs_error_t SrsFormat::on_video(int64_t timestamp, char* data, int size)
{
    srs_error_t err = srs_success;
    
    if (!data || size <= 0) {
        srs_trace("no video present, ignore it.");
        return err;
    }
    
    SrsBuffer* buffer = new SrsBuffer(data, size);
    SrsAutoFree(SrsBuffer, buffer);
    
    // We already checked the size is positive and data is not NULL.
    srs_assert(buffer->require(1));
    
    // @see: E.4.3 Video Tags, video_file_format_spec_v10_1.pdf, page 78
    int8_t frame_type = buffer->read_1bytes();
    SrsVideoCodecId codec_id = (SrsVideoCodecId)(frame_type & 0x0f);

    // Check codec for H.264 and H.265.
    bool codec_ok = (codec_id == SrsVideoCodecIdAVC);
#ifdef SRS_H265
    codec_ok = codec_ok ? true : (codec_id == SrsVideoCodecIdHEVC);
#endif
    if (!codec_ok) return err;
    
    if (!vcodec) {
        vcodec = new SrsVideoCodecConfig();
    }
    if (!video) {
        video = new SrsVideoFrame();
    }
    
    if ((err = video->initialize(vcodec)) != srs_success) {
        return srs_error_wrap(err, "init video");
    }
    
    buffer->skip(-1 * buffer->pos());
    return video_avc_demux(buffer, timestamp);
}

srs_error_t SrsFormat::on_aac_sequence_header(char* data, int size)
{
    srs_error_t err = srs_success;
    
    if (!acodec) {
        acodec = new SrsAudioCodecConfig();
    }
    if (!audio) {
        audio = new SrsAudioFrame();
    }
    
    if ((err = audio->initialize(acodec)) != srs_success) {
        return srs_error_wrap(err, "init audio");
    }
    
    return audio_aac_sequence_header_demux(data, size);
}

bool SrsFormat::is_aac_sequence_header()
{
    return acodec && acodec->id == SrsAudioCodecIdAAC
        && audio && audio->aac_packet_type == SrsAudioAacFrameTraitSequenceHeader;
}

bool SrsFormat::is_mp3_sequence_header()
{
    return acodec && acodec->id == SrsAudioCodecIdMP3
        && audio && audio->aac_packet_type == SrsAudioMp3FrameTraitSequenceHeader;
}

bool SrsFormat::is_avc_sequence_header()
{
    bool h264 = (vcodec && vcodec->id == SrsVideoCodecIdAVC);
    bool h265 = (vcodec && vcodec->id == SrsVideoCodecIdHEVC);
    bool av1 = (vcodec && vcodec->id == SrsVideoCodecIdAV1);
    return vcodec && (h264 || h265 || av1)
        && video && video->avc_packet_type == SrsVideoAvcFrameTraitSequenceHeader;
}

srs_error_t SrsFormat::video_avc_demux(SrsBuffer* stream, int64_t timestamp)
{
    srs_error_t err = srs_success;
    
    // @see: E.4.3 Video Tags, video_file_format_spec_v10_1.pdf, page 78
    int8_t frame_type = stream->read_1bytes();
    SrsVideoCodecId codec_id = (SrsVideoCodecId)(frame_type & 0x0f);
    frame_type = (frame_type >> 4) & 0x0f;
    
    video->frame_type = (SrsVideoAvcFrameType)frame_type;
    
    // ignore info frame without error,
    // @see https://github.com/ossrs/srs/issues/288#issuecomment-69863909
    if (video->frame_type == SrsVideoAvcFrameTypeVideoInfoFrame) {
        srs_warn("avc igone the info frame");
        return err;
    }

    // Check codec for H.264 and H.265.
    bool codec_ok = (codec_id == SrsVideoCodecIdAVC);
#ifdef SRS_H265
    codec_ok = codec_ok ? true : (codec_id == SrsVideoCodecIdHEVC);
#endif
    if (!codec_ok) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "only support video H.264/H.265, actual=%d", codec_id);
    }
    vcodec->id = codec_id;
    
    if (!stream->require(4)) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "avc decode avc_packet_type");
    }
    int8_t avc_packet_type = stream->read_1bytes();
    int32_t composition_time = stream->read_3bytes();
    
    // pts = dts + cts.
    video->dts = timestamp;
    video->cts = composition_time;
    video->avc_packet_type = (SrsVideoAvcFrameTrait)avc_packet_type;
    
    // Update the RAW AVC data.
    raw = stream->data() + stream->pos();
    nb_raw = stream->size() - stream->pos();

    // Parse sequence header for H.265/HEVC.
    if (codec_id == SrsVideoCodecIdHEVC) {
#ifdef SRS_H265
        if (avc_packet_type == SrsVideoAvcFrameTraitSequenceHeader) {
            // TODO: demux vps/sps/pps for hevc
            if ((err = hevc_demux_hvcc(stream)) != srs_success) {
                return srs_error_wrap(err, "demux hevc SPS/PPS");
            }
        } else if (avc_packet_type == SrsVideoAvcFrameTraitNALU) {
            // TODO: demux nalu for hevc
            if ((err = video_nalu_demux(stream)) != srs_success) {
                return srs_error_wrap(err, "demux hevc NALU");
            }
        }
        return err;
#else
        return srs_error_new(ERROR_HEVC_DISABLED, "H.265 is disabled");
#endif
    }

    // Parse sequence header for H.264/AVC.
    if (avc_packet_type == SrsVideoAvcFrameTraitSequenceHeader) {
        // TODO: FIXME: Maybe we should ignore any error for parsing sps/pps.
        if ((err = avc_demux_sps_pps(stream)) != srs_success) {
            return srs_error_wrap(err, "demux SPS/PPS");
        }
    } else if (avc_packet_type == SrsVideoAvcFrameTraitNALU){
        if ((err = video_nalu_demux(stream)) != srs_success) {
            return srs_error_wrap(err, "demux NALU");
        }
    } else {
        // ignored.
    }
    
    return err;
}

// For media server, we don't care the codec, so we just try to parse sps-pps, and we could ignore any error if fail.
// LCOV_EXCL_START

#ifdef SRS_H265
// Parse the hevc vps/sps/pps
srs_error_t SrsFormat::hevc_demux_hvcc(SrsBuffer* stream)
{
    srs_error_t err = srs_success;

    int avc_extra_size = stream->size() - stream->pos();
    if (avc_extra_size > 0) {
        char *copy_stream_from = stream->data() + stream->pos();
        vcodec->avc_extra_data = std::vector<char>(copy_stream_from, copy_stream_from + avc_extra_size);
    }

    const int HEVC_MIN_SIZE = 23; // From configuration_version to numOfArrays
    if (!stream->require(HEVC_MIN_SIZE)) {
        return srs_error_new(ERROR_HEVC_DECODE_ERROR, "requires %d only %d bytes", HEVC_MIN_SIZE, stream->left());
    }

    SrsHevcDecoderConfigurationRecord* dec_conf_rec_p = &(vcodec->hevc_dec_conf_record_);
    dec_conf_rec_p->configuration_version = stream->read_1bytes();
    if (dec_conf_rec_p->configuration_version != 1) {
        return srs_error_new(ERROR_HEVC_DECODE_ERROR, "invalid version=%d", dec_conf_rec_p->configuration_version);
    }

    // Read general_profile_space(2bits), general_tier_flag(1bit), general_profile_idc(5bits)
    uint8_t data_byte = stream->read_1bytes();
    dec_conf_rec_p->general_profile_space = (data_byte >> 6) & 0x03;
    dec_conf_rec_p->general_tier_flag = (data_byte >> 5) & 0x01;
    dec_conf_rec_p->general_profile_idc = data_byte & 0x1F;
    srs_info("hevc version:%d, general_profile_space:%d, general_tier_flag:%d, general_profile_idc:%d",
        dec_conf_rec_p->configuration_version, dec_conf_rec_p->general_profile_space, dec_conf_rec_p->general_tier_flag,
        dec_conf_rec_p->general_profile_idc);

    //general_profile_compatibility_flags: 32bits
    dec_conf_rec_p->general_profile_compatibility_flags = (uint32_t)stream->read_4bytes();

    //general_constraint_indicator_flags: 48bits
    uint64_t data_64bit = (uint64_t)stream->read_4bytes();
    data_64bit = (data_64bit << 16) | (stream->read_2bytes());
    dec_conf_rec_p->general_constraint_indicator_flags = data_64bit;

    //general_level_idc: 8bits
    dec_conf_rec_p->general_level_idc = stream->read_1bytes();
    //min_spatial_segmentation_idc: xxxx 14bits
    dec_conf_rec_p->min_spatial_segmentation_idc = stream->read_2bytes() & 0x0fff;
    //parallelism_type: xxxx xx 2bits
    dec_conf_rec_p->parallelism_type = stream->read_1bytes() & 0x03;
    //chroma_format: xxxx xx 2bits
    dec_conf_rec_p->chroma_format = stream->read_1bytes() & 0x03;
    //bit_depth_luma_minus8: xxxx x 3bits
    dec_conf_rec_p->bit_depth_luma_minus8 = stream->read_1bytes() & 0x07;
    //bit_depth_chroma_minus8: xxxx x 3bits
    dec_conf_rec_p->bit_depth_chroma_minus8 = stream->read_1bytes() & 0x07;
    srs_info("general_constraint_indicator_flags:0x%x, general_level_idc:%d, min_spatial_segmentation_idc:%d, parallelism_type:%d, chroma_format:%d, bit_depth_luma_minus8:%d, bit_depth_chroma_minus8:%d",
        dec_conf_rec_p->general_constraint_indicator_flags, dec_conf_rec_p->general_level_idc,
        dec_conf_rec_p->min_spatial_segmentation_idc, dec_conf_rec_p->parallelism_type, dec_conf_rec_p->chroma_format,
        dec_conf_rec_p->bit_depth_luma_minus8, dec_conf_rec_p->bit_depth_chroma_minus8);

    //avg_frame_rate: 16bits
    vcodec->frame_rate = dec_conf_rec_p->avg_frame_rate = stream->read_2bytes();
    //8bits: constant_frame_rate(2bits), num_temporal_layers(3bits),
    //       temporal_id_nested(1bit), length_size_minus_one(2bits)
    data_byte = stream->read_1bytes();
    dec_conf_rec_p->constant_frame_rate = (data_byte >> 6) & 0x03;
    dec_conf_rec_p->num_temporal_layers = (data_byte >> 3) & 0x07;
    dec_conf_rec_p->temporal_id_nested  = (data_byte >> 2) & 0x01;

    // Parse the NALU size.
    dec_conf_rec_p->length_size_minus_one = data_byte & 0x03;
    vcodec->NAL_unit_length = dec_conf_rec_p->length_size_minus_one;

    // 5.3.4.2.1 Syntax, ISO_IEC_14496-15-AVC-format-2012.pdf, page 16
    // 5.2.4.1 AVC decoder configuration record
    // 5.2.4.1.2 Semantics
    // The value of this field shall be one of 0, 1, or 3 corresponding to a
    // length encoded with 1, 2, or 4 bytes, respectively.
    if (vcodec->NAL_unit_length == 2) {
        return srs_error_new(ERROR_HEVC_DECODE_ERROR, "sps lengthSizeMinusOne should never be 2");
    }

    uint8_t numOfArrays = stream->read_1bytes();
    srs_info("avg_frame_rate:%d, constant_frame_rate:%d, num_temporal_layers:%d, temporal_id_nested:%d, length_size_minus_one:%d, numOfArrays:%d",
        dec_conf_rec_p->avg_frame_rate, dec_conf_rec_p->constant_frame_rate, dec_conf_rec_p->num_temporal_layers,
        dec_conf_rec_p->temporal_id_nested, dec_conf_rec_p->length_size_minus_one, numOfArrays);

    //parse vps/pps/sps
    for (int index = 0; index < numOfArrays; index++) {
        SrsHevcHvccNalu hevc_unit;

        if (!stream->require(5)) {
            return srs_error_new(ERROR_HEVC_DECODE_ERROR, "requires 5 only %d bytes", stream->left());
        }
        data_byte = stream->read_1bytes();
        hevc_unit.array_completeness = (data_byte >> 7) & 0x01;
        hevc_unit.nal_unit_type = data_byte & 0x3f;
        hevc_unit.num_nalus = stream->read_2bytes();

        for (int i = 0; i < hevc_unit.num_nalus; i++) {
            SrsHevcNalData data_item;
            data_item.nal_unit_length = stream->read_2bytes();

            if (!stream->require(data_item.nal_unit_length)) {
                return srs_error_new(ERROR_HEVC_DECODE_ERROR, "requires %d only %d bytes",
                    data_item.nal_unit_length, stream->left());
            }
            //copy vps/pps/sps data
            data_item.nal_unit_data.resize(data_item.nal_unit_length);

            stream->read_bytes((char*)(&data_item.nal_unit_data[0]), data_item.nal_unit_length);
            srs_info("hevc nalu type:%d, array_completeness:%d, num_nalus:%d, i:%d, nal_unit_length:%d",
                hevc_unit.nal_unit_type, hevc_unit.array_completeness, hevc_unit.num_nalus, i, data_item.nal_unit_length);
            hevc_unit.nal_data_vec.push_back(data_item);
        }
        dec_conf_rec_p->nalu_vec.push_back(hevc_unit);

        // demux nalu
        if ((err = hevc_demux_vps_sps_pps(&hevc_unit)) != srs_success) {
            return srs_error_wrap(err, "hevc demux vps sps pps failed");
        }
    }

    return err;
}

srs_error_t SrsFormat::hevc_demux_vps_sps_pps(SrsHevcHvccNalu* nal)
{
    srs_error_t err = srs_success;

    // nal data
    switch (nal->nal_unit_type)
    {
        case SrsHevcNaluType_VPS:
            // TODO: FIXME: implements it.
            break;
        case SrsHevcNaluType_SPS:
            err = hevc_demux_sps(nal);
            break;
        case SrsHevcNaluType_PPS:
            // TODO: FIXME: implements it.
            break;
        default:
            break;
    }

    return err;
}

srs_error_t SrsFormat::hevc_demux_sps(SrsHevcHvccNalu* nal)
{
    srs_error_t err = srs_success;

    if (nal->nal_data_vec.empty()) {
        return err;
    }

    // FIXME: Support for multiple SPS, then pick the first non-empty one.
    char* sps = (char*)(&nal->nal_data_vec[0].nal_unit_data[0]);
    int nb_sps = nal->nal_data_vec[0].nal_unit_length;

    SrsBuffer stream(sps, nb_sps);

    // for NALU, ITU-T H.265 7.3.2.2 Sequence parameter set RBSP syntax
    // T-REC-H.265-202108-I!!PDF-E.pdf, page 33.
    if (!stream.require(1)) {
        return srs_error_new(ERROR_HEVC_DECODE_ERROR, "decode hevc sps requires 1 only %d bytes", stream.left());
    }
    int8_t nutv = stream.read_1bytes();

    // forbidden_zero_bit shall be equal to 0.
    int8_t forbidden_zero_bit = (nutv >> 7) & 0x01;
    if (forbidden_zero_bit) {
        return srs_error_new(ERROR_HEVC_DECODE_ERROR, "hevc forbidden_zero_bit=%d shall be equal to 0", forbidden_zero_bit);
    }

    // 7.4.2 NAL unit semantics
    // T-REC-H.265-202108-I!!PDF-E.pdf, page 64.
    // nal_unit_type specifies the type of RBSP data structure contained in the NAL unit as specified in Table 7-1.
    SrsHevcNaluType nal_unit_type = (SrsHevcNaluType)((nutv >> 1) & 0x3f);
    if (nal_unit_type != SrsHevcNaluType_SPS) {
        return srs_error_new(ERROR_HEVC_DECODE_ERROR, "hevc sps nal_unit_type=%d shall be equal to 33", nal_unit_type);
    }

    // nuh(nuh_layer_id + nuh_temporal_id_plus1)
    int8_t nuh = stream.read_1bytes();
    (void)nuh;

    // decode the rbsp from sps.
    // rbsp[ i ] a raw byte sequence payload is specified as an ordered sequence of bytes.
    std::vector<int8_t> rbsp(nb_sps);

    int nb_rbsp = 0;
    while (!stream.empty()) {
        rbsp[nb_rbsp] = stream.read_1bytes();

        // XX 00 00 03 XX, the 03 byte should be drop.
        if (nb_rbsp > 2 && rbsp[nb_rbsp - 2] == 0 && rbsp[nb_rbsp - 1] == 0 && rbsp[nb_rbsp] == 3) {
            // read 1byte more.
            if (stream.empty()) {
                break;
            }
            rbsp[nb_rbsp] = stream.read_1bytes();
            nb_rbsp++;

            continue;
        }

        nb_rbsp++;
    }

    return hevc_demux_sps_rbsp((char*)&rbsp[0], nb_rbsp);
}

srs_error_t SrsFormat::hevc_demux_sps_rbsp(char* rbsp, int nb_rbsp)
{
    srs_error_t err = srs_success;

    // we donot parse the detail of sps.
    // @see https://github.com/ossrs/srs/issues/474
    if (!avc_parse_sps) {
        return err;
    }

    // reparse the rbsp.
    SrsBuffer stream(rbsp, nb_rbsp);

    /* Rec. ITU-T H.265 7.3.2.2.1, page 35.
     * seq_parameter_set_rbsp
     *      sps_video_parameter_set_id              4 bits
     *      sps_max_sub_layers_minus1               3 bits
     *      sps_temporal_id_nesting_flag            1 bit
     *
     *      sps_seq_parameter_set_id                v
     *      chroma_format_idc                       v
     *      if (chroma_format_idc == 3)
     *          separate_colour_plane_flag          1 bit
     *      pic_width_in_luma_samples               v
     *      pic_height_in_luma_samples              v
     *      conformance_window_flag                 1 bit
     *      if (conformance_window_flag) {
     *          conf_win_left_offset                v
     *          conf_win_right_offset               v
     *          conf_win_top_offset                 v
     *          conf_win_bottom_offset              v
     *      }
     */
    if (!stream.require(2)) {
        return srs_error_new(ERROR_HEVC_DECODE_ERROR, "sps requires 2 only %d bytes", stream.left());
    }
    uint8_t nutv = stream.read_1bytes();
    (void)nutv;

    // id/minus/flag
    int sps_video_parameter_set_id = (nutv >> 4) & 0x0f;
    int sps_max_sub_layers_minus1 = (nutv >> 1) & 0x07;
    int sps_temporal_id_nesting_flag = nutv & 0x01;
    (void)sps_video_parameter_set_id;
    (void)sps_temporal_id_nesting_flag;

    SrsBitBuffer bs(&stream);

    // profile tier level...
    SrsHevcSpsProfileTierLevel profile_tier_level;
    memset((void*)&profile_tier_level, 0, sizeof(SrsHevcSpsProfileTierLevel));

    if ((err = hevc_demux_sps_rbsp_ptl(&bs, &profile_tier_level, 1, sps_max_sub_layers_minus1)) != srs_success) {
        return srs_error_wrap(err, "sps rbsp ptl sps_max_sub_layers_minus1=%d", sps_max_sub_layers_minus1);
    }

    vcodec->hevc_profile = (SrsHevcProfile)profile_tier_level.general_profile_idc;
    vcodec->hevc_level = (SrsHevcLevel)profile_tier_level.general_level_idc;

    // for sps_table
    int sps_seq_parameter_set_id = bs.read_bits_ue();
    (void)sps_seq_parameter_set_id;

    int separate_colour_plane_flag = 0;
    int chroma_format_idc = bs.read_bits_ue();
    if (chroma_format_idc == 3) {
        separate_colour_plane_flag = bs.read_bits_ue();
    }

    int pic_width_in_luma_samples  = bs.read_bits_ue();
    int pic_height_in_luma_samples = bs.read_bits_ue();
    vcodec->width  = pic_width_in_luma_samples;
    vcodec->height = pic_height_in_luma_samples;

    int conformance_window_flag    = bs.read_bit();
    if (conformance_window_flag) {
        int conf_win_left_offset   = bs.read_bits_ue();
        int conf_win_right_offset  = bs.read_bits_ue();
        int conf_win_top_offset    = bs.read_bits_ue();
        int conf_win_bottom_offset = bs.read_bits_ue();

        // Table 6-1, 7.4.3.2.1
        // Recalculate width and height
        // Note: 1 is added to the manual, but it is not actually used
        // https://github.com/mbunkus/mkvtoolnix/issues/1152
        int sub_width_c  = ((1 == chroma_format_idc) || (2 == chroma_format_idc)) && (0 == separate_colour_plane_flag) ? 2 : 1;
        int sub_height_c =  (1 == chroma_format_idc) && (0 == separate_colour_plane_flag) ? 2 : 1;
        vcodec->width  -= (sub_width_c * conf_win_right_offset + sub_width_c * conf_win_left_offset);
        vcodec->height -= (sub_height_c * conf_win_bottom_offset + sub_height_c * conf_win_top_offset);
    }

    // TODO: FIXME: Implements it, you might parse remain bits for seq_parameter_set_rbsp.
    // 7.4.3.2.1
    // T-REC-H.265-202108-I!!PDF-E.pdf, page 35 ~ page 36.
    // bit_depth_luma_minus8
    // bit_depth_chroma_minus8

    // 7.3.2.11 RBSP trailing bits syntax
    // T-REC-H.265-202108-I!!PDF-E.pdf, page 41.
    // rbsp_trailing_bits()

    return err;
}

srs_error_t SrsFormat::hevc_demux_sps_rbsp_ptl(SrsBitBuffer* bs, SrsHevcSpsProfileTierLevel* ptl, int profile_resent_flag, int max_sub_layers_minus1)
{
    srs_error_t err = srs_success;

    if (profile_resent_flag) {
        if (!bs->require_bits(88)) {
            return srs_error_new(ERROR_HEVC_DECODE_ERROR, "ptl profile requires 88 only %d bits", bs->left_bits());
        }

        ptl->general_profile_space = bs->read_bits(2);
        ptl->general_tier_flag     = bs->read_bit();
        ptl->general_profile_idc   = bs->read_bits(5);
        for (int i = 0; i < 32; i++) {
            ptl->general_profile_compatibility_flag[i] = bs->read_bit();
        }

        ptl->general_progressive_source_flag    = bs->read_bit();
        ptl->general_interlaced_source_flag     = bs->read_bit();
        ptl->general_non_packed_constraint_flag = bs->read_bit();
        ptl->general_frame_only_constraint_flag = bs->read_bit();
        if (ptl->general_profile_idc == 4 || ptl->general_profile_compatibility_flag[4] ||
            ptl->general_profile_idc == 5 || ptl->general_profile_compatibility_flag[5] ||
            ptl->general_profile_idc == 6 || ptl->general_profile_compatibility_flag[6] ||
            ptl->general_profile_idc == 7 || ptl->general_profile_compatibility_flag[7])
        {
            ptl->general_max_12bit_constraint_flag      = bs->read_bit();
            ptl->general_max_10bit_constraint_flag      = bs->read_bit();
            ptl->general_max_8bit_constraint_flag       = bs->read_bit();
            ptl->general_max_422chroma_constraint_flag  = bs->read_bit();
            ptl->general_max_420chroma_constraint_flag  = bs->read_bit();
            ptl->general_max_monochrome_constraint_flag = bs->read_bit();
            ptl->general_intra_constraint_flag          = bs->read_bit();
            ptl->general_one_picture_only_constraint_flag = bs->read_bit();
            ptl->general_lower_bit_rate_constraint_flag = bs->read_bit();
            uint64_t tmp1 = bs->read_32bits();
            uint64_t tmp2 = bs->read_bits(2);
            ptl->general_reserved_zero_34bits = tmp1 + tmp2;
        } else {
            uint64_t tmp1 = bs->read_32bits();
            uint64_t tmp2 = bs->read_bits(11);
            ptl->general_reserved_zero_43bits = tmp1 + tmp2;
        }

        if ((ptl->general_profile_idc >= 1 && ptl->general_profile_idc<=5) ||
            ptl->general_profile_compatibility_flag[1] || ptl->general_profile_compatibility_flag[2] ||
            ptl->general_profile_compatibility_flag[3] || ptl->general_profile_compatibility_flag[4] ||
            ptl->general_profile_compatibility_flag[5])
        {
            ptl->general_inbld_flag = bs->read_bit();
        } else {
            ptl->general_reserved_zero_bit = bs->read_bit();
        }
    }

    if (!bs->require_bits(8)) {
        return srs_error_new(ERROR_HEVC_DECODE_ERROR, "ptl level requires 8 only %d bits", bs->left_bits());
    }

    // level
    ptl->general_level_idc = bs->read_8bits();

    ptl->sub_layer_profile_present_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_level_present_flag.resize(max_sub_layers_minus1);
    for (int i = 0; i < max_sub_layers_minus1; i++) {
        if (!bs->require_bits(2)) {
            return srs_error_new(ERROR_HEVC_DECODE_ERROR, "ptl present_flag requires 2 only %d bits", bs->left_bits());
        }
        ptl->sub_layer_profile_present_flag[i] = bs->read_bit();
        ptl->sub_layer_level_present_flag[i]   = bs->read_bit();
    }

    for (int i = max_sub_layers_minus1; max_sub_layers_minus1 > 0 && i < 8; i++) {
        if (!bs->require_bits(2)) {
            return srs_error_new(ERROR_HEVC_DECODE_ERROR, "ptl reserved_zero requires 2 only %d bits", bs->left_bits());
        }
        ptl->reserved_zero_2bits[i] = bs->read_bits(2);
    }

    ptl->sub_layer_profile_space.resize(max_sub_layers_minus1);
    ptl->sub_layer_tier_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_profile_idc.resize(max_sub_layers_minus1);
    ptl->sub_layer_profile_compatibility_flag.resize(max_sub_layers_minus1);
    for (int i = 0; i < max_sub_layers_minus1; i++) {
        ptl->sub_layer_profile_compatibility_flag[i].resize(32);
    }

    ptl->sub_layer_progressive_source_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_interlaced_source_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_non_packed_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_frame_only_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_12bit_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_10bit_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_8bit_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_422chroma_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_420chroma_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_monochrome_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_intra_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_one_picture_only_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_lower_bit_rate_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_reserved_zero_34bits.resize(max_sub_layers_minus1);
    ptl->sub_layer_reserved_zero_43bits.resize(max_sub_layers_minus1);
    ptl->sub_layer_inbld_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_reserved_zero_bit.resize(max_sub_layers_minus1);
    ptl->sub_layer_level_idc.resize(max_sub_layers_minus1);
    for (int i = 0; i < max_sub_layers_minus1; i++) {
        if (ptl->sub_layer_profile_present_flag[i]) {
            if (!bs->require_bits(88)) {
                return srs_error_new(ERROR_HEVC_DECODE_ERROR, "ptl sub_layer_profile requires 88 only %d bits", bs->left_bits());
            }
            ptl->sub_layer_profile_space[i] = bs->read_bits(2);
            ptl->sub_layer_tier_flag[i]     = bs->read_bit();
            ptl->sub_layer_profile_idc[i]   = bs->read_bits(5);
            for (int j = 0; j < 32; j++) {
                ptl->sub_layer_profile_compatibility_flag[i][j] = bs->read_bit();
            }

            ptl->sub_layer_progressive_source_flag[i]    = bs->read_bit();
            ptl->sub_layer_interlaced_source_flag[i]     = bs->read_bit();
            ptl->sub_layer_non_packed_constraint_flag[i] = bs->read_bit();
            ptl->sub_layer_frame_only_constraint_flag[i] = bs->read_bit();
            if (ptl->sub_layer_profile_idc[i] == 4 || ptl->sub_layer_profile_compatibility_flag[i][4] ||
                ptl->sub_layer_profile_idc[i] == 5 || ptl->sub_layer_profile_compatibility_flag[i][5] ||
                ptl->sub_layer_profile_idc[i] == 6 || ptl->sub_layer_profile_compatibility_flag[i][6] ||
                ptl->sub_layer_profile_idc[i] == 7 || ptl->sub_layer_profile_compatibility_flag[i][7])
            {
                ptl->sub_layer_max_12bit_constraint_flag[i]        = bs->read_bit();
                ptl->sub_layer_max_10bit_constraint_flag[i]        = bs->read_bit();
                ptl->sub_layer_max_8bit_constraint_flag[i]         = bs->read_bit();
                ptl->sub_layer_max_422chroma_constraint_flag[i]    = bs->read_bit();
                ptl->sub_layer_max_420chroma_constraint_flag[i]    = bs->read_bit();
                ptl->sub_layer_max_monochrome_constraint_flag[i]   = bs->read_bit();
                ptl->sub_layer_intra_constraint_flag[i]            = bs->read_bit();
                ptl->sub_layer_one_picture_only_constraint_flag[i] = bs->read_bit();
                ptl->sub_layer_lower_bit_rate_constraint_flag[i]   = bs->read_bit();
                uint64_t tmp1 = bs->read_32bits();
                uint64_t tmp2 = bs->read_bits(2);
                ptl->sub_layer_reserved_zero_34bits[i] = tmp1 + tmp2;
            } else {
                uint64_t tmp1 = bs->read_32bits();
                uint64_t tmp2 = bs->read_bits(11);
                ptl->sub_layer_reserved_zero_43bits[i] = tmp1 + tmp2;
            }

            // to check
            if ((ptl->sub_layer_profile_idc[i]>=1 && ptl->sub_layer_profile_idc[i] <= 5) ||
                ptl->sub_layer_profile_compatibility_flag[i][1] ||
                ptl->sub_layer_profile_compatibility_flag[i][2] ||
                ptl->sub_layer_profile_compatibility_flag[i][3] ||
                ptl->sub_layer_profile_compatibility_flag[i][4] ||
                ptl->sub_layer_profile_compatibility_flag[i][5])
            {
                ptl->sub_layer_inbld_flag[i] = bs->read_bit();
            } else {
                ptl->sub_layer_reserved_zero_bit[i] = bs->read_bit();
            }
        }

        if (ptl->sub_layer_level_present_flag[i]) {
            if (!bs->require_bits(8)) {
                return srs_error_new(ERROR_HEVC_DECODE_ERROR, "ptl sub_layer_level requires 8 only %d bits", bs->left_bits());
            }
            ptl->sub_layer_level_idc[i] = bs->read_8bits();
        }
    }

    return err;
}

#endif

srs_error_t SrsFormat::avc_demux_sps_pps(SrsBuffer* stream)
{
    // AVCDecoderConfigurationRecord
    // 5.2.4.1.1 Syntax, ISO_IEC_14496-15-AVC-format-2012.pdf, page 16
    int avc_extra_size = stream->size() - stream->pos();
    if (avc_extra_size > 0) {
        char *copy_stream_from = stream->data() + stream->pos();
        vcodec->avc_extra_data = std::vector<char>(copy_stream_from, copy_stream_from + avc_extra_size);
    }
    
    if (!stream->require(6)) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "avc decode sequence header");
    }
    //int8_t configuration_version = stream->read_1bytes();
    stream->read_1bytes();
    //int8_t AVCProfileIndication = stream->read_1bytes();
    vcodec->avc_profile = (SrsAvcProfile)stream->read_1bytes();
    //int8_t profile_compatibility = stream->read_1bytes();
    stream->read_1bytes();
    //int8_t AVCLevelIndication = stream->read_1bytes();
    vcodec->avc_level = (SrsAvcLevel)stream->read_1bytes();
    
    // parse the NALU size.
    int8_t lengthSizeMinusOne = stream->read_1bytes();
    lengthSizeMinusOne &= 0x03;
    vcodec->NAL_unit_length = lengthSizeMinusOne;
    
    // 5.3.4.2.1 Syntax, ISO_IEC_14496-15-AVC-format-2012.pdf, page 16
    // 5.2.4.1 AVC decoder configuration record
    // 5.2.4.1.2 Semantics
    // The value of this field shall be one of 0, 1, or 3 corresponding to a
    // length encoded with 1, 2, or 4 bytes, respectively.
    if (vcodec->NAL_unit_length == 2) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "sps lengthSizeMinusOne should never be 2");
    }
    
    // 1 sps, 7.3.2.1 Sequence parameter set RBSP syntax
    // ISO_IEC_14496-10-AVC-2003.pdf, page 45.
    if (!stream->require(1)) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "decode SPS");
    }
    int8_t numOfSequenceParameterSets = stream->read_1bytes();
    numOfSequenceParameterSets &= 0x1f;
    if (numOfSequenceParameterSets < 1) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "decode SPS");
    }
    // Support for multiple SPS, then pick the first non-empty one.
    for (int i = 0; i < numOfSequenceParameterSets; ++i) {
        if (!stream->require(2)) {
            return srs_error_new(ERROR_HLS_DECODE_ERROR, "decode SPS size");
        }
        uint16_t sequenceParameterSetLength = stream->read_2bytes();
        if (!stream->require(sequenceParameterSetLength)) {
            return srs_error_new(ERROR_HLS_DECODE_ERROR, "decode SPS data");
        }
        if (sequenceParameterSetLength > 0) {
            vcodec->sequenceParameterSetNALUnit.resize(sequenceParameterSetLength);
            stream->read_bytes(&vcodec->sequenceParameterSetNALUnit[0], sequenceParameterSetLength);
        }
    }

    // 1 pps
    if (!stream->require(1)) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "decode PPS");
    }
    int8_t numOfPictureParameterSets = stream->read_1bytes();
    numOfPictureParameterSets &= 0x1f;
    if (numOfPictureParameterSets < 1) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "decode SPS");
    }
    // Support for multiple PPS, then pick the first non-empty one.
    for (int i = 0; i < numOfPictureParameterSets; ++i) {
        if (!stream->require(2)) {
            return srs_error_new(ERROR_HLS_DECODE_ERROR, "decode PPS size");
        }
        uint16_t pictureParameterSetLength = stream->read_2bytes();
        if (!stream->require(pictureParameterSetLength)) {
            return srs_error_new(ERROR_HLS_DECODE_ERROR, "decode PPS data");
        }
        if (pictureParameterSetLength > 0) {
            vcodec->pictureParameterSetNALUnit.resize(pictureParameterSetLength);
            stream->read_bytes(&vcodec->pictureParameterSetNALUnit[0], pictureParameterSetLength);
        }
    }
    return avc_demux_sps();
}

srs_error_t SrsFormat::avc_demux_sps()
{
    srs_error_t err = srs_success;
    
    if (vcodec->sequenceParameterSetNALUnit.empty()) {
        return err;
    }
    
    char* sps = &vcodec->sequenceParameterSetNALUnit[0];
    int nbsps = (int)vcodec->sequenceParameterSetNALUnit.size();
    
    SrsBuffer stream(sps, nbsps);
    
    // for NALU, 7.3.1 NAL unit syntax
    // ISO_IEC_14496-10-AVC-2012.pdf, page 61.
    if (!stream.require(1)) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "decode SPS");
    }
    int8_t nutv = stream.read_1bytes();
    
    // forbidden_zero_bit shall be equal to 0.
    int8_t forbidden_zero_bit = (nutv >> 7) & 0x01;
    if (forbidden_zero_bit) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "forbidden_zero_bit shall be equal to 0");
    }
    
    // nal_ref_idc not equal to 0 specifies that the content of the NAL unit contains a sequence parameter set or a picture
    // parameter set or a slice of a reference picture or a slice data partition of a reference picture.
    int8_t nal_ref_idc = (nutv >> 5) & 0x03;
    if (!nal_ref_idc) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "for sps, nal_ref_idc shall be not be equal to 0");
    }
    
    // 7.4.1 NAL unit semantics
    // ISO_IEC_14496-10-AVC-2012.pdf, page 61.
    // nal_unit_type specifies the type of RBSP data structure contained in the NAL unit as specified in Table 7-1.
    SrsAvcNaluType nal_unit_type = (SrsAvcNaluType)(nutv & 0x1f);
    if (nal_unit_type != 7) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "for sps, nal_unit_type shall be equal to 7");
    }
    
    // decode the rbsp from sps.
    // rbsp[ i ] a raw byte sequence payload is specified as an ordered sequence of bytes.
    std::vector<int8_t> rbsp(vcodec->sequenceParameterSetNALUnit.size());
    
    int nb_rbsp = 0;
    while (!stream.empty()) {
        rbsp[nb_rbsp] = stream.read_1bytes();
        
        // XX 00 00 03 XX, the 03 byte should be drop.
        if (nb_rbsp > 2 && rbsp[nb_rbsp - 2] == 0 && rbsp[nb_rbsp - 1] == 0 && rbsp[nb_rbsp] == 3) {
            // read 1byte more.
            if (stream.empty()) {
                break;
            }
            rbsp[nb_rbsp] = stream.read_1bytes();
            nb_rbsp++;
            
            continue;
        }
        
        nb_rbsp++;
    }
    
    return avc_demux_sps_rbsp((char*)&rbsp[0], nb_rbsp);
}


srs_error_t SrsFormat::avc_demux_sps_rbsp(char* rbsp, int nb_rbsp)
{
    srs_error_t err = srs_success;
    
    // we donot parse the detail of sps.
    // @see https://github.com/ossrs/srs/issues/474
    if (!avc_parse_sps) {
        return err;
    }
    
    // reparse the rbsp.
    SrsBuffer stream(rbsp, nb_rbsp);
    
    // for SPS, 7.3.2.1.1 Sequence parameter set data syntax
    // ISO_IEC_14496-10-AVC-2012.pdf, page 62.
    if (!stream.require(3)) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "sps shall atleast 3bytes");
    }
    uint8_t profile_idc = stream.read_1bytes();
    if (!profile_idc) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "sps the profile_idc invalid");
    }
    
    int8_t flags = stream.read_1bytes();
    if (flags & 0x03) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "sps the flags invalid");
    }
    
    uint8_t level_idc = stream.read_1bytes();
    if (!level_idc) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "sps the level_idc invalid");
    }
    
    SrsBitBuffer bs(&stream);
    
    int32_t seq_parameter_set_id = -1;
    if ((err = srs_avc_nalu_read_uev(&bs, seq_parameter_set_id)) != srs_success) {
        return srs_error_wrap(err, "read seq_parameter_set_id");
    }
    if (seq_parameter_set_id < 0) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "sps the seq_parameter_set_id invalid");
    }
    
    int32_t chroma_format_idc = -1;
    if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244
        || profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc == 118
        || profile_idc == 128) {
        if ((err = srs_avc_nalu_read_uev(&bs, chroma_format_idc)) != srs_success) {
            return srs_error_wrap(err, "read chroma_format_idc");
        }
        if (chroma_format_idc == 3) {
            int8_t separate_colour_plane_flag = -1;
            if ((err = srs_avc_nalu_read_bit(&bs, separate_colour_plane_flag)) != srs_success) {
                return srs_error_wrap(err, "read separate_colour_plane_flag");
            }
        }
        
        int32_t bit_depth_luma_minus8 = -1;
        if ((err = srs_avc_nalu_read_uev(&bs, bit_depth_luma_minus8)) != srs_success) {
            return srs_error_wrap(err, "read bit_depth_luma_minus8");;
        }
        
        int32_t bit_depth_chroma_minus8 = -1;
        if ((err = srs_avc_nalu_read_uev(&bs, bit_depth_chroma_minus8)) != srs_success) {
            return srs_error_wrap(err, "read bit_depth_chroma_minus8");;
        }
        
        int8_t qpprime_y_zero_transform_bypass_flag = -1;
        if ((err = srs_avc_nalu_read_bit(&bs, qpprime_y_zero_transform_bypass_flag)) != srs_success) {
            return srs_error_wrap(err, "read qpprime_y_zero_transform_bypass_flag");;
        }
        
        int8_t seq_scaling_matrix_present_flag = -1;
        if ((err = srs_avc_nalu_read_bit(&bs, seq_scaling_matrix_present_flag)) != srs_success) {
            return srs_error_wrap(err, "read seq_scaling_matrix_present_flag");;
        }
        if (seq_scaling_matrix_present_flag) {
            int nb_scmpfs = ((chroma_format_idc != 3)? 8:12);
            for (int i = 0; i < nb_scmpfs; i++) {
                int8_t seq_scaling_matrix_present_flag_i = -1;
                if ((err = srs_avc_nalu_read_bit(&bs, seq_scaling_matrix_present_flag_i)) != srs_success) {
                    return srs_error_wrap(err, "read seq_scaling_matrix_present_flag_i");;
                }
            }
        }
    }
    
    int32_t log2_max_frame_num_minus4 = -1;
    if ((err = srs_avc_nalu_read_uev(&bs, log2_max_frame_num_minus4)) != srs_success) {
        return srs_error_wrap(err, "read log2_max_frame_num_minus4");;
    }
    
    int32_t pic_order_cnt_type = -1;
    if ((err = srs_avc_nalu_read_uev(&bs, pic_order_cnt_type)) != srs_success) {
        return srs_error_wrap(err, "read pic_order_cnt_type");;
    }
    
    if (pic_order_cnt_type == 0) {
        int32_t log2_max_pic_order_cnt_lsb_minus4 = -1;
        if ((err = srs_avc_nalu_read_uev(&bs, log2_max_pic_order_cnt_lsb_minus4)) != srs_success) {
            return srs_error_wrap(err, "read log2_max_pic_order_cnt_lsb_minus4");;
        }
    } else if (pic_order_cnt_type == 1) {
        int8_t delta_pic_order_always_zero_flag = -1;
        if ((err = srs_avc_nalu_read_bit(&bs, delta_pic_order_always_zero_flag)) != srs_success) {
            return srs_error_wrap(err, "read delta_pic_order_always_zero_flag");;
        }
        
        int32_t offset_for_non_ref_pic = -1;
        if ((err = srs_avc_nalu_read_uev(&bs, offset_for_non_ref_pic)) != srs_success) {
            return srs_error_wrap(err, "read offset_for_non_ref_pic");;
        }
        
        int32_t offset_for_top_to_bottom_field = -1;
        if ((err = srs_avc_nalu_read_uev(&bs, offset_for_top_to_bottom_field)) != srs_success) {
            return srs_error_wrap(err, "read offset_for_top_to_bottom_field");;
        }
        
        int32_t num_ref_frames_in_pic_order_cnt_cycle = -1;
        if ((err = srs_avc_nalu_read_uev(&bs, num_ref_frames_in_pic_order_cnt_cycle)) != srs_success) {
            return srs_error_wrap(err, "read num_ref_frames_in_pic_order_cnt_cycle");;
        }
        if (num_ref_frames_in_pic_order_cnt_cycle < 0) {
            return srs_error_new(ERROR_HLS_DECODE_ERROR, "sps the num_ref_frames_in_pic_order_cnt_cycle");
        }
        for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            int32_t offset_for_ref_frame_i = -1;
            if ((err = srs_avc_nalu_read_uev(&bs, offset_for_ref_frame_i)) != srs_success) {
                return srs_error_wrap(err, "read offset_for_ref_frame_i");;
            }
        }
    }
    
    int32_t max_num_ref_frames = -1;
    if ((err = srs_avc_nalu_read_uev(&bs, max_num_ref_frames)) != srs_success) {
        return srs_error_wrap(err, "read max_num_ref_frames");;
    }
    
    int8_t gaps_in_frame_num_value_allowed_flag = -1;
    if ((err = srs_avc_nalu_read_bit(&bs, gaps_in_frame_num_value_allowed_flag)) != srs_success) {
        return srs_error_wrap(err, "read gaps_in_frame_num_value_allowed_flag");;
    }
    
    int32_t pic_width_in_mbs_minus1 = -1;
    if ((err = srs_avc_nalu_read_uev(&bs, pic_width_in_mbs_minus1)) != srs_success) {
        return srs_error_wrap(err, "read pic_width_in_mbs_minus1");;
    }
    
    int32_t pic_height_in_map_units_minus1 = -1;
    if ((err = srs_avc_nalu_read_uev(&bs, pic_height_in_map_units_minus1)) != srs_success) {
        return srs_error_wrap(err, "read pic_height_in_map_units_minus1");;
    }

    int8_t frame_mbs_only_flag = -1;
    if ((err = srs_avc_nalu_read_bit(&bs, frame_mbs_only_flag)) != srs_success) {
        return srs_error_wrap(err, "read frame_mbs_only_flag");;
    }
    if(!frame_mbs_only_flag) {
        /* Skip mb_adaptive_frame_field_flag */
        int8_t mb_adaptive_frame_field_flag = -1;
        if ((err = srs_avc_nalu_read_bit(&bs, mb_adaptive_frame_field_flag)) != srs_success) {
            return srs_error_wrap(err, "read mb_adaptive_frame_field_flag");;
        }
    }

    /* Skip direct_8x8_inference_flag */
    int8_t direct_8x8_inference_flag = -1;
    if ((err = srs_avc_nalu_read_bit(&bs, direct_8x8_inference_flag)) != srs_success) {
        return srs_error_wrap(err, "read direct_8x8_inference_flag");;
    }

    /* We need the following value to evaluate offsets, if any */
    int8_t frame_cropping_flag = -1;
    if ((err = srs_avc_nalu_read_bit(&bs, frame_cropping_flag)) != srs_success) {
        return srs_error_wrap(err, "read frame_cropping_flag");;
    }
    int32_t frame_crop_left_offset = 0, frame_crop_right_offset = 0,
            frame_crop_top_offset = 0, frame_crop_bottom_offset = 0;
    if(frame_cropping_flag) {
        if ((err = srs_avc_nalu_read_uev(&bs, frame_crop_left_offset)) != srs_success) {
            return srs_error_wrap(err, "read frame_crop_left_offset");;
        }
        if ((err = srs_avc_nalu_read_uev(&bs, frame_crop_right_offset)) != srs_success) {
            return srs_error_wrap(err, "read frame_crop_right_offset");;
        }
        if ((err = srs_avc_nalu_read_uev(&bs, frame_crop_top_offset)) != srs_success) {
            return srs_error_wrap(err, "read frame_crop_top_offset");;
        }
        if ((err = srs_avc_nalu_read_uev(&bs, frame_crop_bottom_offset)) != srs_success) {
            return srs_error_wrap(err, "read frame_crop_bottom_offset");;
        }
    }

    /* Skip vui_parameters_present_flag */
    int8_t vui_parameters_present_flag = -1;
    if ((err = srs_avc_nalu_read_bit(&bs, vui_parameters_present_flag)) != srs_success) {
        return srs_error_wrap(err, "read vui_parameters_present_flag");;
    }

    vcodec->width = ((pic_width_in_mbs_minus1 + 1) * 16) - frame_crop_left_offset * 2 - frame_crop_right_offset * 2;
    vcodec->height = ((2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16) \
                    - (frame_crop_top_offset * 2) - (frame_crop_bottom_offset * 2);

    return err;
}

// LCOV_EXCL_STOP

srs_error_t SrsFormat::video_nalu_demux(SrsBuffer* stream)
{
    srs_error_t err = srs_success;
    
    // ensure the sequence header demuxed
    if (!vcodec->is_avc_codec_ok()) {
        srs_warn("avc ignore type=%d for no sequence header", SrsVideoAvcFrameTraitNALU);
        return err;
    }

    if (vcodec->id == SrsVideoCodecIdHEVC) {
#ifdef SRS_H265
        // TODO: FIXME: Might need to guess format?
        return do_avc_demux_ibmf_format(stream);
#else
        return srs_error_new(ERROR_HEVC_DISABLED, "H.265 is disabled");
#endif
    }

    // Parse the SPS/PPS in ANNEXB or IBMF format.
    if (vcodec->payload_format == SrsAvcPayloadFormatIbmf) {
        if ((err = avc_demux_ibmf_format(stream)) != srs_success) {
            return srs_error_wrap(err, "avc demux ibmf");
        }
    } else if (vcodec->payload_format == SrsAvcPayloadFormatAnnexb) {
        if ((err = avc_demux_annexb_format(stream)) != srs_success) {
            return srs_error_wrap(err, "avc demux annexb");
        }
    } else {
        if ((err = try_annexb_first ? avc_demux_annexb_format(stream) : avc_demux_ibmf_format(stream)) == srs_success) {
            vcodec->payload_format = try_annexb_first ? SrsAvcPayloadFormatAnnexb : SrsAvcPayloadFormatIbmf;
        } else {
            srs_freep(err);
            if ((err = try_annexb_first ? avc_demux_ibmf_format(stream) : avc_demux_annexb_format(stream)) == srs_success) {
                vcodec->payload_format = try_annexb_first ? SrsAvcPayloadFormatIbmf : SrsAvcPayloadFormatAnnexb;
            } else {
                return srs_error_wrap(err, "avc demux try_annexb_first=%d", try_annexb_first);
            }
        }
    }
    
    return err;
}

srs_error_t SrsFormat::avc_demux_annexb_format(SrsBuffer* stream)
{
    srs_error_t err = srs_success;

    int pos = stream->pos();
    err = do_avc_demux_annexb_format(stream);

    // Restore the stream if error.
    if (err != srs_success) {
        stream->skip(pos - stream->pos());
    }

    return err;
}

srs_error_t SrsFormat::do_avc_demux_annexb_format(SrsBuffer* stream)
{
    srs_error_t err = srs_success;
    
    // not annexb, try others
    if (!srs_avc_startswith_annexb(stream, NULL)) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "not annexb");
    }
    
    // AnnexB
    // B.1.1 Byte stream NAL unit syntax,
    // ISO_IEC_14496-10-AVC-2003.pdf, page 211.
    while (!stream->empty()) {
        // find start code
        int nb_start_code = 0;
        if (!srs_avc_startswith_annexb(stream, &nb_start_code)) {
            return err;
        }
        
        // skip the start code.
        if (nb_start_code > 0) {
            stream->skip(nb_start_code);
        }
        
        // the NALU start bytes.
        char* p = stream->data() + stream->pos();
        
        // get the last matched NALU
        while (!stream->empty()) {
            if (srs_avc_startswith_annexb(stream, NULL)) {
                break;
            }
            
            stream->skip(1);
        }
        
        char* pp = stream->data() + stream->pos();
        
        // skip the empty.
        if (pp - p <= 0) {
            continue;
        }
        
        // got the NALU.
        if ((err = video->add_sample(p, (int)(pp - p))) != srs_success) {
            return srs_error_wrap(err, "add video frame");
        }
    }
    
    return err;
}

srs_error_t SrsFormat::avc_demux_ibmf_format(SrsBuffer* stream)
{
    srs_error_t err = srs_success;

    int pos = stream->pos();
    err = do_avc_demux_ibmf_format(stream);

    // Restore the stream if error.
    if (err != srs_success) {
        stream->skip(pos - stream->pos());
    }

    return err;
}

srs_error_t SrsFormat::do_avc_demux_ibmf_format(SrsBuffer* stream)
{
    srs_error_t err = srs_success;
    
    int PictureLength = stream->size() - stream->pos();
    
    // 5.3.4.2.1 Syntax, ISO_IEC_14496-15-AVC-format-2012.pdf, page 16
    // 5.2.4.1 AVC decoder configuration record
    // 5.2.4.1.2 Semantics
    // The value of this field shall be one of 0, 1, or 3 corresponding to a
    // length encoded with 1, 2, or 4 bytes, respectively.
    srs_assert(vcodec->NAL_unit_length != 2);
    
    // 5.3.4.2.1 Syntax, ISO_IEC_14496-15-AVC-format-2012.pdf, page 20
    for (int i = 0; i < PictureLength;) {
        // unsigned int((NAL_unit_length+1)*8) NALUnitLength;
        // TODO: FIXME: Should ignore error? See https://github.com/ossrs/srs-gb28181/commit/a13b9b54938a14796abb9011e7a8ee779439a452
        if (!stream->require(vcodec->NAL_unit_length + 1)) {
            return srs_error_new(ERROR_HLS_DECODE_ERROR, "PictureLength:%d, i:%d, NaluLength:%d, left:%d",
                PictureLength, i, vcodec->NAL_unit_length, stream->left());
        }
        int32_t NALUnitLength = 0;
        if (vcodec->NAL_unit_length == 3) {
            NALUnitLength = stream->read_4bytes();
        } else if (vcodec->NAL_unit_length == 1) {
            NALUnitLength = stream->read_2bytes();
        } else {
            NALUnitLength = stream->read_1bytes();
        }
        
        // The stream format mighe be incorrect, see: https://github.com/ossrs/srs/issues/183
        if (NALUnitLength < 0) {
            return srs_error_new(ERROR_HLS_DECODE_ERROR, "PictureLength:%d, i:%d, NaluLength:%d, left:%d, NALUnitLength:%d",
                PictureLength, i, vcodec->NAL_unit_length, stream->left(), NALUnitLength);
        }
        
        // NALUnit
        if (!stream->require(NALUnitLength)) {
            return srs_error_new(ERROR_HLS_DECODE_ERROR, "PictureLength:%d, i:%d, NaluLength:%d, left:%d, NALUnitLength:%d",
                PictureLength, i, vcodec->NAL_unit_length, stream->left(), NALUnitLength);
        }
        // 7.3.1 NAL unit syntax, ISO_IEC_14496-10-AVC-2003.pdf, page 44.
        if ((err = video->add_sample(stream->data() + stream->pos(), NALUnitLength)) != srs_success) {
            return srs_error_wrap(err, "avc add video frame");
        }

        stream->skip(NALUnitLength);
        i += vcodec->NAL_unit_length + 1 + NALUnitLength;
    }
    
    return err;
}

srs_error_t SrsFormat::audio_aac_demux(SrsBuffer* stream, int64_t timestamp)
{
    srs_error_t err = srs_success;
    
    audio->cts = 0;
    audio->dts = timestamp;
    
    // @see: E.4.2 Audio Tags, video_file_format_spec_v10_1.pdf, page 76
    int8_t sound_format = stream->read_1bytes();
    
    int8_t sound_type = sound_format & 0x01;
    int8_t sound_size = (sound_format >> 1) & 0x01;
    int8_t sound_rate = (sound_format >> 2) & 0x03;
    sound_format = (sound_format >> 4) & 0x0f;

    SrsAudioCodecId codec_id = (SrsAudioCodecId)sound_format;
    acodec->id = codec_id;

    acodec->sound_type = (SrsAudioChannels)sound_type;
    acodec->sound_rate = (SrsAudioSampleRate)sound_rate;
    acodec->sound_size = (SrsAudioSampleBits)sound_size;

    // we support h.264+mp3 for hls.
    if (codec_id == SrsAudioCodecIdMP3) {
        return srs_error_new(ERROR_HLS_TRY_MP3, "try mp3");
    }

    // only support aac
    if (codec_id != SrsAudioCodecIdAAC) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "not supported codec %d", codec_id);
    }

    if (!stream->require(1)) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "aac decode aac_packet_type");
    }
    
    SrsAudioAacFrameTrait aac_packet_type = (SrsAudioAacFrameTrait)stream->read_1bytes();
    audio->aac_packet_type = (SrsAudioAacFrameTrait)aac_packet_type;
    
    // Update the RAW AAC data.
    raw = stream->data() + stream->pos();
    nb_raw = stream->size() - stream->pos();
    
    if (aac_packet_type == SrsAudioAacFrameTraitSequenceHeader) {
        // AudioSpecificConfig
        // 1.6.2.1 AudioSpecificConfig, in ISO_IEC_14496-3-AAC-2001.pdf, page 33.
        int aac_extra_size = stream->size() - stream->pos();
        if (aac_extra_size > 0) {
            char *copy_stream_from = stream->data() + stream->pos();
            acodec->aac_extra_data = std::vector<char>(copy_stream_from, copy_stream_from + aac_extra_size);
            
            if ((err = audio_aac_sequence_header_demux(&acodec->aac_extra_data[0], aac_extra_size)) != srs_success) {
                return srs_error_wrap(err, "demux aac sh");
            }
        }
    } else if (aac_packet_type == SrsAudioAacFrameTraitRawData) {
        // ensure the sequence header demuxed
        if (!acodec->is_aac_codec_ok()) {
            srs_warn("aac ignore type=%d for no sequence header", aac_packet_type);
            return err;
        }
        
        // Raw AAC frame data in UI8 []
        // 6.3 Raw Data, ISO_IEC_13818-7-AAC-2004.pdf, page 28
        if ((err = audio->add_sample(stream->data() + stream->pos(), stream->size() - stream->pos())) != srs_success) {
            return srs_error_wrap(err, "add audio frame");
        }
    } else {
        // ignored.
    }
    
    // reset the sample rate by sequence header
    if (acodec->aac_sample_rate != SrsAacSampleRateUnset) {
        static int srs_aac_srates[] = {
            96000, 88200, 64000, 48000,
            44100, 32000, 24000, 22050,
            16000, 12000, 11025,  8000,
            7350,     0,     0,    0
        };
        switch (srs_aac_srates[acodec->aac_sample_rate]) {
            case 11025:
                acodec->sound_rate = SrsAudioSampleRate11025;
                break;
            case 22050:
                acodec->sound_rate = SrsAudioSampleRate22050;
                break;
            case 44100:
                acodec->sound_rate = SrsAudioSampleRate44100;
                break;
            default:
                break;
        };
    }
    
    return err;
}

srs_error_t SrsFormat::audio_mp3_demux(SrsBuffer* stream, int64_t timestamp, bool fresh)
{
    srs_error_t err = srs_success;
    
    audio->cts = 0;
    audio->dts = timestamp;
    audio->aac_packet_type = fresh ? SrsAudioMp3FrameTraitSequenceHeader : SrsAudioMp3FrameTraitRawData;
    
    // @see: E.4.2 Audio Tags, video_file_format_spec_v10_1.pdf, page 76
    int8_t sound_format = stream->read_1bytes();
    
    int8_t sound_type = sound_format & 0x01;
    int8_t sound_size = (sound_format >> 1) & 0x01;
    int8_t sound_rate = (sound_format >> 2) & 0x03;
    sound_format = (sound_format >> 4) & 0x0f;
    
    SrsAudioCodecId codec_id = (SrsAudioCodecId)sound_format;
    acodec->id = codec_id;
    
    acodec->sound_type = (SrsAudioChannels)sound_type;
    acodec->sound_rate = (SrsAudioSampleRate)sound_rate;
    acodec->sound_size = (SrsAudioSampleBits)sound_size;
    
    // we always decode aac then mp3.
    srs_assert(acodec->id == SrsAudioCodecIdMP3);
    
    // Update the RAW MP3 data. Note the start is 12 bits syncword 0xFFF, so we should not skip any bytes, for detail
    // please see ISO_IEC_11172-3-MP3-1993.pdf page 20 and 26.
    raw = stream->data() + stream->pos();
    nb_raw = stream->size() - stream->pos();
    
    // mp3 payload.
    if ((err = audio->add_sample(raw, nb_raw)) != srs_success) {
        return srs_error_wrap(err, "add audio frame");
    }
    
    return err;
}

srs_error_t SrsFormat::audio_aac_sequence_header_demux(char* data, int size)
{
    srs_error_t err = srs_success;
    
    SrsBuffer* buffer = new SrsBuffer(data, size);
    SrsAutoFree(SrsBuffer, buffer);
    
    // only need to decode the first 2bytes:
    //      audioObjectType, aac_profile, 5bits.
    //      samplingFrequencyIndex, aac_sample_rate, 4bits.
    //      channelConfiguration, aac_channels, 4bits
    if (!buffer->require(2)) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "audio codec decode aac sh");
    }
    uint8_t profile_ObjectType = buffer->read_1bytes();
    uint8_t samplingFrequencyIndex = buffer->read_1bytes();
    
    acodec->aac_channels = (samplingFrequencyIndex >> 3) & 0x0f;
    samplingFrequencyIndex = ((profile_ObjectType << 1) & 0x0e) | ((samplingFrequencyIndex >> 7) & 0x01);
    profile_ObjectType = (profile_ObjectType >> 3) & 0x1f;
    
    // set the aac sample rate.
    acodec->aac_sample_rate = samplingFrequencyIndex;
    
    // convert the object type in sequence header to aac profile of ADTS.
    acodec->aac_object = (SrsAacObjectType)profile_ObjectType;
    if (acodec->aac_object == SrsAacObjectTypeReserved) {
        return srs_error_new(ERROR_HLS_DECODE_ERROR, "aac decode sh object %d", profile_ObjectType);
    }
    
    // TODO: FIXME: to support aac he/he-v2, see: ngx_rtmp_codec_parse_aac_header
    // @see: https://github.com/winlinvip/nginx-rtmp-module/commit/3a5f9eea78fc8d11e8be922aea9ac349b9dcbfc2
    //
    // donot force to LC, @see: https://github.com/ossrs/srs/issues/81
    // the source will print the sequence header info.
    //if (aac_profile > 3) {
        // Mark all extended profiles as LC
        // to make Android as happy as possible.
        // @see: ngx_rtmp_hls_parse_aac_header
        //aac_profile = 1;
    //}
    
    return err;
}

