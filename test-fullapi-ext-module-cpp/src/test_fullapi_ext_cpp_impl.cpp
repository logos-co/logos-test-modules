#include "test_fullapi_ext_cpp_impl.h"

// Pure echoes, mirroring test_fullapi_ext_rust method for method — so the
// conformance matrix can compare the two providers cell by cell and any
// difference is the generated code or the wire, never the impl.

std::string TestFullapiExtCppImpl::whoAmI() { return "test_fullapi_ext_cpp"; }

Blob    TestFullapiExtCppImpl::echoBlob(const Blob& v)       { return v; }
Wrapper TestFullapiExtCppImpl::echoWrapper(const Wrapper& v) { return v; }

std::vector<Blob> TestFullapiExtCppImpl::echoBlobList(const std::vector<Blob>& v) { return v; }
std::map<std::string, Blob> TestFullapiExtCppImpl::echoBlobMap(
    const std::map<std::string, Blob>& v) { return v; }

std::vector<std::vector<uint8_t>> TestFullapiExtCppImpl::echoBytesList(
    const std::vector<std::vector<uint8_t>>& v) { return v; }
std::map<std::string, std::vector<uint8_t>> TestFullapiExtCppImpl::echoBytesMap(
    const std::map<std::string, std::vector<uint8_t>>& v) { return v; }
std::map<std::string, int64_t> TestFullapiExtCppImpl::echoIntMap(
    const std::map<std::string, int64_t>& v) { return v; }
std::map<std::string, std::string> TestFullapiExtCppImpl::echoStringMap(
    const std::map<std::string, std::string>& v) { return v; }
std::vector<std::vector<int64_t>> TestFullapiExtCppImpl::echoNestedInts(
    const std::vector<std::vector<int64_t>>& v) { return v; }
std::map<std::string, std::vector<std::vector<uint8_t>>> TestFullapiExtCppImpl::echoMapOfBytesLists(
    const std::map<std::string, std::vector<std::vector<uint8_t>>>& v) { return v; }

// Optionality: still pure echoes. The empty state has to survive the round trip
// untouched — an impl that "helpfully" substituted a default would hide exactly
// the defect these cells exist to catch.
Opt TestFullapiExtCppImpl::echoOpt(const Opt& v) { return v; }
std::vector<Opt> TestFullapiExtCppImpl::echoOptList(const std::vector<Opt>& v) { return v; }
std::optional<std::string> TestFullapiExtCppImpl::echoOptional(
    const std::optional<std::string>& v) { return v; }

bool TestFullapiExtCppImpl::fireBlobEvent(const Blob& v) { blobEvent(v); return true; }
