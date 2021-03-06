import kernel
import macros

variable Nat : Type
alias ℕ : Nat

namespace Nat
builtin numeral

builtin add : Nat → Nat → Nat
infixl 65 +  : add

builtin mul : Nat → Nat → Nat
infixl 70 *  : mul

builtin le  : Nat → Nat → Bool
infix  50 <= : le
infix  50 ≤  : le

definition ge (a b : Nat) := b ≤ a
infix 50 >= : ge
infix 50 ≥  : ge

definition lt (a b : Nat) := a + 1 ≤ b
infix 50 <  : lt

definition gt (a b : Nat) := b < a
infix 50 >  : gt

definition id (a : Nat) := a
notation 55 | _ | : id

axiom succ_nz (a : Nat) : a + 1 ≠ 0
axiom succ_inj {a b : Nat} (H : a + 1 = b + 1) : a = b
axiom add_zeror (a : Nat)   : a + 0 = a
axiom add_succr (a b : Nat) : a + (b + 1) = (a + b) + 1
axiom mul_zeror (a : Nat)    : a * 0 = 0
axiom mul_succr (a b : Nat)  : a * (b + 1) = a * b + a
axiom le_def (a b : Nat) : a ≤ b ↔ ∃ c, a + c = b
axiom induction {P : Nat → Bool} (H1 : P 0) (H2 : ∀ (n : Nat) (iH : P n), P (n + 1)) : ∀ a, P a

theorem induction_on {P : Nat → Bool} (a : Nat) (H1 : P 0) (H2 : ∀ (n : Nat) (iH : P n), P (n + 1)) : P a
:= induction H1 H2 a

theorem pred_nz {a : Nat} : a ≠ 0 → ∃ b, b + 1 = a
:= induction_on a
    (λ H : 0 ≠ 0, false_elim (∃ b, b + 1 = 0) (a_neq_a_elim H))
    (λ (n : Nat) (iH : n ≠ 0 → ∃ b, b + 1 = n) (H : n + 1 ≠ 0),
       or_elim (em (n = 0))
           (λ Heq0 : n = 0, exists_intro 0 (calc 0 + 1 = n + 1 : { symm Heq0 }))
           (λ Hne0 : n ≠ 0,
                 obtain (w : Nat) (Hw : w + 1 = n), from (iH Hne0),
                    exists_intro (w + 1) (calc w + 1 + 1 = n + 1 : { Hw })))

theorem discriminate {B : Bool} {a : Nat} (H1: a = 0 → B) (H2 : ∀ n, a = n + 1 → B) : B
:= or_elim (em (a = 0))
             (λ Heq0 : a = 0, H1 Heq0)
             (λ Hne0 : a ≠ 0, obtain (w : Nat) (Hw : w + 1 = a), from (pred_nz Hne0),
                  H2 w (symm Hw))

theorem add_zerol (a : Nat) : 0 + a = a
:= induction_on a
    (show 0 + 0 = 0, from add_zeror 0)
    (λ (n : Nat) (iH : 0 + n = n),
        calc  0 + (n + 1)  =  (0 + n) + 1   :  add_succr 0 n
                    ...    =  n + 1         :  { iH })

theorem add_succl (a b : Nat) : (a + 1) + b = (a + b) + 1
:= induction_on b
    (calc (a + 1) + 0   =  a + 1        :  add_zeror (a + 1)
                  ...   =  (a + 0) + 1  :  { symm (add_zeror a) })
    (λ (n : Nat) (iH : (a + 1) + n = (a + n) + 1),
        calc   (a + 1) + (n + 1)   =   ((a + 1) + n) + 1  :  add_succr (a + 1) n
                              ...  =   ((a + n) + 1) + 1  :  { iH }
                              ...  =   (a + (n + 1)) + 1  :  { show (a + n) + 1 = a + (n + 1), from symm (add_succr a n) })

theorem add_comm (a b : Nat) : a + b = b + a
:= induction_on b
    (calc a + 0   = a     : add_zeror a
            ...   = 0 + a : symm (add_zerol a))
    (λ (n : Nat) (iH : a + n = n + a),
        calc   a + (n + 1)   =   (a + n) + 1 : add_succr a n
                       ...   =   (n + a) + 1 : { iH }
                       ...   =   (n + 1) + a : symm (add_succl n a))

theorem add_assoc (a b c : Nat) : (a + b) + c = a + (b + c)
:= symm (induction_on a
    (calc 0 + (b + c)  =  b + c        : add_zerol (b + c)
                  ...  =  (0 + b) + c  : { symm (add_zerol b) })
    (λ (n : Nat) (iH : n + (b + c) = (n + b) + c),
        calc (n + 1) + (b + c)   =    (n + (b + c)) + 1   :  add_succl n (b + c)
                          ...    =    ((n + b) + c) + 1   :  { iH }
                          ...    =    ((n + b) + 1) + c   :  symm (add_succl (n + b) c)
                          ...    =    ((n + 1) + b) + c   :  { show (n + b) + 1 = (n + 1) + b, from symm (add_succl n b) }))

theorem mul_zerol (a : Nat) : 0 * a = 0
:= induction_on a
    (show 0 * 0 = 0, from mul_zeror 0)
    (λ (n : Nat) (iH : 0 * n = 0),
        calc  0 * (n + 1)  =  (0 * n) + 0 : mul_succr 0 n
                      ...  =  0 + 0       : { iH }
                      ...  =  0           : add_zerol 0)

theorem mul_succl (a b : Nat) : (a + 1) * b = a * b + b
:= induction_on b
    (calc (a + 1) * 0   =  0         : mul_zeror (a + 1)
                   ...  =  a * 0     : symm (mul_zeror a)
                   ...  =  a * 0 + 0 : symm (add_zeror (a * 0)))
    (λ (n : Nat) (iH : (a + 1) * n = a * n + n),
        calc   (a + 1) * (n + 1)  =    (a + 1) * n + (a + 1)  :  mul_succr (a + 1) n
                            ...   = a * n + n + (a + 1)       :  { iH }
                            ...   = a * n + n + a + 1         :  symm (add_assoc (a * n + n) a 1)
                            ...   = a * n + (n + a) + 1       :  { show  a * n + n + a = a * n + (n + a),
                                                                   from add_assoc (a * n) n a }
                            ...   = a * n + (a + n) + 1       :  { add_comm n a }
                            ...   = a * n + a + n + 1         :  { symm (add_assoc (a * n) a n) }
                            ...   = a * (n + 1) + n + 1       :  { symm (mul_succr a n) }
                            ...   = a * (n + 1) + (n + 1)     :  add_assoc (a * (n + 1)) n 1)

theorem mul_onel (a : Nat) : 1 * a = a
:= induction_on a
    (show 1 * 0 = 0, from mul_zeror 1)
    (λ (n : Nat) (iH : 1 * n = n),
        calc  1 * (n + 1)  =  1 * n + 1 :  mul_succr 1 n
                      ...  =  n + 1     : { iH })

theorem mul_oner (a : Nat) : a * 1 = a
:= induction_on a
    (show 0 * 1 = 0, from mul_zeror 1)
    (λ (n : Nat) (iH : n * 1 = n),
        calc  (n + 1) * 1  =  n * 1 + 1 : mul_succl n 1
                      ...  =  n + 1     : { iH })

theorem mul_comm (a b : Nat) : a * b = b * a
:= induction_on b
    (calc a * 0  = 0      : mul_zeror a
            ...  = 0 * a  : symm (mul_zerol a))
    (λ (n : Nat) (iH : a * n = n * a),
         calc  a * (n + 1)   =   a * n + a   : mul_succr a n
                       ...   =   n * a + a   : { iH }
                       ...   =   (n + 1) * a : symm (mul_succl n a))

theorem distributer (a b c : Nat) : a * (b + c) = a * b + a * c
:= induction_on a
    (calc 0 * (b + c)   = 0              : mul_zerol (b + c)
                  ...   = 0 + 0          : add_zeror 0
                  ...   = 0 * b + 0      : { symm (mul_zerol b) }
                  ...   = 0 * b + 0 * c  : { symm (mul_zerol c) })
    (λ (n : Nat) (iH : n * (b + c) = n * b + n * c),
        calc  (n + 1) * (b + c)  =   n * (b + c) + (b + c)     : mul_succl n (b + c)
                            ...  =   n * b + n * c + (b + c)   : { iH }
                            ...  =   n * b + n * c + b + c     : symm (add_assoc (n * b + n * c) b c)
                            ...  =   n * b + (n * c + b) + c   : { add_assoc (n * b) (n * c) b }
                            ...  =   n * b + (b + n * c) + c   : { add_comm (n * c) b }
                            ...  =   n * b + b + n * c + c     : { symm (add_assoc (n * b) b (n * c)) }
                            ...  =   (n + 1) * b + n * c + c   : { symm (mul_succl n b) }
                            ...  =   (n + 1) * b + (n * c + c) : add_assoc ((n + 1) * b) (n * c) c
                            ...  =   (n + 1) * b + (n + 1) * c : { symm (mul_succl n c) })

theorem distributel (a b c : Nat) : (a + b) * c = a * c + b * c
:= calc (a + b) * c  =  c * (a + b)    :  mul_comm (a + b) c
                ...  =  c * a + c * b  :  distributer c a b
                ...  =  a * c + c * b  :  { mul_comm c a }
                ...  =  a * c + b * c  :  { mul_comm c b }

theorem mul_assoc (a b c : Nat) : (a * b) * c = a * (b * c)
:= symm (induction_on a
    (calc  0 * (b * c)    =  0           : mul_zerol (b * c)
                   ...    =  0 * c       : symm (mul_zerol c)
                   ...    =  (0 * b) * c : { symm (mul_zerol b) })
    (λ (n : Nat) (iH : n * (b * c) = n * b * c),
        calc  (n + 1) * (b * c)    =   n * (b * c) + (b * c)   :  mul_succl n (b * c)
                            ...    =   n * b * c + (b * c)     :  { iH }
                            ...    =   (n * b + b) * c         :  symm (distributel (n * b) b c)
                            ...    =   (n + 1) * b * c         :  { symm (mul_succl n b) }))

theorem add_left_comm (a b c : Nat) : a + (b + c) = b + (a + c)
:= left_comm add_comm add_assoc a b c

theorem mul_left_comm (a b c : Nat) : a * (b * c) = b * (a * c)
:= left_comm mul_comm mul_assoc a b c

theorem add_injr {a b c : Nat} : a + b = a + c → b = c
:= induction_on a
    (λ H : 0 + b = 0 + c,
        calc b   =  0 + b   : symm (add_zerol b)
           ...   =  0 + c   : H
           ...   =  c       : add_zerol c)
    (λ (n : Nat) (iH : n + b = n + c → b = c) (H : n + 1 + b = n + 1 + c),
       let L1 : n + b + 1 = n + c + 1
           := (calc n + b + 1  =  n + (b + 1)  : add_assoc n b 1
                          ...  =  n + (1 + b)  : { add_comm b 1 }
                          ...  =  n + 1 + b    : symm (add_assoc n 1 b)
                          ...  =  n + 1 + c    : H
                          ...  =  n + (1 + c)  : add_assoc n 1 c
                          ...  =  n + (c + 1)  : { add_comm 1 c }
                          ...  =  n + c + 1    : symm (add_assoc n c 1)),
           L2 : n + b = n + c := succ_inj L1
       in iH L2)

theorem add_injl {a b c : Nat} (H : a + b = c + b) : a = c
:= add_injr (calc b + a  =  a + b  : add_comm _ _
                   ...   =  c + b  : H
                   ...   =  b + c  : add_comm _ _)

theorem add_eqz {a b : Nat} (H : a + b = 0) : a = 0
:= discriminate
     (λ H1 : a = 0, H1)
     (λ (n : Nat) (H1 : a = n + 1),
         absurd_elim (a = 0)
            H (calc a + b  =  n + 1 + b   : { H1 }
                   ...  =  n + (1 + b) : add_assoc n 1 b
                   ...  =  n + (b + 1) : { add_comm 1 b }
                   ...  =  n + b + 1   : symm (add_assoc n b 1)
                   ...  ≠  0           : succ_nz (n + b)))

theorem le_intro {a b c : Nat} (H : a + c = b) : a ≤ b
:= (symm (le_def a b)) ◂ (show (∃ x, a + x = b), from exists_intro c H)

theorem le_elim {a b : Nat} (H : a ≤ b) : ∃ x, a + x = b
:= (le_def a b) ◂ H

theorem le_refl (a : Nat) : a ≤ a := le_intro (add_zeror a)

theorem le_zero (a : Nat) : 0 ≤ a := le_intro (add_zerol a)

theorem le_trans {a b c : Nat} (H1 : a ≤ b) (H2 : b ≤ c) : a ≤ c
:= obtain (w1 : Nat) (Hw1 : a + w1 = b), from (le_elim H1),
   obtain (w2 : Nat) (Hw2 : b + w2 = c), from (le_elim H2),
      le_intro (calc a + (w1 + w2) =  a + w1 + w2  :  symm (add_assoc a w1 w2)
                                ... =  b + w2         :  { Hw1 }
                                ... =  c              :  Hw2)

theorem le_add {a b : Nat} (H : a ≤ b) (c : Nat) : a + c ≤ b + c
:= obtain (w : Nat) (Hw : a + w = b), from (le_elim H),
      le_intro (calc a + c + w  = a + (c + w) :  add_assoc a c w
                            ...  = a + (w + c)   :  { add_comm c w }
                            ...  = a + w + c     :  symm (add_assoc a w c)
                            ...  = b + c         :  { Hw })

theorem le_antisym {a b : Nat} (H1 : a ≤ b) (H2 : b ≤ a) : a = b
:= obtain (w1 : Nat) (Hw1 : a + w1 = b), from (le_elim H1),
   obtain (w2 : Nat) (Hw2 : b + w2 = a), from (le_elim H2),
    let L1 : w1 + w2 = 0
           := add_injr (calc a + (w1 + w2) = a + w1 + w2 : { symm (add_assoc a w1 w2) }
                                       ... = b + w2        : { Hw1 }
                                       ... = a             : Hw2
                                       ... = a + 0         : symm (add_zeror a)),
        L2 : w1 = 0  := add_eqz L1
    in calc a  =  a  + 0  : symm (add_zeror a)
          ...  =  a  + w1 : { symm L2 }
          ...  =  b       : Hw1

theorem not_lt_0 (a : Nat) : ¬ a < 0
:= not_intro (λ H : a + 1 ≤ 0,
               obtain (w : Nat) (Hw1 : a + 1 + w = 0), from (le_elim H),
                  absurd
                    (calc a + w + 1  =  a + (w + 1)  : add_assoc _ _ _
                                ...  =  a + (1 + w)  : { add_comm _ _ }
                                ...  =  a + 1 + w    : symm (add_assoc _ _ _)
                                ...  =  0            : Hw1)
                    (succ_nz (a + w)))

theorem lt_intro {a b c : Nat} (H : a + 1 + c = b) : a < b
:= le_intro H

theorem lt_elim {a b : Nat} (H : a < b) : ∃ x, a + 1 + x = b
:= le_elim H

theorem lt_le {a b : Nat} (H : a < b) : a ≤ b
:= obtain (w : Nat) (Hw : a + 1 + w = b), from (le_elim H),
     le_intro (calc a + (1 + w) =  a + 1 + w  :  symm (add_assoc _ _ _)
                            ... =  b          : Hw)

theorem lt_ne {a b : Nat} (H : a < b) : a ≠ b
:= not_intro (λ H1 : a = b,
      obtain (w : Nat) (Hw : a + 1 + w = b), from (lt_elim H),
          absurd (calc w + 1 = 1 + w     : add_comm _ _
                        ...  = 0         :
                               add_injr (calc b + (1 + w)  = b + 1 + w   : symm (add_assoc b 1 w)
                                                    ...    = a + 1 + w   : { symm H1 }
                                                    ...    =  b          : Hw
                                                    ...    =  b + 0      : symm (add_zeror b)))
                 (succ_nz w))

theorem lt_nrefl (a : Nat) : ¬ a < a
:= not_intro (λ H : a < a,
      absurd (refl a) (lt_ne H))

theorem lt_trans {a b c : Nat} (H1 : a < b) (H2 : b < c) : a < c
:= obtain (w1 : Nat) (Hw1 : a + 1 + w1 = b), from (lt_elim H1),
   obtain (w2 : Nat) (Hw2 : b + 1 + w2 = c), from (lt_elim H2),
      lt_intro (calc a + 1 + (w1 + 1 + w2) = a + 1 + (w1 + (1 + w2))   : { add_assoc w1 1 w2 }
                                       ... = (a + 1 + w1) + (1 + w2)   : symm (add_assoc _ _ _)
                                       ... =  b + (1 + w2)             :  { Hw1 }
                                       ... =  b + 1 + w2               : symm (add_assoc _ _ _)
                                       ... =  c                        :  Hw2)

theorem lt_le_trans {a b c : Nat} (H1 : a < b) (H2 : b ≤ c) : a < c
:= obtain (w1 : Nat) (Hw1 : a + 1 + w1 = b), from (lt_elim H1),
   obtain (w2 : Nat) (Hw2 : b + w2 = c), from (le_elim H2),
      lt_intro (calc a + 1 + (w1 + w2) = a + 1 + w1 + w2   : symm (add_assoc _ _ _)
                                   ... =  b + w2             :  { Hw1 }
                                   ... =  c                  :  Hw2)

theorem le_lt_trans {a b c : Nat} (H1 : a ≤ b) (H2 : b < c) : a < c
:= obtain (w1 : Nat) (Hw1 : a + w1 = b), from (le_elim H1),
   obtain (w2 : Nat) (Hw2 : b + 1 + w2 = c), from (lt_elim H2),
      lt_intro (calc a + 1 + (w1 + w2) = a + 1 + w1 + w2   : symm (add_assoc _ _ _)
                                   ... = a + (1 + w1) + w2 : { add_assoc a 1 w1 }
                                   ... = a + (w1 + 1) + w2 : { add_comm 1 w1 }
                                   ... = a + w1 + 1 + w2   : { symm (add_assoc a w1 1) }
                                   ... = b + 1 + w2        : { Hw1 }
                                   ... = c                 : Hw2)

theorem ne_lt_succ {a b : Nat} (H1 : a ≠ b) (H2 : a < b + 1) : a < b
:= obtain (w : Nat) (Hw : a + 1 + w = b + 1), from (lt_elim H2),
     let L : a + w = b := add_injl (calc a + w + 1  =  a + (w + 1)  : add_assoc _ _ _
                                                ... =  a + (1 + w)  : { add_comm _ _ }
                                                ... =  a + 1 + w    : symm (add_assoc _ _ _)
                                                ... =  b + 1        : Hw)
     in discriminate (λ Hz : w = 0, absurd_elim (a < b) (calc a   = a + 0  : symm (add_zeror _)
                                                              ... = a + w  : { symm Hz }
                                                              ... = b      : L)
                                                         H1)
                     (λ (p : Nat) (Hp : w = p + 1), lt_intro (calc a + 1 + p =  a + (1 + p)   : add_assoc _ _ _
                                                                        ...  =  a + (p + 1)   : { add_comm _ _ }
                                                                        ...  =  a + w         : { symm Hp }
                                                                        ...  =  b             : L))

theorem strong_induction {P : Nat → Bool} (H : ∀ n, (∀ m, m < n → P m) → P n) : ∀ a, P a
:= take a,
    let stronger : P a ∧ ∀ m, m < a → P m :=
      -- we prove a stronger result by regular induction on a
      induction_on a
        (show P 0 ∧ ∀ m, m < 0 → P m, from
            let c2 : ∀ m, m < 0 → P m := λ (m : Nat) (Hlt : m < 0), absurd_elim (P m) Hlt (not_lt_0 m),
                c1 : P 0                := H 0 c2
            in and_intro c1 c2)
        (λ (n : Nat) (iH : P n ∧ ∀ m, m < n → P m),
            show P (n + 1) ∧ ∀ m, m < n + 1 → P m, from
              let iH1 : P n                    := and_eliml iH,
                  iH2 : ∀ m, m < n → P m     := and_elimr iH,
                  c2  : ∀ m, m < n + 1 → P m := λ (m : Nat) (Hlt : m < n + 1),
                                                    or_elim (em (m = n))
                                                      (λ Heq : m = n, subst iH1 (symm Heq))
                                                      (λ Hne : m ≠ n, iH2 m (ne_lt_succ Hne Hlt)),
                  c1  : P (n + 1)              := H (n + 1) c2
              in and_intro c1 c2)
    in and_eliml stronger

set_opaque add true
set_opaque mul true
set_opaque le true
set_opaque id true
-- We should only mark constants as opaque after we proved the theorems/properties we need.
-- set_opaque ge true
-- set_opaque lt true
-- set_opaque gt true
-- set_opaque id true
end