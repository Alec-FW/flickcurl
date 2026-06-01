# Flickcurl remediation plan — 2026-05-31

Implementation checklist for bringing Flickcurl 1.27 up to date on correctness,
security hygiene, CI, and tests. Derived from Cursor and Codex inspections along
with follow-up analysis.

**Out of scope for this plan:** Flickr API drift audit, merging
`group-topic-apis`, libtool versioning, and broad `sprintf` → `snprintf` sweeps.
Those are listed under [Deferred follow-up](#deferred-follow-up).

**Release note:** Local tree reports 1.27 as unreleased in `NEWS`; last release
tag is `flickcurl_1_26`. Ship fixes as **1.27.1** (or finally release 1.27) once
Phase 1–2 land. Preserve public ABI: no struct layout changes and no removed
symbols in 1.27.x.

**Commit convention:** One focused commit per logical change; update this file
as items land.

---

## Phase 0 — Security and secrets

- [x] Add to `.gitignore`:
  - `*.conf.local`
  - `*.dSYM/`
  - optional: `bugs/bug*` binaries (keep `bugs/*.c` if useful as reproducers)

---

## Phase 1 — Correctness bugs (ship together)

These are data-corruption or auth-breakage issues; treat as one release unit.

### OAuth

- [x] **Fix pointer corruption in `flickcurl_set_oauth_request_token_secret()`**
  (`src/oauth.c`): after `free()`, code sets `request_token_secret = 0` instead
  of clearing `request_token_secret_len`. Must null the pointer and set `len =
  0`.
- [x] **Fix OAuth signing for `flickcurl_prepare_noauth()` paths**
  (`src/oauth.c`): signing is currently gated on `(need_auth && (client_secret
  || token_secret)) || fc->sign`. Callers such as `places-api.c` and
  `reflection-api.c` use `prepare_noauth()` without `flickcurl_set_sign()`.
  Rule: **sign when OAuth consumer credentials or token secrets are present**,
  regardless of `need_auth`. OAuth token exchange paths that call
  `flickcurl_set_sign()` must keep working. Verified with `oauth_sign_test`.
- [x] **Replace MT-based OAuth nonces** with 128-bit platform randomness
  (`getentropy`, `arc4random`, or `/dev/urandom`), retaining libmtwist only as a
  last-resort fallback. Existing `flickcurl_oauth_test` vectors must still pass.

### Config read/write

- [x] **Fix legacy config round-trip** (`src/config.c`):
  - Reader expects `auth_token=` and `secret=`.
  - Writer currently emits `oauth_token=` and `oauth_secret=` for legacy auth —
    fix to match reader.
- [x] **Credential file permissions on POSIX**: create or truncate config files
  with mode `0600` (handle umask; apply on new writes from library and CLI OAuth
  flows).

### `src/person.c` (from Cursor review)

- [x] Size `person_fields_table` using `PERSON_FIELD_LAST`, not
  `PHOTO_FIELD_LAST`.
- [x] Resolve duplicate username mapping: both `./username` (element) and
  `./@username` (attribute) map to `PERSON_FIELD_username`. **Keep element text;
  drop attribute mapping** (document in commit message).
- [x] Reset integer fields with `-1` (int), not
  `(flickcurl_person_field_type)-1`.
- [x] Fix debug typo: use `string_value`, not undefined `value`.

### `src/places-api.c` (found during warning cleanup)

- [x] Fix copy-paste bugs mapping upload/taken date parameters to wrong Flickr
  API keys in `flickcurl_places_tagsForPlace()`.

---

## Phase 2 — CLI and packaging

- [x] **`flickcurl -h` / `--help` and `-v` without config**
  (`utils/flickcurl.c`): skip mandatory config load for help/version; exit 0
  cleanly (no error spam when `HOME=/tmp` or `~/.flickcurl.conf` is missing).
- [x] **`--output` / `-o`**: use `required_argument` in getopt; fail if filename
  missing (today silently no-ops).
- [x] **`flickcurl.pc`**: public header `flickcurl.h` includes libxml. Always
  emit `Requires: libxml-2.0` in `.pc` when libxml is a dependency — not only
  when libxml is detected via pkg-config (Homebrew often uses `xml2-config`,
  which currently skips adding libxml to `PKG_CONFIG_REQUIRES`).
- [x] **`.gitmodules`**: change libmtwist URL from `git://` to
  `https://github.com/dajobe/libmtwist.git`. Do not edit
  `libmtwist/.travis.yml`.

---

## Phase 3 — Hardening

- [x] **libxml parser hardening** (`src/common.c` and any other parse entry
  points):
  - Disable network, DTD subset loading, and entity expansion for untrusted API
    responses.
  - Today push parser sets `replaceEntities = 1` and `loadsubset = 1` — reverse
    that policy consistently on push parser and any `xmlRead*` / offline paths.
  - Document chosen `xmlCtxtUseOptions` / parser flags in commit or code
    comment.

---

## Phase 4 — CI migration

- [x] Delete tracked root `.travis.yml` (and `.travis.yml~` backup residue if
  present).
- [x] Add `.github/workflows/ci.yml`:
  - Matrix: `ubuntu-latest`, `macos-latest`
  - Steps: install deps → `./autogen.sh` → `./configure --disable-gtk-doc` →
    `make -j4` → `make check`
  - Optional maintainer job: `./configure --enable-maintainer-mode
    --disable-gtk-doc` and assert **zero** `warning:` lines in build log.
  - Packages (Ubuntu): `autoconf`, `automake`, `libtool`, `pkg-config`,
    `libcurl4-openssl-dev`, `libxml2-dev`, `libraptor2-dev`
  - macOS: Homebrew `libcurl`, `libxml2`, `raptor2` (or equivalent)
- [ ] **Optional** separate workflow or manual job: `make -C src analyze` (Clang
  static analyzer — do not block default CI on GCC-only analyze gaps).
- [x] Do **not** run `make distcheck` on every push (slow, gtk-doc flaky). Run
  before releases only.

---

## Test plan

Add tests under `src/` and register in `src/Makefile.am` `TESTS` (same pattern
as `flickcurl_oauth_test`).

| Test                                          | Covers                                                                       |
|:----------------------------------------------|:-----------------------------------------------------------------------------|
| Extend or companion to `flickcurl_oauth_test` | Signing on `prepare_noauth()` with client secret only; existing HMAC vectors |
| `config_test` (new)                           | Legacy + OAuth config read/write round-trip; key names                       |
| `config_test`                                 | New credential file mode `0600` on POSIX                                     |
| `person_test` (new)                           | Minimal/captured XML fixtures; username element vs attribute; field integers |
| Local `tests/api-smoke.sh` (optional, gitignored) | Live Flickr smoke tests; never run in CI; use your own IDs locally |

Fixtures: commit anonymized XML under `tests/fixtures/` (no secrets). May adapt
XML from `bugs/bug2014-11-17a.c` / `bugs/bug2014-11-19a.c` scenarios without
modifying those reproducer sources.

### Verification checklist

Update checkboxes as work completes:

```bash
./autogen.sh --enable-maintainer-mode --disable-gtk-doc
./configure --enable-maintainer-mode --disable-gtk-doc
make -j4 2>&1 | grep warning:          # expect no output
make check

# production-style build (no FLICKCURL_DEBUG OAuth trace)
./configure --disable-gtk-doc && make -j4

env HOME=/tmp ./utils/flickcurl -h    # expect exit 0 (after Phase 2)
env HOME=/tmp ./utils/flickcurl -v    # expect exit 0 (after Phase 2)

make -C src analyze                   # optional, Clang
make distcheck                        # pre-release only
```

All verification steps above pass on macOS as of 2026-05-31 (except optional
`analyze` and pre-release `distcheck`).

---

## Deferred follow-up

Document separately; do not fold into this pass:

| Item                        | Notes                                                    |
|:----------------------------|:---------------------------------------------------------|
| Libtool `0:0:0`             | Never bumped; revisit on 1.28                            |
| `coverage.html`             | 90.4% (188/208) from ~2014 baseline                      |
| `group-topic-apis` branch   | Triage for groups.discuss.* gap                          |
| `flickcurl_auth_checkToken` | FIXME: intermittent results (`auth-api.c`)               |
| `sprintf` → `snprintf`      | Wide sweep across `*-api.c`                              |
| Travis / README cleanup     | Remove stale Travis badges or clone URLs                 |
| Nonce / libmtwist           | After Phase 1, evaluate dropping submodule if unused     |

---

## Progress log

| Date       | Phase / item  | Notes                                                                           |
|:-----------|:--------------|:--------------------------------------------------------------------------------|
| 2026-05-31 | —             | Plan written; baseline `make check` passes on macOS                             |
| 2026-05-31 | Build hygiene | Commit `8cc2892`: zero maintainer-mode warnings; `command_photoslist` label fix |
| 2026-05-31 | API smoke     | Manual live Flickr tests OK for core read paths; 2014 NSID now “User deleted”   |
| 2026-05-31 | Remediation   | Correctness, CLI, hardening, CI, and unit tests implemented                   |
