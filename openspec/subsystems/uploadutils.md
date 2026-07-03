# Upload Library (libuploadutil) - Detailed Analysis

**Module:** uploadutils/  
**Public API:** `uploadUtil.h`  
**Dependencies:** libdwnlutil, libfwutils, libcurl  
**Build Artifact:** `libuploadutil.so`  
**Feature Support:** mTLS (rdkcertselector), CodeBig protocol, S3 pre-signed URLs

---

## Module Purpose

Provides **complete HTTP/S3 file upload capabilities** with enterprise features:

- **Two-Stage Upload Workflow:** Metadata POST + file PUT (S3 or direct HTTP)
- **mTLS Certificate Management:** Dynamic certificate rotation via rdkcertselector
- **CodeBig Protocol Integration:** Backend-coordinated S3 upload orchestration
- **Upload Status Tracking:** Persistent state management and retry logic
- **Multiple Upload Backends:** HTTP direct, S3 pre-signed, CodeBig gateway

Primary consumers: device status monitoring, telemetry submission, firmware upload, configuration synchronization.

---

## Architecture Overview

### Subsystem Layers

```
┌─────────────────────────────────────┐
│  Consumer API (uploadUtil.h)        │
│  ├─ performMetadataPost()           │
│  ├─ performS3PutUpload()            │
│  └─ doStopUpload()                  │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│  Transport Layer (mtls_upload.c)    │
│  ├─ Credential rotation             │
│  └─ Certificate selection           │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│  Protocol Layers                    │
│  ├─ codebig_upload.c (CodeBig)      │
│  └─ upload_status.c (Tracking)      │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│  Download Library (libdwnlutil)     │
│  ├─ doCurlInit()                    │
│  └─ urlHelper layer                 │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│  libcurl (HTTP/HTTPS + mTLS I/O)    │
└─────────────────────────────────────┘
```

---

## Public API Surface

### Initialization & Lifecycle

```c
void doStopUpload(void *curl);
// Cleanup upload context (reuses download library cleanup)
// Parameters: curl - context from upload initialization
// Side effects: Releases curl resources, closes connections
```

### Core Upload Functions

```c
int performMetadataPost(
    void *curl,                       // Curl context
    const char *endpoint,             // Backend metadata service URL
    FileUpload_t *upload_spec,        // Upload configuration
    char *output_file                 // Path to save response (S3 URLs)
);
// Stage 1: Submit file metadata to backend service
// Backend returns S3 pre-signed URLs in response
// Usage: Obtain S3 upload credentials from CodeBig or backend
// Returns: 0 success, -1 failure
// Output: Writes S3 pre-signed URLs to output_file

int performS3PutUpload(
    const char *s3url,                // S3 pre-signed URL (from POST response)
    const char *localfile,            // Local file path to upload
    MtlsAuth_t *auth                  // Optional mTLS credentials
);
// Stage 2: Upload file to S3 via HTTP PUT
// Uses pre-signed URL from metadata POST stage
// Parameters: s3url - full S3 URL with query parameters
// Returns: 0 success, -1 failure
// Usage: After metadata POST, use returned URLs to upload
```

### Utility Functions

```c
int extractS3PresignedUrl(
    const char *result_file,          // File containing S3 URLs
    char *out_url,                    // Output buffer for URL
    size_t out_url_sz                 // Output buffer size
);
// Parse S3 pre-signed URL from result file
// Reads first line, strips trailing newline
// Returns: 0 success, -1 failure
// Usage: Extract URL from performMetadataPost() output file
```

### CodeBig Protocol Functions

```c
int performCodebigUpload(
    const char *device_id,            // Device identifier
    const char *local_file,           // File to upload
    const char *backend_url,          // CodeBig service endpoint
    MtlsAuth_t *auth                  // mTLS credentials
);
// Integrated CodeBig upload (metadata + S3 in one call)
// Internally handles two-stage workflow
// Returns: 0 success, -1 failure
// Usage: High-level upload with CodeBig orchestration

int getCodebigCredentials(
    char *out_endpoint,               // Output: CodeBig service URL
    size_t endpoint_sz,
    char *out_service_type            // Output: Service type identifier
);
// Query CodeBig service endpoint
// Reads from RFC or configuration
// Returns: 0 success, -1 failure
```

### mTLS Certificate Management

```c
int getMtlsCertificate(
    MtlsAuth_t *out_auth              // Output: Certificate structure
);
// Obtain current mTLS certificate via rdkcertselector
// Handles certificate rotation and fallback
// Returns: MTLS_CERT_FETCH_SUCCESS (0), MTLS_CERT_FETCH_FAILURE (-1)
// Usage: Called automatically by performMetadataPost() if enabled

int rotateMtlsCertificate(void);
// Trigger certificate rotation cycle
// Requests new certificate from rdkcertselector
// Returns: 0 success, -1 failure
// Usage: After certificate expiration or explicit request
```

### Upload Status Tracking

```c
int getUploadStatus(
    const char *status_file           // Path to status file
);
// Query upload operation result
// Reads persistent state file
// Returns: UPLOAD_SUCCESS (0), UPLOAD_FAIL (-1)

void setUploadStatus(
    long http_code,                   // HTTP response code
    int curl_code                     // libcurl error code
);
// Record upload operation result (internal)
// Persists status for diagnostics
// Invoked automatically by upload functions
```

---

## Data Structures

### FileUpload_t - Upload Configuration

```c
typedef struct {
    char *url;                        // Target endpoint URL
    char *pathname;                   // Local file path
    char *pPostFields;                // POST payload (metadata)
    int sslverify;                    // SSL peer verification (0/1)
    UploadHashData_t *hashData;       // Optional hash headers
} FileUpload_t;
```

### UploadHashData_t - Content Verification

```c
typedef struct {
    const char *hashvalue;            // Hash header (e.g., "x-md5: abc123")
    const char *hashtime;             // Timestamp header
} UploadHashData_t;
// Used in metadata POST to track file integrity across upload
```

### MtlsAuth_t - mTLS Credentials (Shared)

```c
typedef struct credential {
    char cert_name[64];               // Certificate path/identifier
    char cert_type[16];               // Certificate format (PEM, DER)
    char key_pas[65];                 // Key passphrase
#ifdef LIBRDKCERTSELECTOR
    char engine[32];                  // HSM engine name (optional)
#endif
} MtlsAuth_t;
// Retrieved from rdkcertselector when IS_LIBRDKCONFIG_ENABLED
```

---

## Implementation Architecture

### Module Composition

| Component | File | Responsibility |
|-----------|------|-----------------|
| **Core Upload API** | uploadUtil.c | Entry points, two-stage orchestration |
| **mTLS Support** | mtls_upload.c | Certificate management, rotation |
| **CodeBig Protocol** | codebig_upload.c | CodeBig-specific request/response handling |
| **Utilities** | codebigUtils.c | Configuration parsing, credential retrieval |
| **Status Tracking** | upload_status.c | Persistent state management |

### Two-Stage Upload Workflow

**Stage 1: Metadata POST**

```
Consumer: performMetadataPost()
├─ Open output file for S3 URLs
├─ Prepare HTTP POST request
│  ├─ URL endpoint (backend metadata service)
│  ├─ POST fields (JSON metadata: device ID, file name, size, hash)
│  ├─ Headers (Content-Type: application/json, Auth token/mTLS)
│  └─ SSL verification (if enabled)
├─ Call performMetadataPost() via libcurl
│  ├─ If mTLS enabled: getMtlsCertificate() → Obtain cert
│  ├─ Set CURLOPT_POST with fields
│  ├─ Set CURLOPT_HTTPHEADER with auth
│  └─ curl_easy_perform()
├─ Receive response:
│  ├─ JSON with S3 pre-signed URLs
│  ├─ HTTP 200 = success, write URLs to output_file
│  └─ Other codes = error, no URLs
└─ Return 0 (success) or -1 (failure)

Output: File containing lines like:
  https://s3.amazonaws.com/bucket/file.bin?X-Amz-Algorithm=...&X-Amz-Credential=...
```

**Stage 2: S3 PUT Upload**

```
Consumer: performS3PutUpload()
├─ Extract S3 URL from result file (extractS3PresignedUrl())
├─ Open local file for reading
├─ Get file size
├─ Prepare HTTP PUT request
│  ├─ URL: Pre-signed S3 URL
│  ├─ Data source: Local file handle
│  ├─ Headers: Content-Type, optional mTLS
│  └─ Method: PUT (curl_easy_setopt(CURLOPT_PUT, 1L))
├─ Call performS3PutUpload() via libcurl
│  ├─ Set CURLOPT_READDATA (file pointer)
│  ├─ Set CURLOPT_INFILESIZE_LARGE (file size)
│  ├─ Apply mTLS if auth provided
│  ├─ Optional: CURLOPT_VERBOSE if CURL_DEBUG enabled
│  └─ curl_easy_perform()
├─ Receive response:
│  ├─ HTTP 200 = Upload successful
│  └─ Other codes = Upload failed (connection error, auth, quota)
└─ Return 0 (success) or -1 (failure)
```

### CodeBig Integration

**High-Level CodeBig Upload (performCodebigUpload):**

```
Consumer: performCodebigUpload()
├─ Phase 1: Discover CodeBig endpoint
│  ├─ getCodebigCredentials()
│  ├─ Read from /opt/swupdate.conf or RFC
│  └─ Determine service type (SSR_SERVICE, XCONF_SERVICE, etc.)
├─ Phase 2: Metadata submission
│  ├─ buildCodebigMetadata() (internal)
│  ├─ Include device ID, file info, service type
│  ├─ Set Authorization header
│  └─ POST to CodeBig endpoint
├─ Phase 3: Certificate handling (if mTLS required)
│  ├─ getMtlsCertificate() via rdkcertselector
│  ├─ Apply cert to subsequent requests
│  └─ Retry with rotated cert on CURLE_SSL_CERTPROBLEM
├─ Phase 4: S3 URL extraction
│  ├─ Parse JSON response
│  └─ Extract pre-signed S3 URLs
└─ Phase 5: File upload to S3
   ├─ performS3PutUpload() with extracted URLs
   └─ Return final result
```

**CodeBig Service Discovery:**

```c
#define XCONF_SERVICE        2
#define CODEBIG_SERVICE      3
#define DAC15_SERVICE       14

// getCodebigCredentials() logic:
if (RFC_enabled) {
    read RFC parameters → XCONF_SERVICE or CODEBIG_SERVICE
} else if (config_file exists) {
    parse /opt/swupdate.conf → service endpoint
} else {
    return -1 (no configuration found)
}
```

### Certificate Rotation with rdkcertselector

**When IS_LIBRDKCONFIG_ENABLED:**

```c
// During upload:
MtlsAuth_t auth = {0};
int status = getMtlsCertificate(&auth);  // Fetch from rdkcertselector
if (status == MTLS_CERT_FETCH_FAILURE) {
    // Retry with rotated certificate
    rotateMtlsCertificate();
    status = getMtlsCertificate(&auth);
}

// Apply certificate:
if (auth.engine[0]) {
    // HSM-based certificate
    use_pkcs11_engine(auth.engine);
}
curl_easy_setopt(curl, CURLOPT_SSLCERT, auth.cert_name);
curl_easy_setopt(curl, CURLOPT_SSLKEY, auth.cert_name);
curl_easy_setopt(curl, CURLOPT_KEYPASSWD, auth.key_pas);
```

---

## Compile-Time Options

| Flag | Behavior |
|------|----------|
| `IS_LIBRDKCONFIG_ENABLED` | Enable rdkcertselector for mTLS cert rotation, link `-lrdkcertselector -lrdkconfig` |
| `USE_CPC_CODE` | Use alternate CodeBig implementation from CPC library |
| `L2UPLOADENABLED` | Enable Layer 2 container upload mode (affects SSL verification) |
| `CURL_DEBUG` | Enable libcurl verbose logging |
| `-D_ANSC_LINUX` | Linux platform target |

---

## Runtime Configuration

### Configuration Files

| File | Purpose | Format |
|------|---------|--------|
| `/opt/swupdate.conf` | CodeBig endpoint configuration | INI-style, key=value |
| `/opt/secure/RFC/bootstrap.ini` | Bootstrap + RFC configuration | INI-style, service URLs |
| `/tmp/upload_status.json` | Upload operation status (persistent) | JSON |

### Environment-Based Configuration

- **RFC Engine:** If RFC enabled, uses RFC parameters for service discovery
- **Fallback:** If RFC unavailable, uses configuration files
- **Manual Override:** Can pass explicit endpoint URLs to API functions

---

## Error Handling & Recovery

### HTTP Status Codes

| Code | Meaning | Recovery Action |
|------|---------|-----------------|
| 200 | Success | Mark as complete |
| 400-499 | Client error (auth, malformed) | Retry with corrected parameters or abort |
| 500-599 | Server error | Retry with exponential backoff |
| Timeout | Connection/read timeout | Retry with possibly rotated certificate |

### Certificate-Related Errors

| Error | Cause | Recovery |
|-------|-------|----------|
| CURLE_SSL_CERTPROBLEM | Invalid/missing client cert | Call rotateMtlsCertificate(), retry |
| CURLE_SSL_CONNECT_ERROR | SSL handshake failed | Check certificate expiration, rotate |
| CURLE_SSL_CAINFO | Server cert validation failed | Verify CA bundle, network intercept |

### Retry Logic

**Recommended Pattern:**
```c
int max_retries = 3;
for (int i = 0; i < max_retries; i++) {
    int result = performS3PutUpload(s3_url, file, &auth);
    if (result == 0) break;  // Success
    
    if (i < max_retries - 1) {
        sleep(2 ^ i);  // Exponential backoff: 2s, 4s
        if (auth) {
            rotateMtlsCertificate();  // Refresh cert
            getMtlsCertificate(&auth);
        }
    }
}
```

---

## Performance Characteristics

### Memory Usage (Typical)
- libuploadutil.so: ~48 KB (shared library)
- Per-upload context: ~4-8 KB
- File buffering: 0 (streams via curl from file)

### Network Behavior
- **Stage 1 (Metadata POST):** Quick (metadata-only, typically <1 MB)
- **Stage 2 (S3 PUT):** Limited by file size and network bandwidth
- **No connection reuse:** Each upload creates new curl context
- **No resumable uploads:** PUT fails entirely on disconnection

### Throughput
- Limited by network (libcurl pass-through)
- No builtin bandwidth limiting (unlike download library)
- Suitable for:
  - Device logs (1-10 MB)
  - Status updates (< 1 MB)
  - Configuration snapshots (< 5 MB)

### Known Performance Issues
- **Two-Stage Overhead:** Requires two separate HTTP requests
- **Pre-signed URL Expiration:** URLs from Stage 1 may expire before Stage 2 completes (typically 15 min timeout)
- **No resumable uploads:** Large files risk failure mid-transfer
- **Sequential workflow:** Cannot parallelize metadata + upload

---

## Testing Infrastructure

### Unit Tests (gtest)

**Test File:** `unit-test/uploadutil/uploadUtil_gtest.cpp`

**Coverage Areas:**
- Metadata POST parameter validation
- S3 PUT request construction
- Certificate handling
- CodeBig endpoint discovery
- mTLS credential injection
- Error code propagation
- SSL verification modes

**Mocking:**
- `mocks/curl_mock.h` - libcurl function mocking
- `mocks/mock_urlHelper.h` - URL helper layer mocking

### Mock-Based Testing Example

```cpp
// Mock S3 PUT upload success
EXPECT_CALL(g_curlMock, curl_easy_setopt(_, CURLOPT_PUT, 1L))
    .WillOnce(Return(CURLE_OK));
EXPECT_CALL(g_curlMock, curl_easy_perform(_))
    .WillOnce(Return(CURLE_OK));
EXPECT_CALL(g_curlMock, curl_easy_getinfo(_, CURLINFO_RESPONSE_CODE, _))
    .WillOnce(DoAll(SetArgPointee<2>(200L), Return(CURLE_OK)));

// Call function under test
int result = performS3PutUpload(s3_url, "/tmp/file.bin", &auth);

// Verify result
EXPECT_EQ(result, 0);  // Success
```

---

## Integration Guidance

### For Consumer Developers

**Pattern 1: Simple HTTP Upload**
```c
// Direct upload to backend without CodeBig
FileUpload_t config = {
    .url = "https://backend.example.com/upload",
    .pathname = "/tmp/device.log",
    .pPostFields = "{\"device_id\":\"123\"}",
    .sslverify = 1
};

CURL *curl = doCurlInit();
int result = performMetadataPost(curl, config.url, &config, "/tmp/s3_urls.txt");
doStopUpload(curl);
```

**Pattern 2: CodeBig Upload with mTLS**
```c
MtlsAuth_t auth = {0};
getMtlsCertificate(&auth);  // Obtain certificate

int result = performCodebigUpload(
    "device-123",                                    // Device ID
    "/tmp/status.json",                              // File to upload
    "https://codebig.example.com/v1/upload",         // Endpoint
    &auth                                            // mTLS credentials
);

if (result != 0) {
    rotateMtlsCertificate();  // Retry with new cert
}
```

**Pattern 3: Two-Stage with Error Handling**
```c
CURL *curl = doCurlInit();
char s3_url[512];

// Stage 1: Get S3 URL
int result = performMetadataPost(curl, endpoint, &config, "/tmp/s3.txt");
if (result != 0) {
    fprintf(stderr, "Metadata POST failed\n");
    doStopUpload(curl);
    return -1;
}

// Extract S3 URL
extractS3PresignedUrl("/tmp/s3.txt", s3_url, sizeof(s3_url));

// Stage 2: Upload file
result = performS3PutUpload(s3_url, "/tmp/file.bin", NULL);
doStopUpload(curl);

return result;
```

### Dependencies & Linkage
```bash
# With mTLS support
gcc -c myapp.c -I/usr/include

# Link
gcc myapp.o -o myapp \
    -luploadutil -ldwnlutil -lfwutils \
    -lcurl -lrdkloggers -lpthread \
    -lRdkCertSelector -lrdkconfig
```

---

## References

- libcurl documentation: https://curl.se/libcurl/c/
- S3 pre-signed URLs: https://docs.aws.amazon.com/AmazonS3/latest/userguide/PresignedUrlUploadObject.html
- Source: `uploadutils/` directory
- Tests: `unit-test/uploadutil/`
