# Changelog

All notable changes to the USB Cleaner project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-01-25

### 🎉 Added
- **Modern GUI with Nana C++ GUI library**
  - Beautiful interface with color-coded status indicators
  - Real-time progress bars for operations
  - Tooltips and confirmation dialogs
  - About dialog with detailed information
  - Result dialog showing operation statistics

- **USB History Cleanup**
  - Removes disconnected device records from registry
  - Cleans USBSTOR, USB, and MountedDevices keys
  - Preserves currently connected devices
  - Progress tracking during cleanup

- **Flash Drive Label Cleanup**
  - Removes Zone.Identifier NTFS alternate data streams
  - Clears "Downloaded from Internet" security labels
  - Recursive processing of all folders and subfolders
  - Multi-drive support

- **Developer Experience**
  - GitHub Actions CI/CD pipeline
  - Automatic builds on Windows (MinGW-w64 & MSVC)
  - Release automation on tag push
  - Comprehensive documentation
  - MIT License for open source use

### 🔧 Technical Improvements
- C++17 standard compliance
- Thread-safe operations with std::async
- Proper error handling and logging
- Admin rights auto-elevation
- Cross-platform build support (CMake)
- Static linking support

### 📝 Documentation
- Complete README with installation instructions
- Inline code comments in Russian and English
- Setup script for Windows (setup.bat)
- Contributing guidelines
- Troubleshooting section

### 🐛 Fixed
- Registry access permission issues
- Race conditions in multi-threaded operations
- Memory leaks in string handling
- UI freezing during long operations

## [1.0.0] - 2026-01-24

### Added
- Initial release with basic Win32 API tray application
- USB history cleanup functionality
- Zone.Identifier removal
- Basic logging system
- MinGW and MSVC build support

---

## Version History Summary

| Version | Date | Changes |
|---------|------|---------|
| 2.0.0 | 2026-01-25 | Complete rewrite with Nana GUI, modern interface |
| 1.0.0 | 2026-01-24 | Initial version with basic Win32 API |

## Upcoming Features (Roadmap)

### [2.1.0] - Planned
- [ ] System tray integration (minimize to tray)
- [ ] Auto-start with Windows option
- [ ] Scheduled cleanup tasks
- [ ] Export/Import cleanup rules
- [ ] Dark mode theme support
- [ ] Multilingual interface (EN/RU)

### [3.0.0] - Future
- [ ] Plugin system for custom cleaners
- [ ] Network drive support
- [ ] Cloud sync of settings
- [ ] Portable version
- [ ] Linux/macOS native versions

---

**Note:** For the complete list of issues and feature requests, visit our 
[GitHub Issues](https://github.com/oustery/usb-cleaner/issues) page.
