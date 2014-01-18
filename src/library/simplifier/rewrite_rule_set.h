/*
Copyright (c) 2014 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#pragma once
#include <memory>
#include <functional>
#include "kernel/environment.h"
#include "kernel/formatter.h"

namespace lean {
class rewrite_rule_set {
    class imp;
    std::unique_ptr<imp> m_ptr;
public:
    rewrite_rule_set(ro_environment const & env);
    rewrite_rule_set(rewrite_rule_set const & rs);
    ~rewrite_rule_set();
    /**
       \brief Convert the theorem \c th with proof \c proof into conditional rewrite rules, and
       insert the rules into this rule set. The new rules are tagged with the given \c id.
    */
    void insert(name const & id, expr const & th, expr const & proof);
    /**
       \brief Convert the theorem/axiom named \c th_name in the environment into conditional rewrite rules,
       and insert the rules into this rule set. The new rules are tagged with the theorem name.

       This method throws an exception if the environment does not have a theorem/axiom named \c th_name.
    */
    void insert(name const & th_name);

    /**
       \brief Enable/disable the conditional rewrite rules tagged with the given identifier.
    */
    void enable(name const & id, bool flag);
    /**
       \brief Return true iff the conditional rewrite rules tagged with the given id are enabled.
    */
    bool enabled(name const & id) const;

    typedef std::function<bool(expr const &, expr const &, bool is_permutation, expr const &)> match_fn;
    typedef std::function<void(name const &, expr const &, expr const &, bool)> visit_fn;

    /**
       \brief Execute <tt>fn(lhs, ceq, is_perm, proof)</tt> for each (enabled) rule whose the left-hand-side may
       match \c e.
       The traversal is interrupted as soon as \c fn returns true.

       The redundant argument \c lhs is the left-hand-side of \c ceq.
       The redundant argument \c is_perm is true iff \c ceq is a permutation rule.

       The argument \c proof is the proof for \c ceq.
    */
    void for_each_match_candidate(expr const & e, match_fn const & fn);

    /**
       \brief Execute <tt>fn(id, ceq, proof, enabled)</tt> for each rule in this rule set.
    */
    void for_each(visit_fn const & fn);

    /**
       \brief Pretty print this rule set.
    */
    format pp(formatter const & fmt, options const & opts) const;
};
}