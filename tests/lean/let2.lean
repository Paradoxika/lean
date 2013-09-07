
(* Annotating lemmas *)

Theorem simple (p q r : Bool) : (p ⇒ q) ∧ (q ⇒ r) ⇒ p ⇒ r :=
    Discharge (λ H_pq_qr, Discharge (λ H_p,
        let P_pq : (p ⇒ q) := Conjunct1 H_pq_qr,
            P_qr : (q ⇒ r) := Conjunct2 H_pq_qr,
            P_q  : q       := MP P_pq H_p
        in MP P_qr P_q))

Show Environment 1