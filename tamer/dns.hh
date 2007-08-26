#ifndef TAMER_DNS_HH
#define TAMER_DNS_HH 1

#include <tamer/tamer.hh>
#include <tamer/fd.hh>
#include <tamer/ref.hh>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <map>
#include <list>

#define DNS_REPARSE_TIME 600

#define DNS_QUERY_UDP 0
#define DNS_QUERY_TCP 1

#define DNS_QUERY_SEARCH 0
#define DNS_QUERY_NO_SEARCH 1

#define DNS_OPTION_SEARCH 1
#define DNS_OPTION_NAMESERVERS 2
#define DNS_OPTION_MISC 4
#define DNS_OPTIONS_ALL 7

/* No error */
#define DNS_ERR_NONE 0
/* The name server was unable to interpret the query */
#define DNS_ERR_FORMAT 1
/* The name server was unable to process this query due to a problem with the
 * name server */
#define DNS_ERR_SERVERFAILED 2
/* The domain name does not exist */
#define DNS_ERR_NOTEXIST 3
/* The name server does not support the requested kind of query */
#define DNS_ERR_NOTIMPL 4
/* The name server refuses to reform the specified operation for policy
 * reasons */
#define DNS_ERR_REFUSED 5

/* The reply was truncated or ill-formated */
#define DNS_ERR_TRUNCATED 65
/* An unknown error occurred */
#define DNS_ERR_UNKNOWN 66
/* Communication with the server timed out */
#define DNS_ERR_TIMEOUT 67
/* The request was canceled because the DNS subsystem was shut down. */
#define DNS_ERR_SHUTDOWN 68


#define TYPE_A         1
#define TYPE_CNAME     5
#define TYPE_PTR       12
#define TYPE_AAAA      28

#define CLASS_INET     1

namespace tamer {
namespace dns {

inline int get8(const uint8_t * buf, int len, off_t &off, uint8_t &val) {
  if (off + 1 > len)
    return -1;
  
  val = buf[off++];

  return 0;
}

inline int get16(const uint8_t * buf, int len, off_t &off, uint16_t &val) {
  if (off + 2 > len)
    return -1;

  memcpy(&val, buf + off, 2);
  val = ntohs(val);
  off += 2;

  return 0;
}

inline int get32(const uint8_t * buf, int len, off_t &off, uint32_t &val) {
  if (off + 4 > len)
    return -1;

  memcpy(&val, buf + off, 4);
  val = ntohl(val);
  off += 4;

  return 0;
}

inline uint32_t min(uint32_t a, uint32_t b) {
  return (a < b) ? a : b;
}

inline int append16(uint8_t * buf, int len, off_t &off, uint16_t val) {
  uint16_t x;
  if (off + 2 > (off_t)len)
    return -1;

  x = htons(val);
  memcpy(buf + off, &x, 2);
  off += 2;

  return 0;
}

inline int append32(uint8_t * buf, int len, off_t &off, uint32_t val) {
  uint16_t x;
  if (off + 4 > (off_t)len)
    return -1;

  x = htonl(val);
  memcpy(buf + off, &x, 4);
  off += 4;

  return 0;
}

class reply {

  struct reply_state {
    int refcnt;
    int err;
    
    uint32_t type;
    uint32_t have_answer;
    uint32_t ttl; 
    
    std::string name;
    std::vector<uint32_t> addrs;

    uint16_t trans_id;

    reply_state () 
      : refcnt(1), err(0), have_answer(0), ttl(0), trans_id(0xffff) {
    }

    void use() {
      refcnt++;
    }

    void unuse() {
      refcnt--;
      if (!refcnt)
        delete this;
    }
    
    operator bool () {
      return !err;
    }
  };
  
  reply_state * _p;

  int name_parse(uint8_t * buf, int len, off_t &off, char * name, size_t size) const;
  int process(uint8_t * buf, int len);

public:
  typedef reply_state *reply::*unspecified_bool_type;

  inline reply();
  inline reply(uint8_t * packet, int length);
  inline reply(const reply &o);
  inline reply &operator=(const reply &o);
  inline ~reply();

  inline operator unspecified_bool_type() const;
  inline bool operator!() const;
  inline int error() const;

  inline bool operator==(uint16_t trans_id) const;
  inline bool operator==(const reply &o) const;
  inline uint16_t trans_id();

  inline int addrcount();
  inline uint32_t operator[](int i);
  inline std::string name();
};

inline reply::reply() 
  : _p(0) {
}

inline reply::reply(uint8_t *packet, int length) 
  : _p(new reply_state()) {
  if (_p)
    _p->err = process(packet, length);
}

inline reply::reply(const reply &o) {
  _p = o._p;
  if (_p)
    _p->use();
} 

inline reply &reply::operator=(const reply &o) {
  if (o._p)
    o._p->use();
  if (_p)
    _p->unuse();
  _p = o._p;
  return *this;
}

inline reply::~reply() {
  if (_p)
    _p->unuse();
}

inline reply::operator unspecified_bool_type() const {
  return (_p && *_p) ? &reply::_p : 0;
}

inline bool reply::operator!() const {
  return !(_p && *_p);
}

inline int reply::error() const {
  return _p ? _p->err : -1;
}

inline bool reply::operator==(uint16_t i) const {
  return _p ? _p->trans_id == i: 0;
}

inline bool reply::operator==(const reply &o) const {
  return _p ? _p->trans_id == o._p->trans_id : 0;
}

inline uint16_t reply::trans_id() {
  return _p ? _p->trans_id : 0xffff;
}

inline uint32_t reply::operator[](int i) {
  return (_p) ? _p->addrs[i] : 0;
}

inline int reply::addrcount() {
  return (_p) ? _p->addrs.size() : 0;
}

inline std::string reply::name() {
  return (_p && _p->type == TYPE_PTR) ? _p->name : 0;
}

class search {

  struct search_state {
    int refcnt;
    
    int ndomains;
    int ndots;
    
    struct search_domain {  
      int len;
      search_domain * next;
      char * dom;
    };

    search_domain * head;
    search_domain * tail;

    search_state(int ndots) 
      : refcnt(1), ndomains(0), ndots(ndots), head(0) {
    }

    void use() {
      refcnt++;
    }

    void unuse() {
      refcnt--;
      if (!refcnt)
        delete this;
    }

    ~search_state() {
      search_domain * curr, * next;
      curr = head;

      for (int i = 0; i < ndomains; i++) {
        next = curr->next;
        delete [] curr->dom;
        delete curr;
        curr = next;
      }
    }

    void add_domain(const char * dom) {
      search_domain * sdom;
      int len;

      len = strlen(dom);
      
      sdom = new search_domain;
      if (!sdom) return;
      
      sdom->dom = new char[len + 1];
      if (!(sdom->dom)) { delete sdom; return; }
      memcpy(sdom->dom, dom, len);
      
      sdom->len = len;
      sdom->next = NULL;

      if (!head) {
        head = sdom;
        tail = sdom;
      } else {
        tail->next = sdom;
        tail = sdom;
      }

      ndomains++;
    }
  };
  
  search_state * _s;

  struct base {
    int refcnt;
    
    int len;
    int ndots;
    
    search_state::search_domain ** index;
    char * dom;
    
    bool sent_raw;
    bool is_search;

    base(const char * name, search_state::search_domain ** index, int flags) 
      : refcnt(1), len(strlen(name)), ndots(0), index(index),  
      sent_raw(0), is_search((flags & DNS_QUERY_NO_SEARCH) ? 0 : 1)
    {
      dom = new char[len +1];

      if (!(dom))
        return; 
	    
      const char * s = name;
      while ((s = strchr(s, '.'))) {
    		s++;
		    ndots++;
    	}

      memcpy(dom, name, len);
      dom[len] = 0;
    }

    void use() {
      refcnt++;
    }

    void unuse() {
      refcnt--;
      if (!refcnt)
        delete this;
    }

    operator bool() {
      return dom && (!sent_raw || (is_search && *index));
    }
    
    ~base() {
      if (dom)
        delete [] dom;
    }
  };
  
  base * _b;

public:
  typedef search_state *search::*unspecified_bool_type;

  inline search();
  inline search(int dots);
  inline search(const search &sr);
  inline search &operator=(const search &sr);
  inline ~search();

  inline operator unspecified_bool_type() const;
  inline bool operator!() const;

  inline void add_domain(const char * domain);
  inline void set_ndots(int ndots);
  
  const char * next();
  
  search new_search(const char * base, int flags);
};

inline search::search()
  : _s(0), _b(0) {
}

inline search::search(int dots) 
  : _s(new search_state(dots)), _b(0) {
}

inline search::search(const search &sr) {
  _s = sr._s;
  _b = sr._b;

  if (sr._s) {
    _s->use();
    if (sr._b)
      _b->use();
  }
}

inline search &search::operator=(const search &o) {
  if (o._s) {
    o._s->use();
    if (o._b)
      o._b->use();
  }
  
  if (_s) {
    _s->unuse();
    if (_b)
      _b->unuse();
  }

  _s = o._s;
  _b = o._b;
  
  return *this;
}

inline search::~search() {
  if (_s) {
    _s->unuse();  
    if (_b)
      _b->unuse();
  }
}

inline search::operator unspecified_bool_type() const {
  return ((_s && !_b) || (_s && _b && *_b)) ? &search::_s : 0;
}

inline bool search::operator!() const {
  return !((_s && !_b) || (_s && _b && *_b));
}

inline void search::add_domain(const char * dom) {
  if (_s)
    _s->add_domain(dom);
}

inline void search::set_ndots(int ndots) {
  if (_s)
    _s->ndots = ndots;
}

class request {
  
  struct request_state {
    int refcnt;
    int err;
    int tx_count_;

    uint8_t *req;
    off_t offset;
    uint32_t max_len;
    
    int name_len;
    const char * req_name;
    search s;
    
    uint32_t type;
    uint16_t trans_id;
    
    request_state(uint32_t type, uint16_t trans_id, search s) 
      : refcnt(1), err(-1), tx_count_(0), offset(2), s(s),
        type(type), trans_id(trans_id) { 
    
      if (!s) 
        return;
     
      req_name = s.next();

      name_len = strlen(req_name);
      max_len = name_len + 96 + 2 + 4;   

      req = new uint8_t[max_len];
    }

    void use() {
      refcnt++;
    }

    void unuse() {
      refcnt--;
      if (!refcnt)
        delete this;
    }

    operator bool() {
      return req;
    }

    ~request_state() {
      if (req)
        delete [] req;
      if (req_name)
        delete [] req_name;
    }
  };

  struct request_state * _q;

  int dnsname_to_labels(const char * name, const int name_len);
  void build(); 

public:
  typedef request_state *request::*unspecified_bool_type;

  inline request();
  inline request(int type, uint16_t trans_id, search s);
  inline request(const request &o);
  inline request &operator=(request const &o);
  inline ~request();

  inline operator unspecified_bool_type() const;
  inline bool operator!() const;
  inline int error() const;
  
  inline bool operator ==(const request &o) const; 
  inline bool operator <(const request &o) const;
  inline bool operator==(uint16_t i) const;
  inline uint16_t trans_id() const;
  inline int tx_count() const;

  request next_search_request(uint16_t trans_id);
  request &reissue(uint16_t trans_id);
  void send(fd socket, int conn, event<int> result);
};

inline request::request()
  : _q(0) {
}

inline request::request(int type, uint16_t trans_id, search s)
  : _q(new request_state(type, trans_id, s)) {
  if (_q && *_q)
    build();
}

inline request::request(const request &o) {
  _q = o._q;
  if (_q)
    _q->use();
}

inline request &request::operator=(const request &o) {
  if (o._q)
    o._q->use();
  if (_q)
    _q->unuse();
  _q = o._q;
  return *this;
}

inline request::~request() {
  if (_q)
    _q->unuse();
}

inline request::operator unspecified_bool_type() const {
  return (_q && *_q) ? &request::_q : 0;
}

inline bool request::operator!() const {
  return !(_q && *_q);
}

inline int request::error() const {
  return _q ? _q->err : -1;
}

inline bool request::operator ==(const request &o) const { 
  return _q ? _q->trans_id == o._q->trans_id : 0; 
} 

inline bool request::operator <(const request &o) const {
  return _q ? _q->trans_id < o._q->trans_id : 0; 
}

inline bool request::operator==(uint16_t i) const {
  return _q ? _q->trans_id == i : 0;
}

inline uint16_t request::trans_id() const {
  return _q ? _q->trans_id : 0xffff;
}

inline int request::tx_count() const {
  return _q ? _q->tx_count_ : 0;
}

class nameserver {
  
    struct nameserver_state : public enable_ref_ptr_with_full_release<nameserver_state> {
    
    int _conn;
    int _timeouts;
    
    std::map<uint16_t, event<reply> > _reqs;
    
    tamer::fd _socket;
    uint32_t _addr;
    int _port;			// < 0 means nameserver object is shutting down
    
    nameserver_state(uint32_t addr, int port) 
    : _timeouts(0), _addr(addr), _port(port) {
    }

    ~nameserver_state() {
        _socket.close();
    }
    
    operator bool() {
      return _socket;
    }

	void full_release() {
	    _socket.close();
	    _port = -1;
	}

	inline void flush();
	
	void loop_udp(event<> e);
	void loop_tcp(event<> e);
	void query(request q, int timeout, event<int, reply> e);
	void probe(event<> e);
	
	class closure__loop_udp__Q_; void loop_udp(closure__loop_udp__Q_ &, unsigned);
	class closure__loop_tcp__Q_; void loop_tcp(closure__loop_tcp__Q_ &, unsigned);
	class closure__query__7requestiQi5reply_; void query(closure__query__7requestiQi5reply_ &, unsigned);
	class closure__probe__Q_; void probe(closure__probe__Q_ &, unsigned);
    };

  ref_ptr<nameserver_state> _n;
  

public:
  typedef ref_ptr<nameserver_state> nameserver::*unspecified_bool_type;
  
  inline nameserver();
  inline nameserver(uint32_t addr, int port, bool tcp, const event<> &done);
  inline nameserver(const nameserver &ns);
  inline nameserver &operator=(const nameserver &ns);
  inline ~nameserver();
  
  inline operator unspecified_bool_type() const;
  inline bool operator!() const;
  inline int error() const;
  inline int timeouts() const;
  
  inline bool operator==(uint32_t i) const; 

    void query(request q, int timeout, const event<int, reply> &e) {
	_n->query(q, timeout, e);
    }
    
    void probe(const event<> &e) {
	_n->probe(e);
    }
    
};

inline nameserver::nameserver()
  : _n() {
}

inline nameserver::nameserver(uint32_t addr, int port, bool tcp, const event<> &done)
  : _n(new nameserver_state(addr, port)) {
    if (tcp)
	_n->loop_tcp(done);
    else
	_n->loop_udp(done);
}

inline nameserver::nameserver(const nameserver &o) {
  _n = o._n;
}

inline nameserver &nameserver::operator=(const nameserver &o) {
    _n = o._n;
    return *this;
}

inline nameserver::~nameserver(){
}

inline nameserver::operator unspecified_bool_type() const {
  return (_n && *_n) ? &nameserver::_n : 0;
}

inline bool nameserver::operator!() const {
  return !(_n && *_n);
}

inline bool nameserver::operator==(uint32_t i) const {
  return _n ? (i == _n->_addr) : 0;
}

inline int nameserver::timeouts() const {
  return _n ? _n->_timeouts : 0;
}

class resolver {
  struct resolve_state {
    int refcnt;

    int timeout;
    int ndots;
    int max_retransmits;
    int max_timeouts;
    int max_reqs_inflight;

    std::string rcname;
    struct stat fst;

    int reqs_inflight;
    int flags;
    int err;
  
    search global_search;
    
    std::list<request> waiting;
    std::map<uint16_t, event<reply> > requests;
    
    std::list<nameserver> nameservers;
    std::list<nameserver> nameservers_failed;
    std::list<nameserver>::iterator nsindex;

    resolve_state(int flags, std::string rc) 
      : refcnt(1), rcname(rc), reqs_inflight(0), flags(flags), err(0) {
      
      fst.st_ino = 0;
      fst.st_mtime = 0;
      
      nsindex = nameservers.begin();
      global_search = search(1);//TODO

    }

    void use() {
      refcnt++;
    } 

    void unuse() {
      refcnt--;
      if (!refcnt)
        delete this;
    }

    operator bool() { 
      return !err;
    }

    ~resolve_state() {
      nameservers.clear();
      for (std::map<uint16_t, event<reply> >::iterator i = requests.begin();
          i != requests.end();
          i++)
        i->second.trigger(reply());
      requests.clear();
      waiting.clear();
    }
  };

  struct resolve_state * _r;

  /* pump */
  void pump();
  
  void push(request q, event<reply> e);
  void pop(reply p);
  void pop(request q);
  void reissue(request q);
  void next(request q);
  
  uint16_t get_trans_id();

  void failed(nameserver ns);
  
  /* parse */
  void parse_loop();
  void parse(event<> e);
  
  void add_nameserver(const char * str_addr, std::list<nameserver> &nss, event<> e);
  
  inline void add_domain(const char * name);
  inline void clr_domains();
  inline void set_from_hostname();

  inline void set_default_options();
  inline void set_option(const char * option);
  
  inline int strtoint(const char * val, int min, int max);
  inline char * next_line(char *buf);

  class closure__parse_loop; void parse_loop(closure__parse_loop &, unsigned);
  class closure__pump; void pump(closure__pump &, unsigned);
  class closure__resolve_ipv4__PKciQ5reply_; void resolve_ipv4(closure__resolve_ipv4__PKciQ5reply_ &, unsigned);
  class closure__failed__10nameserver; void failed(closure__failed__10nameserver &, unsigned);
  class closure__parse__Q_; void parse(closure__parse__Q_&, unsigned);
  class closure__add_nameserver__PKcRNSt4listI10nameserverEEQ_; void add_nameserver(closure__add_nameserver__PKcRNSt4listI10nameserverEEQ_ &, unsigned);

public:
  typedef resolve_state *resolver::*unspecified_bool_type;
  
  inline resolver(int flags, std::string rc = "/etc/resolv.conf");
  inline resolver(const resolver &o);
  inline resolver &operator=(const resolver &o);
  inline ~resolver();

  inline operator unspecified_bool_type() const;
  inline bool operator!() const;
  inline int error() const;
  
  void resolve_ipv4(const char * name, int flags, event<reply> e);
  //void resolve_reverse();
};

inline resolver::resolver(int flags, std::string rc){
  _r = new struct resolve_state(flags, rc);

  if (!_r)
    return;
  parse_loop();
}

inline resolver::resolver(const resolver &o) {
  _r = o._r;
  if (o._r)
    _r->use();
}

inline resolver &resolver::operator=(const resolver &o) {
  if (o._r)
    o._r->use();
  if (_r)
    _r->unuse();
  _r = o._r;
  return *this;
}

inline resolver::~resolver() {
  _r->unuse();
}

inline resolver::operator unspecified_bool_type() const {
  return (_r && *_r) ? &resolver::_r : 0;
}

inline bool resolver::operator!() const {
  return !(_r && *_r);
}

inline int resolver::error() const {
  return _r ? _r->err : -1;
}

inline void resolver::set_default_options() {
  _r->ndots = 1;
  _r->timeout = 5;
  _r->max_retransmits = 1;
  _r->max_timeouts = 3;
  _r->max_reqs_inflight = 64;
}

inline void resolver::set_from_hostname() {
  char hostname[HOST_NAME_MAX + 1];
  char * domainname;

  if (_r->flags & DNS_OPTION_SEARCH) {
	  if (gethostname(hostname, sizeof(hostname))) return;
  	domainname = strchr(hostname, '.');
  	if (!domainname) return;
  	add_domain(domainname);
  }
}

inline void resolver::clr_domains() {
  _r->global_search = search(_r->ndots);
}

inline void resolver::add_domain(const char * name) {
  _r->global_search.add_domain(name);
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
