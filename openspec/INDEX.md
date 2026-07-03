# OpenSpec Architecture Documentation Index

**Project:** Common Utilities (RDK Embedded Linux)  
**Created:** 2026-07-03  
**Status:** Baseline Architecture - Production-Ready  

---

## Quick Navigation

### 📋 Main Overview
- **[project.md](project.md)** - Executive summary, system purpose, high-level architecture, design principles

### 🏗️ Detailed Subsystem Analysis
1. **[subsystems/dwnlutils.md](subsystems/dwnlutils.md)** - Download library (libdwnlutil)
   - API surface, data structures, thread safety, error handling
   
2. **[subsystems/uploadutils.md](subsystems/uploadutils.md)** - Upload library (libuploadutil)  
   - Two-stage workflow, mTLS support, CodeBig integration
   
3. **[subsystems/parsejson.md](subsystems/parsejson.md)** - JSON parser (libparsejson)
   - cJSON wrapper, environment variable binding, memory management
   
4. **[subsystems/utils.md](subsystems/utils.md)** - Device utilities (libfwutils)
   - Device queries, filesystem operations, system commands

### 🔄 Runtime Operations
1. **[runtime/download_flows.md](runtime/download_flows.md)** - Download operation sequences
   - Standard download, resumable transfers, mTLS, OAuth
   - State machines, error scenarios, performance analysis
   
2. **[runtime/upload_flows.md](runtime/upload_flows.md)** - Upload operation sequences
   - Two-stage workflow diagrams, certificate rotation
   - CodeBig integration, error recovery patterns

### 📊 Architecture Diagrams  
- **[diagrams/module_interactions.md](diagrams/module_interactions.md)** - Visual architecture
  - System architecture, dependency graphs, data flows
  - Thread safety model, compilation architecture, feature flags

---

## Document Organization

```
openspec/
├── project.md                              ← START HERE
│   ├─ System purpose & overview
│   ├─ Architectural overview (high-level)
│   ├─ Runtime models
│   └─ Known gaps & unknowns
│
├── subsystems/                             ← DEEP DIVES
│   ├── dwnlutils.md                       (Download library)
│   ├── uploadutils.md                     (Upload library)
│   ├── parsejson.md                       (JSON parser)
│   └── utils.md                           (Device utils)
│
├── runtime/                                ← OPERATIONAL FLOWS
│   ├── download_flows.md                  (Download sequences)
│   └── upload_flows.md                    (Upload sequences)
│
└── diagrams/                               ← VISUAL REFERENCES
    └── module_interactions.md              (Mermaid diagrams)
```

---

## Reading Paths

### For Architects & Designers
1. Read [project.md](project.md) - Get the big picture
2. Review [diagrams/module_interactions.md](diagrams/module_interactions.md) - Visual overview
3. Dive into relevant subsystems as needed:
   - Download work? → [subsystems/dwnlutils.md](subsystems/dwnlutils.md)
   - Upload work? → [subsystems/uploadutils.md](subsystems/uploadutils.md)

### For API Consumers (Component Developers)
1. Start: [project.md](project.md) - System overview
2. Navigate to relevant subsystem:
   - Need to download? → [subsystems/dwnlutils.md](subsystems/dwnlutils.md) + [runtime/download_flows.md](runtime/download_flows.md)
   - Need to upload? → [subsystems/uploadutils.md](subsystems/uploadutils.md) + [runtime/upload_flows.md](runtime/upload_flows.md)
   - Need device info? → [subsystems/utils.md](subsystems/utils.md)
   - Need JSON parsing? → [subsystems/parsejson.md](subsystems/parsejson.md)
3. Reference "Integration Guidance" sections for code examples

### For Maintainers & Contributors
1. Baseline: [project.md](project.md)
2. Subsystem deep-dives: All subsystems/ documents
3. Operations reference: All runtime/ documents
4. Known issues: Each subsystem has "Known Issues & Limitations" section
5. Testing: Each subsystem references unit/functional tests

### For Operations & Support
1. [project.md](project.md) - System overview
2. [runtime/download_flows.md](runtime/download_flows.md) - Error scenarios
3. [runtime/upload_flows.md](runtime/upload_flows.md) - Error scenarios
4. Relevant subsystems for troubleshooting:
   - Connection issues? → dwnlutils [Error Handling](subsystems/dwnlutils.md#error-handling)
   - Upload failures? → uploadutils [Error Scenarios](subsystems/uploadutils.md#error-handling--recovery)

---

## Key Architectural Insights

### Strengths
✓ **Clean separation of concerns** - Each library has single responsibility  
✓ **Layered architecture** - Clear dependencies; no circular deps  
✓ **Thread-safe design** - Per-thread handles; no global state  
✓ **Extensive error handling** - Return codes + HTTP status codes  
✓ **Well-tested** - Unit tests + functional tests  

### Limitations & Gaps
⚠️ **Fixed timeout values** - No runtime control (30s connect, 2h transfer)  
⚠️ **Memory buffering** - Downloads buffer entire file (not suitable for huge files)  
⚠️ **No connection pooling** - New curl handle per operation  
⚠️ **Presigned URL expiration** - Must complete upload within 15 min  
⚠️ **Function name typo** - `urlHelperPutReuqest` (public API)  

### Design Patterns Used
- **Resource lifecycle management** - Explicit init/cleanup pairs
- **Configuration via structures** - `FileDwnl_t`, `FileUpload_t`, etc.
- **Optional features via compile flags** - Graceful degradation
- **Callback-based streaming** - For download progress
- **Two-stage workflows** - Metadata POST + file PUT (upload)

---

## Common Questions Answered

### Q: How do I download a file?
**A:** See [subsystems/dwnlutils.md](subsystems/dwnlutils.md#integration-guidance) + [runtime/download_flows.md](runtime/download_flows.md)

### Q: How do I upload a file?
**A:** See [subsystems/uploadutils.md](subsystems/uploadutils.md#integration-guidance) + [runtime/upload_flows.md](runtime/upload_flows.md)

### Q: What's the difference between download and upload libraries?
**A:** See [project.md - Runtime Models](project.md#runtime-models)

### Q: Are these libraries thread-safe?
**A:** Yes, with caveats. See [diagrams/module_interactions.md - Thread Safety Model](diagrams/module_interactions.md#diagram-6-thread-safety-model)

### Q: What are the known issues?
**A:** Each subsystem document has "Known Issues & Limitations" section. Also see [project.md - Known Gaps](project.md#known-gaps-and-unknowns)

### Q: How do I handle errors?
**A:** Each subsystem has error handling guidance. Also see runtime flows for specific scenarios.

### Q: What external libraries are required?
**A:** See [project.md - Dependency Graph](project.md#dependency-graph)

---

## Document Characteristics

### Scope & Scale
- **Lines of documentation:** ~3,000+
- **Diagram count:** 8 Mermaid diagrams
- **Subsystems covered:** 4 (dwnlutil, uploadutil, parsejson, utils)
- **Code examples:** 50+ code snippets
- **Test references:** Unit tests + functional tests

### Accuracy & Verification
- **Facts:** Derived from source code analysis (verified)
- **Inferences:** Based on code patterns and structure (high confidence)
- **Assumptions:** Explicitly marked; some require validation
- **Unknowns:** Documented in "Unknowns" sections

### Update Frequency
- **Baseline creation:** 2026-07-03
- **Version alignment:** common_utilities v1.5.5
- **Change trigger:** Major architectural changes, new subsystems, breaking API changes

---

## Using These Documents with OpenSpec

### For Change Proposals
1. **Reference this baseline** when creating new changes
2. **Update relevant subsystem docs** if your change affects APIs
3. **Add sequence diagrams** for new workflows
4. **Document unknowns** in your change artifacts

### For Specs & Design
1. **Use subsystem structure** as template for new modules
2. **Reference error handling patterns** for consistency
3. **Follow threading model** established here

### For Implementation
1. **Consult integration guidance** sections
2. **Reference test patterns** from existing modules
3. **Follow established conventions** (error codes, config structures)

---

## Feedback & Improvements

### Areas for Community Input
- [ ] Presigned URL timeout handling - Is 15 min sufficient?
- [ ] Resumable upload support - Is current approach acceptable?
- [ ] Performance tuning - Are defaults optimal for your use cases?
- [ ] Documentation gaps - What's unclear or missing?

### Known Unknowns Requiring Validation
- OAuth token refresh strategy during long downloads
- Device property file encoding (ASCII vs UTF-8?)
- Exact filesystem paths for custom deployments
- Certificate rotation edge cases with rdkcertselector

---

## Related Resources

- **Source:** `common_utilities/` repository
- **Build System:** GNU Autotools (autoconf/automake)
- **Tests:** `unit-test/` and `test/` directories
- **Dependencies:** libcurl, libcJSON, OpenSSL, rdkloggers

---

## Version History

| Date | Version | Changes |
|------|---------|---------|
| 2026-07-03 | 1.0 | Initial baseline architecture documentation |

---

## Document Metadata

| Property | Value |
|----------|-------|
| Format | Markdown + Mermaid diagrams |
| Location | openspec/ folder |
| Audience | Architects, Developers, Maintainers, Operations |
| Maintenance | Per OpenSpec change workflow |
| License | Apache 2.0 (per project) |

---

## Quick Links

**Project Overview:** [project.md](project.md)  
**Visual Architecture:** [diagrams/module_interactions.md](diagrams/module_interactions.md)  
**Download API:** [subsystems/dwnlutils.md](subsystems/dwnlutils.md)  
**Upload API:** [subsystems/uploadutils.md](subsystems/uploadutils.md)  
**Download Flows:** [runtime/download_flows.md](runtime/download_flows.md)  
**Upload Flows:** [runtime/upload_flows.md](runtime/upload_flows.md)  

---

**Start with [project.md](project.md) for the big picture.**
