
#include "tame.hh"
#include <ctype.h>

//-----------------------------------------------------------------------
// output only the generic callbacks that we need, to speed up compile
// times.
//
std::map<unsigned, bool> _generic_cb_tab;
std::map<unsigned, bool> _generic_mk_cv_tab;

static unsigned cross (unsigned a, unsigned b) 
{
  assert (a <= 0xff && b <= 0xff);
  return ((a << 8) | b);
}

bool generic_cb_exists (unsigned a, unsigned b) 
{ return _generic_cb_tab[cross (a,b)]; }

bool generic_mk_cv_exists (unsigned a, unsigned b)
{ return _generic_mk_cv_tab[cross (a,b)]; }

void generic_cb_declare (unsigned a, unsigned b)
{ _generic_cb_tab[cross (a,b)] = 1; }

void generic_mk_cv_declare (unsigned a, unsigned b)
{ _generic_mk_cv_tab[cross (a,b)] = 1; }

//
//-----------------------------------------------------------------------


var_t::var_t(const type_qualifier_t &t, declarator_t *d, const lstr &arrays, vartyp_t a)
    : _name(d->name()), _type(t.to_str(), d->pointer()), _asc(a), 
      _initializer(d->initializer ()), _flags(t.flags()), _arrays(arrays.str())
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
    for (std::list<tame_el_t *>::iterator i = _lst.begin(); i != _lst.end(); i++) {
	char x[50]; sprintf(x, "/*<%p>*/ ", *i);
	o->output_str(x);
	(*i)->output(o);}
}

// Must match "__CLS" in tame_const.h
#define TAME_CLOSURE_NAME     "__cls"


#define CLOSURE_RFCNT         "__cls_r"
#define CLOSURE_GENERIC       "__cls_g"
#define TAME_PREFIX           "__tame_"

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

str
type_t::to_str() const
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
initializer_t::ref_prefix () const 
{
  return "&";
}

str
array_initializer_t::ref_prefix () const
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

str
mangle (const str &in)
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
tame_fn_t::closure_generic ()
{
  return var_t ("ptr<closure_t>", NULL, CLOSURE_GENERIC);
}

var_t
tame_fn_t::trig ()
{
  return var_t ("ptr<trig_t>", NULL, "trig");
}

var_t
tame_fn_t::mk_closure (bool ref) const
{
  strbuf b;
  b << _name_mangled << "__closure";

  return var_t (b.str(), (ref ? "&" : "*"), TAME_CLOSURE_NAME, NONE, _template_args);
}

str
tame_fn_t::decl_casted_closure (bool do_lhs) const
{
  strbuf b;
  if (do_lhs) {
    b << "  " << _closure.decl()  << " =\n";
  }
  b << "    reinterpret_cast<" << _closure.type ().to_str_w_template_args () 
    << "> (static_cast<closure_t *> (" << closure_generic ().name () << "));";
  return b.str();
}

str
tame_fn_t::reenter_fn () const
{
  strbuf b;
  b << closure ().type ().to_str_w_template_args (false)
    << "::_closure__activate";
  return b.str();
}

str
tame_fn_t::frozen_arg (const str &i) const
{
  strbuf b;
  b << closure_nm () << "->_args." << i ;
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
      b << prfx << _vars[i].name ();
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
  b << _name_mangled << "__label_" << s;
  return b.str();
}

str
tame_fn_t::label (unsigned id) const
{
  strbuf b;
  b << id;
  return label(b.str());
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
  b << "  void _closure__activate(unsigned blockid)\n"
    << "  {\n"
    ;

  b << "    ";
  if (_class.length()) {
    b << "(";
    if (!(_opts & STATIC_DECL)) {
      b << "(*_self).";
    }
    b << "*_method) ";
  } else {
    b << _name ;
  }

  b << " (*this, blockid);\n"
    << "  }\n";
}

void
tame_fn_t::output_set_method_pointer (strbuf &b)
{
  b << "  typedef "
    << _ret_type.to_str () << " (";
  if (!(_opts & STATIC_DECL)) {
    b << _class << "::";
  }
  b << "*method_type_t) (";
  if (_args) {
    _args->paramlist (b, TYPES);
    b << ", ";
  }
  b << "ptr<closure_t>)";
  if (_isconst)
    b << " const";
  b << ";\n";

  b << "  void set_method_pointer (method_type_t m) { _method = m; }\n\n";
    
}

static void
output_is_onstack (strbuf &b)
{
  b << "  bool is_onstack (const void *p) const\n"
    << "  {\n"
    << "    return (static_cast<const void *> (this) <= p &&\n"
    << "            static_cast<const void *> (this + 1) > p);\n"
    << "  }\n";
}

void
tame_fn_t::output_closure (outputter_t *o)
{
  strbuf b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

  if (_template.length()) {
      b << template_str () << "\n";
  }

  b << "class " << _closure.type ().base_type () 
    << " : public tamer::tamerpriv::closure "
    << "{\n"
    << "public:\n"
    << "  " << _closure.type ().base_type () 
    << " (";

  if (need_self ()) {
      b << _self.decl();
      if (_args)
	  b << ", ";
  }

  if (_args) {
      _args->paramlist(b, DECLARATIONS);
  }

  b << ") : tamer::tamerpriv::closure ("
      // << "\"" << state->infile_name () << "\", \"" << _name << "\""
    << ")"
      ;

  if (need_self ()) {
    str s = _self.name ();
    b << ", " << s << " (" << s << ")";
  }

  // output arguments declaration
  if (_args && _args->size ()) {
    b << ", ";
    _args->initialize (b, true);
  }
  
  // output stack declaration
  if (_stack_vars.size ()) {
    strbuf i;
    _stack_vars.initialize (i, false);
    str s (i.str());
    if (s.length() > 0) {
      b << ", " << s << " ";
    }
  }

  
  b << " {}\n\n";


  if (_class.length()) {
    output_set_method_pointer (b);
  }

  output_reenter (b);

  if (_class.length())
    b << "  method_type_t _method;\n";
  
  //output_is_onstack (b);

  if (_args)
      _args->declarations (b, "    ");
  _stack_vars.declarations (b, "    ");

  b << "  tamer::gather_rendezvous _closure__block;\n";

  b << "};\n\n";

  o->output_str(b.str());
  o->switch_to_mode (om);
}

bool
var_t::do_output () const
{
  return (!(_flags & HOLDVAR_FLAG));
}

void
tame_fn_t::output_stack_vars (strbuf &b)
{
  for (unsigned i = 0; i < _stack_vars.size (); i++) {
    const var_t &v = _stack_vars._vars[i];
    if (v.do_output ()) {
      b << "  " << v.ref_decl() << " = " 
	<< closure_nm () << "." << v.name () << ";\n" ;
    }
  } 
}

void
tame_fn_t::output_arg_references (strbuf &b)
{
  for (unsigned i = 0; _args && i < _args->size (); i++) {
    const var_t &v = _args->_vars[i];
    b << "  " << v.ref_decl() << " __attribute__((unused)) = "
      << closure_nm () << "." << v.name () << ";\n";
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
tame_fn_t::signature (bool d, str prfx, bool static_flag) const
{
    strbuf b;
    if (_template.length())
	b << template_str () << "\n";
    if (static_flag)
	b << "static ";
    b << _ret_type.to_str () << "\n"
      << _name << "(";
    if (_args)
	_args->paramlist(b, DECLARATIONS, prfx);
    b << ")";
    if (_isconst) 
	b << " const";
    return b.str();
}

str
tame_fn_t::closure_signature (bool d, str prfx, bool static_flag) const
{
  strbuf b;
  if (_template.length())
      b << template_str () << "\n";
  if (static_flag)
    b << "static ";

  b << _ret_type.to_str () << "\n"
    << _name << "(";
  b << mk_closure(true).decl();
  b << ", unsigned __tame_blockid)";
  if (_isconst) 
    b << " const";

  return b.str();
}

void
tame_fn_t::output_static_decl (outputter_t *o)
{
  strbuf b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);
  b << closure_signature (true, "", true) << ";\n\n";
  o->output_str(b.str());
  o->switch_to_mode (om);
}

void
tame_fn_t::output_firstfn (outputter_t *o)
{
  strbuf b;
  state->set_fn (this);

  output_mode_t om = o->switch_to_mode (OUTPUT_PASSTHROUGH);
  b << signature (false)  << "\n"
    << "{\n";

  // If no vars section was specified, do it now.
  b << "  " << _closure.decl () << ";\n";
  b << "  __cls = new " << _closure.type().base_type() << "(";
  if (_args) {
      _args->paramlist (b, NAMES);
  }
  b << ");\n  __cls->_closure__activate(0);\n  __cls->closure::unuse();\n}\n";

  o->output_str(b.str());

  o->switch_to_mode (om);
}

void
tame_fn_t::output_fn (outputter_t *o)
{
  strbuf b;
  state->set_fn (this);

  output_mode_t om = o->switch_to_mode (OUTPUT_PASSTHROUGH);
  b << closure_signature (false, TAME_PREFIX)  << "\n"
    << "{\n";

  o->output_str(b.str());

  // If no vars section was specified, do it now.
  if (!_vars)
    output_vars (o, _lbrace_lineno);

  element_list_t::output (o);

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
tame_fn_t::output (outputter_t *o)
{
    strbuf b;
    b << "class " << _closure.type ().base_type () << ";\n";
    o->output_str(b.str());
    //if ((_opts & STATIC_DECL) && !_class)
    output_static_decl (o);
    output_closure (o);
    output_firstfn (o);
    output_fn (o);
}

void 
tame_block_ev_t::output (outputter_t *o)
{
  strbuf b;
  str tmp;

  b << "  { {\n#define make_event(...) make_event(__cls._closure__block, ## __VA_ARGS__)\n";
  o->output_str(b.str());

  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);
  b.str(str());
  assert(b.str() == str());
  o->switch_to_mode (om);

  // now we are returning to mainly pass-through code, but with some
  // callbacks thrown in (which won't change the line-spacing)

  for (std::list<tame_el_t *>::iterator el = _lst.begin(); el != _lst.end(); el++) {
      (*el)->output (o);
  }

  o->output_str(" }\n#undef make_event\n");
  om = o->switch_to_mode (OUTPUT_TREADMILL);
  b << _fn->label(_id) << ":\n"
    << "  while (" << TAME_CLOSURE_NAME << "._closure__block.nwaiting()) {\n"
    << "      " << TAME_CLOSURE_NAME << "._closure__block.block(" << TAME_CLOSURE_NAME << ", " << _id << ");\n"
    << "      "
    << _fn->return_expr ()
    << "; }\n"
    << "  }\n"
    ;


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
tame_join_t::output_blocked (strbuf &b, const str &jgn)
{
    b << "    " << jgn << ".block(" << TAME_CLOSURE_NAME << ", " << _id << ");\n"
      << "    "
      << _fn->return_expr()
      << ";\n";
}

void
tame_wait_t::output (outputter_t *o)
{
    strbuf tmp;
    tmp << "(" << join_group ().name () << ")";
    str jgn = tmp.str();

    output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);
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
  _flags |= (m._flags);
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

//
//-----------------------------------------------------------------------
