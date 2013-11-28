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

#ifndef SRS_CORE_FORWARD_HPP
#define SRS_CORE_FORWARD_HPP

/*
#include <srs_core_forward.hpp>
*/
#include <srs_core.hpp>

#include <string>

class SrsSharedPtrMessage;
class SrsOnMetaDataPacket;

/**
* forward the stream to other servers.
*/
class SrsForwarder
{
public:
	SrsForwarder();
	virtual ~SrsForwarder();
public:
	virtual int on_publish(std::string vhost, std::string app, std::string stream, std::string forward_server);
	virtual void on_unpublish();
	virtual int on_meta_data(SrsOnMetaDataPacket* metadata);
	virtual int on_audio(SrsSharedPtrMessage* msg);
	virtual int on_video(SrsSharedPtrMessage* msg);
};

#endif
