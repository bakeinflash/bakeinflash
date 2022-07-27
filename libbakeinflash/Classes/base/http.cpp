//
// SSL TCP socket implementation
//

#include "http.h"
#include "mylog.h"
#include "utils.h"

using namespace std;

//
// http
//

http_parser_settings http::m_parser_settings;

static map<int, string> s_retcode_names = { {200,"OK"},{404,"NotFound"} };
string get_retcode_name(int rc)
{
  const auto& it = s_retcode_names.find(rc);
  return it == s_retcode_names.end() ? "" : it->second;
}

http_packet::http_packet()
  : m_retcode(0)
  , m_method(0)
{
}

void http_packet::set_uri(const string& uri)
{
  m_uri = uri;
  vector<string> a;
  if (split(a, m_uri, '?') && a.size() > 0)
  {
    if (a.size() > 1)
    {
      vector<string> b;
      split(b, a[0], '/');
      m_path = b[b.size() - 1];

      vector<string> pairs;
      split(pairs, a[1], '&');
      for (int i = 0; i < pairs.size(); i++)
      {
        vector<string> pair;
        if (split(pair, pairs[i], '='))
          m_params[pair[0]] = pair.size() > 1 ? pair[1] : "";
      }
    }
    else
      m_path = "";

  }
}

// assign
as_value& http_packet::operator[](const std::string& name)
{
  const auto& it = m_headers.find(name);
  if (it == m_headers.end())
    m_headers[name] = as_value();

  return m_headers[name];
}

const as_value& http_packet::operator[](const std::string& name) const
{
  static as_value undefined;
  undefined.set_undefined();

  const auto& it = m_headers.find(name);
  return it == m_headers.end() ? undefined : it->second;
}

void http_packet::response(membuf& mb, int rc, const char* protocol) const
{
  mb.append(protocol);
  
  char s[32];
  snprintf(s, sizeof(s) - 1, " %03d %s\r\n", rc, get_retcode_name(rc).c_str());
  mb.append(s);

  if (m_body.size() > 0)
  {
    mb.append("Content-Length: ");
    mb.append(to_string(m_body.size()));
    mb.append("\r\n");
  }

  for (const auto& it : m_headers)
  {
    mb.append(it.first);
    mb.append(": ");
    mb.append(it.second.to_string());
    mb.append("\r\n");
  }
  mb.append("\r\n");

  if (m_body.size() > 0)
    mb.append(m_body);

  //mylog(D, TRA, "%s\n", mb->c_str());
}


//
//
//

http::http(sock* ch)
  : m_ch(ch)
  , rbuf(ch->rbuf)
  , wbuf(ch->wbuf)
{
  m_parser_settings.on_url = parser_url;
  m_parser_settings.on_message_begin = parser_message_begin;
  m_parser_settings.on_message_complete = parser_message_complete;
  m_parser_settings.on_body = parser_body;
  m_parser_settings.on_chunk_complete = parser_chunk_complete;
  m_parser_settings.on_chunk_header = parser_chunk_header;
  m_parser_settings.on_headers_complete = parser_headers_complete;
  m_parser_settings.on_header_field = parser_header_field;
  m_parser_settings.on_header_value = parser_header_value;
  m_parser_settings.on_status = parser_on_status;

  m_parser.data = &m_pk;
  http_parser_init(&m_parser, HTTP_BOTH);
}

int http::parse()
{
  if (rbuf.size() == 0)
    return 0;

  //printf("recv\n>>>%s<<<\n", string(data, size).c_str());
  int nparsed = (int)http_parser_execute(&m_parser, &m_parser_settings, rbuf.c_str(), rbuf.size());
  if (m_parser.upgrade)
  {
    //  handle new protocol 
    mylog(E, TRA, "todo: handle new HTTP protocol\n");
    rbuf.clear();
    return 0; // ignore
  }
  else if (nparsed != rbuf.size())
  {
    // Handle error. Usually just close the connection. 
    const char* er2 = http_errno_description((http_errno)m_parser.http_errno);
    mylog(E, TRA, "%s at %s...\n", er2, string(rbuf.c_str() + nparsed, 100).c_str());
    on_error();
    return 0; // ignore
  }

  //printf("***parsed %d bytes***\n%s\n", nparsed, data);
  rbuf.remove(nparsed);
  return nparsed;
}

void http::on_error()
{
  m_ch->rbuf.clear();
}

int http::parser_url(http_parser* parser, const char* at, size_t length)
{
  vector<smart_ptr<http_packet>>& m_pk = *((vector<smart_ptr<http_packet>>*) parser->data);
  if (m_pk.size() > 0)
  {
    http_packet* pk = m_pk[m_pk.size() - 1];
    pk->m_uri_value += string(at, length);
    return 0;
  }
  return -1;
}

int http::parser_message_begin(http_parser* parser)
{
  vector<smart_ptr<http_packet>>& m_pk = *((vector<smart_ptr<http_packet>>*) parser->data);
  m_pk.push_back(new http_packet());
  http_packet* pk = m_pk[m_pk.size() - 1];
  return 0;
}

int http::parser_message_complete(http_parser* parser)
{
  vector<smart_ptr<http_packet>>& m_pk = *((vector<smart_ptr<http_packet>>*) parser->data);
  if (m_pk.size() > 0)
  {
    http_packet* pk = m_pk[m_pk.size() - 1];

    pk->m_retcode = parser->status_code;
    pk->m_method = parser->method;

    //Content-Encoding 
    const auto& it = pk->m_headers.find("Content-Encoding");
    if (it != pk->m_headers.end() && it->second == "deflate")
    {
      uint8_t* buffer = NULL;
      unsigned long unzipped_len = unzip(&buffer, (const char*)pk->m_body.data(), pk->m_body.size());
      if (unzipped_len == static_cast<unsigned long>(-1))
      {
        mylog(E, SYS, "failed to unzip body\n");
        free(buffer);
        //  pk->m_has_read = true;
        parser->http_errno = 1;
        return -1;
      }
      else
      {
        pk->m_body.clear();
        pk->m_body.append(buffer, unzipped_len);
        if (pk->m_headers.find("Content-Length") != pk->m_headers.end())
        {
          pk->m_headers["Content-Length"] = (int)unzipped_len;
        }
      }
      free(buffer);
    }

    //    pk->m_has_read = true;

    if (pk->m_uri_value.size() > 0)
    {
      char* uri = (char*)malloc(pk->m_uri_value.size() + 1);
      int rc = url_decode(pk->m_uri_value.c_str(), uri);
      pk->set_uri(rc > 0 ? uri : pk->m_uri_value);
      free(uri);
    }
    pk->m_uri_value.clear();

#ifdef _DEBUG
    //trace(pk);
#endif

    http_parser_init(parser, HTTP_BOTH);
    return 0;
  }
  http_parser_init(parser, HTTP_BOTH);
  return -1;
}

int http::parser_chunk_complete(http_parser* parser)
{
  vector<smart_ptr<http_packet>>& m_pk = *((vector<smart_ptr<http_packet>>*) parser->data);
  if (m_pk.size() > 0)
  {
    return 0;
  }
  return -1;
}

int http::parser_chunk_header(http_parser* parser)
{
  vector<smart_ptr<http_packet>>& m_pk = *((vector<smart_ptr<http_packet>>*) parser->data);
  if (m_pk.size() > 0)
  {
    return 0;
  }
  return -1;
}

int http::parser_body(http_parser* parser, const char* at, size_t length)
{
  vector<smart_ptr<http_packet>>& m_pk = *((vector<smart_ptr<http_packet>>*) parser->data);
  if (m_pk.size() > 0)
  {
    http_packet* pk = m_pk[m_pk.size() - 1];
    pk->m_body.append(at, (int)length);
    return 0;
  }
  return -1;
}

int http::parser_header_field(http_parser* parser, const char* at, size_t length)
{
  vector<smart_ptr<http_packet>>& m_pk = *((vector<smart_ptr<http_packet>>*) parser->data);
  if (m_pk.size() > 0)
  {
    http_packet* pk = m_pk[m_pk.size() - 1];

    if (pk->m_header_field.size() > 0 && pk->m_header_value.size())
    {
      // save pair
      pk->m_headers[pk->m_header_field] = pk->m_header_value;
      pk->m_header_field.clear();
      pk->m_header_value.clear();
    }

    pk->m_header_field += string(at, length);
    return 0;
  }
  return -1;
}

int http::parser_header_value(http_parser* parser, const char* at, size_t length)
{
  vector<smart_ptr<http_packet>>& m_pk = *((vector<smart_ptr<http_packet>>*) parser->data);
  if (m_pk.size() > 0)
  {
    http_packet* pk = m_pk[m_pk.size() - 1];

    pk->m_header_value += string(at, length);
    return 0;
  }
  return -1;
}

int http::parser_on_status(http_parser* parser, const char* at, size_t length)
{
  vector<smart_ptr<http_packet>>& m_pk = *((vector<smart_ptr<http_packet>>*) parser->data);
  if (m_pk.size() > 0)
  {
    return 0;
  }
  return -1;
}

int http::parser_headers_complete(http_parser* parser)
{
  vector<smart_ptr<http_packet>>& m_pk = *((vector<smart_ptr<http_packet>>*) parser->data);
  if (m_pk.size() > 0)
  {
    http_packet* pk = m_pk[m_pk.size() - 1];

    if (pk->m_header_field.size() > 0 && pk->m_header_value.size())
    {
      pk->m_headers[pk->m_header_field] = pk->m_header_value;
    }
    pk->m_header_field.clear();
    pk->m_header_value.clear();
    return 0;
  }
  return -1;
}

int http::advance()
{
  int rc = m_ch->advance();
  if (rc < 0)
    return -1;

  parse();
  for (int i = 0; i < m_pk.size(); )
  {
    bool keep = handle(*m_pk[i]);
    if (keep)
    {
      i++;
      continue;
    }
    m_pk.erase(m_pk.begin() + i);
  }

  return 0;
}

bool http::handle(const http_packet& pk)
{
  return false;   // not keep
}
