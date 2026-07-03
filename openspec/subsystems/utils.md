# Device Utils (libfwutils) - Detailed Analysis

**Module:** utils/  
**Public APIs:** `common_device_api.h`, `rdk_fwdl_utils.h`, `system_utils.h`  
**Dependencies:** libparsejson, librdkloggers, filesystem I/O  
**Build Artifact:** `libfwutils.so`

---

## Module Purpose

Provides **device state queries, system utilities, and configuration access** for RDK components:

- Query device identity (model, account ID, MAC address)
- Retrieve firmware version and image details
- Access device properties from configuration files
- Execute system commands and manage filesystem operations
- Provide platform-agnostic device abstraction

Primary consumers: firmware updaters, management agents, diagnostic tools, configuration utilities.

---

## Public API Surface

### Device Property Queries

```c
int getDeviceProperties(DeviceProperty_t *pDevice_info);
// Retrieve all device properties in one call
// Reads from /etc/device.properties (or /tmp in test mode)
// Returns: UTILS_SUCCESS (1), UTILS_FAILURE (-1)
// Output: Populates DeviceProperty_t structure
// Usage: Get comprehensive device metadata

typedef struct deviceproperty {
    BUILDTYPE eBuildType;             // Build type enum
    char dev_name[MIN_BUFF_SIZE1];    // Device name (32 bytes)
    char dev_type[MIN_BUFF_SIZE1];    // Device type identifier
    char difw_path[MIN_BUFF_SIZE1];   // Download firmware path
    char log_path[MIN_BUFF_SIZE1];    // Logging directory path
    char persistent_path[MIN_BUFF_SIZE1];  // Persistent storage path
    char maint_status[MIN_BUFF_SIZE1];    // Maintenance status
    char mtls[MIN_BUFF_SIZE1];        // mTLS enabled? (yes/no)
    char model[MIN_BUFF_SIZE1];       // Device model number
    char sw_optout[MIN_BUFF_SIZE1];   // Software opt-out status
} DeviceProperty_t;

// Enum: Build type
typedef enum {
    eUNKNOWN = 0,
    eDEV = 1,
    eVBN = 2,
    ePROD = 3,
    eQA = 4
} BUILDTYPE;
```

### Individual Property Accessors

```c
size_t GetAccountID(char *pAccountID, size_t szBufSize);
// Get device account ID from bootstrap or properties
// Returns: Number of characters copied
// Output: pAccountID (null-terminated string)
// Max Size: Typically 16-32 bytes

size_t GetModelNum(char *pModelNum, size_t szBufSize);
// Get device model number
// Returns: Length of model string
// Example: "4643" (model for XB3 variant)

size_t GetDeviceType(char *pDeviceType, size_t szBufSize);
// Get device type identifier
// Returns: Length of type string
// Example: "mediaclient" or "broadband"

size_t GetVersionNum(char *pVersionNum, size_t szBufSize);
// Get firmware version string
// Returns: Length of version string
// Example: "CGM4331MU-20231201"

int GetImageDetails(ImageDetails_t *pImageDetails);
// Get image metadata structure
// Returns: UTILS_SUCCESS (1), UTILS_FAILURE (-1)
// Output: Populates ImageDetails_t with current_img_name

typedef struct imagedetails {
    char cur_img_name[MIN_BUFF_SIZE];  // Current image name (64 bytes)
} ImageDetails_t;

int getIncludePropertyData(
    const char *dev_prop_name,        // Property key to search
    char *data,                       // Output buffer
    unsigned int buff_size            // Buffer size
);
// Query property from include.properties file
// Returns: UTILS_SUCCESS, UTILS_FAILURE
// Usage: Alternative property source

int getDevicePropertyData(
    const char *dev_prop_name,        // Property key
    char *out_data,                   // Output buffer
    unsigned int buff_size            // Buffer size
);
// Query property from device.properties
// Returns: UTILS_SUCCESS, UTILS_FAILURE
// Usage: Direct property file access

size_t GetHwMacAddress(char *iface, char *pMac, size_t szBufSize);
// Get hardware MAC address for network interface
// Parameters: iface - interface name (e.g., "eth0")
// Returns: Length of MAC address string
// Output Format: "00:11:22:33:44:55"
```

### Firmware Version Information

```c
bool isMediaClientDevice(void);
// Check if device is media client (vs broadband gateway)
// Returns: true (media client), false (other)
// Usage: Conditional firmware update logic

int getImageUpdateFrequency(void);
// Get recommended update frequency
// Returns: Frequency in days, 0 if not specified
```

### System Command Execution

```c
int cmdExec(
    const char *cmd,                  // Linux shell command to execute
    char *output,                     // Output buffer for command results
    unsigned int size_buff            // Buffer size (max 4096)
);
// Execute arbitrary Linux command and capture output
// Returns: UTILS_SUCCESS (0), UTILS_FAILURE (-1)
// Output: Command stdout in output buffer
// Max Output: 4096 bytes
// Usage: Run diagnostic commands, retrieve system info
// Example: cmdExec("ifconfig eth0", output, 4096)
```

### Filesystem Operations

```c
// File System Queries
int filePresentCheck(const char *file_name);
// Check if file exists and is readable
// Returns: RDK_API_SUCCESS (0), RDK_API_FAILURE (-1)

int getFileSize(const char *file_name);
// Get size of file in bytes
// Returns: Size in bytes, -1 on error

int findSize(char *fileName);
// Alternative file size query
// Returns: Size in bytes, or -1

int findFile(char *dir, char *search);
// Search for file in directory (non-recursive)
// Returns: 1 if found, 0 if not

int findPFile(char *dir, char *search, char *out);
// Search directory and return full path
// Returns: 1 if found, fills out with path

int findPFileAll(char *path, char *search, char **out, int *found_t, int max_list);
// Find all files matching pattern
// Returns: Number of files found
// Output: Populates out array with paths

// File/Directory Operations
int fileCheck(char *pFilepath);
// Check if path exists
// Returns: 1 (exists), 0 (not exists), -1 (error)

int folderCheck(char *path);
// Check if directory exists
// Returns: 1 (exists), 0 (not exists), -1 (error)

int createDir(const char *dirname);
// Create directory (mkdir)
// Returns: RDK_API_SUCCESS (0), RDK_API_FAILURE (-1)

int createFile(const char *file_name);
// Create empty file (touch)
// Returns: RDK_API_SUCCESS (0), RDK_API_FAILURE (-1)

int removeFile(char *filePath);
// Delete file (unlink)
// Returns: RDK_API_SUCCESS (0), RDK_API_FAILURE (-1)

int emptyFolder(char *dir);
// Remove all files from directory
// Returns: RDK_API_SUCCESS (0), RDK_API_FAILURE (-1)

int copyFiles(char *src, char *dst);
// Copy file
// Returns: RDK_API_SUCCESS (0), RDK_API_FAILURE (-1)

int tarExtract(char *in_file, char *out_path);
// Extract tar archive
// Returns: RDK_API_SUCCESS (0), RDK_API_FAILURE (-1)

int eraseFolderExceParamFile(
    const char *folder,
    const char* file_name,
    const char* pdri_file_name,
    const char *model_num
);
// Delete folder except specified files
// Returns: RDK_API_SUCCESS (0), RDK_API_FAILURE (-1)

int eraseTGZItemsMatching(const char *folder, const char* file_name);
// Delete items matching pattern from tar
// Returns: RDK_API_SUCCESS (0), RDK_API_FAILURE (-1)
```

### Filesystem Utilities

```c
unsigned int getFreeSpace(char *path);
// Get free disk space at path in MB
// Returns: Free space in megabytes

unsigned int checkFileSystem(char *path);
// Check filesystem at path for errors
// Returns: Status code (0 = OK)

int logFileData(const char *file_path);
// Log contents of file for debugging
// Returns: RDK_API_SUCCESS (0), RDK_API_FAILURE (-1)

char* getExtension(char *filename);
// Extract file extension
// Returns: Pointer to extension (e.g., ".so")

char* getPartStr(char *fullpath, char *delim);
// Extract path component
// Returns: Pointer to component

char* getPartChar(char *fullpath, char delim);
// Extract path component by char delimiter
// Returns: Pointer to component
```

---

## Data Structures

### DeviceProperty_t - Device Configuration

```c
typedef struct deviceproperty {
    BUILDTYPE eBuildType;
    char dev_name[32];
    char dev_type[32];
    char difw_path[32];
    char log_path[32];
    char persistent_path[32];
    char maint_status[32];
    char mtls[32];
    char model[32];
    char sw_optout[32];
} DeviceProperty_t;
```

### ImageDetails_t - Firmware Image Info

```c
typedef struct imagedetails {
    char cur_img_name[64];            // Current firmware image name
} ImageDetails_t;
```

---

## Configuration Files

### Primary Configuration Sources

| File | Purpose | Format | Fallback |
|------|---------|--------|----------|
| `/etc/device.properties` | Device metadata | `key=value` (INI-style) | `/tmp/device.properties` (testing) |
| `/version.txt` | Firmware version | Plain text, first line | `/tmp/version.txt` (testing) |
| `/opt/secure/RFC/bootstrap.ini` | Bootstrap config | INI sections | Not used if GTEST_ENABLE |
| `/opt/www/authService/partnerId3.dat` | Partner ID | Plain text | `/tmp/partnerId3.dat` |
| `/etc/include.properties` | Additional properties | `key=value` | `/tmp/include.properties` |
| `/etc/timeZone_offset_map` | Timezone mappings | Multi-line | `/tmp/timeZone_offset_map` |

### File Path Resolution (GTEST vs Production)

```c
#ifndef GTEST_ENABLE
// Production paths (on device)
#define DEVICE_PROPERTIES_FILE  "/etc/device.properties"
#define VERSION_FILE            "/version.txt"
#define BOOTSTRAP_FILE          "/opt/secure/RFC/bootstrap.ini"
#else
// Testing paths (local filesystem)
#define DEVICE_PROPERTIES_FILE  "/tmp/device.properties"
#define VERSION_FILE            "/tmp/version.txt"
#define BOOTSTRAP_FILE          "/tmp/bootstrap.ini"
#endif
```

---

## Implementation Architecture

### Module Components

| File | Responsibility |
|------|-----------------|
| **rdk_fwdl_utils.c/h** | Device properties, build type parsing |
| **system_utils.c/h** | Filesystem operations, command execution |
| **common_device_api.c/h** | High-level device queries, JSON parsing |
| **rdkv_cdl_log_wrapper.c/h** | Logging infrastructure |

### Configuration File Parsing

**Device Properties Parsing:**

```c
int getDeviceProperties(DeviceProperty_t *pDevice_info) {
    FILE *fp = fopen(DEVICE_PROPERTIES_FILE, "r");
    if (!fp) return UTILS_FAILURE;
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // Parse key=value
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        char key[128], value[128];
        sscanf(line, "%127[^=]=%127[^\n]", key, value);
        
        // Map to struct fields
        if (strcmp(key, "BUILD_TYPE") == 0) {
            pDevice_info->eBuildType = parseEnumBuildType(value);
        } else if (strcmp(key, "MODEL") == 0) {
            strncpy(pDevice_info->model, value, sizeof(pDevice_info->model) - 1);
        }
        // ... etc for other fields
    }
    
    fclose(fp);
    return UTILS_SUCCESS;
}
```

### Command Execution Safety

**cmdExec() Implementation:**

```c
int cmdExec(const char *cmd, char *output, unsigned int size_buff) {
    // Validation
    if (!cmd || !output || size_buff <= 0 || size_buff > 4096) {
        return RDK_API_FAILURE;
    }
    
    // Execute with output capture
    FILE *fp = popen(cmd, "r");
    if (!fp) return RDK_API_FAILURE;
    
    // Read output (bounded)
    size_t pos = 0;
    while (pos < size_buff - 1 && fgets(output + pos, size_buff - pos, fp)) {
        pos += strlen(output + pos);
    }
    
    pclose(fp);
    output[pos] = '\0';
    return RDK_API_SUCCESS;
}
```

---

## Compile-Time Configuration

| Flag | Behavior |
|------|----------|
| `GTEST_ENABLE` | Use `/tmp/` paths instead of `/etc/`, `/opt/` |
| `GETRDMMANIFESTVERSION_IN_SCRIPT` | Use script-based version detection |

---

## Known Issues & Limitations

### Issue 1: Fixed Buffer Sizes
- **Limitation:** Character arrays in DeviceProperty_t fixed at 32 bytes
- **Impact:** Property values > 31 bytes truncated
- **Workaround:** No buffer overflow protection, relies on truncation

### Issue 2: No Configuration Validation
- **Issue:** No schema check for device.properties
- **Impact:** Missing required fields silently result in empty struct members
- **Recommendation:** Consumer should validate before use

### Issue 3: cmdExec() Security
- **Issue:** Passes arbitrary shell commands to popen()
- **Risk:** Shell injection if cmd contains user input
- **Mitigation:** Never pass untrusted input; escape special characters

### Issue 4: File Path Handling
- **Issue:** Paths hardcoded for device filesystem layout
- **Impact:** Difficult to customize for non-standard deployments
- **Workaround:** Recompile with modified paths or use symlinks

### Issue 5: Race Conditions in File Operations
- **Issue:** TOCTOU (Time-of-Check-to-Use) in fileCheck() + file operations
- **Impact:** File can be deleted between check and read
- **Mitigation:** Check errno after operations, handle ENOENT

---

## Performance Characteristics

### Memory & Computation

| Operation | Time | Space |
|-----------|------|-------|
| getDeviceProperties() | ~5-10 ms | ~256 bytes (local buffers) |
| GetAccountID() | ~1-5 ms | ~64 bytes |
| cmdExec("ifconfig") | ~10-50 ms | 4096 bytes (output buffer) |
| findPFileAll() (100 files) | ~50-200 ms | ~4 KB (per file path) |
| tarExtract(1 MB archive) | ~500 ms | ~512 KB (extraction buffers) |

### Filesystem Impact
- **Calls to stat/lstat:** One per file operation
- **Open file limits:** Careful resource management (close after use)
- **Caching:** No internal caching of properties (re-reads file each call)

---

## Error Handling

### Common Error Scenarios

```c
// File not found
if (filePresentCheck("/etc/device.properties") != RDK_API_SUCCESS) {
    fprintf(stderr, "Device properties file missing\n");
}

// Permission denied
FILE *fp = fopen("/etc/device.properties", "r");
if (!fp && errno == EACCES) {
    fprintf(stderr, "Permission denied: Check file ownership\n");
}

// Command execution failure
char output[4096];
if (cmdExec("cat /nonexistent", output, sizeof(output)) != RDK_API_SUCCESS) {
    fprintf(stderr, "Command failed\n");
}
```

---

## Testing Infrastructure

### Unit Tests

**Test File:** `unit-test/utils/common_device_api_gtest.cpp`

**Coverage:**
- Device property parsing
- Individual property accessors
- File operations (create, delete, find)
- Command execution
- Filesystem utilities

### Test Setup

```bash
# Create test files in /tmp
echo "MODEL=XB3" > /tmp/device.properties
echo "4643" > /tmp/version.txt

# Run tests
./common_device_api_gtest
```

---

## Integration Guidance

### For Consumer Developers

**Pattern 1: Get Full Device Info**
```c
DeviceProperty_t props = {0};
if (getDeviceProperties(&props) == UTILS_SUCCESS) {
    printf("Model: %s\n", props.model);
    printf("Build: %d\n", props.eBuildType);
}
```

**Pattern 2: Query Individual Properties**
```c
char account_id[64];
size_t len = GetAccountID(account_id, sizeof(account_id));
if (len > 0) {
    printf("Account: %s\n", account_id);
}
```

**Pattern 3: Execute System Commands**
```c
char output[4096];
if (cmdExec("df /", output, sizeof(output)) == RDK_API_SUCCESS) {
    printf("Disk usage:\n%s\n", output);
}
```

**Pattern 4: Safe File Operations**
```c
if (filePresentCheck("/opt/config.json") == RDK_API_SUCCESS) {
    int size = getFileSize("/opt/config.json");
    if (size > 0) {
        char *buffer = malloc(size);
        // Read file...
    }
}
```

### Linkage

```bash
gcc -c myapp.c -I/usr/include

gcc myapp.o -o myapp \
    -lfwutils -lparsejson -lcjson -lrdkloggers
```

---

## References

- Source: `utils/` directory
- Tests: `unit-test/utils/`, `unit-test/utils/`
- Integration examples: `dwnlutils/downloadUtil.c` (uses device utils for config)
- Related: Common Device API, RDK Firmware Utils
