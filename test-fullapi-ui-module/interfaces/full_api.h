#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// full_api — a DEPENDENCY INTERFACE (method + event contract) decoupled from any
// concrete module. It names NO module; any module whose API is a superset of
// this satisfies it. A consumer binds it to a concrete module name at runtime
// via modules().bind_full_api("some_module").
//
// Both test_fullapi_cpp and test_fullapi_rust implement exactly this surface, so
// the same bound handle drives either provider (and this proxy, which re-exposes
// it, satisfies it too).
//
// Types are std (the consuming module is universal); the generated bound wrapper
// inherits that api-style. NO trailing `// comments` on declaration lines — the
// header parser only accepts a line ending in `;`.
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <string>
#include <vector>

#include <logos_json.h>            // LogosMap, LogosList, nlohmann::json
#include <logos_module_context.h>  // defines the `logos_events` token
#include <logos_result.h>          // StdLogosResult

class IFullApi {
public:
    std::string whoAmI();

    // Scalar echoes: tstr / bstr / int / uint / float64 / bool / any
    std::string          echoString(const std::string& v);
    std::string          echoTriple(int64_t i, const std::string& s, const std::vector<uint8_t>& b);
    std::vector<uint8_t> echoBytes(const std::vector<uint8_t>& v);
    int64_t              echoInt(int64_t v);
    uint64_t             echoUint(uint64_t v);
    double               echoDouble(double v);
    bool                 echoBool(bool v);
    nlohmann::json       echoAny(const nlohmann::json& v);

    // Container echoes: [tstr] / [int] / [uint] / [float64] / [bool] / [any] / {tstr:any}
    std::vector<std::string> echoStringList(const std::vector<std::string>& v);
    std::vector<int64_t>     echoIntList(const std::vector<int64_t>& v);
    std::vector<uint64_t>    echoUintList(const std::vector<uint64_t>& v);
    std::vector<double>      echoDoubleList(const std::vector<double>& v);
    std::vector<bool>        echoBoolList(const std::vector<bool>& v);
    LogosList                echoList(const LogosList& v);
    LogosMap                 echoMap(const LogosMap& v);

    // Return-only: void / result
    void           doVoid();
    StdLogosResult makeResult(bool ok);

    // Event trigger drivers (return bool)
    bool fireStringEvent(const std::string& v);
    bool fireTripleEvent(int64_t i, const std::string& s, const std::vector<uint8_t>& b);
    bool fireBytesEvent(const std::vector<uint8_t>& v);
    bool fireIntEvent(int64_t v);
    bool fireUintEvent(uint64_t v);
    bool fireDoubleEvent(double v);
    bool fireBoolEvent(bool v);
    bool fireAnyEvent(const nlohmann::json& v);
    bool fireStringListEvent(const std::vector<std::string>& v);
    bool fireIntListEvent(const std::vector<int64_t>& v);
    bool fireUintListEvent(const std::vector<uint64_t>& v);
    bool fireDoubleListEvent(const std::vector<double>& v);
    bool fireBoolListEvent(const std::vector<bool>& v);
    bool fireListEvent(const LogosList& v);
    bool fireMapEvent(const LogosMap& v);

logos_events:
    void stringEvent(const std::string& v);
    void tripleEvent(int64_t i, const std::string& s, const std::vector<uint8_t>& b);
    void bytesEvent(const std::vector<uint8_t>& v);
    void intEvent(int64_t v);
    void uintEvent(uint64_t v);
    void doubleEvent(double v);
    void boolEvent(bool v);
    void anyEvent(const nlohmann::json& v);
    void stringListEvent(const std::vector<std::string>& v);
    void intListEvent(const std::vector<int64_t>& v);
    void uintListEvent(const std::vector<uint64_t>& v);
    void doubleListEvent(const std::vector<double>& v);
    void boolListEvent(const std::vector<bool>& v);
    void listEvent(const LogosList& v);
    void mapEvent(const LogosMap& v);
};
