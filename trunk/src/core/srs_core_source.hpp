/*
The MIT License (MIT)

Copyright (c) 2013 winlin

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

#ifndef SRS_CORE_SOURCE_HPP
#define SRS_CORE_SOURCE_HPP

/*
#include <srs_core_source.hpp>
*/

#include <srs_core.hpp>

#include <map>
#include <string>

class SrsMessage;
class SrsOnMetaDataPacket;

/**
* the consumer for SrsSource, that is a play client.
*/
class SrsConsumer
{
public:
	SrsConsumer();
	virtual ~SrsConsumer();
public:
	/**
	* get packets in consumer queue.
	* @msgs SrsMessages*[], output the prt array.
	* @count the count in array.
	* @max_count the max count to dequeue, 0 to dequeue all.
	*/
	virtual int get_packets(int max_count, SrsMessage**& msgs, int& count);
};

/**
* live streaming source.
*/
class SrsSource
{
private:
	static std::map<std::string, SrsSource*> pool;
public:
	/**
	* find stream by vhost/app/stream.
	* @stream_url the stream url, for example, myserver.xxx.com/app/stream
	* @return the matched source, never be NULL.
	* @remark stream_url should without port and schema.
	*/
	static SrsSource* find(std::string stream_url);
private:
	std::string stream_url;
public:
	SrsSource(std::string _stream_url);
	virtual ~SrsSource();
public:
	virtual int on_meta_data(SrsMessage* msg, SrsOnMetaDataPacket* metadata);
	virtual int on_audio(SrsMessage* audio);
	virtual int on_video(SrsMessage* video);
public:
	virtual SrsConsumer* create_consumer();
};

#endif