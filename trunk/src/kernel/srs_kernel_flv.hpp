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

#ifndef SRS_KERNEL_FLV_HPP
#define SRS_KERNEL_FLV_HPP

/*
#include <srs_kernel_flv.hpp>
*/
#include <srs_core.hpp>

class SrsStream;

/**
* file stream to read/write file.
*/
class SrsFileStream
{
private:
    std::string _file;
    int fd;
public:
    SrsFileStream();
    virtual ~SrsFileStream();
public:
    virtual int open_write(std::string file);
    virtual int open_read(std::string file);
    virtual void close();
    virtual bool is_open();
public:
    /**
    * @param pnread, return the read size. NULL to ignore.
    */
    virtual int read(void* buf, size_t count, ssize_t* pnread);
    /**
    * @param pnwrite, return the write size. NULL to ignore.
    */
    virtual int write(void* buf, size_t count, ssize_t* pnwrite);
    /**
    * tell current offset of stream.
    */
    virtual int64_t tellg();
    virtual int64_t lseek(int64_t offset);
    virtual int64_t filesize();
    virtual void skip(int64_t size);
};

/**
* encode data to flv file.
*/
class SrsFlvEncoder
{
private:
    SrsFileStream* _fs;
private:
    SrsStream* tag_stream;
public:
    SrsFlvEncoder();
    virtual ~SrsFlvEncoder();
public:
    /**
    * initialize the underlayer file stream,
    * user can initialize multiple times to encode multiple flv files.
    */
    virtual int initialize(SrsFileStream* fs);
public:
    /**
    * write flv header.
    * write following:
    *   1. E.2 The FLV header
    *   2. PreviousTagSize0 UI32 Always 0
    * that is, 9+4=13bytes.
    */
    virtual int write_header();
    /**
    * write flv metadata. 
    * serialize from:
    *   AMF0 string: onMetaData,
    *   AMF0 object: the metadata object.
    */
    virtual int write_metadata(char* data, int size);
    /**
    * write audio/video packet.
    */
    virtual int write_audio(int64_t timestamp, char* data, int size);
    virtual int write_video(int64_t timestamp, char* data, int size);
private:
    virtual int write_tag(char* header, int header_size, char* tag, int tag_size);
};

/**
* decode flv fast by only decoding the header and tag.
*/
class SrsFlvFastDecoder
{
private:
    SrsFileStream* _fs;
private:
    SrsStream* tag_stream;
public:
    SrsFlvFastDecoder();
    virtual ~SrsFlvFastDecoder();
public:
    /**
    * initialize the underlayer file stream,
    * user can initialize multiple times to encode multiple flv files.
    */
    virtual int initialize(SrsFileStream* fs);
public:
    /**
    * read the flv header and size.
    */
    virtual int read_header(char** pdata, int* psize);
    /**
    * read the sequence header and size.
    */
    virtual int read_sequence_header(int64_t* pstart, int* psize);
public:
    /**
    * for start offset, seed to this position and response flv stream.
    */
    virtual int lseek(int64_t offset);
};

/**
* decode flv file.
*/
class SrsFlvDecoder
{
private:
    SrsFileStream* _fs;
private:
    SrsStream* tag_stream;
public:
    SrsFlvDecoder();
    virtual ~SrsFlvDecoder();
public:
    /**
    * initialize the underlayer file stream,
    * user can initialize multiple times to decode multiple flv files.
    */
    virtual int initialize(SrsFileStream* fs);
public:
    virtual int read_header(char header[9]);
    virtual int read_tag_header(char* ptype, int32_t* pdata_size, u_int32_t* ptime);
    virtual int read_tag_data(char* data, int32_t size);
    virtual int read_previous_tag_size(char ts[4]);
};

#endif