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
#include <srs_utest_amf0.hpp>

VOID TEST(AMF0Test, Size) 
{
	EXPECT_EQ(2+6, SrsAmf0Size::utf8("winlin"));
	EXPECT_EQ(2+0, SrsAmf0Size::utf8(""));
	
	EXPECT_EQ(1+2+6, SrsAmf0Size::str("winlin"));
	EXPECT_EQ(1+2+0, SrsAmf0Size::str(""));
	
	EXPECT_EQ(1+8, SrsAmf0Size::number());
	
	EXPECT_EQ(1, SrsAmf0Size::null());
	
	EXPECT_EQ(1, SrsAmf0Size::undefined());
	
	EXPECT_EQ(1+1, SrsAmf0Size::boolean());
	
	if (true) {
		int size = 1+3;
		SrsAmf0Object obj;
		
		EXPECT_EQ(size, SrsAmf0Size::object(&obj));
	}
	if (true) {
		int size = 1+3;
		SrsAmf0Object obj;
		
		size += SrsAmf0Size::utf8("name")+SrsAmf0Size::str("winlin");
		obj.set("name", new SrsAmf0String("winlin"));
		
		EXPECT_EQ(size, SrsAmf0Size::object(&obj));
	}
	
	if (true) {
		int size = 1+4+3;
		SrsAmf0EcmaArray arr;
		
		EXPECT_EQ(size, SrsAmf0Size::array(&arr));
	}
	if (true) {
		int size = 1+4+3;
		SrsAmf0EcmaArray arr;
		
		size += SrsAmf0Size::utf8("name")+SrsAmf0Size::str("winlin");
		arr.set("name", new SrsAmf0String("winlin"));
		
		EXPECT_EQ(size, SrsAmf0Size::array(&arr));
	}
	if (true) {
		int size = 1+4+3;
		SrsAmf0EcmaArray arr;
		
		size += SrsAmf0Size::utf8("name")+SrsAmf0Size::str("winlin");
		arr.set("name", new SrsAmf0String("winlin"));
		
		SrsAmf0Object* args = new SrsAmf0Object();
		size += SrsAmf0Size::utf8("args")+SrsAmf0Size::object(args);
		arr.set("args", args);
		
		EXPECT_EQ(size, SrsAmf0Size::array(&arr));
	}
}
