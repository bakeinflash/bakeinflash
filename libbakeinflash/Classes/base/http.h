//
// SSL wrapper TCP socket implementation
//

#ifndef _HTTP_H
#define _HTTP_H

#include "tcp.h"
#include "membuf.h"
#include "as_value.h"
#include "http_parser.h"
#include <memory>
#include <vector>
#include <unordered_map>

std::string get_retcode_name(int rc);

struct http_packet : public ref_counted
{
	http_packet();
	virtual ~http_packet() {}

	void set_uri(const std::string& uri);
	const std::string& get_uri() const { return m_uri; }

	as_value& operator[](const std::string& name);
	const as_value& operator[](const std::string& name) const;
	void response(membuf& mb, int rc = 200, const char* protocol = "HTTP/1.1") const;

	std::string m_header_field;   // only for parser
	std::string m_header_value;   // only for parser
	std::string m_uri_value;   // only for parser

	std::map<std::string, as_value> m_headers;
	std::map<std::string, std::string> m_params;
	volatile int m_retcode;  // 200 400 etc
	membuf m_body;
	std::string m_uri;
	std::string m_path;
	int m_method;
};

//
// HTTP
//

struct http : public ref_counted
{
	http(sock* ch);
	virtual ~http() {}

	int advance();
	virtual bool handle(const http_packet& pk);
	virtual void on_error();
	int write(const char* data, int bytes_to_write) { return m_ch->write(data, bytes_to_write); }
	int push(const char* data, int bytes_to_write) { return m_ch->push(data, bytes_to_write); }
	int read(char* buf, int bufsize) { return m_ch->read(buf, bufsize); }
	int get_socket() const { return m_ch->get_socket(); }

protected:
	int parse();
	http_parser m_parser;
	std::vector<smart_ptr<http_packet>> m_pk;
	smart_ptr<sock> m_ch;
	membuf& rbuf;
	membuf& wbuf;

private:

	static http_parser_settings m_parser_settings;
	static int parser_url(http_parser* parser, const char* at, size_t length);
	static int parser_message_begin(http_parser* parser);
	static int parser_message_complete(http_parser* parser);
	static int parser_chunk_complete(http_parser* parser);
	static int parser_chunk_header(http_parser* parser);
	static int parser_body(http_parser* parser, const char* at, size_t length);
	static int parser_header_field(http_parser* parser, const char* at, size_t length);
	static int parser_header_value(http_parser* parser, const char* at, size_t length);
	static int parser_on_status(http_parser* parser, const char* at, size_t length);
	static int parser_headers_complete(http_parser* parser);
};

#endif
