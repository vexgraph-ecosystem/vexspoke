#ifndef ANNOTATION_SECURITY_H
#define ANNOTATION_SECURITY_H

// src/annotation/security.h — the security-purpose markers.
//
// Two markers, one grep: `grep -rn ';;SECURITY\|;;FREEZE'` lists every place in
// the ecosystem that carries a security claim, because the marker set IS the
// security set. A claim that is not in that list is a claim nobody can audit.
//
// SECURITY(text) — mark a surface that carries a security obligation: a cold
//   validation seam, a trust boundary, an authorization check, secret handling,
//   a rate limiter, an audit record. text names the obligation and the thing it
//   protects, e.g. `;;SECURITY("session token compare: constant time, no early
//   exit")`.
//
// FREEZE(text) — mark a spot where the code intentionally WAITS, on purpose, for
//   a security reason: slowing a guesser instead of merely rejecting them. text
//   names the bound and the identity being throttled, e.g.
//   `;;FREEZE("login: one account, PASSWORD_FAIL_FREEZE_MS")`.
//
// A FREEZE is a DELIBERATE EXCEPTION and is governed as one under the Conflict
// Triage Law, because every other law tells code not to wait: the Bounded Wait
// Law forbids an unbounded wait, the Cold-Strict, Hot-Minimal Validation Law
// keeps hot paths from blocking, and the THROW Law keeps blocking diagnostics off
// hot loops. Here the wait IS the mechanism, so it carries rules of its own (see
// SECURITY.md and the Freeze Law in PREFERENCES.md):
//
//   1. COLD PATH ONLY. Never a frame loop, a hot getter, a teardown path or a
//      signal handler. If it is reachable while presenting pixels or servicing a
//      frame, it is in the wrong place.
//   2. BOUNDED AND NAMED. The duration is a named constant with a stated ceiling
//      — never a bare literal (the No Hardcoding Law) — and a test asserts the
//      ceiling. There is no "sleep forever".
//   3. NEVER HOLD A LOCK OR AN ARENA WHILE FROZEN. Sleeping with a lock held is
//      a self-inflicted denial of service against every other caller.
//   4. NEVER SCALE WITH ATTACKER INPUT. The freeze must not sit inside a loop
//      whose count comes from the request; otherwise the defender pays the cost
//      the attacker chooses, which is the inversion that turns a brake into a
//      weapon.
//   5. PAIR IT WITH A COUNT. The freeze is the brake; the counted, observable
//      refusal is the record (the Exhaustion Loudness Law's report-once,
//      count-always shape). A freeze with no counter is a smell.
//   6. IT IS NOT A SECURITY BOUNDARY. It slows guessing; it authenticates
//      nothing, and never replaces a rate limiter, a slow password hash, or a
//      real authorization check.
//
// Neither marker is a substitute for an access check, and neither implies that
// anything is hidden. Anything a caller can run, a caller can read: an attacker
// who can access a platform can decompile it, so no secret may live in code and
// no control may rest on obscurity. Focus on ACCESS — who may do what, proved
// and enforced where the data is — not on hiding the implementation.
//
// Distinct from `;;INTENTION(...)`, which justifies a WAIVER. SECURITY states an
// obligation; FREEZE states that the wait is the mechanism.
//
// See intention.h for the macro convention (C has no language-level
// annotations).

#define SECURITY(text) _Static_assert(1, "@Security(" text ")");
#define FREEZE(text) _Static_assert(1, "@Freeze(" text ")");

#endif
