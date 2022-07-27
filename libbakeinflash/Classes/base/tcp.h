//
// TCP socket implementation
//

#ifndef _NB_TCP_H
#define _NB_TCP_H

#define MAX_PACKET_SIZE 4096

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>

#define socklen_t int

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02
#define socklen_t int
#define MSG_NOSIGNAL 0
#else
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <map>
#include <string>

#define closesocket ::close
#define WSAGetLastError() errno
#define WSAEWOULDBLOCK EAGAIN
#define WSAENOTCONN ENOTCONN
#define WSAENOBUFS EAGAIN
#define WSAEISCONN EISCONN
#define SOCKADDR_IN sockaddr_in
#define LPSOCKADDR sockaddr*
#define SD_RECEIVE      SHUT_RD
#define SD_SEND         SHUT_WR
#define SD_BOTH         SHUT_RDWR
#endif

#include "membuf.h"
#include <map>
#include <stdint.h>
#include "smart_ptr.h"
#include <functional>
#include <vector>

struct tcp_param
{
  tcp_param()
    : port(0)
    , limits(0)
    , sock(-1)
  {}

  ~tcp_param()
  {
    if (sock > 0)
      closesocket(sock);
  }

  std::string ip_mask;
  int port;
  int limits;
  int sock;
};

//
// socket and pipe based class
//

struct mychannel : public ref_counted
{
  mychannel();

  virtual int connect() = 0;
  virtual void close() = 0;
  virtual bool is_connected() = 0;
  virtual std::string get_name() const = 0;
  virtual int advance();

  bool read_token(std::string* token, char delimiter);
  int pushZ(const std::string& data);   // puah string and zero at the end
  int push(const std::string& data);
  int push(const char* data, int bytes);
  int push(const membuf& mb);
  bool pop(std::string* msg, char delimiter = '\n');

  const std::string& get_signature() const { return m_signature; }
  void set_signature(const std::string& sig) { m_signature = sig; }
  void clear()
  {
    rbuf.clear();
    wbuf.clear();
  }

  void on_connect() { if (m_on_connect) m_on_connect(); }
  void on_disconnect() { if (m_on_disconnect) m_on_disconnect(); }
  std::function<void()> m_on_connect;
  std::function<void()> m_on_disconnect;
  std::function<void(mychannel* ch, std::string& msg)> m_on_message;

  virtual int write(const char* data, int bytes_to_write) = 0;
  virtual int read(char* buf, int bufsize) = 0;

  membuf wbuf;
  membuf rbuf;

protected:

  // for traffic limiting
  membuf m_last_data;
  int m_bps;
  int m_1sec_traffic;
  time_t m_last_write_time;
  std::string m_signature;

private:

  int read();   // for advance
  int write();  // for advance
};

//
// socket based main class
//

struct sock : public mychannel
{
  sock();
  virtual ~sock();

  virtual void close() override;
  virtual bool is_connected() override { return m_sock_connected; }
  virtual std::string get_name() const override { return sock2ip(); };
  virtual int listen(const std::string& clients_ip, uint16_t port) = 0;

  void set_nonblock();
  static void set_nonblock(int sock);
  int get_socket() const { return m_sock; }

  void set_host(const std::string& ip) { m_host = ip; }
  void set_port(int port) { m_port = port; }
  const std::string& get_host() const { return m_host; }
  const int get_port() const { return m_port; }

  // helper
  static bool parse_mask(const std::string& mask, tcp_param* p);
  inline std::string sock2ip() const { return sock2ip(m_sock); }
  static std::string sock2ip(int sock);
  static std::string sock2host(int sock);
  static int sock2port(int sock);
  static bool sock2ip(int sock, std::string* ip1, int* port1, std::string* ip2, int* port2);

  static bool is_readable(int sock);
  static bool is_writeable(int sock);

protected:
  bool is_readable() const;
  bool is_writeable() const;

  std::string m_host;
  uint16_t m_port;
  int m_sock;
  sockaddr_in m_addr;
  std::string m_uri;
  bool m_sock_connected;
  uint64_t m_sock_connection_timeout;
};

//
//
//

struct tcp : public sock
{
  tcp();
  tcp(int sock); // for server side
  tcp(const std::string& host, int port);   // for client side
  ~tcp();

  virtual int connect() override;
  virtual int write(const char* data, int bytes_to_write) override;
  virtual int read(char* buf, int bufsize) override;

  // server
  int listen(const std::string& clients_ip, uint16_t port) override;
  int accept();
};

//
// Unix sockets
//

struct utcp : public tcp
{
  utcp(const std::string& usock_path, int channels);		// server
  utcp(const std::string& usock_path);		// client
  virtual ~utcp();
};

//
// UDP
//

struct udp : public sock
{
  udp();
  virtual ~udp();

  virtual bool is_connected() { return m_sock >= 0; }
  virtual int connect() override;
  virtual int write(const char* data, int bytes_to_write) override;
  virtual int read(char* buf, int bufsize) override;
  int listen(const std::string& clients_ip, uint16_t port) override { return 0; }
  bool bind_unicast(const std::string& ipmask, int port);
  bool bind_multicast(const std::string& ipmask, int port);
  void set_peer_addr(sockaddr_in* peer_addr);
};

void enable_keepalive(int sock);
void list_interfaces(std::vector<std::string>* iip);

#endif
