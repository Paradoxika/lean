/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
        Soonho Kong
*/
#include <vector>
#include <sstream>
#include <string>
#include <algorithm>
#include "util/list_fn.h"
#include "util/hash.h"
#include "util/buffer.h"
#include "util/object_serializer.h"
#include "kernel/expr.h"
#include "kernel/expr_eq_fn.h"
#include "kernel/free_vars.h"
#include "kernel/max_sharing.h"

namespace lean {
static expr g_dummy(mk_var(0));
expr::expr():expr(g_dummy) {}

unsigned hash_levels(levels const & ls) {
    unsigned r = 23;
    for (auto const & l : ls)
        r = hash(hash(l), r);
    return r;
}

bool has_meta(levels const & ls) {
    for (auto const & l : ls) {
        if (has_meta(l))
            return true;
    }
    return false;
}

expr_cell::expr_cell(expr_kind k, unsigned h, bool has_mv, bool has_local):
    m_kind(static_cast<unsigned>(k)),
    m_flags(0),
    m_has_mv(has_mv),
    m_has_local(has_local),
    m_hash(h),
    m_rc(0) {
    // m_hash_alloc does not need to be a unique identifier.
    // We want diverse hash codes because given expr_cell * c1 and expr_cell * c2,
    // if c1 != c2, then there is high probability c1->m_hash_alloc != c2->m_hash_alloc.
    // Remark: using pointer address as a hash code is not a good idea.
    //    - each execution run will behave differently.
    //    - the hash is not diverse enough
    static LEAN_THREAD_LOCAL unsigned g_hash_alloc_counter = 0;
    m_hash_alloc = g_hash_alloc_counter;
    g_hash_alloc_counter++;
}

void expr_cell::dec_ref(expr & e, buffer<expr_cell*> & todelete) {
    if (e.m_ptr) {
        expr_cell * c = e.steal_ptr();
        lean_assert(!(e.m_ptr));
        if (c->dec_ref_core())
            todelete.push_back(c);
    }
}

optional<bool> expr_cell::is_arrow() const {
    // it is stored in bits 2-3
    unsigned r = (m_flags & (4+8)) >> 2;
    if (r == 0) {
        return optional<bool>();
    } else if (r == 1) {
        return optional<bool>(true);
    } else {
        lean_assert(r == 2);
        return optional<bool>(false);
    }
}

void expr_cell::set_is_arrow(bool flag) {
    unsigned mask = flag ? 4 : 8;
    m_flags |= mask;
    lean_assert(is_arrow() && *is_arrow() == flag);
}

// Expr variables
expr_var::expr_var(unsigned idx):
    expr_cell(expr_kind::Var, idx, false, false),
    m_vidx(idx) {}

// Expr constants
expr_const::expr_const(name const & n, levels const & ls):
    expr_cell(expr_kind::Constant, ::lean::hash(n.hash(), hash_levels(ls)), has_meta(ls), false),
    m_name(n),
    m_levels(ls) {}

// Expr metavariables and local variables
expr_mlocal::expr_mlocal(bool is_meta, name const & n, expr const & t):
    expr_cell(is_meta ? expr_kind::Meta : expr_kind::Local, n.hash(), is_meta || t.has_metavar(), !is_meta || t.has_local()),
    m_name(n),
    m_type(t) {}
void expr_mlocal::dealloc(buffer<expr_cell*> & todelete) {
    dec_ref(m_type, todelete);
    delete(this);
}

// Composite expressions
expr_composite::expr_composite(expr_kind k, unsigned h, bool has_mv, bool has_local, unsigned d):
    expr_cell(k, h, has_mv, has_local),
    m_depth(d) {}

// Expr dependent pairs
expr_dep_pair::expr_dep_pair(expr const & f, expr const & s, expr const & t):
    expr_composite(expr_kind::Pair, ::lean::hash(f.hash(), s.hash()),
                   f.has_metavar() || s.has_metavar() || t.has_metavar(),
                   f.has_local() || s.has_local() || t.has_local(),
                   std::max(get_depth(f), get_depth(s))+1),
    m_first(f), m_second(s), m_type(t) {
}
void expr_dep_pair::dealloc(buffer<expr_cell*> & todelete) {
    dec_ref(m_first,  todelete);
    dec_ref(m_second, todelete);
    dec_ref(m_type,   todelete);
    delete(this);
}

// Expr pair projection
expr_proj::expr_proj(bool f, expr const & e):
    expr_composite(f ? expr_kind::Fst : expr_kind::Snd, ::lean::hash(17, e.hash()), e.has_metavar(), e.has_local(), get_depth(e)+1),
    m_expr(e) {}
void expr_proj::dealloc(buffer<expr_cell*> & todelete) {
    dec_ref(m_expr, todelete);
    delete(this);
}

// Expr applications
expr_app::expr_app(expr const & fn, expr const & arg):
    expr_composite(expr_kind::App, ::lean::hash(fn.hash(), arg.hash()),
                   fn.has_metavar() || arg.has_metavar(),
                   fn.has_local()   || arg.has_local(),
                   std::max(get_depth(fn), get_depth(arg)) + 1),
    m_fn(fn), m_arg(arg) {}
void expr_app::dealloc(buffer<expr_cell*> & todelete) {
    dec_ref(m_fn, todelete);
    dec_ref(m_arg, todelete);
    delete(this);
}

// Expr binders (Lambda, Pi and Sigma)
expr_binder::expr_binder(expr_kind k, name const & n, expr const & t, expr const & b):
    expr_composite(k, ::lean::hash(t.hash(), b.hash()),
                   t.has_metavar() || b.has_metavar(),
                   t.has_local() || b.has_local(),
                   std::max(get_depth(t), get_depth(b)) + 1),
    m_name(n),
    m_domain(t),
    m_body(b) {
    lean_assert(k == expr_kind::Lambda || k == expr_kind::Pi || k == expr_kind::Sigma);
}
void expr_binder::dealloc(buffer<expr_cell*> & todelete) {
    dec_ref(m_body, todelete);
    dec_ref(m_domain, todelete);
    delete(this);
}

// Expr Sort
expr_sort::expr_sort(level const & l):
    expr_cell(expr_kind::Sort, ::lean::hash(l), has_meta(l), false),
    m_level(l) {
}
expr_sort::~expr_sort() {}

// Expr Let
expr_let::expr_let(name const & n, expr const & t, expr const & v, expr const & b):
    expr_composite(expr_kind::Let, ::lean::hash(v.hash(), b.hash()),
                   t.has_metavar() || v.has_metavar() || b.has_metavar(),
                   t.has_local() || v.has_local() || b.has_local(),
                   std::max({get_depth(t), get_depth(v), get_depth(b)}) + 1),
    m_name(n),
    m_type(t),
    m_value(v),
    m_body(b) {
}
void expr_let::dealloc(buffer<expr_cell*> & todelete) {
    dec_ref(m_body, todelete);
    dec_ref(m_value, todelete);
    dec_ref(m_type, todelete);
    delete(this);
}
expr_let::~expr_let() {}

// Macro attachment
int macro::push_lua(lua_State *) const { return 0; } // NOLINT
void macro::display(std::ostream & out) const { out << get_name(); }
bool macro::operator==(macro const & other) const { return typeid(*this) == typeid(other); }
bool macro::operator<(macro const & other) const {
    if (get_name() == other.get_name())
        return lt(other);
    else
        return get_name() < other.get_name();
}
format macro::pp(formatter const &, options const &) const { return format(get_name()); }
bool macro::is_atomic_pp(bool, bool) const { return true; }
unsigned macro::hash() const { return get_name().hash(); }

typedef std::unordered_map<std::string, macro::reader> macro_readers;
static std::unique_ptr<macro_readers> g_macro_readers;
macro_readers & get_macro_readers() {
    if (!g_macro_readers)
        g_macro_readers.reset(new macro_readers());
    return *(g_macro_readers.get());
}

void macro::register_deserializer(std::string const & k, macro::reader rd) {
    macro_readers & readers = get_macro_readers();
    lean_assert(readers.find(k) == readers.end());
    readers[k] = rd;
}
static expr read_macro(deserializer & d) {
    auto k  = d.read_string();
    macro_readers & readers = get_macro_readers();
    auto it = readers.find(k);
    lean_assert(it != readers.end());
    return it->second(d);
}

expr_macro::expr_macro(macro * m):
    expr_cell(expr_kind::Macro, m->hash(), false, false),
    m_macro(m) {
    m_macro->inc_ref();
}
expr_macro::~expr_macro() {
    m_macro->dec_ref();
}

void expr_cell::dealloc() {
    try {
        buffer<expr_cell*> todo;
        todo.push_back(this);
        while (!todo.empty()) {
            expr_cell * it = todo.back();
            todo.pop_back();
            lean_assert(it->get_rc() == 0);
            switch (it->kind()) {
            case expr_kind::Var:        delete static_cast<expr_var*>(it); break;
            case expr_kind::Macro:      delete static_cast<expr_macro*>(it); break;
            case expr_kind::Meta:
            case expr_kind::Local:      static_cast<expr_mlocal*>(it)->dealloc(todo); break;
            case expr_kind::Constant:   delete static_cast<expr_const*>(it); break;
            case expr_kind::Sort:       delete static_cast<expr_sort*>(it); break;
            case expr_kind::Pair:       static_cast<expr_dep_pair*>(it)->dealloc(todo); break;
            case expr_kind::Fst:
            case expr_kind::Snd:        static_cast<expr_proj*>(it)->dealloc(todo); break;
            case expr_kind::App:        static_cast<expr_app*>(it)->dealloc(todo); break;
            case expr_kind::Lambda:
            case expr_kind::Pi:
            case expr_kind::Sigma:      static_cast<expr_binder*>(it)->dealloc(todo); break;
            case expr_kind::Let:        static_cast<expr_let*>(it)->dealloc(todo); break;
            }
        }
    } catch (std::bad_alloc&) {
        // We need this catch, because push_back may fail when expanding the buffer.
        // In this case, we avoid the crash, and "accept" the memory leak.
    }
}

// Auxiliary constructors
expr mk_app(unsigned num_args, expr const * args) {
    lean_assert(num_args >= 2);
    expr r = mk_app(args[0], args[1]);
    for (unsigned i = 2; i < num_args; i++)
        r = mk_app(r, args[i]);
    return r;
}

static name g_default_var_name("a");
bool is_default_var_name(name const & n) { return n == g_default_var_name; }
expr mk_arrow(expr const & t, expr const & e) { return mk_pi(g_default_var_name, t, e); }
expr mk_cartesian_product(expr const & t, expr const & e) { return mk_sigma(g_default_var_name, t, e); }

expr Bool = mk_sort(mk_level_zero());
expr Type = mk_sort(mk_level_one());
expr mk_Bool() { return Bool; }
expr mk_Type() { return Type; }

unsigned get_depth(expr const & e) {
    switch (e.kind()) {
    case expr_kind::Var:  case expr_kind::Constant: case expr_kind::Sort:
    case expr_kind::Meta: case expr_kind::Local:    case expr_kind::Macro:
        return 1;
    case expr_kind::Lambda: case expr_kind::Pi:   case expr_kind::Sigma:
    case expr_kind::Pair:   case expr_kind::Fst:  case expr_kind::Snd:
    case expr_kind::App:    case expr_kind::Let:
        return static_cast<expr_composite*>(e.raw())->m_depth;
    }
    lean_unreachable(); // LCOV_EXCL_LINE
}

bool operator==(expr const & a, expr const & b) { return expr_eq_fn()(a, b); }

expr update_app(expr const & e, expr const & new_fn, expr const & new_arg) {
    if (!is_eqp(app_fn(e), new_fn) || !is_eqp(app_arg(e), new_arg))
        return mk_app(new_fn, new_arg);
    else
        return e;
}

expr update_proj(expr const & e, expr const & new_arg) {
    if (!is_eqp(proj_arg(e), new_arg))
        return mk_proj(is_fst(e), new_arg);
    else
        return e;
}

expr update_pair(expr const & e, expr const & new_first, expr const & new_second, expr const & new_type) {
    if (!is_eqp(pair_first(e), new_first) || !is_eqp(pair_second(e), new_second) || !is_eqp(pair_type(e), new_type))
        return mk_pair(new_first, new_second, new_type);
    else
        return e;
}

expr update_binder(expr const & e, expr const & new_domain, expr const & new_body) {
    if (!is_eqp(binder_domain(e), new_domain) || !is_eqp(binder_body(e), new_body))
        return mk_binder(e.kind(), binder_name(e), new_domain, new_body);
    else
        return e;
}

expr update_let(expr const & e, expr const & new_type, expr const & new_val, expr const & new_body) {
    if (!is_eqp(let_type(e), new_type) || !is_eqp(let_value(e), new_val) || !is_eqp(let_body(e), new_body))
        return mk_let(let_name(e), new_type, new_val, new_body);
    else
        return e;
}

expr update_mlocal(expr const & e, expr const & new_type) {
    if (!is_eqp(mlocal_type(e), new_type))
        return mk_mlocal(is_metavar(e), mlocal_name(e), new_type);
    else
        return e;
}

bool is_atomic(expr const & e) {
    switch (e.kind()) {
    case expr_kind::Constant: case expr_kind::Sort:
    case expr_kind::Macro:    case expr_kind::Var:
        return true;
    case expr_kind::App:      case expr_kind::Let:
    case expr_kind::Meta:     case expr_kind::Local:
    case expr_kind::Lambda:   case expr_kind::Pi:  case expr_kind::Sigma:
    case expr_kind::Pair:     case expr_kind::Fst: case expr_kind::Snd:
        return false;
    }
    lean_unreachable(); // LCOV_EXCL_LINE
}

bool is_arrow(expr const & t) {
    optional<bool> r = t.raw()->is_arrow();
    if (r) {
        return *r;
    } else {
        bool res = is_pi(t) && !has_free_var(binder_body(t), 0);
        t.raw()->set_is_arrow(res);
        return res;
    }
}

bool is_cartesian(expr const & t) {
    return is_sigma(t) && !has_free_var(binder_body(t), 0);
}

expr copy(expr const & a) {
    switch (a.kind()) {
    case expr_kind::Var:      return mk_var(var_idx(a));
    case expr_kind::Constant: return mk_constant(const_name(a), const_level_params(a));
    case expr_kind::Sort:     return mk_sort(sort_level(a));
    case expr_kind::Macro:    return mk_macro(static_cast<expr_macro*>(a.raw())->m_macro);
    case expr_kind::Pair:     return mk_pair(pair_first(a), pair_second(a), pair_type(a));
    case expr_kind::Fst:      return mk_fst(proj_arg(a));
    case expr_kind::Snd:      return mk_snd(proj_arg(a));
    case expr_kind::App:      return mk_app(app_fn(a), app_arg(a));
    case expr_kind::Lambda:   return mk_lambda(binder_name(a), binder_domain(a), binder_body(a));
    case expr_kind::Pi:       return mk_pi(binder_name(a), binder_domain(a), binder_body(a));
    case expr_kind::Sigma:    return mk_sigma(binder_name(a), binder_domain(a), binder_body(a));
    case expr_kind::Let:      return mk_let(let_name(a), let_type(a), let_value(a), let_body(a));
    case expr_kind::Meta:     return mk_metavar(mlocal_name(a), mlocal_type(a));
    case expr_kind::Local:    return mk_local(mlocal_name(a), mlocal_type(a));
    }
    lean_unreachable(); // LCOV_EXCL_LINE
}

serializer & operator<<(serializer & s, levels const & ls) {
    s << length(ls);
    for (auto const & l : ls)
        s << l;
    return s;
}

levels read_levels(deserializer & d) {
    unsigned num = d.read_unsigned();
    buffer<level> ls;
    for (unsigned i = 0; i < num; i++)
        ls.push_back(read_level(d));
    return to_list(ls.begin(), ls.end());
}

class expr_serializer : public object_serializer<expr, expr_hash_alloc, expr_eqp> {
    typedef object_serializer<expr, expr_hash_alloc, expr_eqp> super;
    max_sharing_fn m_max_sharing_fn;

    void write_core(expr const & a) {
        auto k = a.kind();
        super::write_core(a, static_cast<char>(k), [&]() {
                serializer & s = get_owner();
                switch (k) {
                case expr_kind::Var:
                    s << var_idx(a);
                    break;
                case expr_kind::Constant:
                    s << const_name(a) << const_level_params(a);
                    break;
                case expr_kind::Sort:
                    s << sort_level(a);
                    break;
                case expr_kind::Macro:
                    to_macro(a).write(s);
                    break;
                case expr_kind::Pair:
                    write_core(pair_first(a)); write_core(pair_second(a)); write_core(pair_type(a));
                    break;
                case expr_kind::Fst:  case expr_kind::Snd:
                    write_core(proj_arg(a));
                    break;
                case expr_kind::App:
                    write_core(app_fn(a)); write_core(app_arg(a));
                    break;
                case expr_kind::Lambda: case expr_kind::Pi: case expr_kind::Sigma:
                    s << binder_name(a); write_core(binder_domain(a)); write_core(binder_body(a));
                    break;
                case expr_kind::Let:
                    s << let_name(a); write_core(let_type(a)); write_core(let_value(a)); write_core(let_body(a));
                    break;
                case expr_kind::Meta: case expr_kind::Local:
                    s << mlocal_name(a); write_core(mlocal_type(a));
                    break;
                }
            });
    }
public:
    void write(expr const & a) {
        write_core(m_max_sharing_fn(a));
    }
};

class expr_deserializer : public object_deserializer<expr> {
    typedef object_deserializer<expr> super;
public:
    expr read_binder(expr_kind k) {
        deserializer & d = get_owner();
        name n = read_name(d);
        expr t = read();
        return mk_binder(k, n, t, read());
    }

    expr read() {
        return super::read_core([&](char c) {
                deserializer & d = get_owner();
                auto k = static_cast<expr_kind>(c);
                switch (k) {
                case expr_kind::Var:
                    return mk_var(d.read_unsigned());
                case expr_kind::Constant: {
                    auto n = read_name(d);
                    return mk_constant(n, read_levels(d));
                }
                case expr_kind::Sort:
                    return mk_sort(read_level(d));
                    break;
                case expr_kind::Macro:
                    return read_macro(d);
                case expr_kind::Pair: {
                    expr f = read();
                    expr s = read();
                    return mk_pair(f, s, read());
                }
                case expr_kind::Fst:
                    return mk_fst(read());
                case expr_kind::Snd:
                    return mk_snd(read());
                case expr_kind::App: {
                    expr f = read();
                    return mk_app(f, read());
                }
                case expr_kind::Lambda: case expr_kind::Pi: case expr_kind::Sigma:
                    return read_binder(k);
                case expr_kind::Let: {
                    name n = read_name(d);
                    expr t = read();
                    expr v = read();
                    return mk_let(n, t, v, read());
                }
                case expr_kind::Meta: {
                    name n = read_name(d);
                    return mk_metavar(n, read());
                }
                case expr_kind::Local: {
                    name n = read_name(d);
                    return mk_local(n, read());
                }}
                throw_corrupted_file(); // LCOV_EXCL_LINE
            });
    }
};

struct expr_sd {
    unsigned m_s_extid;
    unsigned m_d_extid;
    expr_sd() {
        m_s_extid = serializer::register_extension([](){ return std::unique_ptr<serializer::extension>(new expr_serializer()); });
        m_d_extid = deserializer::register_extension([](){ return std::unique_ptr<deserializer::extension>(new expr_deserializer()); });
    }
};
static expr_sd g_expr_sd;

serializer & operator<<(serializer & s, expr const & n) {
    s.get_extension<expr_serializer>(g_expr_sd.m_s_extid).write(n);
    return s;
}

expr read_expr(deserializer & d) {
    return d.get_extension<expr_deserializer>(g_expr_sd.m_d_extid).read();
}
}
