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

#ifndef SRS_KERNEL_FLV_HPP
#define SRS_KERNEL_FLV_HPP

/*
#include <srs_kernel_flv.hpp>
*/
#include <srs_core.hpp>

#include <string>

class SrsStream;
class SrsFileWriter;
class SrsFileReader;

/**
* encode data to flv file.
*/
class SrsFlvEncoder
{
private:
    SrsFileWriter* _fs;
private:
    SrsStream* tag_stream;
public:
    SrsFlvEncoder();
    virtual ~SrsFlvEncoder();
public:
    /**
    * initialize the underlayer file stream.
    * @remark user can initialize multiple times to encode multiple flv files.
    * @remark, user must free the fs, flv encoder never close/free it.
    */
    virtual int initialize(SrsFileWriter* fs);
public:
    /**
    * write flv header.
    * write following:
    *   1. E.2 The FLV header
    *   2. PreviousTagSize0 UI32 Always 0
    * that is, 9+4=13bytes.
    */
    virtual int write_header();
    virtual int write_header(char flv_header[9]);
    /**
    * write flv metadata. 
    * @param type, the type of data, or other message type.
    *       @see SrsCodecFlvTag
    * @param data, the amf0 metadata which serialize from:
    *   AMF0 string: onMetaData,
    *   AMF0 object: the metadata object.
    * @remark assert data is not NULL.
    */
    virtual int write_metadata(char type, char* data, int size);
    /**
    * write audio/video packet.
    * @remark assert data is not NULL.
    */
    virtual int write_audio(int64_t timestamp, char* data, int size);
    virtual int write_video(int64_t timestamp, char* data, int size);
public:
    /**
    * get the tag size,
    * including the tag header, body, and 4bytes previous tag size.
    * @remark assert data_size is not negative.
    */
    static int size_tag(int data_size);
private:
    virtual int write_tag(char* header, int header_size, char* tag, int tag_size);
};

/**
* decode flv file.
*/
class SrsFlvDecoder
{
private:
    SrsFileReader* _fs;
private:
    SrsStream* tag_stream;
public:
    SrsFlvDecoder();
    virtual ~SrsFlvDecoder();
public:
    /**
    * initialize the underlayer file stream
    * @remark user can initialize multiple times to decode multiple flv files.
    * @remark, user must free the fs, flv decoder never close/free it.
    */
    virtual int initialize(SrsFileReader* fs);
public:
    /**
    * read the flv header, donot including the 4bytes previous tag size.
    * @remark assert header not NULL.
    */
    virtual int read_header(char header[9]);
    /**
    * read the tag header infos.
    * @remark assert ptype/pdata_size/ptime not NULL.
    */
    virtual int read_tag_header(char* ptype, int32_t* pdata_size, u_int32_t* ptime);
    /**
    * read the tag data.
    * @remark assert data not NULL.
    */
    virtual int read_tag_data(char* data, int32_t size);
    /**
    * read the 4bytes previous tag size.
    * @remark assert previous_tag_size not NULL.
    */
    virtual int read_previous_tag_size(char previous_tag_size[4]);
};

/**
* decode flv fast by only decoding the header and tag.
* used for vod flv stream to read the header and sequence header, 
* then seek to specified offset.
*/
class SrsFlvVodStreamDecoder
{
private:
    SrsFileReader* _fs;
private:
    SrsStream* tag_stream;
public:
    SrsFlvVodStreamDecoder();
    virtual ~SrsFlvVodStreamDecoder();
public:
    /**
    * initialize the underlayer file stream
    * @remark user can initialize multiple times to decode multiple flv files.
    * @remark, user must free the fs, flv decoder never close/free it.
    */
    virtual int initialize(SrsFileReader* fs);
public:
    /**
    * read the flv header and its size.
    * @param header, fill it 13bytes(9bytes header, 4bytes previous tag size).
    * @remark assert header not NULL.
    */
    virtual int read_header_ext(char header[13]);
    /**
    * read the sequence header tags offset and its size.
    * @param pstart, the start offset of sequence header.
    * @param psize, output the size, (tag header)+(tag body)+(4bytes previous tag size).
    * @remark we think the first audio/video is sequence header.
    * @remark assert pstart/psize not NULL.
    */
    virtual int read_sequence_header_summary(int64_t* pstart, int* psize);
public:
    /**
    * for start offset, seed to this position and response flv stream.
    */
    virtual int lseek(int64_t offset);
};

#endif

