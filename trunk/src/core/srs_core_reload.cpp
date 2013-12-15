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

#include <srs_core_reload.hpp>

using namespace std;

#include <srs_core_error.hpp>

ISrsReloadHandler::ISrsReloadHandler()
{
}

ISrsReloadHandler::~ISrsReloadHandler()
{
}

int ISrsReloadHandler::on_reload_listen()
{
	return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_pithy_print()
{
	return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_removed(string /*vhost*/)
{
	return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_gop_cache(string /*vhost*/)
{
	return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_forward(string /*vhost*/)
{
	return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_hls(string /*vhost*/)
{
	return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_transcode(string /*vhost*/)
{
	return ERROR_SUCCESS;
}

