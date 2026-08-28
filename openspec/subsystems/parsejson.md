# JSON Parser (libparsejson) - Detailed Analysis

**Module:** parsejson/  
**Public API:** `json_parse.h`  
**Dependencies:** libcJSON, librdkloggers  
**Build Artifact:** `libparsejson.so`

---

## Module Purpose

Provides **JSON parsing and environment variable binding** for RDK configuration workflows:

- Parse JSON from files or in-memory strings
- Extract key-value pairs and write to output files
- Optionally bind parsed values to environment variables
- Wrapper around libcJSON with RDK-specific extensions

Primary usage: Configuration parsing, device property extraction, RFC data transformation.

---

## Public API Surface

### File-Based Parsing

```c
int SetJsonVars(
    char *fileIn,                     // Input JSON file path
    char *fileOut,                    // Output file (name=value pairs), NULL = skip
    int setenvvars                    // Boolean: Set environment variables (1/0)
);
// Parse JSON file and extract key-value pairs
// Output Format (if fileOut provided):
//   name="value"
//   another_key="another_value"
// Environment Variables (if setenvvars == 1):
//   setenv("name", "value", 1)
// Returns: 0 (success), 1 (failure)
// Usage: Process device configuration from JSON files
```

### In-Memory Parsing

```c
JSON *ParseJsonStr(char *pJsonStr);
// Parse JSON string into object tree
// Parameters: pJsonStr - JSON string (not NULL-terminated in some cases)
// Returns: Pointer to cJSON object tree, NULL on parse error
// Caller Responsibility: Must call FreeJson() to prevent memory leak
// Usage: Parse JSON responses from network operations
```

### Memory Management

```c
int FreeJson(JSON *pJson);
// Delete JSON object created by ParseJsonStr()
// Parameters: pJson - pointer from ParseJsonStr()
// Returns: 0 (success), non-zero (error, e.g., NULL pointer)
// Side Effects: Recursive deallocation of entire object tree
// Critical: Failure to call FreeJson() causes memory leaks
```

### Optional Utility Functions (if enabled at compile time)

```c
// These are conditionally compiled based on build_parsejson_bins
// They provide command-line access to JSON parsing

// jsonget - Query JSON value by key
// Usage: jsonget <json-file> <key>
// Output: Value if found, empty if not

// jsonwrite - Write JSON to file
// Usage: jsonwrite <output-file> <key> <value> ...
// Output: JSON file with specified key-value pairs
```

---

## Data Structures

### JSON Typedef (cJSON Wrapper)

```c
typedef cJSON   JSON;
// Direct mapping to libcJSON's cJSON structure
// Consumers can use standard cJSON functions on returned JSON* pointers
// Provides API stability even if underlying cJSON changes
```

### Implicit Structures (from libcJSON)

```c
// cJSON structure (internal to library):
struct cJSON {
    struct cJSON *next;               // Linked list of siblings
    struct cJSON *prev;               // Backward link
    struct cJSON *child;              // Child objects/array elements
    int type;                          // cJSON type flags
    char *valuestring;                // String value (if string type)
    int valueint;                     // Integer value (if number type)
    double valuedouble;               // Double value (if float type)
    char *string;                     // Object key (if in object)
};

// Type constants (use with cJSON_GetObjectItem, etc.):
// cJSON_Object, cJSON_Array, cJSON_String, cJSON_Number,
// cJSON_True, cJSON_False, cJSON_NULL
```

---

## Implementation Architecture

### Public API Layer (json_parse.c)

**Responsibility:** RDK-specific JSON handling and environment binding

**Key Functions:**
- `SetJsonVars()` - File-based parsing + environment setup
- `ParseJsonStr()` - In-memory parsing wrapper
- `FreeJson()` - Safe deallocation wrapper
- `writeItemVal()` - Internal: Write key=value pair to file
- `searchJsonStr()` - Internal: Find value by key in object

**Workflow (SetJsonVars):**

```c
int SetJsonVars(char *fileIn, char *fileOut, int setenvvars) {
    // 1. Open JSON file for reading
    FILE *fp_in = fopen(fileIn, "r");
    
    // 2. Read file contents into memory (mmap for efficiency)
    char *json_content = readFileToMemory(fileIn);
    
    // 3. Parse JSON string
    JSON *root = ParseJsonStr(json_content);
    if (!root) return 1;  // Parse error
    
    // 4. Open output file if specified
    FILE *fp_out = NULL;
    if (fileOut) fp_out = fopen(fileOut, "w");
    
    // 5. Iterate over JSON object
    for each key-value pair in root {
        // 6. Write to output file
        if (fp_out) {
            writeItemVal(fp_out, key, value, setenvvars);
        }
        // 7. Set environment variable
        if (setenvvars) {
            setenv(key, value, 1);  // Overwrite existing
        }
    }
    
    // 8. Cleanup
    if (fp_out) fclose(fp_out);
    FreeJson(root);
    free(json_content);
    
    return 0;  // Success
}
```

### Integration with libcJSON

**Usage Pattern:**

```c
// Consumer receives JSON* from ParseJsonStr()
JSON *root = ParseJsonStr(json_string);

// Use standard cJSON functions:
cJSON *item = cJSON_GetObjectItem(root, "device_id");
if (item && item->type == cJSON_String) {
    const char *device_id = item->valuestring;
}

// Or iterate:
cJSON *entry = NULL;
cJSON_ArrayForEach(entry, root) {
    // Process each array element
}

// Cleanup
FreeJson(root);  // Wrapper for cJSON_Delete(root)
```

---

## Configuration & Features

### Compile-Time Options

| Flag | Behavior |
|------|----------|
| `build_parsejson_bins` | Enable compilation of jsonget/jsonwrite CLI utilities |
| `GTEST_ENABLE` | Redirect file paths to `/tmp/` for testing |

### Logging Integration

```c
#include "rdkv_cdl_log_wrapper.h"

// Available macros:
COMMONUTILITIES_ERROR(fmt, ...)  // Error level
COMMONUTILITIES_INFO(fmt, ...)   // Info level
// Used for parse errors, missing files, memory allocation failures
```

---

## Memory Management

### Allocation Model

**Critical Point:** ParseJsonStr() allocates memory that must be freed by caller

```c
// LEAK EXAMPLE (DO NOT DO):
JSON *data = ParseJsonStr(json_str);
char *value = data->valuestring;
// DO NOT RETURN without calling FreeJson(data) !!!

// CORRECT EXAMPLE:
JSON *data = ParseJsonStr(json_str);
if (data) {
    char *value = strdup(data->valuestring);  // Copy if needed
    FreeJson(data);                           // Always cleanup
    return value;                             // Safe to return copy
}
```

### File Memory Mapping

**Optimization in SetJsonVars():**
```c
// Large files (> 64KB) use mmap for efficiency:
int fd = open(fileIn, O_RDONLY);
struct stat st;
fstat(fd, &st);

char *content = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
// Parse from mmap'd buffer
munmap(content, st.st_size);
close(fd);
```

---

## Error Handling

### Parse Errors

```c
JSON *root = ParseJsonStr(invalid_json);
if (root == NULL) {
    // JSON parse error (logged by ParseJsonStr)
    // Common causes:
    // - Malformed JSON (missing braces, quotes)
    // - Invalid escape sequences
    // - Unterminated strings
    return -1;
}
```

### File I/O Errors

```c
int result = SetJsonVars("/nonexistent/file.json", NULL, 0);
if (result != 0) {
    // Possible causes:
    // - File not found (errno = ENOENT)
    // - Permission denied (errno = EACCES)
    // - Disk full on write (errno = ENOSPC)
    return -1;
}
```

### Environment Variable Errors

```c
// setenv() can fail if:
if (setenv(key, value, 1) != 0) {
    // Possible causes:
    // - Invalid variable name (contains '=')
    // - No memory for environment
    COMMONUTILITIES_ERROR("setenv failed for %s\n", key);
}
```

---

## Usage Patterns

### Pattern 1: Parse and Extract Single Value

```c
JSON *config = ParseJsonStr(json_string);
if (!config) return -1;

cJSON *device_id = cJSON_GetObjectItem(config, "device_id");
if (device_id && device_id->type == cJSON_String) {
    printf("Device ID: %s\n", device_id->valuestring);
}

FreeJson(config);
```

### Pattern 2: File-to-Environment Workflow

```c
// Parse /opt/config.json and set all values as env vars
int result = SetJsonVars(
    "/opt/config.json",      // Input JSON file
    "/tmp/config.env",       // Optional: also write to file
    1                        // Set environment variables
);

if (result == 0) {
    // All JSON values now available as environment variables
    const char *device_model = getenv("model");
    const char *device_version = getenv("version");
}
```

### Pattern 3: Array Iteration

```c
JSON *array = ParseJsonStr("[{\"id\":1},{\"id\":2},{\"id\":3}]");
if (!array) return -1;

cJSON *element = NULL;
cJSON_ArrayForEach(element, array) {
    cJSON *id_obj = cJSON_GetObjectItem(element, "id");
    if (id_obj) {
        printf("ID: %d\n", id_obj->valueint);
    }
}

FreeJson(array);
```

### Pattern 4: Nested Object Navigation

```c
JSON *root = ParseJsonStr(json_string);
if (!root) return -1;

cJSON *device = cJSON_GetObjectItem(root, "device");
if (device && device->type == cJSON_Object) {
    cJSON *model = cJSON_GetObjectItem(device, "model");
    cJSON *version = cJSON_GetObjectItem(device, "version");
    
    if (model && version) {
        printf("Model: %s, Version: %s\n",
               model->valuestring, version->valuestring);
    }
}

FreeJson(root);
```

---

## Performance Characteristics

### Memory & Computational

| Operation | Complexity | Time |
|-----------|-----------|------|
| ParseJsonStr (100 KB JSON) | O(n) | ~1-5 ms |
| SetJsonVars (small file) | O(n) | ~10-50 ms |
| cJSON_GetObjectItem() | O(n) siblings | ~μs (typically O(1) for cached keys) |
| FreeJson() (1000 objects) | O(n) | ~1-2 ms |

### Memory Usage
- **libparsejson.so:** ~24 KB (shared library)
- **Per-parse (100 KB JSON):** ~300-500 KB (cJSON object tree + strings)
- **Scaling:** Object tree size ≈ 3-5x original JSON file size

---

## Known Issues & Limitations

### Issue 1: No Schema Validation
- **Limitation:** Only validates JSON syntax, not structure
- **Impact:** Typos in key names go undetected (e.g., "ddevice_id" instead of "device_id")
- **Workaround:** Caller must validate presence and type of expected keys

### Issue 2: Memory Leak Risk
- **Issue:** Caller must explicitly call FreeJson()
- **Impact:** Easy to leak memory in error paths
- **Recommendation:** Use helper macro or RAII-like pattern

### Issue 3: No Update Capability
- **Limitation:** libcJSON provides create/modify, but SetJsonVars doesn't support round-trip
- **Impact:** Can only parse → extract, not parse → modify → write
- **Workaround:** Use libcJSON directly for updates

### Issue 4: Environment Variable Overwrite
- **Behavior:** setenv(..., 1) overwrites existing env vars
- **Risk:** Unexpected side effects if consumer has pre-set variables
- **Impact:** No way to preserve original values

---

## Testing Infrastructure

### Unit Tests

**Test File:** `unit-test/parsejson/json_parse_gtest.cpp`

**Coverage:**
- Parse valid JSON objects
- Parse JSON arrays
- Parse nested structures
- Handle parse errors (malformed JSON)
- SetJsonVars file I/O
- Environment variable binding

---

## Integration Guidance

### For Consumer Developers

**Linkage:**
```bash
gcc -c myapp.c -I/usr/include

gcc myapp.o -o myapp \
    -lparsejson -lcjson -lrdkloggers
```

**Safe Usage Pattern:**
```c
#include "json_parse.h"

JSON *parse_config_safe(const char *json_string) {
    if (!json_string) return NULL;
    return ParseJsonStr((char *)json_string);
}

int extract_string_safe(JSON *obj, const char *key, char *out, size_t sz) {
    if (!obj || !key || !out) return -1;
    
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (!item || item->type != cJSON_String) return -1;
    
    strncpy(out, item->valuestring, sz - 1);
    out[sz - 1] = '\0';
    return 0;
}
```

---

## References

- libcJSON documentation: https://github.com/DaveGamble/cJSON
- Source: `parsejson/` directory
- Tests: `unit-test/parsejson/`
- Consumer reference: `utils/common_device_api.c` (uses ParseJsonStr)
