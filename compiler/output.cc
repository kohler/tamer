#include "tame.hh"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

unsigned count_newlines(const str &s)
{
    unsigned n = 0;
    for (str::size_type p = 0; (p = s.find('\n', p)) != str::npos; p++)
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

void outputter_t::fail_exit() {
    if (_fd >= 0 && _outfn.length() && _outfn != "-")
        unlink(_outfn.c_str());
    exit(1);
}

void
outputter_t::start_output ()
{
  _lineno = 1;

  // switching from NONE to PASSTHROUGH
  switch_to_mode (OUTPUT_PASSTHROUGH);
}

void
outputter_t::output_line_number()
{
    if (_output_xlate && (_cur_lineno != _lineno || _cur_file != _infn)) {
	strbuf b;
	if (!_last_char_was_nl)
	    b << "\n";
	b << "# " << _lineno << " \"" << _infn << "\"\n";
	_output_str(b.str(), "");
	_cur_lineno = _lineno;
	_cur_file = _infn;
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
	outputter_t::output_str(s);
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
	if (s == "\n")
	    _lineno--;
	if (s.length() && _do_output_line_number) {
	    output_line_number();
	    _do_output_line_number = false;
	}
	_output_str(s, "");
	if (_mode == OUTPUT_PASSTHROUGH)
	    _lineno += count_newlines(s);
    }
}

void
outputter_t::_output_str(const str &s, const str &sep_str)
{
    if (!s.length())
	return;

    _last_output_in_mode = _mode;

    _buf << s;
    if (sep_str.length()) {
	_buf << sep_str;
	_last_char_was_nl = (sep_str[sep_str.length() - 1] == '\n');
    } else
	_last_char_was_nl = (s.length() && s[s.length() - 1] == '\n');

    for (str::size_type p = 0; (p = s.find('\n', p)) != str::npos; p++)
	_cur_lineno++;
    for (str::size_type p = 0; (p = sep_str.find('\n', p)) != str::npos; p++)
	_cur_lineno++;
}

void
outputter_t::flush()
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
outputter_t::switch_to_mode(output_mode_t m, int nln)
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
