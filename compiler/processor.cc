#include "tame.hh"
#include <ctype.h>

var_t::var_t(const type_qualifier_t &t, declarator_t *d, const lstr &arrays, vartyp_t a)
    : _name(d->name()), _type(t.to_str(), d->pointer()), _asc(a),
      _initializer(d->initializer()), _arrays(arrays.str())
{
}

str ws_strip (str s)
{
    str::iterator f = s.begin(), b = s.end();
    while (f < b && isspace((unsigned char) *f))
	f++;
    while (f < b && isspace((unsigned char) b[-1]))
	b--;
    return str(f, b);
}

str template_args(str s)
{
    str::size_type a = s.find('<');
    str::size_type b = s.rfind('>');
    if (a != str::npos && b != str::npos && a < b)
	return s.substr(a, b - a + 1);
    else
	return str();
}

void element_list_t::output(outputter_t *o) {
    for (std::list<tame_el_t *>::iterator i = _lst.begin(); i != _lst.end(); i++)
	(*i)->output(o);
}

#define TAME_CLOSURE_NAME     "tamer_closure_"

#define TWAIT_BLOCK_RENDEZVOUS "tamer_gather_rendezvous_"

const var_t *
vartab_t::lookup(const str &n) const
{
    std::map<str, unsigned>::const_iterator i = _tab.find(n);
    if (i == _tab.end())
	return 0;
    else
	return &_vars[i->second];
}

str type_t::to_str() const
{
    strbuf b;
    b << _base_type << " ";
    if (_pointer.length())
	b << _pointer;
    return b.str();
}

str
type_t::to_str_w_template_args(bool p) const
{
  strbuf b;
  b << _base_type;
  if (_template_args.length())
    b << _template_args;
  b << " ";

  if (p && _pointer.length())
    b << _pointer;

  return b.str();
}

str var_t::decl() const
{
    strbuf b;
    b << _type.to_str_w_template_args();
    if (_arrays.length() && _arrays[0] == '[') {
	b << "(*" << _name << ")";
	str::size_type rbrace = _arrays.find(']');
	if (rbrace != str::npos)
	    b << _arrays.substr(rbrace + 1);
    } else
	b << _name;
    if (_initializer)
	b << _initializer->output_in_declaration ();
    return b.str();
}

str var_t::param_decl(bool move, bool escape) const
{
    strbuf b;
    if (move && _type.pointer().empty() && _arrays.empty() && !_name.empty())
	b << "TAMER_MOVEARG(" << _type.to_str() << ") ";
    else
	b << _type.to_str();
    if (escape && !_name.empty())
        b << "tamer__";
    b << _name << _arrays;
    return b.str();
}

str var_t::name(bool move, bool escape) const
{
    if (move && _type.pointer().empty() && _arrays.empty()) {
	strbuf b;
	b << "TAMER_MOVE(";
        if (escape)
            b << "tamer__";
        b << _name << ")";
	return b.str();
    } else if (escape) {
        strbuf b;
        b << "tamer__" << _name;
        return b.str();
    } else
	return _name;
}

str var_t::decl(const str &p, int n) const
{
    strbuf b;
    b << _type.to_str() << p << n;
    return b.str();
}

str var_t::decl(const str &p) const
{
    strbuf b;
    b << _type.to_str() << p << _name;
    return b.str();
}

str var_t::ref_decl() const
{
    strbuf b;
    str refit;
    if (_type.is_ref())
	refit = "";
    else if (_initializer)
	refit = _initializer->ref_prefix();
    else
	refit = "&";
    b << _type.to_str();
    if (_arrays.length() && _arrays[0] == '[') {
	b << "(*" << refit << _name << ")";
	str::size_type rbrace = _arrays.find(']');
	if (rbrace != str::npos)
	    b << _arrays.substr(rbrace + 1);
    } else
	b << refit << _name;
    return b.str();
}

str
initializer_t::ref_prefix() const
{
  return "&";
}

str
array_initializer_t::ref_prefix() const
{
  /*
   * useful for when we can handle 2-dim++ arrays, but not really
   * worthwhile for now.
   *
  strbuf b;
  const char *cp = _value.cstr ();
  while (*cp && (cp = strchr (cp, '['))) {
    b << "*";
    cp ++;
  }
  return b.str();
  */
  return "*";
}

str mangle(const str &in)
{
    strbuf b;
    for (str::const_iterator i = in.begin(); i != in.end(); i++)
	if (!isspace((unsigned char) *i))
	    b << (*i == ':' || *i == '<' || *i == '>' || *i == ',' ? '_' : (char) *i);
    return b.str();
}

str
strip_to_method (const str &in)
{
    str::size_type p = in.rfind("::");
    if (p == str::npos)
	return in;
    else
	return in.substr(p + 2);
}

str
strip_off_method (const str &in)
{
    str::size_type p = in.rfind("::");
    if (p == str::npos)
	return str();
    else
	return in.substr(0, p);
}


bool
vartab_t::add(const var_t &v)
{
    if (exists(v.name()))
	return false;
    _vars.push_back(v);
    if (!v.name().empty())
        _tab[v.name()] = _vars.size() - 1;
    return true;
}

void
declarator_t::dump () const
{
  warn << "declarator dump:\n";
  if (_name.length())
    warn << "  name: " << _name << "\n";
  if (_pointer.length())
    warn << "  pntr: " << _pointer << "\n";
  if (_params)
    warn << "  param list size: " << _params->size () << "\n";
}

void
element_list_t::passthrough(const lstr &s)
{
    if (_lst.empty() || !_lst.back()->append(s))
	_lst.push_back(new tame_passthrough_t(s));

}

void
parse_state_t::new_block (tame_block_t *g)
{
  _block = g;
  push (g);
}

void
parse_state_t::new_fork (tame_fork_t *j)
{
  _fork = j;
  push (j);
}

void
parse_state_t::new_nonblock (tame_nonblock_t *b)
{
  _nonblock = b;
  push (b);
}

tame_fn_t::tame_fn_t(const fn_specifier_t &fn, const str &r, declarator_t *d,
                     bool c, unsigned l, str loc)
    : _ret_type(ws_strip(r), ws_strip(d->pointer())),
      _name(d->name()),
      _method_name(strip_to_method(_name)),
      _class(strip_off_method(_name)),
      _self(c ? str("const ") + _class : _class, "*", "__tamer_self"),
      _isconst(c),
      _declaration_only(false),
      _args(d->params()),
      _any_volatile_envs(false),
      _opts(fn._opts),
      _lineno(l),
      _n_labels(1),		// 0 = begin, 1 = exit prematurely
      _n_blocks(0),
      _loc(loc),
      _lbrace_lineno(0),
      _vars(NULL),
      _after_vars_el_encountered(false) {
    the_closure_[0] = the_closure_[1] = 0;
    if ((_class.empty() || template_args(_class).empty())
        && !fn.template_[0].empty())
        function_template_ = fn.template_[0];
    else {
        class_template_ = fn.template_[0];
        function_template_ = fn.template_[1];
    }
}

void
tame_fn_t::add_env (tame_env_t *e)
{
    _envs.push_back(e);
    if (e->is_jumpto())
	e->set_id(++_n_labels);
    if (e->is_volatile())
	_any_volatile_envs = true;
}

void tame_fn_t::add_templates(strbuf& b, const char* sep) const {
    if (!class_template_.empty())
        b << "template< " << class_template_ << " >";
    if (!function_template_.empty())
        b << "template< " << function_template_ << " >";
    b << sep;
}

cpp_initializer_t::cpp_initializer_t(const lstr &v, bool braces)
    : initializer_t(v), braces_(braces)
{
    // rewrite "this" to "__tamer_self".  Do it the right way.
    strbuf b;
    int mode = 0;
    std::string::iterator last = _value.begin();
    for (std::string::iterator a = last; a + 4 <= _value.end(); a++)
	if (*a == '\\')
	    a++;
	else if (mode == 0 && (*a == '\"' || *a == '\''))
	    mode = *a;
	else if (mode == *a && (*a == '\"' || *a == '\''))
	    mode = 0;
	else if (mode == 0 && *a == '/' && a[1] == '/')
	    mode = '/';
	else if (mode == 0 && *a == '/' && a[1] == '*')
	    mode = '*';
	else if (mode == '/' && *a == '\n')
	    mode = 0;
	else if (mode == '*' && *a == '*' && a[1] == '/')
	    mode = 0, a++;
	else if (mode == 0 && *a == 't' && a[1] == 'h' && a[2] == 'i' && a[3] == 's' && (a + 4 == _value.end() || (!isalnum(a[4]) && a[4] != '_' && a[4] != '$'))) {
	    b << std::string(last, a) << "__tamer_self";
	    last = a + 4;
	    a += 3;
	}
    if (last != _value.begin()) {
	b << std::string(last, _value.end());
	_value = lstr(_value.lineno(), b);
    }
}

str
cpp_initializer_t::output_in_constructor() const
{
  strbuf b;
  b << (braces_ ? '{' : '(') << _value.str() << (braces_ ? '}' : ')');
  return b.str();
}

str
array_initializer_t::output_in_declaration() const
{
  strbuf b;
  b << "[" << _value.str() << "]";
  return b.str();
}


//-----------------------------------------------------------------------
// Output utility routines
//

var_t tame_fn_t::mk_closure(bool object, bool ref) const {
    strbuf b;
    b << "closure__" << _method_name;
    if (_args) {
	b << "__";
	_args->mangle(b);
    }
    if (object && !function_template_.empty())
        add_template_function_args(b);
    return var_t(b.str(), (ref ? "&" : "*"), TAME_CLOSURE_NAME, NONE);
}

void vartab_t::mangle(strbuf &b) const
{
    for (unsigned i = 0; i < size(); i++)
	b << _vars[i].type().mangle();
}

class mangler { public:

    mangler(const str &s);

    void make_base(const str & = str());
    void do_base(const str &s);
    void go(const str &s);
    static str cv(int);

    str s();

    int _cvflag;
    int _lflag;
    str _base;
    str _ptr;
    strbuf _result;
};

str mangler::cv(int cvflag)
{
    if (!cvflag)
	return str();
    if (cvflag == 1)
	return "K";
    else if (cvflag == 2)
	return "V";
    str s;
    if (cvflag & 4)
	s += "r";
    if (cvflag & 2)
	s += "V";
    if (cvflag & 1)
	s += "K";
    if (cvflag & 8)
	s += "N";
    return s;
}

void mangler::make_base(const str &nnn)
{
    if (!_base.length()) {
	if (((_lflag & 4) && nnn != "i" && nnn != "d")
	    || ((_lflag & 3) && nnn != "c" && nnn != "s" && nnn != "i"))
	    _base = "i";
    }
    if (!_base.length() && !nnn.length())
	_base = "vv";
    if (_base.length()) {
	if ((_lflag & 8) && _base == "i")
	    _base = "x";
	else if ((_lflag & 4) && _base == "i")
	    _base = "l";
	else if ((_lflag & 4) && _base == "d")
	    _base = "e";
	if ((_lflag & 1) && _base == "c")
	    _base = "a";
	else if ((_lflag & 2) && _base == "c")
	    _base = "h";
	else if ((_lflag & 2) && (_base == "s" || _base == "i" || _base == "l" || _base == "x"))
	    _base[0]++;
	if (_cvflag & 8)
	    _base += "E";
	_base = _ptr + cv(_cvflag) + _base;
	_cvflag = 0;
	_lflag = 0;
	_ptr = "";
    }
}

void mangler::do_base(const str &new_base)
{
    make_base(new_base);
    if (_base.length())
	_result << _base;
    _base = new_base;
}

str mangler::s()
{
    if (_ptr.length() || _cvflag || _lflag)
	make_base("");
    _result << _base;
    return _result.str();
}

static str ntoa(int x)
{
    strbuf b;
    b << x;
    return b.str();
}

mangler::mangler(const str &s)
    : _cvflag(0), _lflag(0)
{
    // It is too hard to mangle correctly given this representation.
    // Just give it a go.

    for (str::const_iterator i = s.begin(); i != s.end(); )
	if (isalpha((unsigned char) *i) || *i == '_') {
	    str::const_iterator j = i;
	    for (i++; i != s.end(); i++)
		if (!isalnum((unsigned char) *i) && *i != '_')
		    break;
	    str s(j, i);
	    if (s == "restrict")
		_cvflag |= 4;
	    else if (s == "volatile")
		_cvflag |= 2;
	    else if (s == "const")
		_cvflag |= 1;
	    else if (s == "void")
		do_base("v");
	    else if (s == "wchar_t")
		do_base("w");
	    else if (s == "size_t")
		do_base("k");
	    else if (s == "bool")
		do_base("b");
	    else if (s == "char")
		do_base("c");
	    else if (s == "signed")
		_lflag |= 1;
	    else if (s == "unsigned")
		_lflag |= 2;
	    else if (s == "int")
		do_base("i");
	    else if (s == "long")
		_lflag |= (_lflag & 4 ? 8 : 4);
	    else if (s == "float")
		do_base("f");
	    else if (s == "double")
		do_base("d");
	    else if (s == "struct" || s == "union" || s == "class" || s == "enum")
		/* do nothing */;
	    else
		do_base(ntoa(s.length()) + s);
	} else if (*i == ':' && i+1 != s.end() && i[1] == ':') {
	    if (_base == "3std")
		_base = "St";
	    else if (_base == "5tamer")
		_base = "Sx";
	    for (i += 2; i != s.end() && isspace((unsigned char) *i); i++)
		/* */;
	    str::const_iterator j = i;
	    for (; i != s.end() && (isalnum((unsigned char) *i) || *i == '_'); i++)
		/* */;
	    if (j != i)
		_base += ntoa(i - j) + str(j, i);
	    if (_base == "St9allocator")
		_base = "Sa";
	    else if (_base == "St12basic_string")
		_base = "Sb";
	    else if (_base == "St6string")
		_base = "Ss";
	    else if (_base == "Sx5event")
		_base = "Q";
	    else if (_base == "Sx2fd")
		_base = "Sf";
	    else
		_cvflag |= 8;
	} else if (*i == '*') {
	    if (_lflag || _cvflag)
		make_base("");
	    _ptr = "P" + _ptr;
	    i++;
	} else if (*i == '&') {
	    if (_lflag || _cvflag)
		make_base("");
	    _ptr = "R" + _ptr;
	    i++;
	} else if (isdigit((unsigned char) *i)) {
	    str::const_iterator j = i;
	    for (i++; i != s.end() && isdigit((unsigned char) *i); i++)
		/* */;
	    do_base(str(j, i));
	} else if (*i == '[') {
	    str::const_iterator j = i+1;
	    for (i++; i != s.end() && *i != ']'; i++)
		/* */;
	    if (_lflag || _cvflag)
		make_base("");
	    if (i != j)
		_ptr = "P" + _ptr;
	    else
		_ptr = "A" + mangler(str(j, i)).s() + "_";
	    if (i != s.end())
		i++;
	} else if (*i == '<') {
	    str::const_iterator j = i+1;
	    int inner = 0;
	    strbuf b;
	    for (++i; i != s.end() && (*i != '>' || inner); ++i)
		if (*i == '<')
		    inner++;
		else if (*i == '>')
		    inner--;
		else if (*i == ',' && !inner) {
		    b << mangler(str(j, i)).s();
		    j = i+1;
		}
	    b << mangler(str(j, i)).s();
	    if (i != s.end())
		i++;
	    if (_base == "5event") {
		_base = "Q";
		_cvflag &= ~8;
	    }
	    str targs = b.str();
	    if (_base == "Q")
		_base = _base + targs + "_";
	    else
		_base = _base + "I" + targs + "E";
	} else if (*i == '.' && i+2 < s.end() && i[1] == '.' && i[2] == '.') {
	    do_base("z");
	    i += 2;
	} else
	    i++;
}

str type_t::mangle() const
{
    // It is too hard to mangle correctly given this representation.
    // Just give it a go.
    return mangler(to_str_w_template_args()).s();
}

void
vartab_t::declarations(strbuf &b, const str &padding) const
{
    for (unsigned i = 0; i < size(); ++i)
        if (!_vars[i].name().empty())
            b << padding << _vars[i].decl() << ";\n";
}

void
vartab_t::initialize(strbuf &b, bool self, outputter_t* o) const
{
  initializer_t *init = 0;
  unsigned lineno;
  for (unsigned i = 0; i < size (); i++) {
      if (!_vars[i].name().empty()
          && (self
              || ((init = _vars[i].initializer())
                  && init->do_constructor_output()))) {
        if (!self && (lineno = init->constructor_lineno())) {
            b << ",\n";
            o->line_number_line(b, lineno);
        } else
            b << ", ";
        b << _vars[i].name();
        if (self)
            b << "(" << _vars[i].name(true, true) << ")";
        else
            b << init->output_in_constructor();
    }
  }
}

void
vartab_t::paramlist(strbuf &b, paramlist_flags list_mode, const char* sep) const
{
    for (unsigned i = 0; i < size () ; i++) {
        if (list_mode != pl_declarations && _vars[i].name().empty())
            continue;
        b << sep;
        sep = ", ";
        switch (list_mode) {
        case pl_declarations:
            b << _vars[i].param_decl(false, false);
            if (_vars[i].initializer())
                b << " = " << _vars[i].initializer()->output_in_constructor();
            break;
        case pl_declarations_named:
            b << _vars[i].param_decl(true, true);
            break;
        case pl_moves_named:
            b << _vars[i].name(true, false);
            break;
        default:
            assert(false);
            break;
        }
    }
}

str
tame_fn_t::label (str s) const
{
    strbuf b;
    b << "__closure_label_" << s;
    return b.str();
}

str
tame_fn_t::label (unsigned id) const
{
    strbuf b;
    b << id;
    return label(b.str());
}

bool
element_list_t::need_implicit_rendezvous() const
{
    for (std::list<tame_el_t *>::const_iterator i = _lst.begin(); i != _lst.end(); i++)
	if ((*i)->need_implicit_rendezvous())
	    return true;
    return false;
}

//
//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Output Routines

void
tame_passthrough_t::output(outputter_t *o)
{
    if (buf_.tellp() != 0) {
	output_mode_t old = o->switch_to_mode (OUTPUT_PASSTHROUGH, lineno_);
	o->output_str(buf_.str());
	o->switch_to_mode(old);
    }
}

void tame_fn_t::add_template_function_args(strbuf& b) const {
    if (!function_template_.empty()) {
        b << '<';
        const char* s = function_template_.data();
        const char* e = s + function_template_.length();
        int depth = 0, mode = 0;
        for (; s != e; ++s) {
            if (isspace((unsigned char) *s) && depth == 0 && mode == 1)
                mode = 2;
            else if (isspace((unsigned char) *s))
                /* skip */;
            else if (*s == ',' && depth == 0 && mode == 2) {
                b << *s;
                mode = 0;
            } else if (depth == 0 && mode == 2)
                b << *s;
            else if (*s == '<')
                ++depth;
            else if (*s == '>')
                --depth;
            else if (mode == 0)
                mode = 1;
        }
        b << '>';
    }
}

str
tame_fn_t::closure_type_name(bool include_template) const
{
    if (!_class.length() && (function_template_.empty() || !include_template))
        return closure(false).type().base_type();
    strbuf b;
    if (_class.length())
        b << _class << "::";
    b << closure(false).type().base_type();
    if (!function_template_.empty() && include_template)
        add_template_function_args(b);
    return b.str();
}

void
tame_fn_t::output_reenter (strbuf &b)
{
    b << "  static void tamer_activator_(tamer::tamerpriv::tamer_closure *super) {\n"
      << "    " << closure_type_name(true) << " *self = static_cast<"
      << closure_type_name(true) << " *>(super);\n"
      << "    ";
    if (_class.length() && !(_opts & STATIC_DECL))
	b << "self->" << _self.name() << "->" << _method_name;
    else
	b << _name;
    b << "(*self);\n"
      << "  }\n";
}

void
tame_fn_t::output_closure(outputter_t *o)
{
  strbuf b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

  const char *base_type = "tamer_closure";

  if (class_template_.length() || function_template_.length())
      add_templates(b, "\n");
  b << "class " << closure_type_name(false) << " : public tamer::tamerpriv::"
    << base_type
    << " {\n"
    << "public:\n"
    << "  " << closure(false).type().base_type()
    << "(";

  const char* sep = "";
  if (need_self()) {
      b << _self.decl();
      sep = ", ";
  }

  if (_args)
      _args->paramlist(b, vartab_t::pl_declarations_named, sep);

  b << ") : " << base_type << "(tamer_activator_)";

  if (need_self()) {
      str s = _self.name();
      b << ", " << s << " (" << s << ")";
  }

  // output arguments declaration
  if (_args && _args->size())
      _args->initialize(b, true, o);

  // output stack declaration
  if (_stack_vars.size())
      _stack_vars.initialize(b, false, o);

  if (need_implicit_rendezvous())
      b << ", " << TWAIT_BLOCK_RENDEZVOUS "(this) ";

  b << " {}\n\n";

  output_reenter(b);

  if (_class.length() && !(_opts & STATIC_DECL))
      b << "  " << _self.decl() << ";\n";

  if (_args)
      _args->declarations(b, "    ");
  _stack_vars.declarations(b, "    ");

  if (need_implicit_rendezvous())
      b << "  tamer::gather_rendezvous " TWAIT_BLOCK_RENDEZVOUS ";\n";

  b << "};\n\n";

  o->output_str(b.str());
  o->switch_to_mode(om);
}

void
tame_fn_t::output_stack_vars(strbuf &b)
{
    for (unsigned i = 0; i < _stack_vars.size (); i++) {
	const var_t &v = _stack_vars._vars[i];
	b << "  " << v.ref_decl() << " TAMER_CLOSUREVARATTR = "
	  << closure(false).name() << "." << v.name () << ";\n" ;
    }
}

void
tame_fn_t::output_arg_references(strbuf &b)
{
    for (unsigned i = 0; _args && i != _args->size(); ++i) {
	const var_t &v = _args->_vars[i];
        if (!v.name().empty())
            b << "  " << v.ref_decl() << " TAMER_CLOSUREVARATTR = "
              << closure(false).name() << "." << v.name() << ";\n";
    }
}

void
tame_fn_t::output_jump_tab (strbuf &b)
{
    b << "  tamer::tamerpriv::closure_owner<" << closure_type_name(true)
      << " > tamer_closure_holder_(" << TAME_CLOSURE_NAME << ");\n"
      << "  switch (" << TAME_CLOSURE_NAME << ".tamer_block_position_) {\n"
      << "  case 0: break;\n";
  for (unsigned i = 0; i < _envs.size (); i++) {
    if (_envs[i]->is_jumpto ()) {
      int id_tmp = _envs[i]->id ();
      assert (id_tmp);
      b << "  case " << id_tmp << ":\n"
	<< "    goto " << label (id_tmp) << ";\n"
	<< "    break;\n";
    }
  }
  b << "  default: return; }\n";
}

str
tame_fn_t::signature() const
{
    strbuf b;
    if (class_template_.length() || function_template_.length())
        add_templates(b, " ");
    if ((_opts & STATIC_DECL) && !_class.length())
	b << "static ";
    if (_opts & VIRTUAL_DECL)
	b << "virtual ";
    if (_opts & INLINE_DECL)
	b << "inline ";
    b << _ret_type.to_str() << " " << _name << "(";
    if (_args)
	_args->paramlist(b, vartab_t::pl_declarations, "");
    b << ")";
    if (_isconst)
	b << " const";
    return b.str();
}

str
tame_fn_t::closure_signature() const
{
    strbuf b;
    if (class_template_.length() || function_template_.length())
        add_templates(b, " ");
    if ((_opts & STATIC_DECL) && !_class.length())
	b << "static ";
    b << _ret_type.to_str() << " " << _name << "("
      << mk_closure(true, true).decl() << ")";
    if (_isconst)
	b << " const";
    return b.str();
}

void
tame_fn_t::output_firstfn(outputter_t *o)
{
    strbuf b;
    state->set_fn(this);

    output_mode_t om = o->switch_to_mode(OUTPUT_PASSTHROUGH);
    b << signature() << "\n{\n";

    // If no vars section was specified, do it now.
    b << "  " << closure(true).decl() << " = new "
      << closure(true).type().base_type() << "(";
    const char* sep = "";
    if (_class.length() && !(_opts & STATIC_DECL)) {
        b << "this";
        sep = ", ";
    }
    if (_args)
	_args->paramlist(b, vartab_t::pl_moves_named, sep);
    b << ");\n"
      << "  " << TAME_CLOSURE_NAME << "->tamer_activator_("
      << TAME_CLOSURE_NAME << ");\n}\n";

    o->output_str(b.str());
    o->switch_to_mode(om);
}

void
tame_fn_t::output_fn(outputter_t *o)
{
    strbuf b;
    state->set_fn (this);

    output_mode_t om = o->switch_to_mode(OUTPUT_PASSTHROUGH);
    b << closure_signature() << "\n{\n";

    o->output_str(b.str());

    // If no vars section was specified, do it now.
    if (!_vars)
	output_vars(o, _lbrace_lineno);

    element_list_t::output(o);

    o->switch_to_mode (om);
}

void
tame_vars_t::output (outputter_t *o)
{
  _fn->output_vars (o, _lineno);
}

void
tame_fn_t::output_vars (outputter_t *o, int ln)
{
  strbuf b;

  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL, ln);

  output_stack_vars (b);
  b << "\n";
  output_arg_references (b);
  b << "\n";

  output_jump_tab (b);
  o->output_str(b.str());

  // will switch modes as appropriate (internally)
  o->switch_to_mode (om);
}

void
tame_fn_t::output(outputter_t *o)
{
    strbuf b;
    if (!_class.length() || _declaration_only) {
        if (!function_template_.empty())
            b << "template < " << function_template_ << " > ";
	b << "class " << closure(false).type().base_type() << ";";
    }
    if (_declaration_only)
	b << " " << signature() << ";";
    if (!_class.length())
        b << " " << closure_signature() << ";";
    str bstr = b.str();
    if (bstr.length())
	o->output_str(bstr + "\n");
    if (!_declaration_only) {
	output_closure(o);
	output_firstfn(o);
	output_fn(o);
    }
}

static bool description_empty(const str& x) {
    for (str::const_iterator it = x.begin(); it != x.end(); ++it)
        if (!isspace(*it))
            return false;
    return true;
}

void
tame_block_ev_t::output(outputter_t *o)
{
  strbuf b;
  str tmp;

  b << "/*twait{*/ do { ";
  if (_fn->any_volatile_envs())
      b << TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ".set_volatile(" << _isvolatile << "); ";
  b << "do {\n";
  if (tamer_debug)
      b << "#define make_event(...) make_annotated_event(__FILE__, __LINE__, " TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ", ## __VA_ARGS__)\n";
  else
      b << "#define make_event(...) make_event(" TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ", ## __VA_ARGS__)\n";
  b << "    tamer::tamerpriv::rendezvous_owner<tamer::gather_rendezvous> " TWAIT_BLOCK_RENDEZVOUS "_holder(" TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ");\n";
  o->output_str(b.str());

  output_mode_t om = o->switch_to_mode(OUTPUT_TREADMILL);
  b.str(str());
  assert(b.str() == str());
  o->switch_to_mode(om);

  // now we are returning to mainly pass-through code, but with some
  // callbacks thrown in (which won't change the line-spacing)

  for (std::list<tame_el_t *>::iterator el = _lst.begin(); el != _lst.end(); el++) {
      (*el)->output (o);
  }

  int lineno = o->lineno();
  o->switch_to_mode(OUTPUT_TREADMILL, lineno);
  b << "/*}twait*/ " TWAIT_BLOCK_RENDEZVOUS "_holder.reset(); } while (0); ";
  b << "  if (" TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ".has_waiting()) {\n"
    << "    " TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ".set_location(__FILE__, __LINE__);\n";
  if (!description_empty(description_) && tamer_debug)
      b << "    " TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ".set_description((" << description_ << "));\n";
  b << _fn->label(_id) << ":\n"
    << "    if (" TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ".has_waiting()) {\n"
    << "        " TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ".block(" TAME_CLOSURE_NAME ", " << _id << ");\n"
    << "        tamer_closure_holder_.reset();\n"
    << "        " << _fn->return_expr() << "; }}\n"
    << "  } while (0);\n";
  o->output_str(b.str());
  o->switch_to_mode(OUTPUT_PASSTHROUGH);
  o->output_str("\n#undef make_event\n");
  o->switch_to_mode(om);
}

void
tame_block_thr_t::output (outputter_t *o)
{
  strbuf b;

  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

  b << "  do {\n"
    << "      thread_implicit_rendezvous_t "
    << " _tirv (__cls, __FL__);\n"
    << "  thread_implicit_rendezvous_t *__cls = &_tirv;\n";

  o->output_str(b.str());
  b.str(str());
  o->switch_to_mode (om);

  for (std::list<tame_el_t *>::iterator el = _lst.begin(); el != _lst.end(); el++) {
      (*el)->output (o);
  }

  om = o->switch_to_mode (OUTPUT_TREADMILL);
  b << "  } while (0);\n";

  o->output_str(b.str());
  o->switch_to_mode (om);
}


str
tame_fn_t::return_expr () const
{
    if (_default_return.length()) {
	strbuf b;
	b << "do { " << _default_return << "} while (0)";
	return b.str();
    } else {
	return "return";
    }
}

void
parse_state_t::output (outputter_t *o)
{
  o->start_output ();
  element_list_t::output (o);
}

bool
expr_list_t::output_vars (strbuf &b, bool first, const str &prfx,
			  const str &sffx)
{
  for (unsigned i = 0; i < size (); i++) {
    if (!first) b << ", ";
    else first = false;
    b << prfx << (*this)[i].name () << sffx;
  }
  return first;
}

void
tame_wait_t::output_blocked(strbuf &b, const str &jgn)
{
    b << "    " << jgn << ".set_location(__FILE__, __LINE__);\n";
    if (!description_empty(description_) && tamer_debug)
        b << "    " << jgn << ".set_description((" << description_ << "));\n";
    b << "    " << jgn << ".block(" TAME_CLOSURE_NAME ", " << _id << ");\n"
      << "    tamer_closure_holder_.reset();\n"
      << "    " << _fn->return_expr() << ";\n";
}

void
tame_wait_t::output (outputter_t *o)
{
    strbuf tmp;
    tmp << "(" << join_group ().name () << ")";
    str jgn = tmp.str();

    output_mode_t om = o->switch_to_mode(OUTPUT_TREADMILL);
    strbuf b;
    b << _fn->label(_id) << ":\n";
    b << "do {\n";
    o->line_number_line(b, lineno_);
    b << "  if (!" << jgn << ".join (";
    for (size_t i = 0; i < n_args (); i++) {
	if (i > 0) b << ", ";
	b << "" << arg (i).name () << "";
    }
    b << ")) {\n";
    output_blocked (b, jgn);
    b << "  }\n"
      << "} while (0);\n";

    o->output_str(b.str());
    o->switch_to_mode (om);
}

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// handle return semantics
//

void
tame_ret_t::output (outputter_t *o)
{
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);
  strbuf b;

  o->switch_to_mode (OUTPUT_PASSTHROUGH, _line_number);
  b << "return";
  if (_params.length()) {
      b << " ";
      b << _params;
  }
  o->output_str(b.str());

  tame_env_t::output (o);
  o->switch_to_mode (om);
}

void
tame_fn_return_t::output (outputter_t *o)
{
  strbuf b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

  b << "  " << _fn->return_expr () << ";\n";
  o->output_str (b.str());
  o->switch_to_mode (om);
}

str
parse_state_t::loc (unsigned l) const
{
  strbuf b;
  b << _infile_name << ":" << l << ": in function "
    << function_const ().name ();
  return b.str();
}

type_qualifier_t &
type_qualifier_t::concat (const type_qualifier_t &m)
{
    for (size_t i = 0; i < m._v.size (); i++) {
	_v.push_back (m._v[i]);
    }
    return (*this);
}

str
type_qualifier_t::to_str () const
{
  strbuf _b;
  for (size_t i = 0; i < _v.size (); i++) {
    if ( i != 0 ) _b << " ";
    _b << _v[i];
  }
  return _b.str();
}
