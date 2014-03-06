// -*- mode: c++ -*-
// osptracker.tcc - A tracker implementation for a simple peer-to-peer
//   protocol, designed for teaching.  For a description of the protocol,
//   see http://www.read.cs.ucla.edu/111/2007spring/lab4

/* Copyright (c) 2007, Eddie Kohler
 * Copyright (c) 2007, Regents of the University of California
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Tamer LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Tamer LICENSE file; the license in that file is
 * legally binding.
 */

#include <tamer/tamer.hh>
#include <tamer/fd.hh>
#include <tamer/bufferedio.hh>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string>
#include <ctype.h>
#include <vector>
#include <deque>
#include <map>
#include <sstream>
#include <algorithm>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include "md5.h"

#define SHORT_TIMEOUT 60
#define LONG_TIMEOUT (60*10)

struct peer;

struct havefile { public:

	havefile()
		: _ngood(0) {
	}

	operator bool() {
		return _ngood > 0;
	}

	const std::string &md5sum() const {
		return _md5sum;
	}

	enum { add_badsum = -1, add_ok = 0, add_repeat = 1 };
	int add_peer(const std::string &filename, peer *p, const std::string &theirsum = std::string());
	enum { remove_ok = 0, remove_absent = 1 };
	int remove_peer(const std::string &filename, peer *p);

	typedef std::vector<peer *>::iterator peer_iterator;
	const std::vector<peer *> &peers() const {
		return _peers;
	}
	peer_iterator begin_peers() {
		return _peers.begin();
	}
	peer_iterator end_peers() {
		return _peers.end();
	}

    private:

	std::string _md5sum;
	std::vector<peer *> _peers;
	int _ngood;

};

std::map<std::string, havefile> havefiles;

inline bool operator==(struct in_addr a, struct in_addr b) {
	return a.s_addr == b.s_addr;
}

inline bool operator!=(struct in_addr a, struct in_addr b) {
	return a.s_addr != b.s_addr;
}

inline bool firewalled(struct in_addr) {
	//uint32_t x = ntohl(a.s_addr);
	//return (x >= 0x806121AA && x <= 0x806121BA);
	return false;
}

inline bool inform_addr(struct in_addr src, struct in_addr dst) {
	return !firewalled(dst) || src == dst;
}

struct peer {
	std::string calias;
	struct in_addr caddr;
	int cport;
	struct in_addr xaddr;
	int xaddr_num;
	tamer::fd cfd;
	bool notify;
	bool good;
	std::vector<std::string> havefiles;

	bool blocknotify;
	std::deque<peer *> notifyq;
	tamer::event<> notifywake;

	peer(tamer::fd cfd);
	~peer() {
		assert(!_usecount);
		unnotify();
	}
	bool real() const {
		return caddr.s_addr != INADDR_ANY;
	}
	void set_addr(const std::string &alias, struct in_addr addr, int port);
	void unnotify() {
		while (notifyq.size()) {
			peer *p = notifyq.front();
			p->unuse();
			notifyq.pop_front();
		}
		notifywake.trigger();
	}
	void use() {
		++_usecount;
	}
	void unuse() {
		if (!--_usecount)
			delete this;
	}

    private:

	int _usecount;

};

peer::peer(tamer::fd fd)
	: calias(), cport(0), cfd(fd),
	  notify(false), good(true), blocknotify(false), _usecount(1)
{
	caddr.s_addr = INADDR_ANY;

	struct sockaddr_in saddr;
	socklen_t saddrlen = sizeof(saddr);
	if (getpeername(fd.value(), (struct sockaddr *) &saddr, &saddrlen) != -1
	    && saddr.sin_family == AF_INET)
		xaddr = saddr.sin_addr;
	else
		xaddr.s_addr = INADDR_ANY;
}

void peer::set_addr(const std::string &alias, struct in_addr addr, int port) {
	calias = alias;
	caddr = addr;
	cport = port;
}


std::vector<peer *> peers;

int allow_local = 0;
int exclusive = 0;
int fakers = 0;



int havefile::add_peer(const std::string &filename, peer *p,
		       const std::string &theirsum)
{
	if (_md5sum.length() && theirsum.length() && _md5sum != theirsum)
		return add_badsum;
	if (theirsum.length())
		_md5sum = theirsum;
	if (std::find(_peers.begin(), _peers.end(), p) != _peers.end())
		return add_repeat;
	_peers.push_back(p);
	p->havefiles.push_back(filename);
	if (p->good)
		++_ngood;
	return add_ok;
}

int havefile::remove_peer(const std::string &filename, peer *p)
{
	std::vector<peer *>::iterator i = std::find(_peers.begin(), _peers.end(), p);
	if (i == _peers.end())
		return remove_absent;
	else {
		_peers.erase(i);

		std::vector<std::string>::iterator ii = std::find(p->havefiles.begin(), p->havefiles.end(), filename);
		assert(ii != p->havefiles.end());
		p->havefiles.erase(ii);

		if (p->good)
			--_ngood;

		return remove_ok;
	}
}


static void die(const char *format, ...) __attribute__((noreturn));

static int xvalue(unsigned char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	else
		return tolower(c) - 'a' + 10;
}

void to_words(std::string &s, std::vector<std::string> &words) {
	words.clear();
	std::string::iterator i = s.begin();
	while (i != s.end()) {
		while (i != s.end() && isspace((unsigned char) *i))
			i++;
		std::string::iterator j = i;
		std::string::iterator o = i;
		while (i != s.end() && !isspace((unsigned char) *i)) {
			if (*i == '%' && i + 2 < s.end()
			    && isxdigit((unsigned char) i[1])
			    && isxdigit((unsigned char) i[2])) {
				*o++ = 16 * xvalue(i[1]) + xvalue(i[2]);
				i += 3;
			} else
				*o++ = *i++;
		}
		if (o != j)
			words.push_back(std::string(j, o));
	}
}

void uppercase(std::string &s) {
	for (std::string::iterator i = s.begin(); i != s.end(); i++)
		*i = toupper((unsigned char) *i);
}

char xdigit(int c) {
	if (c >= 0 && c <= 9)
		return '0' + c;
	else if (c <= 15)
		return 'A' + c - 10;
	else
		return '~';
}

std::string protect(const std::string &s) {
	std::ostringstream os;
	std::string::const_iterator i = s.begin();
	for (std::string::const_iterator j = i; j != s.end(); j++) {
		unsigned char c = *j;
		if (isalnum(c) || c == '$' || c == '-'
		    || c == '_' || c == '.' || c == '+' || c == '!'
		    || c == '*' || c == '\'' || c == '(' || c == ')'
		    || c == ',')
			continue;
		os << std::string(i, j) << '%' << xdigit((c / 16) & 15) << xdigit(c & 15);
		i = j + 1;
	}
	if (i == s.begin())
		return s;
	os << std::string(i, s.end());
	return os.str();
}

std::string ntoa(int x) {
	std::ostringstream os;
	os << x;
	return os.str();
}

bool check_md5sum(const std::string &s) {
	if (s.length() != MD5_TEXT_DIGEST_SIZE)
		return false;
	for (int i = 0; i < MD5_TEXT_DIGEST_SIZE; i++) {
		unsigned char c = s[i];
		if (!((c >= 'A' && c <= 'Z')
		      || (c >= 'a' && c <= 'z')
		      || (c >= '0' && c <= '9')
		      || c == '_' || c == '@'))
			return false;
		if (i == MD5_TEXT_DIGEST_SIZE - 1
		    && !(c >= 'a' && c <= 'd'))
			return false;
	}
	return true;
}

tamed void kill_connection(tamer::fd cfd, const std::string &s) {
	tvars { int ret; }
	twait { cfd.write(s, make_event(ret)); }
	cfd.close();
}

tamed void timeout_connection(tamer::fd cfd, int to, tamer::event<> *canceler) {
	tvars { int ret; tamer::rendezvous<int> r; }
	tamer::at_delay_sec(to, tamer::make_event(r, 0));
	if (canceler)
		*canceler = tamer::make_event(r, 1);
	twait(r, ret);
	if (ret == 0)
		kill_connection(cfd, "500 Connection timed out: goodbye!\n");
}

tamed void notify(peer *pp, peer *cpeer)
{
	tvars { int ret; tamer::fd sock; struct sockaddr_in saddr; }
	pp->use();
	cpeer->use();
	twait {
		tamer::event<tamer::fd> e = make_event(sock);
		pp->cfd.at_close(e.unblocker());
		tamer::tcp_connect(pp->caddr, pp->cport, tamer::add_timeout_sec(5, e, -ETIMEDOUT));
	}
	// will be a noop if sock is not connected
	twait {
		sock.write("NOTIFY " + protect(cpeer->calias) + " " + std::string(inet_ntoa(cpeer->caddr)) + ":" + ntoa(cpeer->cport) + " OSP2P\n", make_event(ret));
	}
	sock.close();
	pp->unuse();
	cpeer->unuse();
}


tamed void write_peers(const std::vector<peer *> &peers, peer *cpeer,
		       bool always_fakers, tamer::event<int> done)
{
	tvars {
		int ret = 0;
		int n = 0, idx;
		std::vector<std::string> strs;
	}
	cpeer->use();
	for (std::vector<peer *>::const_iterator i = peers.begin();
	     i != peers.end(); ++i) {
		peer *px = *i;
		if (px->good && px->real() && inform_addr(cpeer->caddr, px->caddr))
			strs.push_back("PEER " + px->calias
				       + " " + std::string(inet_ntoa(px->caddr))
				       + ":" + ntoa(px->cport) + "\n");
	}
	if (fakers && (always_fakers || strs.size())) {
		for (n = 0; n < 250 && ret >= 0; ++n)
			twait {
				cpeer->cfd.write("PEER fakepeer" + ntoa(n) + " 127.0.0.1:" + ntoa(11211 + n) + "\n", make_event(ret));
			}
	}
	while (ret >= 0 && strs.size()) {
		idx = random() % strs.size();
		twait {
			cpeer->cfd.write(strs[idx], make_event(ret));
		}
		strs[idx] = strs.back();
		strs.pop_back();
		++n;
	}
	done.trigger(ret >= 0 ? n : ret);
	cpeer->unuse();
}


static bool parse_addr_port(const char *s, struct in_addr *addr, int *port)
{
	const char *colon = strchr(s, ':');
	int tmpport;
	const char *ports;
	char *tmp;

	if (colon && colon != s) {
		tmp = (char *) malloc(colon - s + 1);
		memcpy(tmp, s, colon - s);
		tmp[colon - s] = 0;
		if (inet_aton(tmp, addr) == 0) {
			fprintf(stderr, "osppeer: bad IP address '%s'\n", tmp);
			return false;
		}
		ports = colon + 1;
		free(tmp);
	} else if (!colon && inet_aton(s, addr) != 0)
		// 's' is a bare IP address
		return true;
	else
		ports = (colon ? colon + 1 : s);

	if (*ports) {
		if ((tmpport = strtol(ports, &tmp, 0)) <= 0
		    || tmpport >= 65535 || *tmp) {
			fprintf(stderr, "osppeer: bad port '%s'\n", ports);
			return false;
		} else
			*port = tmpport;
	}

	return true;
}


tamed void handle_addr(const std::string &alias, const std::string &addrport,
		       peer *cpeer, tamer::fd cfd, tamer::event<int> done)
{
	tvars {
		struct sockaddr_in saddr;
		socklen_t saddrlen;
		struct in_addr xaddr;
		int xport;
		std::string s;
		int ret = 0;
	}

	if (!cfd)
		return;

	// check syntax of request
	if (cpeer->real()) {
		cfd.write("300-You have already registered an address.\n300 To change your address, you must open a new connection.\n", done);
		return;
	} else if (addrport.find(':') == std::string::npos
		   || !parse_addr_port(addrport.c_str(), &xaddr, &xport)) {
		cfd.write("300 Syntax error, expected 'ADDR alias addr:port'.\n", done);
		return;
	} else if (!xaddr.s_addr) {
		cfd.write("300 '0.0.0.0' is an invalid IP address.\n", done);
		return;
	} else if (alias.length() > 2047) {
		cfd.write("300 Alias too long, max length 2047 characters.\n", done);
		return;
	}

	// check client's declared endpoint address vs. its actual address
	saddrlen = sizeof(saddr);
	if (getpeername(cfd.value(), (struct sockaddr *) &saddr, &saddrlen) == -1
	    || saddr.sin_family != AF_INET) {
		// If getpeername fails, just check the provided address.
		fprintf(stderr, "getpeername: %s\n", strerror(errno));
		if ((ntohl(xaddr.s_addr) & 0xFF000000) == 0x7F000000 && !allow_local) {
			cfd.write("300-You have attempted to register a '127.x.x.x' address.\n\
300-These addresses are useful only when a machine is talking to itself.\n\
300 The address registration has been ignored.\n", done);
			return;
		} else if ((ntohl(xaddr.s_addr) & 0xFF000000) == 0x0A000000
			   || (ntohl(xaddr.s_addr) & 0xFFFF0000) == 0xC0A80000) {
			cfd.write("300-You have attempted to register a '10.x.x.x' or '192.168.x.x' address.\n\
300-These addresses are only useful behind firewalls.\n\
300 The address registration has been ignored.\n", done);
			return;
		}
	} else if (saddr.sin_addr.s_addr != xaddr.s_addr) {
		s = inet_ntoa(saddr.sin_addr);
		twait {
			cfd.write("220-The address you provided, " + std::string(inet_ntoa(xaddr)) + ", differs from the address\n\
220-you connected with, " + s + ".  I'll use " + s + ".\n\
220-Probably there is a firewall between the peer and the tracker.\n\
220-As a result, OTHER PEERS WILL LIKELY NOT BE ABLE TO CONNECT TO YOU.\n", make_event(ret));
		}
		xaddr = saddr.sin_addr;
	}
	if (firewalled(xaddr))
		twait {
			cfd.write("200-You appear to be connected from the Linux lab.\n\
200-Only peers located on your machine will be able to connect to your peer.\n\
200-Run /root/fixMachine to set up peers on your machine.\n", make_event(ret));
		}

	cpeer->set_addr(alias, xaddr, xport);

	for (std::vector<peer *>::iterator i = peers.begin();
	     i != peers.end(); ++i) {
		if ((*i)->notify && *i != cpeer)
			notify(*i, cpeer);
		if ((*i)->blocknotify && *i != cpeer) {
			cpeer->use();
			(*i)->notifyq.push_back(cpeer);
			(*i)->notifywake.trigger();
		}
	}

	if (exclusive
	    && saddr.sin_addr.s_addr != htonl(0x83B3508B)
	    && saddr.sin_addr.s_addr != htonl(0x83B350A0)
	    && saddr.sin_addr.s_addr != htonl(0x83B3501F)
	    && saddr.sin_addr.s_addr != htonl(0x7F000001)
	    && !firewalled(saddr.sin_addr)) {
		cpeer->good = false;
		cfd.write("220-This tracker is set up to ignore external peers,\n\
220-so that you don't have to worry about problems caused by other students.\n\
220 The address registration has been ignored.\n", done);
	} else
		cfd.write("200 Address registered.\n", done);
}


void handle_have(const std::string &filename, const std::string &md5sum,
		 peer *cpeer, tamer::fd cfd, tamer::event<int> done) {
	if (md5sum.length() && !check_md5sum(md5sum)) {
		cfd.write("500 Invalid MD5 checksum '" + protect(md5sum) + "' in HAVE command.\n", done);
		return;
	} else if (!cpeer->real()) {
		cfd.write("300-Ignoring your HAVE command until you register your address with ADDR.\n300 Until you register, no one can contact you.\n", done);
		return;
	} else if (cpeer->havefiles.size() >= 1000) {
		cfd.write("300 You've already registered 1000 files.  I won't let you register more.\n", done);
		return;
	}

	havefile &hf = havefiles[filename];
	int x = hf.add_peer(filename, cpeer, md5sum);
	if (x == havefile::add_badsum)
		cfd.write("500-The MD5 checksum you provided for this file does not match the checksum\n500-already registered by another peer.\n500 Ignoring your HAVE command.\n", done);
	else if (x == havefile::add_repeat)
		cfd.write("220 You've already registered this file.\n", done);
	else
		cfd.write("200 File registered.\n", done);
}


tamed void handle_want(const std::string &filename, peer *cpeer,
		       tamer::event<int> done) {
	tvars {
		int n = 0;
		std::map<std::string, havefile>::iterator i = havefiles.find(filename);
	}
	cpeer->use();
	if (i != havefiles.end())
		twait { write_peers(i->second.peers(), cpeer, false, make_event(n)); }
	if (n > 0)
		cpeer->cfd.write("200 Number of peers: " + ntoa(n) + "\n", done);
	else if (n == 0)
		cpeer->cfd.write("300 No peers have that file.\n", done);
	else
		done.trigger(n);
	cpeer->unuse();
}


void handle_md5sum(const std::string &filename, tamer::fd cfd,
		   tamer::event<int> done) {
	std::map<std::string, havefile>::iterator i = havefiles.find(filename);
	if (i != havefiles.end() && i->second.md5sum().length())
		cfd.write(i->second.md5sum() + "\n200 MD5 sum reported.\n", done);
	else
		cfd.write("300 No MD5 sum reported for that file.\n", done);
}


void handle_checkpeer(const std::string &alias, const std::string &addrport,
		      tamer::fd cfd, tamer::event<int> done) {
	struct in_addr xaddr;
	int xport;
	if (addrport.find(':') == std::string::npos
	    || !parse_addr_port(addrport.c_str(), &xaddr, &xport)) {
		cfd.write("300 Syntax error, expected 'CHECKPEER alias addr:port'.\n", done);
		return;
	}
	for (std::vector<peer *>::iterator i = peers.begin();
	     i != peers.end(); i++)
		if ((*i)->calias == alias
		    && (*i)->caddr.s_addr == xaddr.s_addr
		    && (*i)->cport == xport
		    && (*i)->cfd
		    && (*i)->real()) {
			std::string s;
			if ((*i)->havefiles.size()) {
				int n = random() % (*i)->havefiles.size();
				s = "FILE " + protect((*i)->havefiles[n]) + "\n";
			}
			cfd.write(s + "200 Peer is still logged in.\n", done);
			return;
		}
	cfd.write("300 Peer no longer logged in.\n", done);
}


tamed void handle_blocknotify(peer *cpeer, tamer::fd cfd,
			      tamer::event<int> done) {
	tvars { peer *p; }
	cpeer->blocknotify = true;
	while (cpeer->cfd) {
		if (!cpeer->notifyq.size()) {
			twait volatile { cpeer->notifywake = make_event(); }
			continue;
		}
		p = cpeer->notifyq.front();
		cpeer->notifyq.pop_front();
		if (!p->cfd) {
			p->unuse();
			continue;
		} else {
			cpeer->cfd.write("PEER " + protect(p->calias) + " " + std::string(inet_ntoa(p->caddr)) + ":" + ntoa(p->cport) + "\n200 New peer logged in.\n", done);
			p->unuse();
			return;
		}
	}
	done.trigger(-ECANCELED);
}


tamed void handle_connection(tamer::fd cfd) {
	tvars {
		std::string s;
		tamer::rendezvous<> r;
		tamer::buffer b;
		int ret;
		std::vector<std::string> words;
		int n;
		peer *cpeer = new peer(cfd), *pp;
		tamer::event<> tocancel;
		int timeout = SHORT_TIMEOUT;
		std::vector<std::string> strs;
	}

	peers.push_back(cpeer);

	twait { cfd.write("200-Welcome to the OSP2P tracker!\n\
200-You will be timed out after " + ntoa(LONG_TIMEOUT) + " seconds of inactivity.\n\
200 Try 'HELP' to get a list of commands.\n", make_event(ret)); }

	while (ret >= 0) {
		timeout_connection(cfd, timeout, &tocancel);
		twait { b.take_until(cfd, '\n', 4096, s, make_event(ret)); }
		//fprintf(stderr, "%.*s\n", s.length(), s.data());
		tocancel.trigger();
		timeout = LONG_TIMEOUT;
		to_words(s, words);
		if (!words.size() || ret < 0)
			continue;
		if (words.size())
			uppercase(words[0]);

		if (words[0] == "HELP" && words.size() == 1) {
			twait { cfd.write("200-This OSP2P tracker supports the following commands.\n\
200-HELP                  Print this help message.\n\
200-ADDR alias addr:port  Register peer.\n\
200-HAVE file [md5sum]    This peer has 'file' (with optional MD5 checksum).\n\
200-DONTHAVE file         This peer does not have 'file' any more.\n\
200-LIST                  List all registered files.\n\
200-WHO                   List all registered peers.\n\
200-WANT file             List other peers that have 'file'.\n\
200-MD5SUM file           Report the MD5 checksum of 'file'.\n\
200-CHECKPEER alias addr:port  Check whether a peer is still logged in.\n\
200 QUIT                  Logout.\n", make_event(ret)); }
			continue;

		} else if (words[0] == "ADDR" && words.size() == 3) {
			twait { handle_addr(words[1], words[2], cpeer, cfd, make_event(ret)); }
			continue;

		} else if (words[0] == "HAVE" && (words.size() == 2 || words.size() == 3)) {
			twait { handle_have(words[1], (words.size() == 3 ? words[2] : std::string()), cpeer, cfd, make_event(ret)); }
			continue;

		} else if (words[0] == "DONTHAVE" && words.size() == 2) {
			{
				std::map<std::string, havefile>::iterator i = havefiles.find(words[1]);
				if (i != havefiles.end()
				    && cpeer->real()
				    && i->second.remove_peer(words[1], cpeer) == havefile::remove_ok)
					s = "200 File registration removed.\n";
				else
					s = "220 File was not registered, so DONTHAVE does nothing.\n";
			}
			twait { cfd.write("200 File removed.\n", make_event(ret)); }
			continue;

		} else if (words[0] == "WANT" && words.size() == 2) {
			twait { handle_want(words[1], cpeer, make_event(ret)); }
			continue;

		} else if (words[0] == "WHO" && words.size() == 1) {
			twait { write_peers(peers, cpeer, true, make_event(ret)); }
			if (ret >= 0)
				twait { cfd.write("200 Number of peers: " + ntoa(ret) + "\n", make_event(ret)); }
			continue;

		} else if (words[0] == "CHECKPEER" && words.size() == 3) {
			twait { handle_checkpeer(words[1], words[2], cfd, make_event(ret)); }
			continue;

		} else if (words[0] == "LIST" && words.size() == 1) {
			strs.clear();
			for (std::map<std::string, havefile>::iterator i = havefiles.begin();
			     i != havefiles.end(); ++i)
				if (i->second)
					strs.push_back("FILE " + protect(i->first) + "\n");
			for (n = 0; strs.size(); strs.pop_back(), ++n)
				twait { cfd.write(strs.back(), make_event(ret)); }
			twait { cfd.write("200 Number of registered files: " + ntoa(n) + "\n", make_event(ret)); }
			continue;

		} else if (words[0] == "NOTIFY" && words.size() == 1) {
			if (cpeer->real()) {
				cpeer->notify = true;
				twait { cfd.write("200 You will be notified of new peer registrations.\n", make_event(ret)); }
			} else
				twait { cfd.write("300 Ignoring your NOTIFY command since you have not registered with ADDR.\n", make_event(ret)); }
			continue;

		} else if (words[0] == "NONOTIFY" && words.size() == 1) {
			if (cpeer->real()) {
				cpeer->notify = false;
				twait { cfd.write("200 Turned off notifications of new peer registrations.\n", make_event(ret)); }
			} else
				twait { cfd.write("220 Ignoring your NOTIFY command since you have not registered with ADDR.\n", make_event(ret)); }
			continue;

		} else if (words[0] == "BLOCKNOTIFY" && words.size() == 1) {
			twait { handle_blocknotify(cpeer, cfd, make_event(ret)); }
			continue;

		} else if (words[0] == "MD5SUM" && words.size() == 2) {
			twait { handle_md5sum(words[1], cfd, make_event(ret)); }
			continue;

		} else if (words[0] == "QUIT" || words[0] == "Q")
			break;

		twait { cfd.write("500-Syntax error in command.\n500 Try 'HELP' to get a list of commands.\n", make_event(ret)); }
	}

	twait { cfd.write("200 Goodbye!\n", make_event(ret)); }

	if (cpeer) {
		std::vector<peer *>::iterator i = std::find(peers.begin(), peers.end(), cpeer);
		assert(i != peers.end());
		peers.erase(i);

		while (cpeer->havefiles.size()) {
			havefile &h = havefiles[cpeer->havefiles[0]];
			h.remove_peer(cpeer->havefiles[0], cpeer);
		}
		cpeer->unnotify();
		cpeer->unuse();
	}

	cfd.close();
}

static bool peer_xaddr_compare(const peer *a, const peer *b) {
	return a->xaddr.s_addr < b->xaddr.s_addr;
}

static std::vector<int> xaddr_counts;

static bool peer_xaddr_count_compare(const peer *a, const peer *b) {
 	return xaddr_counts[a->xaddr_num] < xaddr_counts[b->xaddr_num]
		|| (xaddr_counts[a->xaddr_num] == xaddr_counts[b->xaddr_num]
		    && a->xaddr.s_addr < b->xaddr.s_addr);
}

tamed void accept_loop(int port) {
	tvars { tamer::fd lf, cf; int ret; }

	// Create the socket, exit on error
	twait { tamer::tcp_listen(port, make_event(lf)); }
	if (!lf)
		die("listen");
	if (fcntl(lf.value(), F_SETFD, FD_CLOEXEC) < 0)
		die("fcntl");

	while (1) {
		twait { lf.accept(0, 0, make_event(cf)); }
		if (cf)
			handle_connection(cf);
		else if (cf.error() == -ENFILE || cf.error() == -EMFILE) {
			// Too many connections.  Kill some victims.
			std::vector<peer *> ordered_peers(peers);
			std::sort(ordered_peers.begin(), ordered_peers.end(), peer_xaddr_compare);
			xaddr_counts.clear();
			for (std::vector<peer *>::iterator i = ordered_peers.begin(); i < ordered_peers.end(); ++i) {
				if (i == ordered_peers.begin() || (*i)->xaddr.s_addr != i[-1]->xaddr.s_addr)
					xaddr_counts.push_back(0);
				(*i)->xaddr_num = xaddr_counts.size();
				++xaddr_counts.back();
			}
			std::sort(ordered_peers.begin(), ordered_peers.end(), peer_xaddr_count_compare);
			for (std::vector<peer *>::size_type i = ordered_peers.size();
			     i >= ordered_peers.size() - 20 && i > 0; --i)
				kill_connection(ordered_peers[i - 1]->cfd, "500 Killing peer because of out-of-control registrations from its address.\n");
		}
	}
}

int main(int argc, char *argv[]) {
	int port = 11111;
	char *s;

	tamer::initialize();
	srandom(time(NULL) + getpid() * getppid());

	// Lots and lots of open files
	struct rlimit rlim;
	getrlimit(RLIMIT_NOFILE, &rlim);
	rlim.rlim_cur = 5000;
	setrlimit(RLIMIT_NOFILE, &rlim);

	// Ignore broken-pipe signals: if a connection dies, server should not
	signal(SIGPIPE, SIG_IGN);

    argprocess:
	if (argc >= 3 && strcmp(argv[1], "-p") == 0
	    && (port = strtol(argv[2], &s, 0)) > 0 && port < 65536
	    && *s == 0) {
		argc -= 2;
		argv += 2;
		goto argprocess;
	} else if (argc >= 2 && argv[1][0] == '-' && argv[1][1] == 'p'
		   && (port = strtol(argv[1]+2, &s, 0)) > 0 && port < 65536
		   && *s == 0) {
		--argc, ++argv;
		goto argprocess;
	} else if (argc >= 2 && strcmp(argv[1], "-l") == 0) {
		allow_local = 1;
		--argc, ++argv;
		goto argprocess;
	} else if (argc >= 2 && strcmp(argv[1], "-x") == 0) {
		exclusive = 1;
		--argc, ++argv;
		goto argprocess;
	} else if (argc >= 2 && strcmp(argv[1], "-f") == 0) {
		fakers = 1;
		--argc, ++argv;
		goto argprocess;
	} else if (argc > 1) {
		fprintf(stderr, "Usage: osptracker [-pPORT] [-l] [-x] [-f]\n");
		exit(1);
	}

	accept_loop(port);
	tamer::loop();
}



// die(format, ...)
//	Print an error message and exit.
//	Acts like perror() if 'format' doesn't end with "\n".
static void
die(const char *format, ...)
{
	const char *errstr;
	va_list val;
	va_start(val, format);
	errstr = strerror(errno);
	vfprintf(stderr, format, val);
	if (strchr(format, '\n') == NULL)
		fprintf(stderr, ": %s\n", errstr);
	exit(1);
}
