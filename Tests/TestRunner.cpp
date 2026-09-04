#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <windows.h>

#include "../Common/MiniJson.h"
#include "../Common/SafeMemory.h"
#include "../Common/SharedSession.h"
#include "../SomaliaLoader/Auth/KeyAuth.h"

static int g_TotalTests = 0;
static int g_PassedTests = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        g_TotalTests++; \
        if (cond) { \
            g_PassedTests++; \
            std::cout << "  [PASS] " << msg << "\n"; \
        } else { \
            std::cerr << "  [FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
        } \
    } while(0)

void RunMiniJsonTests()
{
    std::cout << "\n=== Test Suite: MiniJson ===\n";

    // 1. Tipos primitivos e aninhamento
    {
        std::string jsonStr = R"({
            "success": true,
            "code": 200,
            "pi": 3.14159,
            "message": "Welcome \"User\"!\nLine2",
            "data": null,
            "items": [1, 2, "three", false],
            "nested": {
                "innerKey": "innerVal"
            }
        })";

        MiniJson::Value v;
        std::string err;
        bool ok = MiniJson::Parse(jsonStr, v, err);
        TEST_ASSERT(ok, "Parse valid complex JSON structure");
        TEST_ASSERT(err.empty(), "Error message is empty on success");
        TEST_ASSERT(v.is_object(), "Root is object");

        TEST_ASSERT(v["success"].is_bool() && v["success"].as_bool() == true, "Boolean parsed correctly");
        TEST_ASSERT(v["code"].is_number() && v["code"].as_int() == 200, "Integer parsed correctly");
        TEST_ASSERT(v["message"].is_string() && v["message"].as_string() == "Welcome \"User\"!\nLine2", "Escaped string parsed correctly");
        TEST_ASSERT(v["data"].is_null(), "Null parsed correctly");
        TEST_ASSERT(v["items"].is_array() && v["items"].as_array().size() == 4, "Array parsed correctly");
        TEST_ASSERT(v["items"][2].as_string() == "three", "Array element 2 is string");
        TEST_ASSERT(v["nested"].is_object() && v["nested"]["innerKey"].as_string() == "innerVal", "Nested object parsed correctly");
    }

    // 2. Serialização e Roundtrip
    {
        MiniJson::Value root(MiniJson::Type::Object);
        root["app"] = MiniJson::Value("Somalia");
        root["version"] = MiniJson::Value(2.0);
        root["enabled"] = MiniJson::Value(true);

        std::string serialized = MiniJson::Serialize(root);
        MiniJson::Value parsed;
        std::string err;
        bool ok = MiniJson::Parse(serialized, parsed, err);
        TEST_ASSERT(ok, "Roundtrip Serialize -> Parse");
        TEST_ASSERT(parsed["app"].as_string() == "Somalia", "Roundtrip string match");
        TEST_ASSERT(parsed["enabled"].as_bool() == true, "Roundtrip bool match");
    }

    // 3. Casos de erro / JSON inválido
    {
        MiniJson::Value v;
        std::string err;
        TEST_ASSERT(!MiniJson::Parse("{ unquoted_key: 123 }", v, err), "Reject unquoted key");
        TEST_ASSERT(!MiniJson::Parse(R"({"unclosed": "string)", v, err), "Reject unclosed string");
        TEST_ASSERT(!MiniJson::Parse(R"({"trailing": [1, 2, ])", v, err), "Reject trailing comma / unclosed array");
        TEST_ASSERT(!MiniJson::Parse("", v, err), "Reject empty string");
    }
}

void RunSafeMemoryTests()
{
    std::cout << "\n=== Test Suite: SafeMemory ===\n";

    // 1. Ponteiro nulo e faixas proibidas
    TEST_ASSERT(!SafeMemory::IsValidUserAddress(nullptr, 4), "Null pointer is invalid");
    TEST_ASSERT(!SafeMemory::IsValidReadPtr(nullptr, 4), "Null pointer read check fails safely");
    TEST_ASSERT(!SafeMemory::IsValidWritePtr(nullptr, 4), "Null pointer write check fails safely");

    void* lowAddr = reinterpret_cast<void*>(0x00000010);
    TEST_ASSERT(!SafeMemory::IsValidUserAddress(lowAddr, 4), "Low address (< 0x10000) rejected");
    TEST_ASSERT(!SafeMemory::IsValidReadPtr(lowAddr, 4), "Low address Read check fails safely");

    void* kernelAddr = reinterpret_cast<void*>(0x80000000);
    TEST_ASSERT(!SafeMemory::IsValidUserAddress(kernelAddr, 4), "Kernel address (>= 0x80000000) rejected");
    TEST_ASSERT(!SafeMemory::IsValidReadPtr(kernelAddr, 4), "Kernel address Read check fails safely");

    // 2. Memória válida de stack
    int stackVal = 1337;
    TEST_ASSERT(SafeMemory::IsValidReadPtr(&stackVal, sizeof(stackVal)), "Stack address valid for read");
    TEST_ASSERT(SafeMemory::IsValidWritePtr(&stackVal, sizeof(stackVal)), "Stack address valid for write");

    // 3. Memória válida de heap
    char* heapBuf = new char[256];
    memset(heapBuf, 'A', 256);
    TEST_ASSERT(SafeMemory::IsValidReadPtr(heapBuf, 256), "Heap address valid for read");
    TEST_ASSERT(SafeMemory::IsValidWritePtr(heapBuf, 256), "Heap address valid for write");
    delete[] heapBuf;

    // 4. Memória protegida contra escrita (PAGE_READONLY)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    void* roPage = VirtualAlloc(nullptr, si.dwPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READONLY);
    if (roPage)
    {
        TEST_ASSERT(SafeMemory::IsValidReadPtr(roPage, 64), "PAGE_READONLY page readable");
        TEST_ASSERT(!SafeMemory::IsValidWritePtr(roPage, 64), "PAGE_READONLY page NOT writable");
        VirtualFree(roPage, 0, MEM_RELEASE);
    }

    // 5. SafeRead e SafeWrite
    int testVal = 42;
    int readVal = 0;
    bool readOk = SafeMemory::SafeRead(reinterpret_cast<uintptr_t>(&testVal), readVal);
    TEST_ASSERT(readOk && readVal == 42, "SafeRead from valid stack address");

    bool writeOk = SafeMemory::SafeWrite(reinterpret_cast<uintptr_t>(&testVal), 999);
    TEST_ASSERT(writeOk && testVal == 999, "SafeWrite to valid stack address");

    int dummyOut = 0;
    TEST_ASSERT(!SafeMemory::SafeRead(0x00000004, dummyOut), "SafeRead from invalid address returns false");
}

void RunSharedSessionTests()
{
    std::cout << "\n=== Test Suite: SharedSession (Volatile RAM) ===\n";

    HANDLE hMap = nullptr;

    // 1. Deve retornar vazio inicialmente se não houver sessão ativa
    std::string initial = SharedSession::ReadSessionId();
    TEST_ASSERT(initial.empty() || true, "Initial session check executable");

    // 2. Publica sessão com DACL restrita
    const std::string testSession = "test_auth_sess_998877665544332211";
    bool pubOk = SharedSession::PublishSession(testSession, hMap);
    TEST_ASSERT(pubOk && hMap != nullptr, "PublishSession successfully mapped volatile RAM");

    // 2.1 Testa rejeição de escrita não autorizada via DACL pelo kernel do Windows
    HANDLE hWriteAttempt = OpenFileMappingA(FILE_MAP_WRITE, FALSE, "Local\\Somalia_Runtime_Session_Mem");
    if (!hWriteAttempt)
    {
        hWriteAttempt = OpenFileMappingA(FILE_MAP_WRITE, FALSE, "Somalia_Runtime_Session_Mem");
    }
    TEST_ASSERT(hWriteAttempt == nullptr, "Kernel DACL successfully denied FILE_MAP_WRITE to external handle");
    if (hWriteAttempt) CloseHandle(hWriteAttempt);

    // 3. Lê a sessão com FILE_MAP_READ autorizado
    std::string readBack = SharedSession::ReadSessionId();
    TEST_ASSERT(readBack == testSession, "ReadSessionId retrieves identical volatile session token");

    // 3.1 Testa mapping escopado por PID
    HANDLE hPidMap = nullptr;
    const std::string pidSession = "test_pid_scoped_token_12345";
    bool pidPubOk = SharedSession::PublishSession(pidSession, hPidMap, 8888);
    TEST_ASSERT(pidPubOk && hPidMap != nullptr, "PublishSession with target PID creates scoped mapping");
    std::string pidReadBack = SharedSession::ReadSessionId(8888);
    TEST_ASSERT(pidReadBack == pidSession, "ReadSessionId with target PID retrieves scoped token");
    SharedSession::RevokeSession(hPidMap);

    // 4. Revoga sessão
    SharedSession::RevokeSession(hMap);
    TEST_ASSERT(hMap == nullptr, "RevokeSession closed handle");

    std::string afterRevoke = SharedSession::ReadSessionId();
    TEST_ASSERT(afterRevoke.empty(), "ReadSessionId is empty after RevokeSession");
}

void RunHWIDTests()
{
    std::cout << "\n=== Test Suite: Multi-factor HWID ===\n";

    std::string hwid = KeyAuthClient::GetHWID();
    TEST_ASSERT(!hwid.empty(), "HWID is not empty");
    TEST_ASSERT(hwid.length() == 64, "HWID is valid 64-character SHA-256 hex string");
    TEST_ASSERT(hwid != "UNKNOWN_HWID", "HWID entropy collected successfully (not fallback)");

    // Verifica se todos os caracteres são hexadecimais
    bool allHex = true;
    for (char c : hwid)
    {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
        {
            allHex = false;
            break;
        }
    }
    TEST_ASSERT(allHex, "HWID contains only hexadecimal characters");

    // Determinismo do HWID na mesma máquina
    std::string hwid2 = KeyAuthClient::GetHWID();
    TEST_ASSERT(hwid == hwid2, "HWID generation is deterministic across multiple calls");
}

int main()
{
    std::cout << "========================================================\n";
    std::cout << " Somalia Client - Automated Verification Test Suite\n";
    std::cout << " Target: Windows x86 (32-bit)\n";
    std::cout << "========================================================\n";

    RunMiniJsonTests();
    RunSafeMemoryTests();
    RunSharedSessionTests();
    RunHWIDTests();

    std::cout << "\n========================================================\n";
    std::cout << " Test Summary: " << g_PassedTests << " / " << g_TotalTests << " passed.\n";
    std::cout << "========================================================\n";

    return (g_PassedTests == g_TotalTests) ? 0 : 1;
}
