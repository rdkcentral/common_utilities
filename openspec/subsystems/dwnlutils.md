# Download Library (libdwnlutil) - Detailed Analysis

**Module:** dwnlutils/  
**Public API:** `downloadUtil.h`  
**Internal Layer:** `urlHelper.c/h`  
**Dependencies:** libcurl, pthreads, librdkloggers  
**Build Artifact:** `libdwnlutil.so`

---

## Module Purpose

Provides **reliable, resilient HTTP/HTTPS file download** capabilities with support for:
- mTLS certificate-based authentication
- OAuth token-based authorization
- Bandwidth throttling and speed limiting
- Resumable downloads from specific byte offsets
- Chunk-based downloads with retry logic
- Progress tracking and cancellation
- Multiple authentication modes (mTLS, OAuth, bearer token)

Primary consumers: firmware updaters, RFC engines, RDK managers, asset downloaders.

---

## Public API Surface

### Initialization & Lifecycle

```c
void *doCurlInit(void);
// Initialize curl context for download operations
// Returns: opaque CURL* pointer (void* for API stability)
// Consumer responsibility: Call doCurlInit() once, reuse for multiple downloads

void doStopDownload(void *curl);
// Cleanup and destroy curl context
// Parameters: curl - context from doCurlInit()
// Side effects: Releases socket connections, memory, threads
```

### Core Download Operations

```c
int doHttpFileDownload(
    void *in_curl,                    // Context from doCurlInit()
    FileDwnl_t *pfile_dwnl,           // Download configuration
    MtlsAuth_t *auth,                 // Optional mTLS credentials
    unsigned int max_dwnl_speed,      // Bandwidth limit (bytes/sec), 0=unlimited
    char *dnl_start_pos,              // Resume offset (bytes), NULL=fresh start
    int *out_httpCode                 // HTTP status code (output parameter)
);
// Standard mTLS-authenticated download
// Returns: DWNL_SUCCESS (1), DWNL_FAIL (-1)
// Usage: Firmware downloads, file retrieval with certificate auth

int doAuthHttpFileDownload(
    void *in_curl,
    FileDwnl_t *pfile_dwnl,
    int *out_httpCode
);
// OAuth token-based download
// Token expected in FileDwnl_t.pPostFields
// Returns: DWNL_SUCCESS (1), DWNL_FAIL (-1)
// Usage: Public API downloads with bearer token auth

int getJsonRpcData(
    void *in_curl,
    FileDwnl_t *pfile_dwnl,
    char *jsonrpc_auth_token,         // JSON-RPC auth token
    int *out_httpCode
);
// Download via JSON-RPC protocol
// Sends JSON-encoded request, expects JSON response
// Returns: DWNL_SUCCESS (1), DWNL_FAIL (-1)
// Usage: Legacy JSON-RPC server communication

int doCurlPutRequest(
    void *in_curl,
    FileDwnl_t *pfile_dwnl,
    char *jsonrpc_auth_token,
    int *out_httpCode
);
// HTTP PUT request (minimal documented usage)
// Returns: DWNL_SUCCESS (1), DWNL_FAIL (-1)
// Quirk: Function name typo "doCurlPutRequest" (note: used, not "doCurlPutReuqest")
```

### Runtime Control & Monitoring

```c
int doInteruptDwnl(
    void *in_curl,
    unsigned int max_dwnl_speed       // New speed limit (bytes/sec)
);
// Pause download, apply speed limit, resume
// Enables mid-transfer bandwidth throttling
// Returns: DWNL_SUCCESS (0), DWNL_FAIL (-1), DWNL_UNPAUSE_FAIL (-2)
// Sequence: pause → apply throttle → resume

unsigned int doGetDwnlBytes(void *in_curl);
// Query bytes downloaded so far
// Non-blocking query of transfer progress
// Returns: bytes transferred (0 if error or no transfer)
// Usage: Progress bars, resume offset calculation

int setForceStop(int value);
// Global flag to force-stop all downloads
// Parameters: value=1 (stop), value=0 (clear)
// Behavior: Affects all active downloads using this curl context
```

---

## Data Structures

### FileDwnl_t - Download Configuration

```c
typedef struct filedwnl {
    char *pPostFields;                // POST body data (optional)
    char *pHeaderData;                // Custom HTTP headers
    DownloadData *pDlData;            // Downloaded file content buffer
    DownloadData *pDlHeaderData;      // Response headers buffer (if capturing)
    int chunk_dwnl_retry_time;        // Retry delay for chunk failures (seconds)
    char url[BIG_BUF_LEN];           // URL (max 1024 bytes)
    char pathname[DWNL_PATH_FILE_LEN]; // Local file save path (max 128 bytes)
    bool sslverify;                   // SSL peer verification (true/false)
    hashParam_t *hashData;            // Hash value tracking (MD5/SHA, optional)
} FileDwnl_t;
```

### DownloadData - Buffer Management

```c
typedef struct CommonDownloadData {
    void* pvOut;                      // Pointer to downloaded data
    size_t datasize;                  // Actual bytes downloaded
    size_t memsize;                   // Allocated buffer size
} DownloadData;
// Memory management pattern: Consumer allocates, library populates
```

### MtlsAuth_t - mTLS Credentials

```c
typedef struct credential {
    char cert_name[64];               // Certificate filename/identifier
    char cert_type[16];               // Certificate type (PEM, DER, etc.)
    char key_pas[65];                 // Key passphrase (max 64 chars)
#ifdef LIBRDKCERTSELECTOR
    char engine[32];                  // Certificate engine (HSM, etc.)
#endif
} MtlsAuth_t;
```

### hashParam_t - Content Verification

```c
typedef struct hashParam {
    char *hashvalue;                  // Hash string (hex-encoded, e.g., MD5)
    char *hashtime;                   // Timestamp of hash computation
} hashParam_t;
// Usage: Optional verification of downloaded file integrity
```

---

## Implementation Architecture

### Public Interface Layer (downloadUtil.c)

**Responsibility:** High-level download orchestration and consumer API

- Wraps urlHelper functions with download-specific semantics
- Manages FileDwnl_t configuration parsing
- Implements state tracking (initializing → downloading → complete)
- Handles auth mode selection (mTLS vs OAuth vs JSON-RPC)
- Returns standardized integer status codes

**Key Functions:**
- `doCurlInit()` - Delegate to `urlHelperCreateCurl()`
- `doHttpFileDownload()` - Call `urlHelperDownloadFile()` with mTLS handling
- `doAuthHttpFileDownload()` - Inject OAuth token into header, call download
- `getJsonRpcData()` - Marshal JSON request, call download with JSON response parsing
- `doInteruptDwnl()` - Call `curl_easy_pause()` with throttle configuration
- `doGetDwnlBytes()` - Query `CURLINFO_SIZE_DOWNLOAD_T` from curl

### Internal Implementation Layer (urlHelper.c/h)

**Responsibility:** libcurl wrapper providing download infrastructure

**Lower-Level Functions:**
- `urlHelperCreateCurl()` - Initialize curl, thread-safe via `pthread_once`
- `urlHelperDestroyCurl()` - Cleanup curl resources
- `setCommonCurlOpt()` - Configure standard curl options (SSL, timeouts, etc.)
- `urlHelperDownloadFile()` - File-to-filesystem download with callback
- `urlHelperDownloadToMem()` - In-memory buffer download
- `urlHelperPutReuqest()` - HTTP PUT implementation (**NOTE: Typo in name**)
- `SetRequestHeaders()` - Build HTTP header list (curl_slist)
- `SetPostFields()` - Configure POST body
- `setMtlsHeaders()` - Apply mTLS certificate headers
- `printCurlError()` - Convert CURLcode to human-readable error message
- `setThrottleMode()` - Configure bandwidth limiting via `CURLOPT_MAX_RECV_SPEED_LARGE`

**Thread Safety Pattern:**
```c
// One CURL* handle per thread (curl_easy_init is NOT thread-safe)
static pthread_once_t initOnce = PTHREAD_ONCE_INIT;
static void urlHelperInit(void) {
    curl_global_init(CURL_GLOBAL_ALL);  // One-time global init
}
CURL *urlHelperCreateCurl(void) {
    pthread_once(&initOnce, urlHelperInit);  // Thread-safe one-time init
    return curl_easy_init();                  // Thread-local curl handle
}
```

### Write Callback Pattern

**Download Data Population:**
- Consumer provides `FileDwnl_t.pDlData` with allocated buffer
- libcurl invokes write callback for each data chunk
- Callback appends to buffer, tracks size
- Final size available as `pDlData->datasize`

```c
// Pseudo-implementation (actual in urlHelper.c)
static size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
    DownloadData *dl = (DownloadData *)userp;
    size_t bytes = size * nmemb;
    
    // Grow buffer if needed
    if (dl->datasize + bytes > dl->memsize) {
        dl->pvOut = realloc(dl->pvOut, dl->memsize * 2);
        dl->memsize *= 2;
    }
    
    memcpy((char*)dl->pvOut + dl->datasize, data, bytes);
    dl->datasize += bytes;
    return bytes;
}
```

---

## Runtime Behavior Analysis

### Download Sequence (Happy Path)

```
1. Consumer calls doCurlInit()
   → urlHelperCreateCurl()
   → curl_easy_init() 
   → Return CURL* handle

2. Consumer prepares FileDwnl_t:
   - Set url, pathname, SSL verify
   - Allocate DownloadData buffer
   - Set pPostFields (if OAuth)
   - Set pHeaderData (if custom headers)

3. Consumer calls doHttpFileDownload()
   → setCommonCurlOpt(curl, url, postfields, ssl_verify)
     - CURLOPT_URL = url
     - CURLOPT_CAINFO = ca cert path
     - CURLOPT_SSL_VERIFYPEER = ssl_verify
     - CURLOPT_CONNECTTIMEOUT = 30s
     - CURLOPT_TIMEOUT = 7200s (2 hours)
   → setMtlsHeaders(curl, cert, key)
   → curl_easy_perform(curl)
   → Write callback populates DownloadData buffer
   → curl_easy_getinfo(CURLINFO_RESPONSE_CODE, &httpCode)

4. Return DWNL_SUCCESS and HTTP code

5. Consumer processes downloaded data from FileDwnl_t.pDlData

6. Consumer calls doStopDownload()
   → urlHelperDestroyCurl()
   → curl_easy_cleanup()
   → Free resources
```

### Error Scenarios

**Network Connectivity Issues:**
```
- CURLE_COULDNT_CONNECT: Cannot reach host
- CURLE_COULDNT_RESOLVE_HOST: DNS resolution failed
- CURLE_OPERATION_TIMEDOUT: Connection timeout (30s) or transfer timeout (2h)
- CURLE_GOT_NOTHING: Connection established but no data received
```
→ Library logs error, returns DWNL_FAIL, HTTP code 0

**SSL/Certificate Issues:**
```
- CURLE_SSL_CERTPROBLEM: Invalid or missing client certificate
- CURLE_SSL_CONNECT_ERROR: SSL handshake failed
- CURLE_SSL_CAINFO: Server certificate validation failed (if verify enabled)
```
→ Library logs error, returns DWNL_FAIL

**File I/O Issues:**
```
- CURLE_WRITE_ERROR: Write callback failed (buffer allocation, disk full)
```
→ Propagates as DWNL_FAIL, callback return value affects result

**Throttling Failures:**
```
- doInteruptDwnl() returns DWNL_UNPAUSE_FAIL if curl_easy_pause(CONTINUE) fails
- Partial throttling applied if pause succeeds but resume fails
```

---

## Configuration & Feature Flags

### Compile-Time Options

| Flag | Purpose | Impact |
|------|---------|--------|
| `IS_LIBRDKCERTSEL_ENABLED` | Enable rdkcertselector for mTLS | Adds `-lRdkCertSelector` linker flag, enables dynamic cert rotation |
| `CURL_DEBUG` | Enable libcurl verbose logging | Activates `CURLOPT_VERBOSE` + debug callback |
| `GTEST_ENABLE` | Google Test integration | Redirects paths to `/tmp/` instead of `/etc/`, `/opt/` |
| `L2UPLOADENABLED` | Layer 2 container support | Affects SSL verification in upload scenarios |
| `-DANSC_LINUX` | Linux target platform | Enables Linux-specific system calls |
| `-D_ANSC_LITTLE_ENDIAN_` | Little-endian architecture | Affects binary protocol parsing (if used) |

### Runtime Configuration

**SSL Verification:**
- Controlled by `FileDwnl_t.sslverify` boolean
- `true` = Verify server certificate (requires CA bundle)
- `false` = Skip verification (insecure, use only for testing)

**Timeouts:**
- Connection: 30 seconds (hardcoded in urlHelper.c)
- Transfer: 7200 seconds / 2 hours (hardcoded in urlHelper.c)
- No runtime override mechanism

**Bandwidth Limits:**
- Specified as `max_dwnl_speed` parameter in bytes/sec
- 0 = unlimited, 1+ = limit to N bytes/sec
- Applied via `curl_easy_pause()` + `CURLOPT_MAX_RECV_SPEED_LARGE`

---

## Known Issues & Limitations

### Issue 1: Function Name Typo
- **Function:** `urlHelperPutReuqest()` (should be `Request`)
- **Scope:** Public API in header
- **Usage:** Used in 22+ locations across dwnlutils and mocks
- **Impact:** Documentation confusion, potential IDE autocomplete errors
- **Status:** Long-standing, backward compatibility required for rename

### Issue 2: TODO Comment on Struct
- **Location:** Line 47 of `urlHelper.h`, `MtlsAuth_t` struct definition
- **Text:** Bare `//TODO` with no explanation
- **Implication:** Incomplete struct documentation or unfinished feature?
- **Status:** Unclear purpose

### Issue 3: Fixed Timeout Values
- **Issue:** No runtime control over connection/transfer timeouts
- **Impact:** All downloads share 30s connection, 2h transfer timeout
- **Workaround:** Must rebuild with modified values
- **Limitation:** Not suitable for slow networks or very large files

### Issue 4: Resume Offset (dnl_start_pos) Documentation
- **Parameter:** `char *dnl_start_pos` - Expected format?
- **Assumption:** Byte offset as string (e.g., "1024") or binary?
- **Usage:** Not clear from header documentation

### Issue 5: Global Force Stop Flag
- **Function:** `setForceStop(1)` - Affects ALL downloads globally
- **Thread Safety:** Unclear if volatile or atomic
- **Race Condition Risk:** Between check and stop in concurrent downloads

---

## Testing Infrastructure

### Unit Tests (gtest)

**Test File:** `unit-test/dwnlutils/downloadUtil_gtest.cpp`

**Test Categories:**
- URL helper parameter validation (NULL checks)
- Download with valid/invalid URLs
- mTLS credential handling
- OAuth token injection
- SSL verification modes
- Throttle control
- Error condition responses

**Mock Infrastructure:**
- `mocks/curl_mock.h` - Mock libcurl functions
- `mocks/mock_urlHelper.h` - Mock urlHelper layer
- Uses gmock for behavior verification

### Functional Tests (Python)

**Test File:** `test/functional-tests/tests/test_file_download.py`

**Test Execution:**
```bash
./dwnl_lib_test 2    # Standard file download
./dwnl_lib_test 3    # File not present (error case)
./dwnl_lib_test 4    # Bandwidth throttling
./dwnl_lib_test 6    # Chunk download with resume
```

---

## Performance Characteristics

### Memory Footprint (Typical)
- libdwnlutil.so: ~52 KB (shared library image)
- Per-download context: ~8-16 KB (curl handle + buffers)
- Buffer scaling: Depends on file size (consumer-managed)

### Network Performance
- **Throughput:** Limited by network, not library (libcurl pass-through)
- **Latency:** 30 sec connection timeout, up to 2h transfer timeout
- **Concurrency:** One CURL* per thread (no multiplexing within thread)

### Known Performance Issues
- **Bandwidth Throttling:** Uses pause/resume, not ideal for fine-grained control
- **Memory Buffering:** No streaming mode for large files (entire content buffered in memory)
- **Connection Pooling:** No connection reuse across downloads (new curl handle per doCurlInit)

---

## Integration Guidance

### For Consumer Developers

**Best Practice 1: Resource Lifecycle**
```c
CURL *curl = doCurlInit();
if (!curl) { /* handle error */ }

// Reuse curl for multiple downloads
for (each_file) {
    FileDwnl_t config = {...};
    doHttpFileDownload(curl, &config, NULL, 0, NULL, &http_code);
    // Process downloaded data
}

doStopDownload(curl);  // Single cleanup for batch
```

**Best Practice 2: Error Handling**
```c
int result = doHttpFileDownload(...);
int http_code = 0;
if (result != DWNL_SUCCESS || http_code != 200) {
    // Both result and http_code can indicate failure
    // Retry logic should consider both
}
```

**Best Practice 3: mTLS Credential Management**
```c
// Obtain credentials from rdkcertselector or HSM
MtlsAuth_t auth = {0};
strcpy(auth.cert_name, "/path/to/cert.pem");
strcpy(auth.cert_type, "PEM");
strcpy(auth.key_pas, "passphrase");

// Pass to download function
doHttpFileDownload(curl, &config, &auth, 0, NULL, &http_code);
```

### Dependencies & Linkage
```bash
# Compilation
gcc -c myapp.c -I/usr/include -I/opt/rdk/include

# Linking
gcc myapp.o -o myapp -ldwnlutil -lcurl -lrdkloggers -lpthread
```

---

## References

- libcurl documentation: https://curl.se/libcurl/c/
- CURL error codes: https://curl.se/libcurl/c/libcurl-errors.html
- Source: `dwnlutils/` directory
- Tests: `unit-test/dwnlutils/`, `test/functional-tests/`
