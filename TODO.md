# TODO for Reformat Print & Comment Tool

This document outlines potential improvements, new features, and ideas for future development of the Reformat Print & Comment Tool. Contributions and suggestions are very welcome!

---

## Installation & Build Improvements

- **Makefile:**
  - Create a robust `Makefile` to build the project.
    - Targets:
      - `all`: Compile with default flags.
      - `debug`: Compile with `-g` flag.
      - `clean`: Remove generated binaries and temporary files.
  - Include installation targets:
    - `install`: Copy binary to `/usr/local/bin/` (or configurable location).
    - `uninstall`: Remove installed binary.
- **Cross-Platform Support:**
  - Investigate building on different platforms (Linux, macOS, Windows with MinGW/MSYS2).
  - Consider using CMake for more flexible cross-platform builds.
- **Dependency Checks:**
  - Automate checking for dependencies (like Black) during build or via a pre-build script.
  - Optionally bundle a version-check script.

---

## Documentation Improvements

- **README.md Enhancements:**
  - Include detailed usage examples and screenshots.
  - Add FAQ section covering common issues and troubleshooting tips.
  - Provide example output (before and after) for each rule.
- **API Documentation:**
  - Document each function in the source code with more detailed comments.
  - Generate API docs (if desired) using a tool like Doxygen.
- **User Manual:**
  - Create a separate user manual (PDF/HTML) with step-by-step instructions, configuration examples, and best practices.

---

## Feature Enhancements

- **Additional Formatting Options:**
  - Allow configurable maximum line length (instead of hardcoding 79).
  - Provide command-line flags to select which rules to apply (e.g., skip triple-quote reflow, or inline splitting).
- **Improved Heuristics:**
  - Use a more advanced parser for Python comments (possibly leveraging Python’s own tokenizer via an external call) for better accuracy.
  - Fine-tune word-wrapping logic (e.g., allow user-specified break characters).
- **Error Handling & Logging:**
  - Improve error messages and logging (possibly with a verbose flag).
  - Generate a log file of changes made for review.
- **Backup & Dry-Run Options:**
  - Add an option to create backups of files before modifying them.
  - Include a dry-run mode that shows what changes would be made without writing to disk.

---

## Testing & Quality Assurance

- **Unit Tests:**
  - Write unit tests for helper functions (e.g., `wrap_text()`, `rtrim()`, `ltrim()`) using a C unit testing framework.
- **Integration Tests:**
  - Create test cases for sample Python files that cover all transformation rules.
  - Automate testing with a CI system (GitHub Actions, Travis CI, or CircleCI) to run tests on push.
- **Static Analysis:**
  - Integrate with tools like `cppcheck` or `clang-tidy` to ensure code quality.
- **User Feedback:**
  - Add a mechanism for users to submit bug reports or feature requests (link to GitHub issues).

---

## Code Refactoring & Cleanup

- **Modularization:**
  - Consider splitting the code into multiple source files (e.g., one for file processing, one for text transformation) for better maintainability.
- **Configuration File:**
  - Allow configuration of parameters (e.g., MAX_LEN, temporary file directory) via an external config file or command-line options.
- **Improve Memory Management:**
  - Audit the code for potential memory leaks.
  - Consider using a more robust dynamic string library if available.

---

## Other Useful Ideas

- **GUI Frontend:**
  - Develop a simple GUI wrapper (using, for example, Qt or GTK) for users who prefer not to use the command line.
- **Integration with Editors:**
  - Provide plugins or snippets for popular editors (e.g., Vim, VSCode) to trigger reformatting on save.
- **Documentation Website:**
  - Host project documentation on GitHub Pages for easy reference.
- **Internationalization:**
  - Add support for multiple languages in the error messages and logs.
- **Benchmarking:**
  - Measure the tool's performance on large repositories and optimize bottlenecks if necessary.
- **Extensibility:**
  - Provide a plugin or hook system that allows users to add their own transformation rules.

---

## Contributing

Feel free to fork the repository, open issues, and submit pull requests with improvements, fixes, and new features. Your contributions will help make this tool even more robust and user-friendly!

---

*End of TODO.md*

