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

#include <srs_core_log.hpp>
#include <srs_core_error.hpp>
#include <srs_core_server.hpp>
#include <srs_core_config.hpp>

#include <stdlib.h>
#include <signal.h>

void handler(int signo)
{
	srs_trace("get a signal, signo=%d", signo);
	_server()->on_signal(signo);
}

int main(int argc, char** argv){
	int ret = ERROR_SUCCESS;
	
	signal(SIGNAL_RELOAD, handler);
	
	if ((ret = config->parse_options(argc, argv)) != ERROR_SUCCESS) {
		return ret;
	}
	
	if ((ret = _server()->initialize()) != ERROR_SUCCESS) {
		return ret;
	}
	
	// TODO: create log dir in config->get_log_dir()
	
	if ((ret = _server()->listen()) != ERROR_SUCCESS) {
		return ret;
	}
	
	if ((ret = _server()->cycle()) != ERROR_SUCCESS) {
		return ret;
	}
	
    return 0;
}
