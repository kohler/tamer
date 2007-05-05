// -*-c++-*-
/* $Id: tame.hh,v 1.5 2007-05-05 17:12:28 kohler Exp $ */

/*
 *
 * Copyright (C) 2005 Max Krohn (email: my last name AT mit DOT edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef _TAME_TAME_H
#define _TAME_TAME_H

/*
 * netdb.h hack
 *
 * There is a conflict betwen flex version 2.5.4 and /usr/include/netdb.h
 * on Linux. flex generates code that #define's __unused to be empty, but
 * in struct gaicb within netdb.h, there is a member field of a struct
 * called __unused, which gets blanked out, causing a compile error.
 * (Note that netdb.h is included from sysconf.h).  Use this hack to
 * not include netdb.h for now...
 */
#ifndef _NETDB_H
# define _SKIP_NETDB_H
# define _NETDB_H
#endif

#ifdef _SKIP_NETDB_H
# undef _NETDB_H
# undef _SKIP_NETDB_H
#endif
/*
 * end netdb.h hack
 */

//#include "vec.h"
//#include "union.h"
//#include "qhash.h"
//#include "list.h"
//#include "ihash.h"
#include <string.h>
#include <assert.h>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>

typedef std::string str;
typedef std::stringstream strbuf;
extern std::ostream &warn;

extern int yylex ();
extern int yyparse ();
#undef yyerror
extern int yyerror (str);

extern int yyparse ();
extern int yydebug;
extern FILE *yyin;
extern int get_yy_lineno ();
extern str get_yy_loc ();

typedef enum { NONE = 0, ARG = 1, STACK = 2, CLASS = 3, EXPR = 4 } vartyp_t ;

str ws_strip (str s);
str template_args (str s);

class lstr {
public:
    lstr() : _s(), _lineno(0) {}
    explicit lstr(const char *c) : _s(c), _lineno(0) {}
    explicit lstr(const std::string &s) : _s(s), _lineno(0) {}
    lstr(unsigned ln, const std::string &s) : _s(s), _lineno(ln) {}
    lstr(unsigned ln, const strbuf &s) : _s(s.str()), _lineno(ln) {}
    const std::string &str() const { return _s; }
    std::string::size_type length() const { return _s.length(); }
    void set_lineno (unsigned l) { _lineno = l; }
    unsigned lineno () const { return _lineno; }
  private:
    std::string _s;
    unsigned _lineno;
};

inline strbuf &operator<<(strbuf &ostr, const lstr &l)
{
    ostr << l.str();
    return ostr;
}


#define HOLDVAR_FLAG     (1 << 0)
#define CONST_FLAG       (2 << 0)
/*
 * object for holding type modifiers like "unsigned", "const",
 * "typename", etc, but also for holding some flags that should
 * never be output, that are internal to tame. Examples include
 * 'holdvar'.
 */
struct type_qualifier_t {
    type_qualifier_t () : _flags (0) {}
    type_qualifier_t (const type_qualifier_t &t) 
	: _lineno (t._lineno), _v (t._v), _flags (t._flags) {}
    type_qualifier_t (const lstr &s, unsigned f = 0) 
	: _flags (f) { if (s.length()) add_lstr (s); }

    void add_str(const str &s) { _v.push_back(s); }
    void add_lstr(const lstr &s) { add_str(s.str()); _lineno = s.lineno(); }

    type_qualifier_t &concat (const type_qualifier_t &m);

    str to_str () const;
    unsigned flags () const { return _flags; }
    unsigned lineno() const { return _lineno; }
    
  private:
    unsigned _lineno;
    std::vector<str> _v;
    unsigned _flags;
};


typedef enum { OUTPUT_NONE = 0,
	       OUTPUT_PASSTHROUGH = 1,
	       OUTPUT_TREADMILL = 2,
	       OUTPUT_BIG_NEW_CHUNK = 3 } output_mode_t;

class outputter_t {
public:
  outputter_t (const str &in, const str &out, bool ox) 
    : _mode (OUTPUT_NONE), _infn (in), _outfn (out), _fd (-1), 
      _lineno (1), _output_xlate (ox), _need_xlate (false), 
      _last_char_was_nl (false), _last_output_in_mode (OUTPUT_NONE),
      _last_lineno (-1), _did_output (false), 
      _do_output_line_number (false) {}
  virtual ~outputter_t ();
  bool init ();

  void start_output ();
  void flush ();

  output_mode_t switch_to_mode (output_mode_t m, int ln = -1);
  output_mode_t mode () const { return _mode; }
  virtual void output_str(const str &s);
protected:
  void output_line_number ();
    void _output_str (const str &s, const str &sep_str = str());
  output_mode_t _mode;
private:
  const str _infn, _outfn;
  int _fd;
  int _lineno;
  bool _output_xlate;

  strbuf _buf;
  bool _need_xlate;
  bool _last_char_was_nl;
  output_mode_t _last_output_in_mode;
  int _last_lineno;
  bool _did_output;
  bool _do_output_line_number;
};

/**
 * Horizontal outputter -- doesn't output newlines when in treadmill mode
 */
class outputter_H_t : public outputter_t {
public:
  outputter_H_t (const str &in, const str &out, bool ox) 
    : outputter_t (in, out, ox) {}
protected:
  void output_str (const str &s);
};

class tame_el_t {
public:
    tame_el_t () {}
    virtual ~tame_el_t () {}
    virtual bool append (const str &) { return false; }
    virtual void output(outputter_t *o) = 0;
    virtual bool goes_after_vars () const { return true; }
    virtual bool need_implicit_rendezvous() const { return false; }
};

class element_list_t : public tame_el_t {
  public:
    virtual void output(outputter_t *o);
    void passthrough (const lstr &l) ;
    void push(tame_el_t *e) {
	if (_lst.empty() || _lst.back() != e) {
	    push_hook(e);
	    _lst.push_back(e);
	}
    }
    virtual void push_hook (tame_el_t *) {}
    bool need_implicit_rendezvous() const;
  protected:
    std::list<tame_el_t *> _lst;
};

class tame_env_t : public element_list_t {
public:
  virtual void output(outputter_t *o) { element_list_t::output(o); }
  virtual bool is_jumpto () const { return false; }
  virtual void set_id (int) {}
  virtual int id () const { return 0; }
  virtual bool needs_counter () const { return false; }
};

class tame_fn_t; 

class tame_vars_t : public tame_el_t {
public:
  tame_vars_t (tame_fn_t *fn, int lineno) : _fn (fn), _lineno (lineno) {}
  void output(outputter_t *o) ;
  bool goes_after_vars () const { return false; }
  int lineno () const { return _lineno; }
private:
  tame_fn_t *_fn;
  int _lineno;
};

class tame_passthrough_t : public tame_el_t {
  public:
    tame_passthrough_t(const lstr &s) { append (s); }
    bool append(const lstr &s) {
	_strs.push_back(s);
	_buf << s;
	return true;
    }
    void output(outputter_t *o);
  private:
    strbuf _buf;
    std::vector<lstr> _strs;
};

class type_t {
  public:
    type_t() {}
    type_t(const str &t, const str &p)
	: _base_type(t), _pointer(p) {}
    type_t(const str &t, const str &p, const str &ta)
	: _base_type(t), _pointer(p), _template_args(ta) {}
    str base_type() const { return _base_type; }
    str pointer() const { return _pointer; }
    str arrays() const { return _arrays; }
    str to_str() const;
    str to_str_w_template_args(bool p = true) const;
    str mk_ptr() const;
    str alloc_ptr(const str &nm, const str &args) const;
    str type_without_pointer() const;
    void set_base_type(const str &t) { _base_type = t; }
    void set_pointer(const str &p) { _pointer = p; }
    void set_arrays(const str &a) { _arrays = a; }
    bool is_complete() const { return _base_type.length(); }
    bool is_void() const {
	return (_base_type == "void" && _pointer.length() == 0);
    }
    bool is_ref() const { return _pointer.find('&') != str::npos; }
private:
    str _base_type, _pointer, _arrays, _template_args;
};

class initializer_t {
public:
  initializer_t () : _value (0, "") {}
  initializer_t (const lstr &v) : _value (v) {}
  virtual ~initializer_t () {}
  virtual str output_in_constructor () const { return ""; }
  virtual str output_in_declaration () const { return ""; }
  virtual bool do_constructor_output () const { return false; }
  virtual str ref_prefix () const;
protected:
  lstr _value;
};

class cpp_initializer_t : public initializer_t {
public:
  cpp_initializer_t (const lstr &v) : initializer_t (v) {}
  str output_in_constructor () const;
  bool do_constructor_output () const { return true; }
};

class array_initializer_t : public initializer_t {
public:
  array_initializer_t (const lstr &v) : initializer_t (v) {}
  str output_in_declaration () const;
  str ref_prefix () const;
};

class declarator_t;
class var_t {
public:
    var_t() : _initializer(0) {}
    var_t(const str &n, vartyp_t a = NONE) : _name (n), _asc (a), _initializer(0), _flags (0) {}
    var_t(const type_qualifier_t &m, declarator_t *d, const lstr &arrays, vartyp_t a = NONE);
    var_t(const str &t, const str &p, const str &n, vartyp_t a = NONE)
	: _name(n), _type(t, p), _asc(a), _initializer(0), _flags(0) {}
    var_t(const type_t &t, const str &n, vartyp_t a = NONE)
	: _name(n), _type(t), _asc(a), _initializer(0), _flags(0) {}
    var_t(const str &t, const str &p, const str &n, vartyp_t a, const str &ta)
	: _name(n), _type(t, p, ta), _asc(a), _initializer(0), _flags(0) {}

    const type_t &type() const { return _type; }
    const str &name() const { return _name; }
    type_t *get_type() { return &_type; }
    const type_t *get_type_const() const { return &_type; }
    bool is_complete() const { return _type.is_complete (); }

    // ASC = Args, Stack or Class
    void set_asc(vartyp_t a) { _asc = a; }
    vartyp_t get_asc() const { return _asc; }

    void set_type(const type_t &t) { _type = t; }
    initializer_t *initializer() const { return _initializer; }
    bool do_output() const;

    str param_decl(const str &prfx) const;
    str decl() const;
    str decl(const str &prfx, int n) const;
    str decl(const str &prfx) const;
    str ref_decl() const;
    str _name;

protected:
    type_t _type;
    vartyp_t _asc;
    initializer_t *_initializer;
    unsigned _flags;
    str _arrays;
};

class expr_list_t : public std::vector<var_t>
{
public:
  bool output_vars (strbuf &b, bool first = false, const str &prfx = "",
		    const str &sffx = "");
};

typedef enum { DECLARATIONS, NAMES, TYPES } list_mode_t;

class vartab_t {
public:
    vartab_t() {}
    vartab_t(const var_t &v) { add(v); }
    ~vartab_t() {}
    size_t size () const { return _vars.size (); }
    bool add(const var_t &v) ;
    void declarations (strbuf &b, const str &padding) const;
    void paramlist (strbuf &b, list_mode_t m, str prfx = "") const;
    void initialize (strbuf &b, bool self) const;
    bool exists (const str &n) const { return _tab.find(n) != _tab.end(); }
    const var_t *lookup (const str &n) const;

    std::vector<var_t> _vars;
    std::map<str, unsigned> _tab;
};


class tame_block_t;
class tame_nonblock_t;
class tame_join_t;
class tame_fork_t;

/*
 * corresponds to the yacc rule for parsing C declarators -- either for
 * variables or for function definitions
 */
class declarator_t {
public:
    declarator_t (const str &n, vartab_t *v)
	: _name (n), _params (v), _initializer(0) {}
    declarator_t (const str &n) : _name (n), _params(0), _initializer(0) {}
  void set_pointer (const str &s) { _pointer = s; }
  str pointer () const { return _pointer; }
  str name () const { return _name; }
  vartab_t *params () { return _params; }
  void set_params (vartab_t *v) { _params = v; }
  void set_initializer (initializer_t *i) { _initializer = i; }
  void dump () const ;
  initializer_t *initializer () const { return _initializer; }
private:
  const str _name;
  str _pointer;
  vartab_t *_params;
  initializer_t *_initializer;
};

// Convert:
//
//   foo_t::max<int,int> => foo_t__max_int_int_
//
str mangle (const str &in);

//
// convert 
//
//   foo_t::max<int,int>::bar  => bar
//
str strip_to_method (const str &in);
str strip_off_method (const str &in);

class tame_block_t;

#define STATIC_DECL           (1 << 0)
#define CONST_DECL            (2 << 0)

// function specifier embodies static and template keywords and options
// at present, and perhaps more in the future.
struct fn_specifier_t {
  fn_specifier_t (unsigned o = 0, str t = "")
    : _opts (o), _template (t) {}
  unsigned _opts;
  str _template;
};

//
// Unwrap Function Type
//
class tame_fn_t : public element_list_t {
public:
  tame_fn_t (const fn_specifier_t &fn, const str &r, declarator_t *d, 
	     bool c, unsigned l, str loc)
      : _ret_type (ws_strip (r), ws_strip(d->pointer())),
      _name (d->name ()),
      _method_name (strip_to_method (_name)),
      _class (strip_off_method (_name)), 
      _self (c ? str("const ") + _class : _class, "*", "_closure__self"),
      _isconst (c),
      _template (fn._template),
      _template_args (_class.length() ? template_args (_class) : ""),
      _closure (mk_closure (false)), 
      _args (d->params ()), 
      _opts (fn._opts),
      _lineno (l),
      _n_labels (0),
      _n_blocks (0),
      _loc (loc),
      _lbrace_lineno (0),
      _vars (NULL),
      _after_vars_el_encountered (false)
  { }

  vartab_t *stack_vars () { return &_stack_vars; }
  vartab_t *args () { return _args; }
  vartab_t *class_vars_tmp () { return &_class_vars_tmp; }

  void push_hook (tame_el_t *el)
  {
    if (el->goes_after_vars ())
      _after_vars_el_encountered = true;
  }

  // called from tame_vars_t class
  void output_vars (outputter_t *o, int ln);

  // default return statement is "return;"; can be overidden,
  // but only once.
  bool set_default_return (str s) 
  { 
      bool ret = _default_return.length() == 0;
      _default_return = s; 
      return ret;
  }

    // if non-void return, then there must be a default return
    bool check_return_type () const {
	return (_ret_type.is_void () || _default_return.length());
    }

  str classname () const { return _class; }
  str name () const { return _name; }
  str closure_signature (bool decl, str prfx = "", bool sttc = false) const;
  str signature (bool decl, str prfx = "", bool sttc = false) const;

  void set_opts (int i) { _opts = i; }
  int opts () const { return _opts; }

    bool need_self () const { return (_class.length() && !(_opts & STATIC_DECL)); }

  void output(outputter_t *o);

  void add_env (tame_env_t *g) ;

  var_t closure () const { return _closure; }
  static var_t trig () ;

  void hit_tame_block () { _n_blocks++; }

  str closure_nm () const { return _closure.name (); }
  str reenter_fn  () const ;
  str frozen_arg (const str &i) const ;

  str label (str s) const;
  str label (unsigned id) const ;
  str loc () const { return _loc; }

  str return_expr () const;

    str template_str () const {
	if (_template.length())
	    return str("template< ") + _template + str(" >");
	else
	    return str();
    }

  void set_lbrace_lineno (unsigned i) { _lbrace_lineno = i ; }

  bool set_vars (tame_vars_t *v) 
  { _vars = v; return (!_after_vars_el_encountered); }
  const tame_vars_t *get_vars () const { return _vars; }

private:
  const type_t _ret_type;
  const str _name;
  const str _method_name;
  const str _class;
  const var_t _self;

  const bool _isconst;
  str _template;
  str _template_args;
  const var_t _closure;

  var_t mk_closure (bool ref) const ;

  vartab_t *_args;
  vartab_t _stack_vars;
  vartab_t _class_vars_tmp;
    std::vector<tame_env_t *> _envs;

  void output_reenter (strbuf &b);
  void output_closure (outputter_t *o);
  void output_firstfn (outputter_t *o);
  void output_fn (outputter_t *o);
  void output_static_decl (outputter_t *o);
  void output_stack_vars (strbuf &b);
  void output_arg_references (strbuf &b);
  void output_jump_tab (strbuf &b);
  void output_block_cb_switch (strbuf &b);
  
  int _opts;
  unsigned _lineno;
  unsigned _n_labels;
  unsigned _n_blocks;
  str _default_return;
  str _loc; // filename:linenumber where this function was declared
  unsigned _lbrace_lineno;  // void foo () { ... where the '{' was
  tame_vars_t *_vars;
  bool _after_vars_el_encountered;
};


class tame_ret_t : public tame_env_t 
{
public:
  tame_ret_t (unsigned l, tame_fn_t *f) : _line_number (l), _fn (f) {}
  void add_params (const lstr &l) { _params = l; }
  virtual void output(outputter_t *o);
protected:
  unsigned _line_number;
  tame_fn_t *_fn;
  lstr _params;
};

class tame_fn_return_t : public tame_ret_t {
public:
  tame_fn_return_t (unsigned l, tame_fn_t *f) : tame_ret_t (l, f) {}
  void output(outputter_t *o);
};

class tame_unblock_t : public tame_ret_t {
public:
  tame_unblock_t (unsigned l, tame_fn_t *f) : tame_ret_t (l, f) {}
  virtual ~tame_unblock_t () {}
  void output(outputter_t *o);
  virtual str macro_name () const { return "SIGNAL"; }
  virtual void do_return_statement (strbuf &) const {}
};

class tame_resume_t : public tame_unblock_t {
public:
  tame_resume_t (unsigned l, tame_fn_t *f) : tame_unblock_t (l, f) {}
  ~tame_resume_t () {}
  str macro_name () const { return "RESUME"; }
  void do_return_statement (strbuf &b) const;
};

class parse_state_t : public element_list_t {
public:
  parse_state_t () : _xlate_line_numbers (false),
		     _need_line_xlate (true) 
  {
    _lists.push_back (this);
  }

  void new_fn (tame_fn_t *f) { new_el (f); _fn = f; }
  void new_el (tame_el_t *e) { _fn = NULL; push (e); }
  void set_fn (tame_fn_t *f) { _fn = f; }
  void clear_fn () { set_fn (NULL); }
  tame_fn_t *fn () { return _fn; }

  void passthrough (const lstr &l) { top_list ()->passthrough (l); }
  void push (tame_el_t *e) { top_list ()->push (e); }

  element_list_t *top_list () { return _lists.back (); }
  void push_list (element_list_t *l) { _lists.push_back (l); }
  void pop_list () { _lists.pop_back (); }

  // access variable tables in the currently active function
  vartab_t *stack_vars () { return _fn ? _fn->stack_vars () : NULL; }
  vartab_t *class_vars_tmp () { return _fn ? _fn->class_vars_tmp () : NULL ; }
  vartab_t *args () { return _fn ? _fn->args () : NULL; }

  void set_decl_specifier (const type_qualifier_t &m) { _decl_specifier = m; }
  const type_qualifier_t &decl_specifier () const { return _decl_specifier; }
  tame_fn_t *function () { return _fn; }
  const tame_fn_t &function_const () const { return *_fn; }

  void new_block (tame_block_t *g);
  tame_block_t *block () { return _block; }
  void clear_block () { _block = NULL; }

  void new_nonblock (tame_nonblock_t *s);
  tame_nonblock_t *nonblock () { return _nonblock; }

  void new_fork (tame_fork_t *f);
  tame_fork_t *fork () { return _fork; }

  void output(outputter_t *o);

  void clear_sym_bit () { _sym_bit = false; }
  void set_sym_bit () { _sym_bit = true; }
  bool get_sym_bit () const { return _sym_bit; }

  void set_infile_name (const str &i) { _infile_name = i; }
  str infile_name () const { return _infile_name; }
  str loc (unsigned l) const ;

protected:
  type_qualifier_t _decl_specifier;
  tame_fn_t *_fn;
  tame_block_t *_block;
  tame_nonblock_t *_nonblock;
  tame_fork_t *_fork;
  bool _sym_bit;

  // lists of elements (to reflect nested structure)
    std::vector<element_list_t *> _lists;

  str _infile_name;
  bool _xlate_line_numbers;
  bool _need_line_xlate;
};

class tame_block_t : public tame_env_t {
public:
    tame_block_t (int l) : _lineno (l) {}
    virtual ~tame_block_t () {}
    bool need_implicit_rendezvous() const { return true; }
protected:
    int _lineno;
};

class tame_block_thr_t : public tame_block_t {
public:
  tame_block_thr_t (int l) : tame_block_t (l) {}
  void output(outputter_t *o);
};

class tame_block_ev_t : public tame_block_t {
public:
  tame_block_ev_t (tame_fn_t *f, int l) 
    : tame_block_t (l), _fn (f), _id (0) {}
  ~tame_block_ev_t () {}
  
  void output(outputter_t *o);
  bool is_jumpto () const { return true; }
  void set_id (int i) { _id = i; }
  int id () const { return _id; }
  void add_class_var (const var_t &v) { _class_vars.add (v); }
  bool needs_counter () const { return true; }
  
protected:
  tame_fn_t *_fn;
  int _id;
  vartab_t _class_vars;
};


  
class tame_nonblock_t : public tame_env_t {
public:
  tame_nonblock_t (expr_list_t *l) : _args (l) {}
  ~tame_nonblock_t () {}
  void output(outputter_t *o) { element_list_t::output (o); }
  expr_list_t *args () const { return _args; }
private:
  expr_list_t *_args;
};

class tame_join_t : public tame_env_t {
public:
  tame_join_t (tame_fn_t *f, expr_list_t *l) : _fn (f), _args (l) {}
  bool is_jumpto () const { return true; }
  void set_id (int i) { _id = i; }
  int id () const { return _id; }
  virtual void output(outputter_t *o) = 0;
  var_t join_group () const { return (*_args)[0]; }
  var_t arg (unsigned i) const { return (*_args)[i+1]; }
  size_t n_args () const 
  { assert (_args->size () > 0); return _args->size () - 1; }
protected:
  void output_blocked (strbuf &b, const str &jgn);
  tame_fn_t *_fn;
  expr_list_t *_args;
  int _id;
};

class tame_fork_t : public tame_env_t {
public:
  tame_fork_t (tame_fn_t *f, expr_list_t *l) : _fn (f), _args (l) {}
  bool is_jumpto () const { return false; }
  void set_id (int i) { _id = i; }
  int id () const { return _id; }
  var_t join_group () const { return (*_args)[0]; }
  var_t arg (unsigned i) const { return (*_args)[i+1]; }
  size_t n_args () const 
  { assert (_args->size () > 0); return _args->size () - 1; }
protected:
  tame_fn_t *_fn;
  expr_list_t *_args;
  int _id;
};

class tame_wait_t : public tame_join_t {
public:
  tame_wait_t (tame_fn_t *f, expr_list_t *l, int ln) 
    : tame_join_t (f, l), _lineno (ln) {}
  void output(outputter_t *o);
private:
  int _lineno;
};

extern parse_state_t *state;
extern str infile_name;

struct YYSTYPE {
    lstr              str;
    declarator_t*     decl;
    vartab_t*         vars;
    var_t             var;
    bool              opt;
    char              ch;
    tame_fn_t *       fn;
    tame_el_t *       el;
    expr_list_t*      exprs;
    type_t            typ;
    std::vector<declarator_t *> decls;
    unsigned          opts;
    tame_ret_t *      ret;
    fn_specifier_t    fn_spc;
    type_qualifier_t  typ_mod;
    initializer_t*    initializer;
};
extern YYSTYPE yylval;
extern str filename;

#define CONCAT(ln,in,out)                                 \
do {                                                      \
      strbuf b;                                           \
      b << in;                                            \
      out = lstr (ln, b.str());				  \
} while (0)

extern bool tamer_debug;

#endif /* _TAME_TAME_H */
