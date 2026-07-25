# Contributing to USB Cleaner

Thank you for your interest in contributing to **USB Cleaner**! This document provides guidelines and instructions for contributing to the project.

## 🤝 How to Contribute

### Reporting Bugs

Before creating a bug report, please check existing issues to avoid duplicates.

When creating a bug report, include:

1. **Description**: Clear description of the problem
2. **Steps to Reproduce**: Detailed steps to reproduce the issue
3. **Expected Behavior**: What should happen
4. **Actual Behavior**: What actually happens
5. **Environment**:
   - Windows version (7/8/10/11)
   - Build type (Debug/Release)
   - Compiler (MinGW/MSVC)
   - Nana GUI version
6. **Screenshots**: If applicable
7. **Logs**: Include `usb_cleaner.log` if available

**Bug Report Template:**

```markdown
## Bug Description
[Clear description of the issue]

## Steps to Reproduce
1. Step one
2. Step two
3. See error

## Expected Behavior
[What should happen]

## Actual Behavior
[What actually happens]

## Environment
- OS: Windows 10 Pro x64, build 19045
- Compiler: MinGW-w64 12.2.0
- Build: Release
- Version: v2.0.0

## Logs
[Paste relevant log entries here]
```

### Suggesting Enhancements

Enhancement suggestions are welcome! Please include:

1. **Use Case**: Why this feature would be useful
2. **Proposed Solution**: How you envision it working
3. **Alternatives**: Other solutions you've considered
4. **Mockups**: UI mockups if applicable

### Pull Requests

We welcome pull requests! Here's how to contribute code:

#### 1. Fork and Clone

```bash
# Fork the repository on GitHub, then clone your fork
git clone https://github.com/YOUR_USERNAME/usb-cleaner.git
cd usb-cleaner

# Add upstream remote
git remote add upstream https://github.com/oustery/usb-cleaner.git
```

#### 2. Create a Branch

```bash
# Create a branch for your feature/fix
git checkout -b feature/your-feature-name

# Or for bug fixes
git checkout -b fix/bug-description
```

Branch naming conventions:
- `feature/description` — New features
- `fix/description` — Bug fixes
- `docs/description` — Documentation updates
- `refactor/description` — Code refactoring

#### 3. Make Changes

Follow our coding standards (see below) and make your changes.

#### 4. Test Your Changes

```bash
# Build the project
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --parallel

# Test the application
./usb_cleaner.exe

# Run tests (when available)
ctest --output-on-failure
```

#### 5. Commit Your Changes

Use clear commit messages:

```bash
git add .
git commit -m "feat: add dark mode support

- Implement theme switching between light/dark modes
- Add color scheme configuration
- Update README with theme documentation

Closes #123"
```

Commit message format:
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation changes
- `style:` Code formatting (no logic change)
- `refactor:` Code refactoring
- `test:` Adding/updating tests
- `chore:` Maintenance tasks

#### 6. Push and Create PR

```bash
# Push to your fork
git push origin feature/your-feature-name

# Create Pull Request on GitHub
# Target: dev branch
```

PR Description Template:

```markdown
## Description
[Brief description of changes]

## Type of Change
- [ ] Bug fix (non-breaking change fixing an issue)
- [ ] New feature (non-breaking change adding functionality)
- [ ] Breaking change (fix or feature causing existing functionality to change)
- [ ] Documentation update

## Testing
[Describe how you tested your changes]

## Screenshots (if applicable)
[Add screenshots here]

## Checklist
- [ ] My code follows the project's coding style
- [ ] I have tested my changes
- [ ] I have updated documentation as needed
- [ ] My changes generate no new warnings
- [ ] I have checked my code with static analysis tools
```

## 📋 Coding Standards

### C++ Style Guide

1. **Naming Conventions:**
   - Classes: `PascalCase` → `USBCleanerApp`
   - Functions: `PascalCase` → `CleanUSBHistory()`
   - Variables: `camelCase` → `itemsProcessed`
   - Constants: `UPPER_SNAKE_CASE` → `APP_VERSION`
   - Namespaces: `PascalCase` → `USBUtils`
   - Private members: `trailing_` → `isProcessing_`

2. **Formatting:**
   - Indentation: 4 spaces (no tabs)
   - Line length: Max 120 characters
   - Braces: Allman style (new line)
   - One statement per line

3. **Documentation:**
   - Comment complex logic in English or Russian
   - Use Doxygen-style comments for public APIs
   - Include file headers with purpose and author

4. **Best Practices:**
   - Use `constexpr` where possible
   - Prefer `enum class` over plain enums
   - Use smart pointers over raw pointers
   - Avoid raw `new`/`delete`
   - Use RAII for resource management

Example:

```cpp
/**
 * @brief Cleans USB device history from registry
 * 
 * Removes entries for disconnected devices while preserving
 * currently connected device records.
 * 
 * @param progressCallback Function to report progress (0-100%)
 * @return OperationResult containing status and statistics
 * 
 * @note Requires administrator privileges
 * @warning This operation modifies system registry
 */
OperationResult CleanUSBHistory(std::function<void(int)> progressCallback);
```

### Project Structure

```
src/
├── main.cpp           # Main application entry point
├── gui/
│   ├── main_window.cpp    # Main window implementation
│   ├── about_dialog.cpp   # About dialog
│   └── result_dialog.cpp  # Result display dialog
├── core/
│   ├── registry.cpp       # Registry operations
│   ├── usb_utils.cpp      # USB device utilities
│   └── zone_cleaner.cpp   # Zone.Identifier removal
├── utils/
│   ├── logger.cpp         # Logging system
│   └── admin_check.cpp    # Admin rights check
└── resources/
    ├── icons/             # Application icons
    └── strings/           # Localization files
```

*Note: Current version is single-file; refactoring planned for v2.1*

## 🧪 Testing

Currently, manual testing is required. Automated tests are planned for v2.1.

### Manual Testing Checklist

When submitting a PR, verify:

- [ ] Application compiles without warnings on MinGW-w64
- [ ] Application compiles without warnings on MSVC
- [ ] Application runs on Windows 10/11
- [ ] Admin elevation works correctly
- [ ] USB cleanup function works (test with virtual machine!)
- [ ] Flash label cleanup works
- [ ] Progress bars update correctly
- [ ] Error handling displays proper messages
- [ ] No memory leaks (use Visual Studio debugger)

## 🌐 Localization

We support multiple languages. To add or update translations:

1. Edit language strings in source code
2. Follow existing format: Russian (primary), English (secondary)
3. Test all UI elements display correctly

## 📄 License

By contributing, you agree that your contributions will be licensed under the **MIT License**.

## ❓ Questions?

- Open an issue for bugs or questions
- Join discussions in GitHub Discussions
- Check existing documentation first

## 🙏 Recognition

Contributors will be recognized in:
- README.md Contributors section
- Release notes
- GitHub contributor statistics

Thank you for making USB Cleaner better! 🎉
