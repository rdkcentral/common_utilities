# Architecture Diagrams - Visual Analysis

**Scope:** Common Utilities Ecosystem Architecture  
**Format:** Mermaid diagrams with textual explanations

---

## Diagram 1: Complete System Architecture

```mermaid
graph TB
    subgraph "RDK Consumer Components"
        FWUPD["🔧 Firmware Updater<br/>(rdkfwupdater)"]
        RFC["⚙️ RFC Engine<br/>(RFC Service)"]
        RDM["📱 RDK Manager<br/>(Management Daemon)"]
        MONITOR["📊 Status Monitor<br/>(Telemetry)"]
        DIAG["🔍 Diagnostic Tool<br/>(System Health)"]
    end

    subgraph "Common Utilities Ecosystem (OpenSource)"
        subgraph "Download & Upload"
            DL["📥 libdwnlutil<br/>(Download Library)"]
            UL["📤 libuploadutil<br/>(Upload Library)"]
        end
        
        subgraph "Configuration & Device"
            JSON["📋 libparsejson<br/>(JSON Parser)"]
            DEV["🖥️ libfwutils<br/>(Device Utils)"]
        end
    end

    subgraph "External Dependencies (3rd Party)"
        CURL["libcurl<br/>(HTTP/HTTPS Transport)"]
        CJSON["libcJSON<br/>(JSON Parsing)"]
        SSL["OpenSSL<br/>(TLS/mTLS)"]
        CERTSEL["librdkcertselector<br/>(mTLS Certs)"]
    end

    subgraph "RDK Infrastructure"
        LOG["rdkv_cdl_log_wrapper<br/>(Logging)"]
        RFC_SVC["RFC Service<br/>(Configuration)"]
    end

    subgraph "Backend Services"
        XCONF["🌐 XConf Service<br/>(Metadata + Download)"]
        CODEBIG["☁️ CodeBig Gateway<br/>(Upload Orchestration)"]
        S3["💾 S3 Storage<br/>(File Repository)"]
    end

    FWUPD -->|Uses| DL
    RFC -->|Uses| JSON
    RDM -->|Uses| DL
    MONITOR -->|Uses| UL
    DIAG -->|Uses| DEV

    DL -->|Depends| CURL
    DL -->|Depends| SSL
    UL -->|Depends| DL
    UL -->|Depends| DEV
    UL -->|Depends| JSON
    JSON -->|Depends| CJSON
    DEV -->|Depends| JSON
    DEV -->|Depends| LOG

    DL -->|Logs via| LOG
    UL -->|Logs via| LOG
    JSON -->|Logs via| LOG
    DEV -->|Logs via| LOG

    CURL -->|Talks to| XCONF
    CURL -->|Talks to| CODEBIG
    CURL -->|Talks to| S3

    DEV -->|Reads| RFC_SVC
    UL -->|Reads| RFC_SVC

    CERTSEL -->|Provides certs for| DL
    CERTSEL -->|Provides certs for| UL

    style FWUPD fill:#e1f5ff
    style RFC fill:#e1f5ff
    style RDM fill:#e1f5ff
    style MONITOR fill:#e1f5ff
    style DIAG fill:#e1f5ff
    style DL fill:#c8e6c9
    style UL fill:#c8e6c9
    style JSON fill:#c8e6c9
    style DEV fill:#c8e6c9
    style CURL fill:#ffe0b2
    style CJSON fill:#ffe0b2
    style SSL fill:#ffe0b2
    style CERTSEL fill:#ffe0b2
    style XCONF fill:#f8bbd0
    style CODEBIG fill:#f8bbd0
    style S3 fill:#f8bbd0
```

---

## Diagram 2: Dependency Graph (Layered)

```mermaid
graph TD
    subgraph "Layer 0: External Libraries (System)"
        L0A["libcurl"]
        L0B["libcJSON"]
        L0C["OpenSSL"]
        L0D["pthreads"]
        L0E["glibc"]
    end

    subgraph "Layer 1: RDK Base Libraries"
        L1A["librdkloggers"]
        L1B["rdkcertselector"]
    end

    subgraph "Layer 2: Foundation Libraries"
        L2A["libparsejson"]
        L2B["libdwnlutil"]
    end

    subgraph "Layer 3: Integrated Services"
        L3A["libfwutils"]
        L3B["libuploadutil"]
    end

    subgraph "Layer 4: RDK Consumers"
        L4A["rdkfwupdater"]
        L4B["RFC Engine"]
        L4C["RDK Manager"]
        L4D["Status Monitor"]
    end

    L0A --> L2B
    L0B --> L2A
    L0C --> L2B
    L0D --> L2B
    L0E --> |base|L0A
    L1A --> L2A
    L1A --> L2B
    L1A --> L3A
    L1A --> L3B
    L1B --> L3B

    L2A --> L3A
    L2A --> L3B
    L2B --> L3B

    L3A --> L4A
    L3A --> L4B
    L3A --> L4C
    L3A --> L4D
    L3B --> L4D
    L2B --> L4A
    L2B --> L4B
    L2B --> L4C

    style L0A fill:#ffe0b2
    style L0B fill:#ffe0b2
    style L0C fill:#ffe0b2
    style L0D fill:#ffe0b2
    style L0E fill:#ffe0b2
    style L1A fill:#fff9c4
    style L1B fill:#fff9c4
    style L2A fill:#c8e6c9
    style L2B fill:#c8e6c9
    style L3A fill:#bbdefb
    style L3B fill:#bbdefb
    style L4A fill:#f8bbd0
    style L4B fill:#f8bbd0
    style L4C fill:#f8bbd0
    style L4D fill:#f8bbd0
```

**Interpretation:**
- **Layer 0 (External):** Open-source dependencies (libcurl, cJSON, OpenSSL)
- **Layer 1 (RDK Base):** Logging and certification infrastructure
- **Layer 2 (Foundation):** Standalone libraries (JSON parser, download utility)
- **Layer 3 (Integration):** Combined services (upload, device utils)
- **Layer 4 (Consumers):** RDK components that use the ecosystem

---

## Diagram 3: Data Flow - Download Operation

```mermaid
graph LR
    subgraph "Consumer"
        REQ["Request:<br/>URL, Path, Auth"]
    end

    subgraph "libdwnlutil"
        INIT["1. Initialize<br/>doCurlInit"]
        CONFIG["2. Configure<br/>setCommonCurlOpt<br/>setMtlsHeaders"]
        PERFORM["3. Perform<br/>curl_easy_perform"]
        CALLBACK["4. Write Callback<br/>Stream to Buffer"]
        STATUS["5. Get Status<br/>curl_easy_getinfo"]
    end

    subgraph "Transport"
        CURL["libcurl"]
        SOCKET["TCP Socket"]
        TLS["TLS 1.2+"]
    end

    subgraph "Remote"
        HTTP["HTTP Server"]
    end

    subgraph "Consumer Storage"
        BUFFER["Download Buffer<br/>DownloadData"]
        FILE["Filesystem"]
    end

    REQ -->|Config| INIT
    INIT -->|Handle| CONFIG
    CONFIG -->|Options| PERFORM
    PERFORM -->|Execute| CURL
    CURL -->|Network| SOCKET
    SOCKET -->|Encrypt| TLS
    TLS -->|Protocol| HTTP
    HTTP -->|Response| TLS
    TLS -->|Data| SOCKET
    SOCKET -->|Chunks| CURL
    CURL -->|Invoke| CALLBACK
    CALLBACK -->|Append| BUFFER
    CALLBACK -->|Write| FILE
    PERFORM -->|Complete| STATUS
    STATUS -->|HTTP Code| REQ

    style REQ fill:#e1f5ff
    style INIT fill:#c8e6c9
    style CONFIG fill:#c8e6c9
    style PERFORM fill:#c8e6c9
    style CALLBACK fill:#c8e6c9
    style STATUS fill:#c8e6c9
    style CURL fill:#ffe0b2
    style SOCKET fill:#ffe0b2
    style TLS fill:#ffe0b2
    style HTTP fill:#f8bbd0
    style BUFFER fill:#e1f5ff
    style FILE fill:#e1f5ff
```

---

## Diagram 4: Data Flow - Upload Operation

```mermaid
graph LR
    subgraph "Consumer"
        FILE_REQ["Request:<br/>File Path, Metadata"]
    end

    subgraph "Stage 1: Metadata POST"
        DISCOVER["1. Discover<br/>getCodebigCredentials"]
        BUILD["2. Build Metadata<br/>JSON Payload"]
        POST["3. POST Metadata<br/>curl_easy_perform"]
        PARSE["4. Parse Response<br/>Extract URLs"]
    end

    subgraph "Stage 2: S3 Upload"
        EXTRACT["5. Extract URL<br/>extractS3PresignedUrl"]
        OPEN["6. Open File<br/>Stream Setup"]
        PUT["7. PUT to S3<br/>curl_easy_perform"]
        VERIFY["8. Verify Status<br/>HTTP 200 Check"]
    end

    subgraph "Backend"
        METADATA_SVC["Metadata Service<br/>(CodeBig/XConf)"]
        S3_SVC["S3 Service<br/>(AWS)"]
    end

    FILE_REQ -->|Config| DISCOVER
    DISCOVER -->|Endpoint| BUILD
    BUILD -->|JSON| POST
    POST -->|Request| METADATA_SVC
    METADATA_SVC -->|Response| PARSE
    PARSE -->|URLs| EXTRACT
    EXTRACT -->|URL| OPEN
    OPEN -->|File Handle| PUT
    PUT -->|Upload| S3_SVC
    S3_SVC -->|Response| VERIFY
    VERIFY -->|Complete| FILE_REQ

    style FILE_REQ fill:#e1f5ff
    style DISCOVER fill:#bbdefb
    style BUILD fill:#bbdefb
    style POST fill:#bbdefb
    style PARSE fill:#bbdefb
    style EXTRACT fill:#c8e6c9
    style OPEN fill:#c8e6c9
    style PUT fill:#c8e6c9
    style VERIFY fill:#c8e6c9
    style METADATA_SVC fill:#f8bbd0
    style S3_SVC fill:#f8bbd0
```

---

## Diagram 5: Module Interaction Matrix

```
┌─────────────────────────────────────────────────────────────┐
│                MODULE INTERACTION MATRIX                     │
├──────────────┬──────────┬──────────┬──────────┬──────────────┤
│ From\To      │ dwnlutil │ uploadutil│ parsejson│ utils        │
├──────────────┼──────────┼──────────┼──────────┼──────────────┤
│ dwnlutil     │    —     │   ✓      │    ✗     │    ✗         │
│              │          │ (used by)│          │              │
├──────────────┼──────────┼──────────┼──────────┼──────────────┤
│ uploadutil   │    —     │    —     │    ✓     │    ✓         │
│              │          │          │ (uses)   │  (uses)      │
├──────────────┼──────────┼──────────┼──────────┼──────────────┤
│ parsejson    │    ✗     │    ✗     │    —     │    ✓         │
│              │          │          │          │ (used by)    │
├──────────────┼──────────┼──────────┼──────────┼──────────────┤
│ utils        │    ✗     │    ✗     │    ✗     │    —         │
│              │          │          │          │              │
├──────────────┼──────────┼──────────┼──────────┼──────────────┤
│ Symbols:                                                      │
│ ✓ = Direct dependency                                        │
│ ✗ = No dependency                                            │
│ — = Self (N/A)                                               │
└──────────────┴──────────┴──────────┴──────────┴──────────────┘

Call Flow:
  uploadutil → libdwnlutil (transport layer)
  uploadutil → parsejson (JSON parsing)
  uploadutil → utils (device config)
  utils → parsejson (JSON parsing)
```

---

## Diagram 6: Thread Safety Model

```mermaid
graph TD
    subgraph "Multi-threaded Consumer"
        T1["Thread 1"]
        T2["Thread 2"]
        T3["Thread 3"]
    end

    subgraph "libdwnlutil (Thread-Safe)"
        CURL1["CURL* Handle<br/>per Thread"]
        CURL2["CURL* Handle<br/>per Thread"]
        CURL3["CURL* Handle<br/>per Thread"]
        GLOBAL["Global Init<br/>pthread_once"]
    end

    subgraph "libcurl (Thread-Safe)"
        LS["Per-Handle<br/>Local Storage"]
    end

    T1 -->|Call doCurlInit| CURL1
    T2 -->|Call doCurlInit| CURL2
    T3 -->|Call doCurlInit| CURL3
    CURL1 --> GLOBAL
    CURL2 --> GLOBAL
    CURL3 --> GLOBAL
    CURL1 -->|Unique| LS
    CURL2 -->|Unique| LS
    CURL3 -->|Unique| LS

    style T1 fill:#e1f5ff
    style T2 fill:#e1f5ff
    style T3 fill:#e1f5ff
    style CURL1 fill:#c8e6c9
    style CURL2 fill:#c8e6c9
    style CURL3 fill:#c8e6c9
    style GLOBAL fill:#fff9c4
    style LS fill:#ffe0b2

    %%{init: {'flowchart': {'htmlLabels': false}}}%%
```

**Key Points:**
- Each thread must call `doCurlInit()` to get its own CURL* handle
- No global state shared between threads
- Global initialization happens once via `pthread_once`
- Safe for concurrent downloads/uploads from different threads

---

## Diagram 7: Compilation Architecture (Build System)

```mermaid
graph TB
    subgraph "Build Inputs"
        AC["configure.ac"]
        AM["Makefile.am"]
        SRC["Source Files<br/>(.c, .h)"]
        DEPS["Dependencies<br/>(pkg-config)"]
    end

    subgraph "GNU Autotools Pipeline"
        AUTOCONF["autoconf"]
        AUTOMAKE["automake"]
        CONFIG["./configure"]
    end

    subgraph "Per-Module Build"
        subgraph "dwnlutils"
            DL_OBJ["urlHelper.o<br/>downloadUtil.o<br/>curl_debug.o"]
            DL_LIB["libdwnlutil.la"]
        end
        subgraph "parsejson"
            JSON_OBJ["json_parse.o"]
            JSON_LIB["libparsejson.la"]
        end
        subgraph "utils"
            UTIL_OBJ["rdk_fwdl_utils.o<br/>system_utils.o<br/>..."]
            UTIL_LIB["libfwutils.la"]
        end
        subgraph "uploadutils"
            UP_OBJ["uploadUtil.o<br/>mtls_upload.o<br/>codebig_upload.o<br/>..."]
            UP_LIB["libuploadutil.la"]
        end
    end

    subgraph "Linking"
        LINK["libtool"]
    end

    subgraph "Final Output"
        DL_SO["libdwnlutil.so<br/>(52 KB)"]
        JSON_SO["libparsejson.so<br/>(24 KB)"]
        UTIL_SO["libfwutils.so<br/>(64 KB)"]
        UP_SO["libuploadutil.so<br/>(48 KB)"]
    end

    AC -->|Generates| CONFIG
    AM -->|Processed by| AUTOCONF
    DEPS -->|Resolved by| CONFIG
    CONFIG -->|Produces| Makefile
    
    SRC -->|Compile| DL_OBJ
    SRC -->|Compile| JSON_OBJ
    SRC -->|Compile| UTIL_OBJ
    SRC -->|Compile| UP_OBJ

    DL_OBJ -->|Link| DL_LIB
    JSON_OBJ -->|Link| JSON_LIB
    UTIL_OBJ -->|Link| UTIL_LIB
    UP_OBJ -->|Link| UP_LIB

    DL_LIB -->|libtool| DL_SO
    JSON_LIB -->|libtool| JSON_SO
    UTIL_LIB -->|libtool| UTIL_SO
    UP_LIB -->|libtool| UP_SO

    LINK -->|Manages| DL_SO
    LINK -->|Manages| JSON_SO
    LINK -->|Manages| UTIL_SO
    LINK -->|Manages| UP_SO

    style AC fill:#fff9c4
    style AM fill:#fff9c4
    style SRC fill:#ffe0b2
    style DEPS fill:#ffe0b2
    style AUTOCONF fill:#f8bbd0
    style AUTOMAKE fill:#f8bbd0
    style CONFIG fill:#f8bbd0
    style DL_LIB fill:#c8e6c9
    style JSON_LIB fill:#c8e6c9
    style UTIL_LIB fill:#c8e6c9
    style UP_LIB fill:#c8e6c9
    style DL_SO fill:#e1f5ff
    style JSON_SO fill:#e1f5ff
    style UTIL_SO fill:#e1f5ff
    style UP_SO fill:#e1f5ff
```

---

## Diagram 8: Feature Flags & Build Variants

```mermaid
graph TD
    BASE["Base Compilation<br/>-Wall -Werror -fPIC"]

    BASE -->|Flag: IS_LIBRDKCERTSEL_ENABLED| CERT["✓ mTLS Support<br/>-lRdkCertSelector"]
    BASE -->|Flag: NOT CERT| NO_CERT["✗ No mTLS<br/>mTLS functions unavailable"]

    BASE -->|Flag: IS_LIBRDKCONFIG_ENABLED| CONFIG["✓ RFC Integration<br/>-lrdkconfig"]
    BASE -->|Flag: NOT CONFIG| NO_CONFIG["✗ Hardcoded Config<br/>Files only"]

    BASE -->|Flag: CURL_DEBUG| DEBUG["✓ Verbose Logging<br/>CURL debug callback"]
    BASE -->|Flag: NOT DEBUG| NO_DEBUG["✗ Minimal Logging<br/>Errors only"]

    BASE -->|Flag: GTEST_ENABLE| TEST["✓ Test Mode<br/>Paths: /tmp/"]
    BASE -->|Flag: NOT GTEST| PROD["✗ Production Mode<br/>Paths: /etc/, /opt/"]

    BASE -->|Flag: L2UPLOADENABLED| L2["✓ L2 Upload<br/>Special SSL mode"]
    BASE -->|Flag: NOT L2| NO_L2["✗ Standard Upload"]

    CERT --> DL1["libdwnlutil.so<br/>with mTLS"]
    NO_CERT --> DL2["libdwnlutil.so<br/>basic only"]
    CONFIG --> UP1["libuploadutil.so<br/>with RFC"]
    NO_CONFIG --> UP2["libuploadutil.so<br/>file-based"]
    DEBUG --> LOG1["Verbose output"]
    NO_DEBUG --> LOG2["Minimal output"]
    TEST --> PATH1["Test Paths"]
    PROD --> PATH2["Device Paths"]
    L2 --> UP3["L2 Mode"]
    NO_L2 --> UP4["Standard Mode"]

    style BASE fill:#fff9c4
    style CERT fill:#c8e6c9
    style NO_CERT fill:#ffcdd2
    style CONFIG fill:#c8e6c9
    style NO_CONFIG fill:#ffcdd2
    style DEBUG fill:#c8e6c9
    style NO_DEBUG fill:#ffcdd2
    style TEST fill:#c8e6c9
    style PROD fill:#c8e6c9
    style L2 fill:#c8e6c9
    style NO_L2 fill:#ffcdd2
```

---

## Key Observations

### Layering
- **Clean separation:** Each module has a well-defined responsibility
- **Low coupling:** Minimal cross-module dependencies
- **High cohesion:** Related functionality grouped together

### Thread Safety
- **Per-thread handles:** Each thread owns its CURL* context
- **No global state:** Library stateless (aside from init)
- **Safe for concurrent use:** Multiple threads can upload/download simultaneously

### Resource Management
- **Explicit lifecycle:** doCurlInit() / doStopDownload()
- **No implicit cleanup:** Consumer responsibility for resource release
- **Risk of leaks:** Forgotten doStopDownload() → resource leak

### Performance
- **Streaming model:** Upload streams from disk (efficient)
- **Buffering model:** Download buffers in memory (simpler API)
- **Network-bound:** Throughput limited by libcurl/network, not library

---

## References

- Module Responsibilities: [project.md](../project.md)
- Detailed Subsystem Analysis: [subsystems/](../subsystems/)
- Runtime Flows: [runtime/](../runtime/)
