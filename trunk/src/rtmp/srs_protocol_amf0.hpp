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

#ifndef SRS_RTMP_PROTOCOL_AMF0_HPP
#define SRS_RTMP_PROTOCOL_AMF0_HPP

/*
#include <srs_protocol_amf0.hpp>
*/

#include <srs_core.hpp>

#include <string>
#include <vector>

class SrsStream;

/**
* any amf0 value.
* 2.1 Types Overview
* value-type = number-type | boolean-type | string-type | object-type 
* 		| null-marker | undefined-marker | reference-type | ecma-array-type 
* 		| strict-array-type | date-type | long-string-type | xml-document-type 
* 		| typed-object-type
*/
class SrsAmf0Any
{
public:
	char marker;
public:
	SrsAmf0Any();
	virtual ~SrsAmf0Any();
public:
	virtual bool is_string();
	virtual bool is_boolean();
	virtual bool is_number();
	virtual bool is_null();
	virtual bool is_undefined();
	virtual bool is_object();
	virtual bool is_object_eof();
	virtual bool is_ecma_array();
public:
	/**
	* get the string of any when is_string() indicates true.
	* user must ensure the type is a string, or assert failed.
	*/
	virtual std::string to_str();
public:
	virtual int size() = 0;
public:
	static SrsAmf0Any* str(const char* value = NULL); 
};

/**
* read amf0 string from stream.
* 2.4 String Type
* string-type = string-marker UTF-8
* @return default value is empty string.
* @remark: use SrsAmf0Any::str() to create it.
*/
class __SrsAmf0String : public SrsAmf0Any
{
public:
	std::string value;

	__SrsAmf0String(const char* _value);
	virtual ~__SrsAmf0String();
	
	virtual int size();
};

/**
* read amf0 boolean from stream.
* 2.4 String Type
* boolean-type = boolean-marker U8
* 		0 is false, <> 0 is true
* @return default value is false.
*/
class SrsAmf0Boolean : public SrsAmf0Any
{
public:
	bool value;

	SrsAmf0Boolean(bool _value = false);
	virtual ~SrsAmf0Boolean();
	
	virtual int size();
};

/**
* read amf0 number from stream.
* 2.2 Number Type
* number-type = number-marker DOUBLE
* @return default value is 0.
*/
class SrsAmf0Number : public SrsAmf0Any
{
public:
	double value;

	SrsAmf0Number(double _value = 0.0);
	virtual ~SrsAmf0Number();
	
	virtual int size();
};

/**
* read amf0 null from stream.
* 2.7 null Type
* null-type = null-marker
*/
class SrsAmf0Null : public SrsAmf0Any
{
public:
	SrsAmf0Null();
	virtual ~SrsAmf0Null();
	
	virtual int size();
};

/**
* read amf0 undefined from stream.
* 2.8 undefined Type
* undefined-type = undefined-marker
*/
class SrsAmf0Undefined : public SrsAmf0Any
{
public:
	SrsAmf0Undefined();
	virtual ~SrsAmf0Undefined();
	
	virtual int size();
};

/**
* 2.11 Object End Type
* object-end-type = UTF-8-empty object-end-marker
* 0x00 0x00 0x09
*/
class SrsAmf0ObjectEOF : public SrsAmf0Any
{
public:
	int16_t utf8_empty;

	SrsAmf0ObjectEOF();
	virtual ~SrsAmf0ObjectEOF();
	
	virtual int size();
};

/**
* to ensure in inserted order.
* for the FMLE will crash when AMF0Object is not ordered by inserted,
* if ordered in map, the string compare order, the FMLE will creash when
* get the response of connect app.
*/
class SrsUnSortedHashtable
{
private:
	typedef std::pair<std::string, SrsAmf0Any*> SrsObjectPropertyType;
	std::vector<SrsObjectPropertyType> properties;
public:
	SrsUnSortedHashtable();
	virtual ~SrsUnSortedHashtable();
	
	virtual int size();
	virtual void clear();
	virtual std::string key_at(int index);
	virtual SrsAmf0Any* value_at(int index);
	virtual void set(std::string key, SrsAmf0Any* value);
	
	virtual SrsAmf0Any* get_property(std::string name);
	virtual SrsAmf0Any* ensure_property_string(std::string name);
	virtual SrsAmf0Any* ensure_property_number(std::string name);
};

/**
* 2.5 Object Type
* anonymous-object-type = object-marker *(object-property)
* object-property = (UTF-8 value-type) | (UTF-8-empty object-end-marker)
*/
class SrsAmf0Object : public SrsAmf0Any
{
private:
	SrsUnSortedHashtable properties;
public:
	SrsAmf0ObjectEOF eof;

	SrsAmf0Object();
	virtual ~SrsAmf0Object();

	virtual int read(SrsStream* stream);
	virtual int write(SrsStream* stream);
	
	virtual int size();
	virtual std::string key_at(int index);
	virtual SrsAmf0Any* value_at(int index);
	virtual void set(std::string key, SrsAmf0Any* value);

	virtual SrsAmf0Any* get_property(std::string name);
	virtual SrsAmf0Any* ensure_property_string(std::string name);
	virtual SrsAmf0Any* ensure_property_number(std::string name);
};

/**
* 2.10 ECMA Array Type
* ecma-array-type = associative-count *(object-property)
* associative-count = U32
* object-property = (UTF-8 value-type) | (UTF-8-empty object-end-marker)
*/
class SrsAmf0EcmaArray : public SrsAmf0Any
{
private:
	SrsUnSortedHashtable properties;
public:
	int32_t count;
	SrsAmf0ObjectEOF eof;

	SrsAmf0EcmaArray();
	virtual ~SrsAmf0EcmaArray();

	virtual int read(SrsStream* stream);
	virtual int write(SrsStream* stream);
	
	virtual int size();
	virtual void clear();
	virtual std::string key_at(int index);
	virtual SrsAmf0Any* value_at(int index);
	virtual void set(std::string key, SrsAmf0Any* value);

	virtual SrsAmf0Any* get_property(std::string name);
	virtual SrsAmf0Any* ensure_property_string(std::string name);
};

/**
* the class to get amf0 object size
*/
class SrsAmf0Size
{
public:
	static int utf8(std::string value);
	static int str(std::string value);
	static int number();
	static int null();
	static int undefined();
	static int boolean();
	static int object(SrsAmf0Object* obj);
	static int object_eof();
	static int array(SrsAmf0EcmaArray* arr);
	static int any(SrsAmf0Any* o);
};

/**
* read amf0 utf8 string from stream.
* 1.3.1 Strings and UTF-8
* UTF-8 = U16 *(UTF8-char)
* UTF8-char = UTF8-1 | UTF8-2 | UTF8-3 | UTF8-4
* UTF8-1 = %x00-7F
* @remark only support UTF8-1 char.
*/
extern int srs_amf0_read_utf8(SrsStream* stream, std::string& value);
extern int srs_amf0_write_utf8(SrsStream* stream, std::string value);

/**
* read amf0 string from stream.
* 2.4 String Type
* string-type = string-marker UTF-8
*/
extern int srs_amf0_read_string(SrsStream* stream, std::string& value);
extern int srs_amf0_write_string(SrsStream* stream, std::string value);

/**
* read amf0 boolean from stream.
* 2.4 String Type
* boolean-type = boolean-marker U8
* 		0 is false, <> 0 is true
*/
extern int srs_amf0_read_boolean(SrsStream* stream, bool& value);
extern int srs_amf0_write_boolean(SrsStream* stream, bool value);

/**
* read amf0 number from stream.
* 2.2 Number Type
* number-type = number-marker DOUBLE
*/
extern int srs_amf0_read_number(SrsStream* stream, double& value);
extern int srs_amf0_write_number(SrsStream* stream, double value);

/**
* read amf0 null from stream.
* 2.7 null Type
* null-type = null-marker
*/
extern int srs_amf0_read_null(SrsStream* stream);
extern int srs_amf0_write_null(SrsStream* stream);

/**
* read amf0 undefined from stream.
* 2.8 undefined Type
* undefined-type = undefined-marker
*/
extern int srs_amf0_read_undefined(SrsStream* stream);
extern int srs_amf0_write_undefined(SrsStream* stream);

extern int srs_amf0_read_any(SrsStream* stream, SrsAmf0Any*& value);

/**
* read amf0 object from stream.
* 2.5 Object Type
* anonymous-object-type = object-marker *(object-property)
* object-property = (UTF-8 value-type) | (UTF-8-empty object-end-marker)
*/
extern int srs_amf0_read_object(SrsStream* stream, SrsAmf0Object*& value);
extern int srs_amf0_write_object(SrsStream* stream, SrsAmf0Object* value);

/**
* read amf0 object from stream.
* 2.10 ECMA Array Type
* ecma-array-type = associative-count *(object-property)
* associative-count = U32
* object-property = (UTF-8 value-type) | (UTF-8-empty object-end-marker)
*/
extern int srs_amf0_read_ecma_array(SrsStream* stream, SrsAmf0EcmaArray*& value);
extern int srs_amf0_write_ecma_array(SrsStream* stream, SrsAmf0EcmaArray* value);

/**
* convert the any to specified object.
* @return T*, the converted object. never NULL.
* @remark, user must ensure the current object type, 
* 		or the covert will cause assert failed.
*/
template<class T>
T* srs_amf0_convert(SrsAmf0Any* any)
{
	T* p = dynamic_cast<T*>(any);
	srs_assert(p != NULL);
	return p;
}

#endif