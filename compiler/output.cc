#include "tame.hh"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

unsigned count_newlines (const str &s)
{
    unsigned n = 0;
    for (str::const_iterator i = s.begin(); i != s.end(); i++)
	if (*i == '\n')
	    n++;
    return n;
}

bool
outputter_t::init ()
{
    if (_outfn.length() && _outfn != "-") {
	if ((_fd = open (_outfn.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0644)) < 0) {
	    warn << "cannot open file for writing: " << _outfn << "\n";
	    return false;
	}
    } else {
	_fd = 1;
    }
    return true;
}

void
outputter_t::start_output ()
{
  _lineno = 1;

  // switching from NONE to PASSTHROUGH
  switch_to_mode (OUTPUT_PASSTHROUGH);
}

void
outputter_t::output_line_number ()
{
  if (_output_xlate &&
      (_did_output || _last_lineno != _lineno)) {
    strbuf b;
    if (!_last_char_was_nl)
      b << "\n";
    b << "# " << _lineno << " \"" << _infn << "\"\n";
    _output_str (b.str(), "");
    _last_lineno = _lineno;
    _did_output = false;
  }
}

void
outputter_H_t::output_str(const str &s)
{
    if (_mode == OUTPUT_TREADMILL) {
	output_line_number();
	str::size_type p = 0;
	do {
	    str::size_type q = s.find('\n', p);
	    if (q == str::npos)
		q = s.length();
	    _output_str(s.substr(p, q - p), (q >= s.length() - 1 ? "\n" : " "));
	    p = q + 1;
	} while (p < s.length());
    } else
	outputter_t::output_str (s);
}

void
outputter_t::output_str(const str &s)
{
    if (_mode == OUTPUT_TREADMILL) {
	output_line_number();
	str::size_type p = 0;
	do {
	    str::size_type q = s.find('\n', p);
	    if (q == str::npos)
		q = s.length();
	    output_line_number();
	    _output_str(s.substr(p, q - p), "\n");
	    p = q + 1;
	} while (p < s.length());
    } else {
	// we might have set up a defered output_line_number from
	// within switch_to_mode; now is the time to do it.
	if (s.length() && _do_output_line_number) {
	    output_line_number ();
	    _do_output_line_number = false;
	}

	_output_str (s, "");
	if (_mode == OUTPUT_PASSTHROUGH)
	    _lineno += count_newlines (s);
    }
}

void
outputter_t::_output_str(const str &s, const str &sep_str)
{
  if (!s.length())
    return;

  _last_output_in_mode = _mode;
  _did_output = true;

  _buf << s;
  if (sep_str.length()) {
    _buf << sep_str;
    _last_char_was_nl = (sep_str[sep_str.length() - 1] == '\n');
  } else {
    if (s.length() && s[s.length() - 1] == '\n') {
      _last_char_was_nl = true;
    } else {
      _last_char_was_nl = false;
    }
  }
}

void
outputter_t::flush ()
{
    str bufstr = _buf.str();
    const char *x = bufstr.data(), *end = x + bufstr.length();
    while (x < end) {
	size_t len = (end - x > 32768 ? 32768 : end - x);
	ssize_t w = write(_fd, x, len);
	if (w < 0 && errno != EAGAIN && errno != EINTR) {
	    warn << strerror(errno) << "\n";
	    return;
	} else if (w > 0)
	    x += w;
    }
}

outputter_t::~outputter_t ()
{
  if (_fd >= 0) {
    flush ();
    close (_fd); 
  }
}

output_mode_t 
outputter_t::switch_to_mode (output_mode_t m, int nln)
{
  output_mode_t old = _mode;
  int oln = _lineno;

  if (nln >= 0)
    _lineno = nln;
  else
    nln = oln;

  if (m == OUTPUT_PASSTHROUGH &&
      (oln != nln ||
       (old != OUTPUT_NONE && 
	_last_output_in_mode != OUTPUT_PASSTHROUGH))) {

    // don't call output_line_number() directly, since
    // maybe this will be an empty environment
    _do_output_line_number = true;
  }
  _mode = m;
  return old;
}
