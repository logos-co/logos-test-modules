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

    bool fireBlobEvent(const Blob& v);

logos_events:
    void blobEvent(const Blob& v);
};
