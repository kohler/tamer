#include "tame.hh"
#include <ctype.h>

const lstr::size_type lstr::npos;

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
#define TAMER_SELF_NAME       "tamer_self_"
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

str type_t::to_str() const {
    strbuf b;
    b << _base_type;
    if (_pointer.length())
        b << _pointer;
    return b.str();
}

str type_t::decl_to_str() const {
    strbuf b;
    size_t l = _pointer.length(), p = _base_type.length();

    // XXX volatile, etc. GHETTO
    if (!l) {
        int last = ' ', depth = 0;
        for (p = 0; p != _base_type.length(); ++p) {
            switch (_base_type[p]) {
            case '<': case '(':
                ++depth;
                break;
            case '>': case ')':
                --depth;
                break;
            case 'c':
                if (depth == 0
                    && p + 5 <= _base_type.length()
                    && memcmp(&_base_type[p], "const", 5) == 0
                    && !isalnum((unsigned char) last)
                    && last != '_'
                    && (p + 5 == _base_type.length()
                        || (!isalnum((unsigned char) _base_type[p + 5])
                            && _base_type[p + 5] != '_')))
                    goto found;
            }
            last = _base_type[p];
        }
    }

 found:
    if (p != _base_type.length()) {
        b << _base_type.substr(0, p);
        b << _base_type.substr(p + 5, _base_type.length() - (p + 5));
    } else
        b << _base_type;

    if (l) {
        while (l != 0 && _pointer[l - 1] == '&')
            --l;
        if (l != _pointer.length())
            b << _pointer.substr(0, l) << "*";
        else
            b << _pointer;
    }
    return b.str();
}

str var_t::decl(bool include_name) const
{
    strbuf b;
    b << _type.decl_to_str();
    if (_arrays.length() && _arrays[0] == '[') {
        str::size_type rbrace = _arrays.find(']') + 1;
        b << (rbrace == _arrays.length() ? "*" : "(*");
        if (include_name)
            b << " " << _name;
        if (rbrace != _arrays.length())
            b << ")" << _arrays.substr(rbrace);
    } else if (include_name)
        b << " " << _name;
    if (_initializer && include_name)
        _initializer->finish_type(b);
    return b.str();
}

str var_t::param_decl(bool move, bool escape) const
{
    strbuf b;
    if (move && _type.pointer().empty() && _arrays.empty() && !_name.empty())
        b << "TAMER_MOVEARG(" << _type.to_str() << ") ";
    else
        b << _type.to_str() << " ";
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

str var_t::ref_decl(bool noref) const {
    strbuf b;
    const char* refit;
    if (_type.is_ref())
        refit = "";
    else if (_initializer)
        refit = _initializer->ref_prefix(noref);
    else
        refit = "&";
    b << _type.to_str();
    if (_arrays.length() && _arrays[0] == '[') {
        str::size_type rbrace = _arrays.find(']') + 1;
        b << (rbrace == _arrays.length() ? "*" : "(*") << refit << " " << _name;
        if (rbrace != _arrays.length())
            b << ")" << _arrays.substr(rbrace);
    } else
        b << refit << " " << _name;
    if (_initializer)
        b << _initializer->ref_suffix(noref);
    return b.str();
}

const char* initializer_t::ref_prefix(bool) const {
    return "&";
}

str initializer_t::ref_suffix(bool) const {
    return "";
}

const char* array_initializer_t::ref_prefix(bool noref) const {
    if (!noref)
        return "(&";
    else if (_value.find('[') == lstr::npos)
        return "*";
    else
        return "(*";
}

str array_initializer_t::ref_suffix(bool noref) const {
    strbuf b;
    if (!noref)
        b << ")[" << _value << "]";
    else {
        lstr::size_type pos = _value.find('[');
        if (pos != lstr::npos)
            b << ")" << _value.substr(pos) << "]";
    }
    return b.str();
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
      _self(c ? str("const ") + _class : _class, "*", TAMER_SELF_NAME),
      _isconst(c),
      _declaration_only(false),
      _args(d->params()),
      _any_volatile_envs(false),
      _opts(fn._opts),
      _lineno(l),
      _n_labels(1),             // 0 = begin, 1 = exit prematurely
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
    : initializer_t(v), braces_(braces) {
}

void initializer_t::finish_type(strbuf&) const {
}

void array_initializer_t::finish_type(strbuf& b) const {
    b << "[" << _value << "]";
}

void initializer_t::initializer(strbuf&, bool) const {
}

void cpp_initializer_t::initializer(strbuf& b, bool is_ref) const {
    b << (braces_ ? '{' : '(') << (is_ref ? "&" : "")
      << _value.str() << (braces_ ? '}' : ')');
}

void array_initializer_t::append_array(const lstr& v) {
    if (_value.empty())
        _value = v;
    else {
        strbuf b;
        b << _value.str() << "][" << v.str();
        _value = lstr(_value.lineno(), b.str());
    }
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

void type_t::set_pointer(const str& p) {
    size_t l = p.length();
    while (l != 0 && isspace((unsigned char) p[l - 1]))
        --l;
    _pointer = (l == p.length() ? p : p.substr(0, l));
}

str type_t::mangle() const
{
    // It is too hard to mangle correctly given this representation.
    // Just give it a go.
    return mangler(to_str()).s();
}

void vartab_t::closure_declarations(strbuf& b, const str& padding) const
{
    for (unsigned i = 0; i != size(); ++i)
        if (!_vars[i].name().empty())
            b << padding << _vars[i].decl(true) << ";\n";
}

void
vartab_t::initializers_and_reference_declarations(strbuf& b, outputter_t* o,
                                                  tame_fn_t* fn) const
{
    unsigned lineno;
    for (unsigned i = 0; i != size(); ++i) {
        const var_t& v = _vars[i];
        if (v.name().empty())
            continue;
        initializer_t* init = v.initializer();
        if ((lineno = init->constructor_lineno()))
            o->set_lineno(lineno, b);
        b << "  if (" TAME_CLOSURE_NAME ".tamer_block_position_ == 0)\n"
          << "    new ((void*) &" TAME_CLOSURE_NAME "."
          << v.name(false, false) << ") ("
          << v.decl(false);
        if (init)
            init->finish_type(b);
        b << ")";
        // tamer::destroy_guard objects need special initialization
        if (fn && init
            && (v.type().base_type() == "tamer::destroy_guard"
                || v.type().base_type() == "destroy_guard"))
            b << "(" TAME_CLOSURE_NAME ", " << init->value() << ")";
        else if (init)
            init->initializer(b, v.type().is_ref());
        b << ";\n";
        v.reference_declaration(b, "  ");
    }
}

void
vartab_t::paramlist(strbuf &b, paramlist_flags list_mode, const char* sep) const
{
    for (unsigned i = 0; i < size () ; i++) {
        if (list_mode != pl_declarations && _vars[i].name().empty())
            continue;
        b << sep;
        if (list_mode != pl_assign_moves_named)
            sep = ", ";
        switch (list_mode) {
        case pl_declarations:
            b << _vars[i].param_decl(false, false);
            if (initializer_t* init = _vars[i].initializer()) {
                b << " = ";
                init->initializer(b, false);
            }
            break;
        case pl_assign_moves_named:
            b << "  new ((void*) &" << TAME_CLOSURE_NAME "->"
              << _vars[i].name(false, false) << ") ("
              << _vars[i].decl(false) << ")"
              << "(" << (_vars[i].type().is_ref() ? "&" : "")
              << _vars[i].name(true, false) << ");\n";
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
    b << "__tamer_closure_" << s;
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
    b << "  static void tamer_activator_(tamer::tamerpriv::closure* super) {\n"
      << "    " << closure_type_name(true) << "* self = static_cast<"
      << closure_type_name(true) << "*>(super);\n"
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

    if (class_template_.length() || function_template_.length())
        add_templates(b, "\n");
    b << "class " << closure_type_name(false)
      << " : public tamer::tamerpriv::closure {\npublic:\n";

    output_reenter(b);

    if (_class.length() && !(_opts & STATIC_DECL))
        b << "  " << _self.decl(true) << ";\n";
    if (_args)
        _args->closure_declarations(b, "    ");
    _stack_vars.closure_declarations(b, "    ");

    if (need_implicit_rendezvous())
        b << "  tamer::gather_rendezvous " TWAIT_BLOCK_RENDEZVOUS ";\n";

    b << "};\n\n";

    o->output_str(b.str());
    o->switch_to_mode(om);
}

void var_t::reference_declaration(strbuf& b, const str& padding) const {
    if (!name().empty()) {
        b << padding << ref_decl(false) << " TAMER_CLOSUREVARATTR = ";
        if (type().is_ref())
            b << "*";
        b << TAME_CLOSURE_NAME "." << name() << ";\n";
    }
}
void vartab_t::reference_declarations(strbuf& b, const str& padding) const {
    for (unsigned i = 0; i != size(); ++i)
        _vars[i].reference_declaration(b, padding);
}

void
tame_fn_t::output_jump_tab(strbuf &b)
{
    b << "  switch (" << TAME_CLOSURE_NAME << ".tamer_block_position_) {\n"
      << "  case 0: break;\n";
    for (unsigned i = 0; i < _envs.size(); i++) {
        if (_envs[i]->is_jumpto()) {
            int id_tmp = _envs[i]->id();
            assert(id_tmp);
            b << "  case " << id_tmp << ":\n"
              << "    goto " << label(id_tmp) << ";\n"
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
      << mk_closure(true, true).ref_decl(true) << ")";
    if (_isconst)
        b << " const";
    return b.str();
}

void
tame_fn_t::output_firstfn(outputter_t *o)
{
    state->set_fn(this);
    output_mode_t om = o->switch_to_mode(OUTPUT_PASSTHROUGH);
    str closure_type = closure(true).type().base_type();

    strbuf b;
    b << signature() << "\n{\n";

    b << "  " << closure(true).decl(true) << " = "
      << "std::allocator< " << closure(true).type().base_type() << " >()."
      << "allocate(1);\n"
      << "  ((typename " << closure_type << "::tamer_closure_type*) " TAME_CLOSURE_NAME ")->initialize_closure("
      << closure(true).type().base_type() << "::tamer_activator_";
    if (_class.length() && !(_opts & STATIC_DECL))
        b << ", this";
    b << ");\n";
    if (_class.length() && !(_opts & STATIC_DECL))
        b << "  " << TAME_CLOSURE_NAME "->" TAMER_SELF_NAME " = this;\n";
    if (_args)
        _args->paramlist(b, vartab_t::pl_assign_moves_named, "");
    if (need_implicit_rendezvous())
        b << "  new ((void*) &" TAME_CLOSURE_NAME "->" TWAIT_BLOCK_RENDEZVOUS
            ") tamer::gather_rendezvous;\n";
    b << "  " << TAME_CLOSURE_NAME << "->tamer_activator_("
      << TAME_CLOSURE_NAME << ");\n}\n";

    o->output_str(b.str());
    o->switch_to_mode(om);
}

void
tame_fn_t::output_fn(outputter_t *o)
{
    strbuf b;
    state->set_fn(this);

    output_mode_t om = o->switch_to_mode(OUTPUT_PASSTHROUGH);
    b << closure_signature() << "\n{\n";

    o->output_str(b.str());

    // If no vars section was specified, do it now.
    if (!_vars)
        output_vars(o, _lbrace_lineno);

    element_list_t::output(o);

    o->output_str("}\n");
    o->switch_to_mode(om);
}

void
tame_vars_t::output(outputter_t *o)
{
    _fn->output_vars(o, _lineno);
}

void
tame_fn_t::output_vars(outputter_t *o, int ln)
{
    strbuf b;
    output_mode_t om = o->switch_to_mode(OUTPUT_TREADMILL, ln);

    if (_args)
        _args->reference_declarations(b, "  ");
    b << "  tamer::tamerpriv::closure_owner<" << closure_type_name(true)
      << " > tamer_closure_holder_(" << TAME_CLOSURE_NAME << ");\n";

    _stack_vars.initializers_and_reference_declarations(b, o, this);

    output_jump_tab(b);

    o->output_str(b.str());
    // will switch modes as appropriate (internally)
    o->switch_to_mode(om);
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
      b << "#define make_event(...) make_annotated_event(__FILE__, __LINE__, " TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ", ## __VA_ARGS__)\n"
        << "#define make_preevent(...) make_annotated_preevent(__FILE__, __LINE__, " TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ", ## __VA_ARGS__)\n";
  else
      b << "#define make_event(...) make_event(" TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ", ## __VA_ARGS__)\n"
        << "#define make_preevent(...) make_preevent(" TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ", ## __VA_ARGS__)\n";
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
    << "    " TAME_CLOSURE_NAME ".set_location(__FILE__, __LINE__);\n";
  if (!description_empty(description_) && tamer_debug)
      b << "    " TAME_CLOSURE_NAME ".set_description((" << description_ << "));\n";
  b << _fn->label(_id) << ":\n"
    << "    if (" TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ".has_waiting()) {\n"
    << "        " TAME_CLOSURE_NAME "." TWAIT_BLOCK_RENDEZVOUS ".block(" TAME_CLOSURE_NAME ", " << _id << ");\n"
    << "        tamer_closure_holder_.reset();\n"
    << "        " << _fn->return_expr() << "; }}\n"
    << "  } while (0);\n";
  o->output_str(b.str());
  o->switch_to_mode(OUTPUT_PASSTHROUGH);
  o->output_str("\n#undef make_event\n#undef make_preevent\n");
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

void
tame_wait_t::output_blocked(strbuf &b, const str &jgn)
{
    b << "    " TAME_CLOSURE_NAME ".set_location(__FILE__, __LINE__);\n";
    if (!description_empty(description_) && tamer_debug)
        b << "    " TAME_CLOSURE_NAME ".set_description((" << description_ << "));\n";
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
    o->set_lineno(lineno_, b);
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
