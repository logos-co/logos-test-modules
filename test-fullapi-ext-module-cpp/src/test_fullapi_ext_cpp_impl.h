#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test_fullapi_ext_cpp — the C++ mirror of test_fullapi_ext_rust.
//
// HEADER-FIRST, like test_fullapi_cpp: the contract is DERIVED from this file,
// records included. The structs below become `type` declarations in the
// generated .lidl, and the generator emits the codec that moves them across the
// wire — so the author names the structs directly, `Blob echoBlob(const Blob&)`,
// instead of digging fields out of a LogosMap.
//
// Every method is an echo, so the matrix compares a value against itself: any
// difference is the wire, the codegen or the dispatch, never this file.
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <logos_json.h>            // LogosMap, LogosList
#include <logos_module_context.h>  // LogosModuleContext base + `logos_events`

// The records this contract declares. A struct here becomes a `type` decl in
// the derived .lidl; field types use the same C++ spellings the method
// signatures do.
struct Blob {
    std::string          id;
    uint64_t             n;
    std::vector<uint8_t> payload;
};

struct Wrapper {
    Blob                     inner;
    std::vector<std::string> tags;
    std::vector<Blob>        blobs;
};

// ─────────────────────────────────────────────────────────────────────────────
// Optionality. `?T` is TWO-state — a value of T, or empty — and in C++ that is
// std::optional<T> and nothing else: one LIDL type, one type per language, one
// empty inhabitant (nullopt). `required` is the control: leniency applies in an
// OPTIONAL slot only, so a required field must still reject absence and null.
//
// AUTHORED AT THE TARGET: these declarations were written before any generator
// could compile them, so that the implementation had something to hit rather
// than a behaviour to bless. That target has been reached — logos-cpp-sdk#125
// landed the three pieces below together and this header builds. Kept as the
// record of what had to be true, because each piece is a way for `?T` to
// degrade silently rather than loudly:
//
//   1. logos-cpp-generator --header-to-lidl used to map anything it did not
//      recognise — std::optional<T> included — to `any`, with no warning
//      (impl_header_parser.cpp, the "Fallback: treat as opaque" arm). The
//      derived contract came out as `maybe: any`, `count: any`, `blob: any`
//      and `method echoOptional(v: any) -> any`, silently disagreeing with the
//      Rust provider's contract. std::optional now maps to TypeExpr::Optional.
//   2. The codec generated for that degraded contract was
//      `Codec<LogosMap>::to(v.maybe)` — a LogosMap codec applied to a
//      std::optional<std::string>, and that was the compile error it produced.
//      logos_codec.h now has Codec<std::optional<T>>: absent OR null decodes to
//      nullopt; nullopt encodes by OMITTING a named key and as null in a
//      positional slot.
//   3. `?T` was additionally a HARD CODEGEN REJECT: typeSupported()
//      (lidl_gen_cdylib.cpp) had no TypeExpr::Optional arm, so the module was
//      refused outright with "not cdylib-eligible: method 'echoOptional':
//      parameter 'v' has a type outside the cdylib-supported (Qt-free) subset".
//      It has an Optional arm now.
//
// What the ext table still registers is OPT2, which none of this fixes: an
// empty optional RETURN is spelled null, and null is already the failure token
// one layer up. See known-ext.json.
// ─────────────────────────────────────────────────────────────────────────────
struct Opt {
    std::string                         required;
    std::optional<std::string>          maybe;
    std::optional<uint64_t>             count;
    std::optional<std::vector<uint8_t>> blob;
};

class TestFullapiExtCppImpl : public LogosModuleContext {
public:
    TestFullapiExtCppImpl() = default;
    ~TestFullapiExtCppImpl() = default;

    std::string whoAmI();

    // ── Records ─────────────────────────────────────────────────────────────
    // Typed all the way down: Wrapper holds a Blob, Blob holds a bstr.
    Blob    echoBlob(const Blob& v);
    Wrapper echoWrapper(const Wrapper& v);
    std::vector<Blob> echoBlobList(const std::vector<Blob>& v);
    std::map<std::string, Blob> echoBlobMap(const std::map<std::string, Blob>& v);

    // ── Bytes at depth, typed maps, nested composites ───────────────────────
    std::vector<std::vector<uint8_t>> echoBytesList(const std::vector<std::vector<uint8_t>>& v);
    std::map<std::string, std::vector<uint8_t>> echoBytesMap(const std::map<std::string, std::vector<uint8_t>>& v);
    std::map<std::string, int64_t> echoIntMap(const std::map<std::string, int64_t>& v);
    std::map<std::string, std::string> echoStringMap(const std::map<std::string, std::string>& v);
    std::vector<std::vector<int64_t>> echoNestedInts(const std::vector<std::vector<int64_t>>& v);
    std::map<std::string, std::vector<std::vector<uint8_t>>> echoMapOfBytesLists(const std::map<std::string, std::vector<std::vector<uint8_t>>>& v);

    // ── Optionality ─────────────────────────────────────────────────────────
    // In a record (named slots: empty is spelled by OMITTING the key), in a
    // list of records (per-element presence is independent), and as a bare
    // positional slot (no key to omit, so empty is null and the arity never
    // changes). See the note on `struct Opt` for what had to land for this to
    // build at all.
    Opt echoOpt(const Opt& v);
    std::vector<Opt> echoOptList(const std::vector<Opt>& v);
    std::optional<std::string> echoOptional(const std::optional<std::string>& v);

    bool fireBlobEvent(const Blob& v);

logos_events:
    void blobEvent(const Blob& v);
};
