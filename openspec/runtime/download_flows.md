# Download Operation Sequences - Runtime Flows

**Module:** dwnlutils - Download Library  
**Scope:** HTTP/HTTPS file download with optional mTLS and resumable transfers

---

## Sequence Diagram: Standard File Download

```mermaid
sequenceDiagram
    participant Consumer
    participant libdwnlutil as Download Lib
    participant urlHelper as URL Helper
    participant libcurl as libcurl
    participant Server
    participant Filesystem

    Consumer->>libdwnlutil: doCurlInit()
    libdwnlutil->>urlHelper: urlHelperCreateCurl()
    urlHelper->>libcurl: curl_easy_init()
    libcurl-->>urlHelper: CURL* handle
    urlHelper-->>libdwnlutil: CURL* handle
    libdwnlutil-->>Consumer: void* curl

    Consumer->>Consumer: Prepare FileDwnl_t
    Note over Consumer: Set URL, path, SSL, headers

    Consumer->>libdwnlutil: doHttpFileDownload(curl, config, auth, speed, offset, &http_code)
    libdwnlutil->>urlHelper: setCommonCurlOpt(curl, url, NULL, ssl_verify)
    urlHelper->>libcurl: CURLOPT_URL, CURLOPT_SSL_*
    libdwnlutil->>urlHelper: setMtlsHeaders(curl, cert)
    urlHelper->>libcurl: CURLOPT_SSLCERT, CURLOPT_SSLKEY
    libdwnlutil->>libcurl: curl_easy_perform()
    libcurl->>Server: CONNECT (TCP 3-way)
    libcurl->>Server: TLS Handshake
    Server-->>libcurl: TLS established
    libcurl->>Server: GET /file.bin HTTP/1.1
    Server-->>libcurl: 200 OK, Content-Length: 1024
    libcurl-->>libcurl: Invoke write_callback()
    libcurl-->>Filesystem: Write chunk to buffer
    libcurl-->>libcurl: Repeat for each chunk
    Server-->>libcurl: Connection close
    libcurl->>libcurl: Finalize transfer
    libcurl-->>libdwnlutil: CURLE_OK
    libdwnlutil->>libcurl: curl_easy_getinfo(CURLINFO_RESPONSE_CODE)
    libcurl-->>libdwnlutil: 200
    libdwnlutil-->>Consumer: DWNL_SUCCESS, http_code=200

    Consumer->>libdwnlutil: doStopDownload(curl)
    libdwnlutil->>urlHelper: urlHelperDestroyCurl()
    urlHelper->>libcurl: curl_easy_cleanup()
    libcurl-->>urlHelper: OK
    urlHelper-->>libdwnlutil: OK
    libdwnlutil-->>Consumer: OK
```

---

## Sequence Diagram: Resumable Download (With Bandwidth Throttling)

```mermaid
sequenceDiagram
    participant Consumer
    participant libdwnlutil as Download Lib
    participant urlHelper as URL Helper
    participant libcurl as libcurl
    participant Server

    Consumer->>libdwnlutil: doCurlInit()
    Note over libdwnlutil: curl* initialized

    Consumer->>libdwnlutil: doHttpFileDownload(curl, config, NULL, 256KB, "1000", &http_code)
    Note over libdwnlutil: Resume from byte offset 1000
    libdwnlutil->>urlHelper: setCommonCurlOpt(curl, ...)
    urlHelper->>libcurl: CURLOPT_RANGE: "bytes=1000-"
    libdwnlutil->>libcurl: curl_easy_perform()
    libcurl->>Server: GET /file.bin HTTP/1.1<br/>Range: bytes=1000-
    Server-->>libcurl: 206 Partial Content
    libcurl-->>libcurl: Download starts...
    
    par Download in Progress
        libcurl-->>libdwnlutil: Data chunking
    and Monitoring
        Consumer->>libdwnlutil: doGetDwnlBytes(curl)
        libdwnlutil->>libcurl: curl_easy_getinfo(CURLINFO_SIZE_DOWNLOAD_T)
        libcurl-->>libdwnlutil: 50000 bytes
        libdwnlutil-->>Consumer: 50000
    end

    Note over Consumer: 50% complete, apply throttle
    Consumer->>libdwnlutil: doInteruptDwnl(curl, 256*1024)
    libdwnlutil->>libcurl: curl_easy_pause(CURLPAUSE_ALL)
    libcurl-->>libdwnlutil: OK (download paused)
    libdwnlutil->>urlHelper: setThrottleMode(curl, 256KB)
    urlHelper->>libcurl: CURLOPT_MAX_RECV_SPEED_LARGE: 262144
    libdwnlutil->>libcurl: curl_easy_pause(CURLPAUSE_CONT)
    libcurl-->>libdwnlutil: OK (resumed at throttle)
    
    libcurl-->>libcurl: Continue download @ 256 KB/s
    libcurl-->>Server: Receive remaining data
    Server-->>libcurl: End of stream
    libcurl-->>libdwnlutil: CURLE_OK

    libdwnlutil-->>Consumer: DWNL_SUCCESS, http_code=206

    Consumer->>libdwnlutil: doStopDownload(curl)
```

---

## Sequence Diagram: OAuth-Based Download

```mermaid
sequenceDiagram
    participant Consumer
    participant libdwnlutil as Download Lib
    participant libcurl as libcurl
    participant AuthServer as OAuth Provider
    participant FileServer

    Consumer->>Consumer: Obtain OAuth token
    Note over Consumer: token = "Bearer abc123..."

    Consumer->>Consumer: Prepare FileDwnl_t
    Note over Consumer: pPostFields = "Bearer abc123..."

    Consumer->>libdwnlutil: doCurlInit()
    libdwnlutil-->>Consumer: curl*

    Consumer->>libdwnlutil: doAuthHttpFileDownload(curl, config, &http_code)
    libdwnlutil->>libdwnlutil: Extract token from pPostFields
    libdwnlutil->>libcurl: CURLOPT_HTTPHEADER: Authorization: Bearer abc123...
    libdwnlutil->>libcurl: curl_easy_perform()
    
    libcurl->>FileServer: GET /file.bin HTTP/1.1<br/>Authorization: Bearer abc123...
    FileServer->>AuthServer: Validate token (async)
    AuthServer-->>FileServer: Token valid, user=device123
    FileServer-->>libcurl: 200 OK, stream file
    libcurl-->>libdwnlutil: CURLE_OK, response 200

    libdwnlutil-->>Consumer: DWNL_SUCCESS

    Consumer->>libdwnlutil: doStopDownload(curl)
```

---

## Sequence Diagram: mTLS Download

```mermaid
sequenceDiagram
    participant Consumer
    participant libdwnlutil as Download Lib
    participant urlHelper as URL Helper
    participant libcurl as libcurl
    participant certSelector as rdkcertselector
    participant FileServer

    Consumer->>Consumer: Load MtlsAuth_t
    Note over Consumer: cert_name, cert_type, key_pas

    Consumer->>libdwnlutil: doCurlInit()
    libdwnlutil-->>Consumer: curl*

    Consumer->>libdwnlutil: doHttpFileDownload(curl, config, &auth, 0, NULL, &http_code)
    libdwnlutil->>urlHelper: setMtlsHeaders(curl, &auth)
    urlHelper->>libcurl: CURLOPT_SSLCERT: auth.cert_name
    urlHelper->>libcurl: CURLOPT_SSLKEY: auth.cert_name
    urlHelper->>libcurl: CURLOPT_KEYPASSWD: auth.key_pas
    
    alt If HSM Engine Enabled
        urlHelper->>certSelector: Load cert from HSM
        certSelector-->>urlHelper: cert_handle
        urlHelper->>libcurl: CURLOPT_SSLCERT: (HSM handle)
    end

    libdwnlutil->>libcurl: curl_easy_perform()
    libcurl->>FileServer: CONNECT (TCP)
    libcurl->>FileServer: TLS Handshake
    libcurl->>libcurl: Load client cert from auth.cert_name
    libcurl->>FileServer: Send CLIENT CERTIFICATE message
    FileServer->>FileServer: Validate certificate chain
    FileServer-->>libcurl: TLS established
    libcurl->>FileServer: GET /file.bin HTTP/1.1
    FileServer-->>libcurl: 200 OK, stream file
    libcurl-->>libdwnlutil: CURLE_OK

    libdwnlutil-->>Consumer: DWNL_SUCCESS, http_code=200
```

---

## Sequence Diagram: Error Scenario - Connection Timeout

```mermaid
sequenceDiagram
    participant Consumer
    participant libdwnlutil as Download Lib
    participant libcurl as libcurl
    participant Network
    participant Server

    Consumer->>libdwnlutil: doHttpFileDownload(curl, config, NULL, 0, NULL, &http_code)
    libdwnlutil->>libcurl: curl_easy_perform()
    libcurl->>Network: DNS lookup: example.com
    Network-->>libcurl: TIMEOUT (30 sec elapsed)
    libcurl-->>libcurl: Return CURLE_OPERATION_TIMEDOUT
    libcurl-->>libdwnlutil: CURLE_OPERATION_TIMEDOUT

    libdwnlutil->>libdwnlutil: Log error
    libdwnlutil->>libcurl: curl_easy_getinfo(CURLINFO_RESPONSE_CODE)
    libcurl-->>libdwnlutil: 0 (no connection)

    libdwnlutil-->>Consumer: DWNL_FAIL, http_code=0

    Note over Consumer: Handle error: retry, backoff, or fail
```

---

## State Machine: Download Session Lifecycle

```
┌─────────────────────────────────────────────────────────────┐
│                      DOWNLOAD SESSION                        │
└─────────────────────────────────────────────────────────────┘

[INIT]
  │
  └─> doCurlInit()
      ├─> curl_global_init() [once]
      ├─> curl_easy_init()
      └─> curl* handle created
          │
          ▼
[READY]
  │
  ├─> Consumer sets up FileDwnl_t config
  │   (URL, file path, SSL settings, headers)
  │
  └─> Consumer calls doHttpFileDownload()
      │
      ▼
[CONNECTING]
  │
  ├─> setCommonCurlOpt()
  │   ├─> CURLOPT_URL
  │   ├─> CURLOPT_CONNECTTIMEOUT = 30s
  │   ├─> CURLOPT_TIMEOUT = 7200s
  │   └─> CURLOPT_SSL_VERIFYPEER
  │
  ├─> setMtlsHeaders() [if auth provided]
  │   ├─> CURLOPT_SSLCERT
  │   └─> CURLOPT_SSLKEY
  │
  └─> curl_easy_perform()
      │
      ├─ DNS Resolution
      ├─ TCP Connect (socket)
      ├─ TLS Handshake (if HTTPS)
      └─ HTTP Request Send
          │
          ▼
[DOWNLOADING]
  │
  ├─> libcurl write_callback() invoked per chunk
  │   └─> Consumer DownloadData buffer populated
  │
  ├─> doGetDwnlBytes() can query progress
  │
  ├─> doInteruptDwnl() can apply throttling
  │   ├─> curl_easy_pause(CURLPAUSE_ALL)
  │   ├─> setThrottleMode()
  │   └─> curl_easy_pause(CURLPAUSE_CONT)
  │
  └─> curl_easy_perform() continues
      │
      ├─ Server sends response body
      ├─ libcurl writes to callback
      └─ Final chunk received
          │
          ▼
[COMPLETE/ERROR]
  │
  ├─> CURLE_OK → DWNL_SUCCESS
  │   └─> HTTP code available (200, 206, etc.)
  │
  ├─> CURLcode error → DWNL_FAIL
  │   ├─ CURLE_OPERATION_TIMEDOUT
  │   ├─ CURLE_SSL_CERTPROBLEM
  │   ├─ CURLE_COULDNT_CONNECT
  │   └─ [other libcurl errors]
  │
  └─> doStopDownload()
      ├─> urlHelperDestroyCurl()
      ├─> curl_easy_cleanup()
      └─ Sockets closed, memory freed
          │
          ▼
[DESTROYED]
```

---

## Error Recovery Patterns

### Pattern 1: Retry with Exponential Backoff

```c
int download_with_retry(
    const char *url, 
    const char *output_file, 
    int max_retries
) {
    for (int attempt = 0; attempt < max_retries; attempt++) {
        CURL *curl = doCurlInit();
        if (!curl) continue;
        
        FileDwnl_t config = {0};
        strcpy(config.url, url);
        strcpy(config.pathname, output_file);
        
        int http_code = 0;
        int result = doHttpFileDownload(curl, &config, NULL, 0, NULL, &http_code);
        
        doStopDownload(curl);
        
        if (result == DWNL_SUCCESS && http_code == 200) {
            return 0;  // Success
        }
        
        if (attempt < max_retries - 1) {
            int backoff_ms = 1000 * (1 << attempt);  // 1s, 2s, 4s, ...
            usleep(backoff_ms * 1000);
        }
    }
    
    return -1;  // Failed after retries
}
```

### Pattern 2: Resumable Download After Failure

```c
int download_resumable(const char *url, const char *output_file) {
    CURL *curl = doCurlInit();
    char resume_offset[32] = "0";
    
    // Check if partial file exists
    if (filePresentCheck(output_file)) {
        int partial_size = getFileSize(output_file);
        snprintf(resume_offset, sizeof(resume_offset), "%d", partial_size);
    }
    
    FileDwnl_t config = {0};
    strcpy(config.url, url);
    strcpy(config.pathname, output_file);
    
    int http_code = 0;
    int result = doHttpFileDownload(curl, &config, NULL, 0, resume_offset, &http_code);
    
    doStopDownload(curl);
    
    if (result == DWNL_SUCCESS && http_code == 206) {
        // 206 Partial Content - resume successful
        return 0;
    } else if (result == DWNL_SUCCESS && http_code == 200) {
        // 200 OK - fresh download
        return 0;
    }
    
    return -1;
}
```

---

## Performance Timeline: Large File Download

```
Timeline (seconds):
0    │ doCurlInit()
     │
5    │ curl_easy_perform() starts
     │ ├─ DNS resolution
     │ ├─ TCP connection
     │ ├─ TLS handshake
     │ └─ HTTP GET sent
     │
10   │ First data chunk received
     │ Throughput: ~10 MB/s (network dependent)
     │
15   │ doGetDwnlBytes() → 50 MB downloaded (progress)
     │
30   │ doInteruptDwnl() called
     │ ├─ curl_easy_pause(CURLPAUSE_ALL)
     │ ├─ Throttle set to 256 KB/s
     │ ├─ curl_easy_pause(CURLPAUSE_CONT)
     │ └─ Throughput now: 256 KB/s (throttled)
     │
65   │ doGetDwnlBytes() → 100 MB downloaded
     │
120  │ Final chunk received
     │ Total: ~200 MB file
     │ Time: ~115 seconds (120 - 5 for handshake)
     │ Effective throughput: ~1.7 MB/s (including throttle)
     │
121  │ curl_easy_perform() returns CURLE_OK
     │ http_code = 200
     │
122  │ doStopDownload() called
     │ Cleanup complete
     │
Total operation time: ~122 seconds
```

---

## Memory Usage Timeline

```
Memory utilization during download:

Phase 1: Initialization (doCurlInit)
├─ libcurl handle: ~8 KB
├─ SSL context: ~16 KB
└─ Internal buffers: ~32 KB
   Total: ~56 KB

Phase 2: Download setup (setCommonCurlOpt, setMtlsHeaders)
├─ cURL options: ~4 KB
├─ Header strings: ~2 KB
└─ (No significant increase)
   Total: ~60 KB

Phase 3: Downloading (write_callback invoked)
├─ libcurl internal buffers: ~64 KB
├─ Consumer DownloadData buffer: Variable (consumer-managed)
│  (Consumer is responsible for this - can be 1 MB, 100 MB, etc.)
├─ SSL session cache: ~16 KB
└─ Total (excluding consumer buffer): ~80+ KB
   Total (with 100 MB consumer buffer): ~100 MB+

Phase 4: Cleanup (doStopDownload)
├─ SSL context freed
├─ Socket buffers freed
├─ libcurl handle freed
└─ Total freed: ~60 KB
   Remaining: 0 KB
```

---

## References

- libcurl Performance Tips: https://curl.se/libcurl/c/libcurl-tutorial.html
- CURL Error Codes: https://curl.se/libcurl/c/libcurl-errors.html
- HTTP Resume Capability: RFC 7233 (Range Requests)
