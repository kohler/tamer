#ifndef TAMER_DNS_HH
#define TAMER_DNS_HH 1
#include <tamer/tamer.hh>
#include <tamer/fd.hh>
#include <tamer/ref.hh>
#include <arpa/inet.h>
#include <map>
#include <list>
#include <set>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


#define DNS_REPARSE_TIME 60

#define DNS_OPTION_SEARCH 1
#define DNS_OPTION_NAMESERVERS 2
#define DNS_OPTION_MISC 4
#define DNS_OPTIONS_ALL 7

/* no error */
#define DNS_ERR_NONE 0
/* name server was unable to interpret the query */
#define DNS_ERR_FORMAT 1
/* name server was unable to process this query due to a problem with the name server */
#define DNS_ERR_SERVERFAILED 2
/* domain name does not exist */
#define DNS_ERR_NOTEXIST 3
/* name server does not support the requested kind of query */
#define DNS_ERR_NOTIMPL 4
/* name server refuses to perform the specified operation for policy reasons */
#define DNS_ERR_REFUSED 5
/* an unknown error occurred */
#define DNS_ERR_UNKNOWN 16
/* reply was truncated */
#define DNS_ERR_TRUNCATED 17

#define CLASS_INET     1
#define TYPE_A         1
#define TYPE_PTR       12

namespace tamer {
namespace dns {

class packet_imp;
struct reply_imp;
struct search_list_imp;
class request_imp;
class nameserver_imp;

typedef ref_ptr<packet_imp> packet;
typedef ref_ptr<reply_imp> reply;
typedef ref_ptr<search_list_imp> search_list;
typedef ref_ptr<request_imp> request;
typedef ref_ptr<nameserver_imp> nameserver;

}

void gethostbyname(std::string name, bool search, event<dns::reply> result);
void gethostbyaddr(struct in_addr *in, event<dns::reply> result);

namespace dns {

packet make_packet(size_t size);
packet make_packet(uint8_t * buf, size_t size);
reply make_reply(packet p);
search_list make_search_list(int ndots);
request make_request_a(std::string name, bool search = false, search_list s = search_list());
request make_request_ptr(struct in_addr *in);
nameserver make_nameserver(uint32_t addr, int port = 53);

class packet_imp : public enable_ref_ptr_with_full_release<packet_imp> {
public:
  struct _strlen_t { int len; _strlen_t(int l) : len(l) {}};

private:
  typedef void (packet_imp::*unspecified_bool_type)() const;
  void unspecified_method() const {}

  off_t _off;
  ssize_t _fill;
  ssize_t _max;
  uint8_t * _buf;
  _strlen_t _strlen;

  void expand() { if (_off > _fill) _fill = _off; }

public:
  packet_imp(size_t size);
  packet_imp(uint8_t * buf, size_t size);
  void full_release();

  operator unspecified_bool_type() const;

  static _strlen_t set_strlen(int len);
  packet_imp &operator>>(_strlen_t _strlen);
  packet_imp &operator<<(_strlen_t _strlen);

  void reset();
  ssize_t size() const;
  off_t offset() const;
  packet_imp &operator+=(unsigned int p);

  const uint8_t * getbuf() const;

  packet_imp &operator>>(uint8_t &val);
  packet_imp &operator>>(uint16_t &val);
  packet_imp &operator>>(uint32_t &val);
  packet_imp &operator>>(struct in_addr &val);
  packet_imp &operator>>(std::string &val);

  packet_imp &operator<<(uint8_t val);
  packet_imp &operator<<(uint16_t val);
  packet_imp &operator<<(uint32_t val);
  packet_imp &operator<<(std::string val);
};

inline packet make_packet(size_t size) {
  return packet(new packet_imp(size));
}

inline packet make_packet(uint8_t * buf, size_t size) {
  return packet(new packet_imp(buf, size));
}

inline packet_imp::packet_imp(size_t size)
  : _off(0), _fill(0), _max(size), _buf(new uint8_t[size]), _strlen(_strlen_t(-1)) {
  memset(_buf, 0, size);
}

inline packet_imp::packet_imp(uint8_t * buf, size_t size)
  : _off(0), _fill(size), _max(size), _buf(new uint8_t[size]), _strlen(_strlen_t(-1)) {
  memcpy(_buf, buf, size);
}

inline packet_imp::operator unspecified_bool_type() const {
  return (_off > _max || _fill > _max) ? 0 : &packet_imp::unspecified_method;
}

inline void packet_imp::full_release() {
  delete [] _buf;
}

inline packet_imp::_strlen_t packet_imp::set_strlen(int len) {
  return _strlen_t(len);
}

inline packet_imp &packet_imp::operator>>(_strlen_t s) {
  _strlen = s;
  return *this;
}

inline packet_imp &packet_imp::operator<<(_strlen_t s) {
  _strlen = s;
  return *this;
}
inline void packet_imp::reset() {
  _off = 0;
}

inline ssize_t packet_imp::size() const {
  return _fill;
}

inline off_t packet_imp::offset() const {
  return _off;
}

inline packet_imp &packet_imp::operator+=(unsigned int p) {
  _off += p;
  expand();
  return *this;
}

inline const uint8_t * packet_imp::getbuf() const {
  return _buf;
}

inline packet_imp &packet_imp::operator>>(uint8_t &val) {
  val = _buf[_off];
  _off += sizeof(uint8_t);
  return *this;
}

inline packet_imp &packet_imp::operator>>(uint16_t &val) {
  val = htons(*(uint16_t *)&_buf[_off]);
  _off += sizeof(uint16_t);
  return *this;
}

inline packet_imp &packet_imp::operator>>(uint32_t &val) {
  val = htonl(*(uint32_t *)&_buf[_off]);
  _off += sizeof(uint32_t);
  return *this;
}

inline packet_imp &packet_imp::operator>>(struct in_addr &val) {
  val.s_addr = *(uint32_t *)&_buf[_off];
  _off += sizeof(uint32_t);
  return *this;
}

inline packet_imp &packet_imp::operator>>(std::string &val) {
  if (_strlen.len < 0) {
    val = std::string((char *)&_buf[_off]);
    _off += val.length() + 1;// for \0 byte
  } else {
    val = std::string((char *)&_buf[_off], _strlen.len);
    _off += _strlen.len;
    _strlen.len = -1;
  }
  return *this;
}

inline packet_imp &packet_imp::operator<<(uint8_t val) {
  _buf[_off] = val;
  _off += sizeof(uint8_t);
  expand();
  assert(*this);
  return *this;
}

inline packet_imp &packet_imp::operator<<(uint16_t val) {
  *(uint16_t *)&_buf[_off] = htons(val);
  _off += sizeof(uint16_t);
  expand();
  assert(*this);
  return *this;
}

inline packet_imp &packet_imp::operator<<(uint32_t val) {
  *(uint32_t *)&_buf[_off] = htonl(val);
  _off += sizeof(uint32_t);
  expand();
  assert(*this);
  return *this;
}

inline packet_imp &packet_imp::operator<<(std::string val) {
  if (_strlen.len < 0) {
    strcpy((char *)&_buf[_off], val.c_str());
    _off += val.length();
  } else {
    memcpy(&_buf[_off], val.c_str(), _strlen.len);
    _off += _strlen.len;
    _strlen.len = -1;
  }
  expand();
  assert(*this);
  return *this;
}

struct reply_imp : public enable_ref_ptr {
private:
  typedef void (reply_imp::*unspecified_bool_type)() const;
  void unspecified_method() const {};

public:
  int err;
  uint16_t trans_id;
  uint32_t ttl;
  std::vector<uint32_t> addrs;
  std::string name;

  reply_imp(packet p);
  operator unspecified_bool_type() const;

private:
  static int skip_name(ref_ptr<packet_imp> &p);
};

inline reply make_reply(packet p) {
  return reply(new reply_imp(p));
}

inline reply_imp::operator unspecified_bool_type() const {
  return (err) ? 0 : &reply_imp::unspecified_method;
}

inline int reply_imp::skip_name(ref_ptr<packet_imp> &p) {
  uint8_t len;

  while(*p) {
    *p >> len;
    if (!len) break; // done
    if (len & 0xC0){ *p += 1; break; } // pointer
    if (len > 63) return -1; // label too long
    *p += len;
  }

  return *p ? 0 : -1; // read past the buffer?
}

struct search_list_imp : public enable_ref_ptr {
  int ndots;
  std::list<std::string> domains;
  search_list_imp(int n) : ndots(n) {}
};

inline search_list make_search_list(int ndots) {
  return search_list(new search_list_imp(ndots));
}

class request_imp : public enable_ref_ptr {
  typedef void (request_imp::*unspecified_bool_type)() const;
  void unspecified_method() const {}

protected:
  uint16_t _type;

  int _err;
  int _tx_count;
  bool _sent_raw;

  uint16_t _trans_id;
  std::string _curr_name;

public:
  request_imp(uint16_t type);
  virtual ~request_imp() {};
  operator unspecified_bool_type() const;
  int error() const;
  int tx_count() const;
  uint16_t trans_id() const;

  virtual bool hasnext() const = 0;
  virtual void next(uint16_t trans_id) = 0;
  void reissue(uint16_t trans_id);

  int getpacket(packet &p, bool tcp = false);
};

inline request_imp::request_imp(uint16_t type)
  : _type(type), _err(-1), _tx_count(0), _sent_raw(0), _trans_id(0xFFFF) {
}

inline request_imp::operator unspecified_bool_type() const {
  return (_err) ? 0 : &request_imp::unspecified_method;
}

inline int request_imp::error() const {
  return _err;
}

inline int request_imp::tx_count() const {
  return _tx_count;
}

inline uint16_t request_imp::trans_id() const {
  return _trans_id;
}

inline void request_imp::reissue(uint16_t trans_id) {
  _trans_id = trans_id;
  _tx_count++;
}

class request_a : public request_imp {
  int _search;
  std::string _name;

  int _ndots;
  search_list _s;
  std::list<std::string>::const_iterator _doms_it;

  static bool check_name(std::string name);

public:
  request_a(std::string name, bool search = false, search_list s = search_list());
  ~request_a() {}
  bool hasnext() const;
  void next(uint16_t trans_id);
};

inline request make_request_a(std::string name, bool search, search_list s) {
  return request(new request_a(name, search, s));
}

inline request_a::request_a(std::string name, bool search, search_list s)
  : request_imp(TYPE_A), _search(search), _name(name), _ndots(0), _s(s) {

  if ((_err = check_name(name) ? 0 : -1))
    return;

  if (search) {
    _doms_it = _s->domains.begin();
    for (int i = name.find('.', 0); i >= 0; i = name.find('.', i + 1))
      _ndots++;
  }
}

inline bool request_a::hasnext() const {
  return (!_sent_raw || (_search && _doms_it != _s->domains.end())) ? 1 : 0;
}

inline bool request_a::check_name(std::string name) {
  size_t i = 0;
  if (name.size() > 255)
    return false;
  for (size_t p = name.find('.'); p != std::string::npos; i = p + 1, p = name.find('.', i))
    if (p - i > 63)
      return false;
  if (name.size() - i > 63)
    return false;
  return true;
}

class request_ptr : public request_imp {
public:
  request_ptr(struct in_addr * addr);
  ~request_ptr() {}
  bool hasnext() const;
  void next(uint16_t trans_id);
};

inline request make_request_ptr(struct in_addr *in) {
  return request(new request_ptr(in));
}

inline request_ptr::request_ptr(struct in_addr * addr) : request_imp(TYPE_PTR) {
  std::ostringstream s;
  uint32_t a;

  _err = 0;

  a = ntohl(addr->s_addr);
  s << (int)((a    )&0xff) << "." <<
       (int)((a>>8 )&0xff) << "." <<
       (int)((a>>16)&0xff) << "." <<
       (int)((a>>24)&0xff) << ".in-addr.arpa";
  _curr_name = s.str();
}

inline bool request_ptr::hasnext() const {
  return !_sent_raw;
}

class nameserver_imp
  : public enable_ref_ptr_with_full_release<nameserver_imp> {
public:
  int timeouts;
  std::list<reply> received;
  event<int> ready;

private:
  typedef void (nameserver_imp::*unspecified_bool_type)() const;
  void unspecified_method() const {}

  uint32_t _addr;
  int _port;

  std::list<packet> _tcp_outgoing;
  int _tcp_outbound;

  tamer::fd _udp;
  tamer::fd _tcp;

public:
  nameserver_imp(uint32_t addr, int port);

  operator unspecified_bool_type() const;
  bool operator< (const nameserver_imp &n2) const;

  void full_release() {
    if (ready) ready.trigger(-1);
    if (_udp) _udp.close();
    if (_tcp) _tcp.close();
  }

  void print() {
    struct in_addr ina;
    ina.s_addr = _addr;
    printf("ns: %s\n", inet_ntoa(ina));
  }

  void init(event<int> e);
  void init_tcp(event<int> e, timeval timeout);
  void query(packet p);
  void query_tcp(packet p, timeval timeout);

private:
  class closure__init__Qi_;
  void init(closure__init__Qi_ &);

  class closure__init_tcp__Qi_7timeval;
  void init_tcp(closure__init_tcp__Qi_7timeval &);

  class closure__query__6packet;
  void query(closure__query__6packet&);

  class closure__query_tcp__6packet7timeval;
  void query_tcp(closure__query_tcp__6packet7timeval&);
};

class nameserver_comp {
public:
  bool operator() (const nameserver &n1, const nameserver &n2) const {
    return *n1 < *n2;
  }
};

typedef std::set<nameserver, nameserver_comp> nameservers;

inline nameserver make_nameserver(uint32_t addr, int port) {
  return nameserver(new nameserver_imp(addr, port));
}

inline nameserver_imp::nameserver_imp(uint32_t addr, int port)
  : timeouts(0), _addr(addr), _port(port), _tcp_outbound(0) {
}

inline nameserver_imp::operator unspecified_bool_type() const {
  return (!_udp) ? 0 : &nameserver_imp::unspecified_method;
}

inline bool nameserver_imp::operator< (const nameserver_imp &n) const {
  return _addr < n._addr || (_addr == n._addr && _port < n._port);
}

/////////////////////

struct query {
  request q;
  event<reply> p;
  query() : q(), p() {}
  query(request q, event<reply> p) : q(q), p(p) {}
  operator uint16_t() { return q->trans_id(); }
};

class resolver : public enable_ref_ptr_with_full_release<resolver> {
  typedef void (resolver::*unspecified_bool_type)() const;
  void unspecified_method() const {}

  std::string _rcname;
  int _flags;

  timeval _timeout;
  int _ndots;
  int _max_retransmits;
  int _max_timeouts;
  int _max_reqs_inflight;
  search_list _search_list;

  int _err;
  int _reqs_inflight;
  struct stat _fst;

  std::map<uint16_t, query> _requests;

  nameservers _nameservers;
  nameservers _failed;
  nameservers::iterator _nsindex;
  event<> _init;
  bool _is_init;

  event<> _reparse;

  uint16_t get_trans_id();

  void resolve(request q, event<reply> e);

  void add_nameservers(nameservers n, event<>);
  void handle_nameserver(nameserver n);
  void failed_nameserver(nameserver n);
  nameserver next_nameserver();

  void parse_loop();
  void parse();

  void set_default(event<> e);
  void set_default_options();
  void set_option(const char * option);
  void set_from_hostname();
  void add_domain(const char * name);

  static int strtoint(const char * val, int min, int max);
  static char * next_line(char *buf);

  class closure__ready__Q_;
  void ready(closure__ready__Q_&);

  class closure__parse_loop;
  void parse_loop(closure__parse_loop&);

  class closure__parse;
  void parse(closure__parse&);

  class closure__resolve__7requestQ5reply_;
  void resolve(closure__resolve__7requestQ5reply_&);

  class closure__add_nameservers__11nameserversQ_;
  void add_nameservers(closure__add_nameservers__11nameserversQ_&);

  class closure__failed_nameserver__10nameserver;
  void failed_nameserver(closure__failed_nameserver__10nameserver&);

  class closure__handle_nameserver__10nameserver;
  void handle_nameserver(closure__handle_nameserver__10nameserver&);

#if defined(HOST_NAME_MAX)
    enum { host_name_max = HOST_NAME_MAX };
#elif defined(_POSIX_HOST_NAME_MAX)
    enum { host_name_max = _POSIX_HOST_NAME_MAX };
#else
    enum { host_name_max = 255 };
#endif

  public:
    resolver(int flags, std::string rc = "/etc/resolv.conf");

    operator unspecified_bool_type() const;
    int error() const;

    void ready(event<> e);
    void resolve_a(std::string name, bool search, event<reply> e);
    void resolve_ptr(struct in_addr *in, event<reply> e);

    void full_release();
};

inline resolver::resolver(int flags, std::string rc)
  : _rcname(rc), _flags(flags), _err(0), _reqs_inflight(0),
    _fst(), _nsindex(_nameservers.begin()), _is_init(false) {
  struct timeval tv;
  set_default_options();
  gettimeofday(&tv, NULL);
  srand(tv.tv_usec);
  parse_loop();
}

inline resolver::operator unspecified_bool_type() const {
  return (_err) ? 0 : &resolver::unspecified_method;
}

inline int resolver::error() const {
  return _err;
}

inline void resolver::resolve_a(std::string name, bool search, event<reply> e) {
  request q;

  q = make_request_a(name, search, _search_list);
  if (!*q)
    e.trigger(reply());
  else
    resolve(q, e);
}

inline void resolver::resolve_ptr(struct in_addr *in, event<reply> e) {
  request q;

  q = make_request_ptr(in);
  resolve(q, e);
}

inline void resolver::full_release() {
  _nameservers.clear();
  _failed.clear();
  for (std::map<uint16_t, query >::iterator i = _requests.begin();
      i != _requests.end(); i++)
    i->second.p.trigger(reply());
  _requests.clear();
}

inline nameserver resolver::next_nameserver() {
  nameserver n;
  assert(_nameservers.size());
  if (_nsindex == _nameservers.end())
    _nsindex = _nameservers.begin();
  n = *_nsindex;
  _nsindex++;
  return n;
}

//TODO use preprocessor constants
inline void resolver::set_default_options() {
  _ndots = 1;
  _timeout.tv_sec = 5;
  _timeout.tv_usec = 0;
  _max_retransmits = 1;
  _max_timeouts = 3;
  _max_reqs_inflight = 64;
}

inline void resolver::set_from_hostname() {
    char hostname[host_name_max + 1];
    char *domainname;

    _search_list = make_search_list(_ndots);
    if (gethostname(hostname, sizeof(hostname))) return;
    domainname = strchr(hostname, '.');
    if (!domainname) return;
    add_domain(domainname);
}

inline void resolver::add_domain(const char * name) {
  assert(_search_list);
  _search_list->domains.push_back(name);
}

inline int resolver::strtoint(const char * val, int min, int max) {
  char *end;
  int r = strtol(val, &end, 10);

  if (*end)
    return -1;
  else if (r < min)
    return min;
  else if (r > max)
    return max;
  else
    return r;
}

inline char* resolver::next_line(char * buf) {
  char *r;
  return (r = strchr(buf, '\n')) ? (*r = 0) + r + 1 : 0;
}

}}
#endif /* TAMER_DNS_HH */
