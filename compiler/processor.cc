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

str template_args (str s)
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

// Must match "__CLS" in tame_const.h
#define TAME_CLOSURE_NAME     "__cls"

#define CLOSURE_GENERIC       "__cls_g"

str
type_t::type_without_pointer () const
{
  strbuf b;
  b << _base_type;
  if (_template_args.length())
    b << _template_args << " ";
  return b.str();
}

str
type_t::mk_ptr () const
{
  strbuf b;
  b << "ptr<" << type_without_pointer() << " >";
  return b.str();
}

str
type_t::alloc_ptr(const str &n, const str &args) const
{
  strbuf b;
  b << mk_ptr() << " " << n << " = new refcounted<"
    << type_without_pointer() << " > (" << args << ")";
  return b.str();
}

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

str var_t::param_decl(const str &p) const
{
    strbuf b;
    b << _type.to_str() << p << _name << _arrays;
    return b.str();
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
    if (_lst.empty() || !_lst.back()->append(s.str()))
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

void
tame_fn_t::add_env (tame_env_t *e)
{
  _envs.push_back (e); 
  if (e->is_jumpto ()) 
    e->set_id (++_n_labels);
}

cpp_initializer_t::cpp_initializer_t(const lstr &v)
    : initializer_t(v)
{
    // rewrite "this" to "__self".  Do it the right way.
    strbuf b;
    int mode = 0;
    std::string::iterator last = _value.begin();
    for (std::string::iterator a = last; a + 4 < _value.end(); a++)
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
	    mode = 0;
	else if (mode == 0 && *a == 't' && a[1] == 'h' && a[2] == 'i' && a[3] == 's' && (a + 4 == _value.end() || (!isalnum(a[4]) && a[4] != '_' && a[4] != '$'))) {
	    b << std::string(last, a) << "_closure__self";
	    last = a + 4;
	    a += 3;
	}
    if (last != _value.begin()) {
	b << std::string(last, _value.end());
	_value = lstr(_value.lineno(), b);
    }
    
}

str
cpp_initializer_t::output_in_constructor () const
{
  strbuf b;
  b << "(" << _value.str() << ")";
  return b.str();
}

str
array_initializer_t::output_in_declaration () const 
{
  strbuf b;
  b << "[" << _value.str() << "]";
  return b.str();
}


//-----------------------------------------------------------------------
// Output utility routines
//

var_t
tame_fn_t::mk_closure(bool ref) const
{
    strbuf b;
    b << "closure__" << _method_name;
    if (_args) {
	b << "__";
	_args->mangle(b);
    }
    return var_t(b.str(), (ref ? "&" : "*"), TAME_CLOSURE_NAME, NONE, _template_args);
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
	    str::const_iterator x = i+1;
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

str tame_fn_t::reenter_fn() const
{
  strbuf b;
  b << closure().type().to_str_w_template_args(false)
    << "::tamer_closure_activate";
  return b.str();
}

void
vartab_t::declarations (strbuf &b, const str &padding) const
{
  for (unsigned i = 0; i < size (); i++) {
    b << padding << _vars[i].decl() << ";\n";
  }
}

void
vartab_t::initialize (strbuf &b, bool self) const
{
  bool first = true;
  initializer_t *init = 0;
  for (unsigned i = 0; i < size (); i++) {
    if (self || 
	((init = _vars[i].initializer ()) && 
	 init->do_constructor_output ())) {
      
      if (!first) b << ", ";
      first = false;
      b << _vars[i].name () << " ";
      
      if (self)  b << "(" <<  _vars[i].name () << ")"; 
      else       b << init->output_in_constructor (); 

    }
  }
}

void
vartab_t::paramlist (strbuf &b, list_mode_t list_mode, str prfx) const
{
  for (unsigned i = 0; i < size () ; i++) {
    if (i != 0) b << ", ";
    switch (list_mode) {
    case DECLARATIONS:
      b << _vars[i].param_decl(prfx);
      break;
    case NAMES:
      b << prfx << _vars[i].name();
      break;
    case TYPES:
      b << _vars[i].type().to_str();
      break;
    default:
      assert (false);
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
    if (_strs.size()) {
	int ln = _strs[0].lineno();
	output_mode_t old = o->switch_to_mode (OUTPUT_PASSTHROUGH, ln);
	o->output_str(_buf.str());
	o->switch_to_mode(old);
    }
}

void
tame_fn_t::output_reenter (strbuf &b)
{
    b << "  void tamer_closure_activate(unsigned blockid) {\n    ";
    //if (tamer_debug)
    //	b << "tamer_debug_closure::set_block_landmark(0, 0);\n    ";
    if (_class.length() && !(_opts & STATIC_DECL))
	b << _self.name() << "->" << _method_name;
    else
	b << _name;
    b << "(*this, blockid);\n"
      << "  }\n";
}

void
tame_fn_t::output_closure(outputter_t *o)
{
  strbuf b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

  if (_template.length()) {
      b << template_str () << "\n";
  }

  b << "class ";
  if (_class.length())
      b << _class << "::";
  b << closure().type().base_type() 
    << " : public tamer::tamerpriv::"
    << (tamer_debug ? "tamer_debug_closure" : "tamer_closure")
    << " {\n"
    << "public:\n"
    << "  " << closure().type().base_type () 
    << " (";

  if (need_self ()) {
      b << _self.decl();
      if (_args)
	  b << ", ";
  }

  if (_args) {
      _args->paramlist(b, DECLARATIONS);
  }

  b << ")";
  const char *separator = " : ";

  if (need_self ()) {
      str s = _self.name ();
      b << separator << s << " (" << s << ")";
      separator = ", ";
  }

  // output arguments declaration
  if (_args && _args->size ()) {
      b << separator;
      _args->initialize(b, true);
      separator = ", ";
  }
  
  // output stack declaration
  if (_stack_vars.size ()) {
      strbuf i;
      _stack_vars.initialize (i, false);
      str s (i.str());
      if (s.length() > 0) {
	  b << separator << s << " ";
	  separator = ", ";
      }
  }

  if (need_implicit_rendezvous())
      b << separator << "_closure__block(this) ";
  
  b << " {}\n\n";

  output_reenter (b);

  if (_class.length() && !(_opts & STATIC_DECL))
      b << "  " << _self.decl() << ";\n";
  
  if (_args)
      _args->declarations (b, "    ");
  _stack_vars.declarations (b, "    ");

  if (need_implicit_rendezvous())
      b << "  tamer::gather_rendezvous _closure__block;\n";

  b << "};\n\n";

  o->output_str(b.str());
  o->switch_to_mode (om);
}

void
tame_fn_t::output_stack_vars(strbuf &b)
{
    for (unsigned i = 0; i < _stack_vars.size (); i++) {
	const var_t &v = _stack_vars._vars[i];
	b << "  " << v.ref_decl() << " TAMER_CLOSUREVARATTR = "
	  << closure_nm () << "." << v.name () << ";\n" ;
    }
}

void
tame_fn_t::output_arg_references(strbuf &b)
{
    for (unsigned i = 0; _args && i < _args->size(); i++) {
	const var_t &v = _args->_vars[i];
	b << "  " << v.ref_decl() << " TAMER_CLOSUREVARATTR = "
	  << closure_nm() << "." << v.name() << ";\n";
    }
}

void
tame_fn_t::output_jump_tab (strbuf &b)
{
    b << "  switch (__tame_blockid) {\n"
    << "  case 0: break;\n"
    ;
  for (unsigned i = 0; i < _envs.size (); i++) {
    if (_envs[i]->is_jumpto ()) {
      int id_tmp = _envs[i]->id ();
      assert (id_tmp);
      b << "  case " << id_tmp << ":\n"
	<< "    goto " << label (id_tmp) << ";\n"
	<< "    break;\n";
    }
  }
  b << "  default:\n"
    << "    throw tamer::tamer_error(\"bad closure unblock\");\n"
    << "  }\n";
}

str
tame_fn_t::signature() const
{
    strbuf b;
    if (_template.length())
	b << template_str() << " ";
    if ((_opts & STATIC_DECL) && !_class.length())
	b << "static ";
    if (_opts & VIRTUAL_DECL)
	b << "virtual ";
    if (_opts & INLINE_DECL)
	b << "inline ";
    b << _ret_type.to_str() << " " << _name << "(";
    if (_args)
	_args->paramlist(b, DECLARATIONS);
    b << ")";
    if (_isconst) 
	b << " const";
    return b.str();
}

str
tame_fn_t::closure_signature() const
{
    strbuf b;
    if (_template.length())
	b << template_str() << " ";
    if ((_opts & STATIC_DECL) && !_class.length())
	b << "static ";
    b << _ret_type.to_str() << " " << _name << "("
      << mk_closure(true).decl() << ", unsigned __tame_blockid)";
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
    b << "  " << closure().decl() << ";\n";
    b << "  __cls = new " << closure().type().base_type() << "(";
    if (_class.length() && !(_opts & STATIC_DECL))
	b << "this" << (_args ? ", " : "");
    if (_args)
	_args->paramlist(b, NAMES);
    b << ");\n"
      << "  __cls->tamer_closure_activate(0);\n"
      << "  __cls->tamer_closure::unuse();\n}\n";
    
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
    
    o->output_str("\n");
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
    if (!_class.length() || _declaration_only)
	b << "class " << closure().type().base_type() << ";";
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

void 
tame_block_ev_t::output(outputter_t *o)
{
  strbuf b;
  str tmp;

  b << "  { ";
  if (tamer_debug)
      b << TAME_CLOSURE_NAME << ".tamer_debug_closure::set_block_landmark(__FILE__, __LINE__); ";
  b << "do {\n#define make_event(...) make_event(__cls._closure__block, ## __VA_ARGS__)\n";
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
  o->output_str(" } while (0);");
  o->switch_to_mode(OUTPUT_PASSTHROUGH);
  o->output_str("\n#undef make_event\n");
  o->switch_to_mode(OUTPUT_TREADMILL, lineno);
  b << _fn->label(_id) << ":\n"
    << "  while (" << TAME_CLOSURE_NAME << "._closure__block.nwaiting()) {\n"
    << "      " << TAME_CLOSURE_NAME << "._closure__block.block(" << TAME_CLOSURE_NAME << ", " << _id << ");\n"
    << "      " << _fn->return_expr() << "; }\n"
    << "  }\n";

  o->output_str(b.str());
  o->switch_to_mode (om);
}

void
tame_block_thr_t::output (outputter_t *o)
{
  strbuf b;

  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

  b << "  do {\n"
    << "      thread_implicit_rendezvous_t " 
    << " _tirv (" CLOSURE_GENERIC ", __FL__);\n"
    << "  thread_implicit_rendezvous_t *" CLOSURE_GENERIC " = &_tirv;\n";

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
tame_join_t::output_blocked(strbuf &b, const str &jgn)
{
    b << "    " << jgn << ".block(" << TAME_CLOSURE_NAME << ", " << _id << ");\n";
    if (tamer_debug)
	b << "    " << TAME_CLOSURE_NAME << ".tamer_debug_closure::set_block_landmark(__FILE__, __LINE__);\n";
    b << "    " << _fn->return_expr() << ";\n";
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
    b << "do {\n"
      << "  if (!" << jgn << ".join (";
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

  // always do end of scope checks
  b << "  do { /*" << TAME_CLOSURE_NAME << "->end_of_scope_checks (" 
    << _line_number << ");*/\n";
  o->output_str(b.str());
  b.str(str());

  o->switch_to_mode (OUTPUT_PASSTHROUGH, _line_number);
  
  b << "    return ";
  if (_params.length())
    b << _params;
  b << ";  } while (0)";
  o->output_str(b.str());

  tame_env_t::output (o);
  o->switch_to_mode (om);

}

void
tame_unblock_t::output (outputter_t *o)
{
  const str tmp ("__cb_tmp");
  strbuf b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);
  str loc = state->loc (_line_number);
  b << "  do {\n";
  
  str n = macro_name ();
  b << n << " (\"" << loc << "\", " << tmp;
  if (_params.length()) {
      b << ", " << _params.str();
  }
  b << "); ";
  do_return_statement (b);
  b << "  } while (0);\n";

  o->output_str(b.str());
  o->switch_to_mode (om);
}

void
tame_resume_t::do_return_statement (strbuf &b) const
{
    b << _fn->return_expr () << "; ";
}

void
tame_fn_return_t::output (outputter_t *o)
{
  strbuf b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

  b << "  do {\n";

  b << "  /*" << TAME_CLOSURE_NAME << "->end_of_scope_checks (" 
    << _line_number << ");*/\n";
  b << "  " << _fn->return_expr () << ";\n";
  b << "  } while (0);\n";
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
