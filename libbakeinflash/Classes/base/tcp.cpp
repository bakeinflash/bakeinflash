//
// TCP socket implementation
//

#include "utility.h"
#include <string.h>
#include "tcp.h"
//#include "mylog.h"
#include <limits.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <iphlpapi.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <ifaddrs.h>
#include <linux/if.h>
#endif

// Sockets are not part of C++ Standard so it depends on implementation.
// Generally they are not thread safe since send is not an atomic operation

using namespace std;

//
// mychannel
//

mychannel::mychannel()
  : m_bps(0)
  , m_1sec_traffic(0)
  , m_last_write_time(0)
{
#ifdef _WIN32
  static bool s_inited = false;
  if (s_inited == false)
  {
    WORD version_requested = MAKEWORD(1, 1);
    WSADATA wsa;
    s_inited = WSAStartup(version_requested, &wsa) == 0;
  }
#endif
}

int mychannel::advance()
{
  if (is_connected() == false)
    return connect();

  int bytes = read();

  if (write() < 0)
    return -1;

  // handle cmd
  if (m_on_message)
  {
    string msg;
    while (pop(&msg))
    {
      m_on_message(this, msg);
    }
  }

  return bytes;
}

int mychannel::write()
{
  // sanity check, 100mb
  assert(wbuf.size() < 1024 * 1024 * 100);

  int bytes_to_write = wbuf.size();
  if (bytes_to_write == 0)
    return 0;

  bytes_to_write = imin(bytes_to_write, MAX_PACKET_SIZE);

  time_t now;
  time(&now);
  if (now != m_last_write_time)
  {
    m_last_write_time = now;
    m_1sec_traffic = 0;
  }

  if (m_bps > 0)
  {
    bytes_to_write = imin(bytes_to_write, m_bps - m_1sec_traffic);
    if (bytes_to_write <= 0)
      return 0;
  }

  //mylog(T,TRA,"%d\n",wbuf.size());
  const char* data = (const char*)wbuf.data();
  if (m_last_data.size() > 0)
  {
    data = m_last_data.c_str();
    bytes_to_write = m_last_data.size();
  }

  int sent_bytes = write(data, bytes_to_write);
  if (sent_bytes > 0)
  {
    if (m_last_data.size() > 0)
      m_last_data.clear();
    else
      wbuf.remove(sent_bytes);

    m_1sec_traffic += sent_bytes;
  }
  return sent_bytes;
}

int mychannel::read()
{
  char buf[MAX_PACKET_SIZE];
  int bytes = read(buf, MAX_PACKET_SIZE);
  if (bytes > 0)
  {
    rbuf.append(buf, bytes);
    // mylog(D, TRA, "mychannel read: %s", string(buf, bytes).c_str());
  }

  // sanity check, 100mb
  assert(rbuf.size() < 1024 * 1024 * 100);

  return rbuf.size() > 0 ? rbuf.size() : bytes;
}

bool mychannel::read_token(std::string* token, char delimiter)
{
  if (rbuf.size() > 0)
  {
    const char* data = rbuf.c_str();
    const char* ch = (const char*)memchr(data, delimiter, rbuf.size());
    if (ch)
    {
      int len = (int)(ch - data) + 1;
      *token = std::string(data, len);
      rbuf.remove(len);
      return true;
    }
  }
  return false;
}

int mychannel::push(const std::string& data)
{
  return push(data.c_str(), (int)data.size());
}

int mychannel::pushZ(const std::string& data)
{
  push(data.c_str(), (int)data.size());
  return push("\n", 1);
}

int mychannel::push(const char* data, int pbytes_to_write)
{
  wbuf.append(data, pbytes_to_write);
  return pbytes_to_write;
}

int mychannel::push(const membuf& mb)
{
  wbuf.append(mb);
  return mb.size();
}

bool mychannel::pop(string* msg, char delimiter)
{
  assert(msg);
  msg->clear();
  while (rbuf.size() > 0)
  {
    char* deli = (char*)memchr(rbuf.c_str(), delimiter, rbuf.size());
    if (deli == NULL)
      break;    // no delimiter yet
    else
    {
      int len = (int)(deli - rbuf.c_str());
      string cmd;
      if (len > 0)
      {
        cmd = trim(string(rbuf.c_str(), len), "\t\n\v\f\r ");

        // òðèìîâàòü ñèìâîë " òîëüêî åñëè îí ïåðâûé â êîìàíäå
        if (cmd.size() >= 2 && cmd[0] == '"' && cmd[cmd.size() - 1] == '"')
        {
          cmd.erase(cmd.begin());
          cmd.pop_back();
        }
        *msg = cmd;
      }
      rbuf.remove(len + 1);    // + zero

      if (cmd.size() == 0)
        continue;

      return true;
    }
  }
  return false;
}


//
//
//
sock::sock()
  : m_host("")
  , m_port(0)
  , m_sock(-1)
  , m_sock_connected(false)
  , m_sock_connection_timeout(0)
{
  m_addr = {};
}

sock::~sock()
{
  close();
}

void sock::close()
{
  if (m_sock >= 0)
  {
    // mylog(D, TRA, "%s dtor tcp\n", sock2ip().c_str());

     // write_buffer();
    shutdown(m_sock, SD_SEND);
    closesocket(m_sock);
    m_sock = -1;
  }
  m_sock_connected = false;
  wbuf.clear();
  rbuf.clear();
}

void sock::set_nonblock()
{
  set_nonblock(m_sock);
}

void sock::set_nonblock(int sock)
{
  // Set non-blocking mode for the socket, so that
  // accept() doesn't block if there's no pending
  // connection.
#ifdef WIN32
  int mode = 1;
  ioctlsocket(sock, FIONBIO, (u_long FAR*) & mode);
#else
  int mode = fcntl(sock, F_GETFL, 0);
  mode |= O_NONBLOCK;
  fcntl(sock, F_SETFL, mode);
#endif
}

bool sock::is_readable() const
{
  return is_readable(m_sock);
}

bool sock::is_readable(int sock)
// Return true if this socket has incoming data available.
{
  if (sock < 0)
  {
    return false;
  }

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sock, &fds);
  struct timeval tv = { 0, 0 };

#ifdef WIN32
  // the first arg to select in win32 is ignored.
  // It's included only for compatibility with Berkeley sockets.
  select(1, &fds, NULL, NULL, &tv);
#else
  // It should be the value of the highest numbered FD within any of the passed fd_sets,
  // plus one... Because, the max FD value + 1 == the number of FDs
  // that select() must concern itself with, from within the passed fd_sets...)
  select(sock + 1, &fds, NULL, NULL, &tv);
#endif

  return FD_ISSET(sock, &fds) != 0;
}

bool sock::is_writeable() const
{
  return is_writeable(m_sock);
}

bool sock::is_writeable(int sock)
{
  if (sock < 0)
    return false;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sock, &fds);
  struct timeval tv = { 0, 0 };

#ifdef WIN32
  // the first arg to select in win32 is ignored.
  // It's included only for compatibility with Berkeley sockets.
  select(1, NULL, &fds, NULL, &tv);
#else
  // It should be the value of the highest numbered FD within any of the passed fd_sets,
  // plus one... Because, the max FD value + 1 == the number of FDs
  // that select() must concern itself with, from within the passed fd_sets...)
  select(sock + 1, NULL, &fds, NULL, &tv);
#endif

  bool rc = FD_ISSET(sock, &fds) != 0;
  //  mylog(T,TRA, "socket is_writeable %d\n", rc);
  return rc;
}

bool sock::parse_mask(const string& mask, tcp_param* p)
{
  // 0.0.0.0:5555(99)
  string s = mask;
  if (s[s.size() - 1] != ')')
    s += "(0)";

  int i1, i2, i3, i4;
  int field_count = sscanf(s.c_str(), "%d.%d.%d.%d:%d(%d)", &i1, &i2, &i3, &i4, &p->port, &p->limits);
  if (field_count == 6)
  {
    if (p->limits == 0)
      p->limits = INT_MAX;

    p->ip_mask = to_string(i1) + '.' + to_string(i2) + '.' + to_string(i3) + '.' + to_string(i4);
    //p->sock = open_listener(p->ip_mask, p->port);    // create server socket
    return true;;
  }
  return false;
}

string sock::sock2host(int sock)
{
  string ip1, ip2;
  int port1, port2;
  if (sock2ip(sock, &ip1, &port1, &ip2, &port2))
  {
    return  ip1 == ip2 ? ip1 : ip2;
  }
  return "";
}

int sock::sock2port(int sock)
{
  string ip1, ip2;
  int port1, port2;
  if (sock2ip(sock, &ip1, &port1, &ip2, &port2))
  {
    return  ip1 == ip2 ? port1 : port2;
  }
  return 0;
}

string sock::sock2ip(int sock)
{
  string ip1, ip2;
  int port1, port2;
  string s;

  if (sock2ip(sock, &ip1, &port1, &ip2, &port2))
    s = ip1 == ip2 ? (ip1 + ':' + to_string(port1)) : (ip1 + ':' + to_string(port1) + '/' + ip2 + ':' + to_string(port2));

  return s;
}

bool sock::sock2ip(int sock, string* ip1, int* port1, string* ip2, int* port2)
{
  struct sockaddr_in addr1 = {};
  struct sockaddr_in addr2 = {};
  socklen_t len = sizeof(addr1);

  *ip1 = "";
  *port1 = 0;
  *ip2 = "";
  *port2 = 0;
  if (getsockname(sock, (struct sockaddr*)&addr1, &len) == 0)
  {
    *ip1 = inet_ntoa(addr1.sin_addr);
    *port1 = ntohs(addr1.sin_port);
    if (getpeername(sock, (struct sockaddr*)&addr2, &len) == 0)
    {
      *ip2 = inet_ntoa(addr2.sin_addr);
      *port2 = ntohs(addr2.sin_port);
      return true;
    }
  }
  return false;
}

//
// tcp
//

//for tcp server
tcp::tcp()
{
}

// for client side
tcp::tcp(const string& host, int port)
{
  m_host = host;
  m_port = port;
}

// for server side clients
tcp::tcp(int client)
{
  m_sock = client;
  set_nonblock();
}

tcp::~tcp()
{
}

int tcp::write(const char* data, int bytes_to_write)
{
  return (int)send(m_sock, data, bytes_to_write, MSG_NOSIGNAL);
}

int tcp::read(char* buf, int bufsize)
{
  int bytes_read = ::recv(m_sock, buf, bufsize, 0);
  if (bytes_read > 0)
    return bytes_read;

  int err = WSAGetLastError();
  if (bytes_read < 0 && (err == WSAEWOULDBLOCK || err == EINPROGRESS)) //  || m_last_error == WSAENOTCONN
    return 0;    // wait

//  mylog(E, TRA, "%s, %s\n", sock2ip().c_str(), GetLastErrorAsString(err));
  close();
  return -1;
}

int tcp::connect()
{
  if (m_sock_connected)
    return 1;

  if (m_sock >= 0)
  {
    // wait for socket connection
    if (is_writeable())
    {
      m_sock_connected = true;
      on_connect();
      mylog(D, TRA, "%s tcp sock connected\n", sock2ip().c_str());
      return 1;
    }

    int err = 0;
#ifdef _WIN32
    int len = sizeof(err);
    int rc = getsockopt(m_sock, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
#else
    socklen_t len = sizeof(err);
    int rc = getsockopt(m_sock, SOL_SOCKET, SO_ERROR, &err, &len);
#endif
    if (rc < 0 || err != 0)
    {
      mylog(E, SYS, "%s:%d, %s\n", m_host.c_str(), m_port, GetLastErrorAsString(err));
      close();
      return -1;
    }

    // 30 sec
    if (get_ticks() - m_sock_connection_timeout >= 30000)
    {
      mylog(E, SYS, "connection timeout\n");
      close();
      return -1;
    }
    return 0; // wait
  }

  if (m_host.size() == 0)
  {
    mylog(E, SYS, "tcp::connect(), no hostname\n");
    return -1;
  }

  // create socket and start connecting

  struct hostent* host = NULL;
  if ((host = gethostbyname(m_host.c_str())) == NULL)
  {
    mylog(E, SYS, "%s\n", LASTERR);
    return -1;
  }

  m_sock = (int)socket(PF_INET, SOCK_STREAM, 0);
  if (m_sock < 0)
  {
    mylog(E, SYS, "%s\n", LASTERR);
    return -1;
  }

  int one = 1;
#if defined(__GNUC__) && !defined(__MINGW32__)
  setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  {
    // for Linux only, set connection timeout
    struct timeval timeout;
    timeout.tv_sec = 7;  // after 7 seconds connect() will timeout
    timeout.tv_usec = 0;
    setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  }
  enable_keepalive(m_sock);
#else
  int rc = setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one));
#endif //  _MSC_VER

  sock::set_nonblock(m_sock);

  memset(&m_addr, sizeof(m_addr), 0);
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons(m_port);
  m_addr.sin_addr.s_addr = *(long*)(host->h_addr);

  ::connect(m_sock, (struct sockaddr*)&m_addr, sizeof(m_addr));
  int err = GetLastError();
  if (err != WSAEWOULDBLOCK && err != EINPROGRESS)
  {
    mylog(E, SYS, "%s\n", LASTERR);
    return -1;
  }

  m_sock_connection_timeout = get_ticks();
  return 0; // wait
}

int tcp::listen(const std::string& clients_ip, uint16_t port)
{
  // keep
  m_host = clients_ip;
  m_port = port;

  m_sock = (int) ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_sock < 0)
  {
    return -1;
  }

  int one = 1;
#if defined(_MSC_VER) || defined(__MINGW32__)
  setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one));
#else
  setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#endif //  _MSC_VER

  // disbale Nagle's algorithm.
  //	setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));

  // set socket bufsize
  //	int BDP = 625 * 1024; // 100MBps * 0.050 sec / 8 = 0.625MB = 625KB
  //	int sock_buf_size = BDP;
  //	setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size, sizeof(sock_buf_size));
  //	setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size, sizeof(sock_buf_size));

  // Set the address.
  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  saddr.sin_addr.s_addr = inet_addr(clients_ip.c_str());

  // bind the address
  int ret = ::bind(m_sock, (LPSOCKADDR)&saddr, sizeof(saddr));
  if (ret < 0)
  {
    closesocket(m_sock);
    m_sock = -1;
    return -2;
  }

  // gethostname

  // Start listening.
  ret = ::listen(m_sock, SOMAXCONN);
  if (ret < 0)
  {
    mylog(E, SYS, "listen() failed, %s:%d, errno=%d\n", m_host.c_str(), m_port, errno);
    closesocket(m_sock);
    m_sock = -1;
    return -1;
  }

  // Set non-blocking mode for the socket, so that
  // accept() doesn't block if there's no pending
  // connection.
  set_nonblock();
  return 0;
}

int tcp::accept()
{
  // Accept an incoming request.
#if defined(_MSC_VER) || defined(__MINGW32__)
  int ln;
#else
  uint32_t ln;
#endif

  sockaddr_in client_sockaddr = {};
  ln = sizeof(sockaddr_in);
  int sock = (int) ::accept(m_sock, (sockaddr*)&client_sockaddr, &ln);
  if (sock >= 0)
    set_nonblock(sock);

  return sock;
}

void enable_keepalive(int sock)
{
#if defined(__GNUC__) && !defined(__MINGW32__)	
  int yes = 1;
  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int));
  int idle = 1; //The time (in seconds) the connection needs to remain idle before TCP starts sending keepalive probes,
  setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int));
  int interval = 1;	// The time (in seconds) between individual keepalive probes.
  setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int));
  int maxpkt = 10;	//  The maximum number of keepalive probes TCP should send before dropping the connection.
  setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(int));
#endif
}

//
//
//

// "/tmp/rdsnetkit.sock");
// unix socket
utcp::utcp(const std::string& usock_path, int channels)
{
  // m_host = usock_path;

#if defined(__GNUC__) && !defined(__MINGW32__)
  unlink(m_host.c_str());
  m_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (m_sock < 0)
  {
    mylog(E, TRA, "Failed to create sock channel, err=%d\n", errno);
    return;
  }

  set_nonblock();

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, m_host.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(m_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1)
  {
    mylog(E, TRA, "Failed to bind usock channel, err=%d\n", errno);
    close();
    unlink(m_host.c_str());
    return;
  }

  if (::listen(m_sock, channels) == -1)
  {
    mylog(D, PIP, "Failed to listen sock channel\n");
    close();
    unlink(m_host.c_str());
    return;
  }
#endif
}

// client
utcp::utcp(const std::string& usock_path)
{
#if defined(__GNUC__) && !defined(__MINGW32__)
  m_host = usock_path;
  struct sockaddr_un  serv_addr = {};
  serv_addr.sun_family = AF_UNIX;
  strcpy(serv_addr.sun_path, usock_path.c_str());
  int servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
  m_sock = socket(AF_UNIX, SOCK_STREAM, 0);

  set_nonblock();

  /*  if (::connect(m_sock, (struct sockaddr*)&serv_addr, servlen) < 0)
    {
      closesocket(m_sock);
      m_sock = -1;
      return false;
    }*/
#endif
}

utcp::~utcp()
{
#if defined(__GNUC__) && !defined(__MINGW32__)
  if (m_sock >= 0)
  {
    ::close(m_sock);
    unlink(m_host.c_str());
  }
#endif
}

//
// UDP
//

// for server side clients
udp::udp()
{
  m_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (m_sock < 0)
    mylog(E, SYS, "%s\n", LASTERR);

  set_nonblock();

  // increase socket stack
  int buffsize = 64 * 1024;
  setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (const char*)&buffsize, sizeof(buffsize));
  setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&buffsize, sizeof(buffsize));
}

udp::~udp()
{
}

bool udp::bind_unicast(const string& ipmask, int port)
{
  struct sockaddr_in addr = { 0 };
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ipmask.c_str());
  addr.sin_port = htons(port);

  if (::bind(m_sock, (struct sockaddr*)&addr, sizeof addr) == -1)
  {
    mylog(E, TRA, "%s\n", LASTERR);
    return false;
  }
  return true;
}

bool udp::bind_multicast(const string& ipmask, int port)
{
  struct sockaddr_in addr = { 0 };
  addr.sin_family = AF_INET;    // IPv4
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // inet_addr(ipmask.c_str());
  addr.sin_port = htons(port);

  // connect to multicast group, listen
  struct ip_mreq mreq;
  memset(&mreq, 0, sizeof(struct ip_mreq));

#if defined(__GNUC__) && !defined(__MINGW32__)
  // fixme for mingw
  inet_pton(AF_INET, ipmask.c_str(), &(mreq.imr_multiaddr));
#endif
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  int one = 1;
#ifdef _WIN32
  int rc1 = setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(int));
  int rc2 = setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
#else
  int rc1 = setsockopt(m_sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(int));
  int rc2 = setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
#endif
  if (rc1 < 0 || rc2 < 0)
  {
    mylog(E, TRA, "bind_multicast: %s\n", LASTERR);
    return false;
  }

  if (::bind(m_sock, (struct sockaddr*)&addr, sizeof addr) == -1)
  {
    mylog(E, TRA, "bind_multicast: %s\n", LASTERR);
    return false;
  }
  return true;
}

void udp::set_peer_addr(sockaddr_in* peer_addr)
{
  m_addr = *peer_addr;
}

// for clients
int udp::connect()
{
  return 1;
}

int udp::write(const char* data, int bytes_to_write)
{
  // mylog(D,TRA,"udp %s write %d bytes\n", sock2ip(m_sock).c_str(), bytes_to_write);
  int rc = ::sendto(m_sock, data, bytes_to_write, 0, (sockaddr*)&m_addr, sizeof(sockaddr_in));
  if (rc > 0)
    return rc;

  int err = WSAGetLastError();
  if (rc < 0 && (err == WSAEWOULDBLOCK || err == EINPROGRESS)) //  || m_last_error == WSAENOTCONN
    return 0;    // wait

//  mylog(E, TRA, "%s, %s\n", sock2ip().c_str(), GetLastErrorAsString(err));
  close();
  mylog(E, TRA, "%s\n", GetLastErrorAsString(rc));
  return rc;
}

int udp::read(char* buf, int bufsize)
{
  if (is_readable() == false)
    return 0;

  socklen_t client_addr_size = sizeof(sockaddr_in);
  int bytes_read = (int)recvfrom(m_sock, buf, bufsize, 0, (sockaddr*)&m_addr, &client_addr_size);
  if (bytes_read > 0)
  {
    // mylog(D, TRA, "udp %s read\n%s\n", sock2ip(m_sock).c_str(), buf);
    return bytes_read;
  }

  int err = WSAGetLastError();
  if (bytes_read < 0 && (err == WSAEWOULDBLOCK || err == EINPROGRESS)) //  || m_last_error == WSAENOTCONN
    return 0;    // wait

  mylog(E, TRA, "%s, %s\n", sock2ip().c_str(), GetLastErrorAsString(err));
  close();
  return -1;
}

// helper
void list_interfaces(vector<string>* iip)
{
#ifndef _WIN32
  int fd = socket(PF_INET, SOCK_DGRAM, 0);    // IPv4, use PF_INET6 for IPv6
  if (fd >= 0)
  {
    char host[128];
    char buf[16384];
    struct ifconf ifconf;
    ifconf.ifc_len = sizeof buf;
    ifconf.ifc_buf = buf;
    if (ioctl(fd, SIOCGIFCONF, &ifconf) == 0)
    {
      struct ifreq* ifreq = ifconf.ifc_req;
      for (int i = 0; i < ifconf.ifc_len;)
      {
        // some systems have ifr_addr.sa_len and adjust the length that way, but not mine. weird
#ifdef WIN32
        size_t len = IFNAMSIZ + ifreq->ifr_addr.sa_len;
#else
        size_t len = sizeof * ifreq;
#endif
        getnameinfo(&ifreq->ifr_addr, sizeof ifreq->ifr_addr, host, sizeof host, 0, 0, NI_NUMERICHOST);
        if (strcmp(host, "127.0.0.1") != 0)
          iip->push_back(host);

        ifreq = (struct ifreq*)((char*)ifreq + len);
        i += len;
      }
    }
    close(fd);
  }
#else
  {
    assert(0);
  }
#endif
}