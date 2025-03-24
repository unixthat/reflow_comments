# Reformat Print & Comment Tool

Reformat Print & Comment Tool is a command-line utility written in C that reformats Python source files to improve comment readability and enforce PEP8-style limits. It performs several automated transformations:

- **Commented-Out Print Statements (Rule A):**  
  Detects full-line comments that appear to be print statements (e.g., `# print(...)`), uncomments them, formats the code using the Black formatter (with a 79-character limit), and re-wraps the output in a triple-quoted block (`""" ... """`).

- **Inline Comment Splitting (Rule B):**  
  If a code line contains an inline comment that causes the line to exceed 79 characters, the tool splits the line into two: the comment is moved to a separate line above the code (with a `# ` prefix), preserving original indentation.

- **Merging Full-Line Comments (Rule C):**  
  Consecutive full-line comments that are too long are merged into a single comment block. Their content is flattened, rewrapped (by inserting newlines at reasonable breakpoints), and enclosed in a triple-quoted block.

- **Reflowing Existing Triple-Quoted Blocks (Rule D):**  
  Existing triple-quoted comment blocks are detected and their inner content is merged and rewrapped so that no resulting line exceeds 79 characters, without introducing extra blank lines.

Additionally, the tool trims trailing whitespace from reflowed comment blocks.

## Features

- **PEP8 Compliance:** Enforces a maximum line length of 79 characters.
- **Integration with Black:** Uses the Black formatter (which must be installed and available in the system PATH) for formatting code segments.
- **Recursive Processing:** Can process a single file or all Python (`.py`) files in a directory recursively.
- **Heuristic Reflowing:** Attempts to intelligently reflow comments, splitting at spaces or punctuation where appropriate.

## Requirements

- **C Compiler:** GNU GCC or Clang (tested with `gcc -O2`).
- **Black:** The Python code formatter must be installed and available in your PATH. Install via:
  pip install black
  

## Installation

1. **Clone or download the source code into your project directory (e.g., `~/GitHubRepos/reflow_comments`).**

2. **Compile the tool:**
   gcc -O2 -g -o reformat_print reflow_comments.c

3. **(Optional) Install the binary to a directory in your PATH:**
   sudo mv reformat_print /usr/local/bin/

## Usage

You can run the tool on a single Python file or a directory containing Python files.

- **Process a Single File:**
  reformat_print path/to/file.py

- **Process an Entire Directory (recursively):**
  reformat_print path/to/directory

The tool will modify the files in place. **Always back up your files or use version control before running the tool.**

## How It Works

1. **File Reading:**  
   The tool reads the entire file (or each file in a directory) into memory.

2. **Transformation Rules:**  
   It applies the following rules:
   - **Rule A:** Detects full-line commented-out print statements, uncomments them, formats them using Black, and wraps them in triple quotes.
   - **Rule B:** Splits inline comments (on code lines) into separate lines if the total length exceeds 79 characters.
   - **Rule C:** Merges consecutive full-line comments into a single block, rewraps the merged content, and encloses it in triple quotes.
   - **Rule D:** Detects and reflows existing triple-quoted comment blocks so that their inner content is rewrapped to adhere to the 79-character limit.

3. **File Writing:**  
   The modified content is written back to the file(s) in place.

4. **Trailing Whitespace:**  
   The tool also removes trailing whitespace from reflowed comment blocks.

## Limitations

- The tool uses simple heuristics and does not fully parse Python syntax.
- Some edge cases (e.g., comments inside string literals or unusual formatting) might not be handled perfectly.
- It is recommended to review changes (e.g., via version control) after running the tool.

## Contributing

Contributions and suggestions are welcome! Please fork the repository and submit pull requests with improvements or fixes.

## License

This tool is released under the MIT License.
