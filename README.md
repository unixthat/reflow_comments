# Reformat Print & Comment Tool

Reformat Print & Comment Tool is a command-line utility written in C that reformats Python source files to improve comment readability and enforce PEP8-style limits. It performs several automated transformations:

- **Commented-Out Print Statements (Rule A):**  
  Detects full-line comments that appear to be print statements (e.g., `# print(...)`), uncomments them, formats the code using the Black formatter (with a 79-character limit), and re-wraps the output in a triple-quoted block (`""" ... """`).

- **Inline Comment Splitting (Rule B):**  
  If a code line contains an inline comment that causes the line to exceed 79 characters, the tool splits the line into two: the comment is moved to a separate line above the code (with a `#` prefix), preserving original indentation.

- **Merging Full-Line Comments (Rule C):**  
  Consecutive full-line comments that are too long are merged into a single comment block. Their content is flattened, reflowed (by wrapping words at sensible breakpoints), and enclosed in a triple-quoted block.

- **Reflowing Existing Triple-Quoted Blocks (Rule D):**  
  Existing triple-quoted comment blocks are detected and their inner content is merged and rewrapped so that each resulting line does not exceed 79 characters, without introducing extra blank lines.

Additionally, the tool trims trailing whitespace from the comment blocks.

## Features

- **PEP8 Compliance:** Enforces a maximum line length of 79 characters.
- **Integration with Black:** Uses the Black formatter (which must be installed and in the system PATH) for formatting code segments.
- **Recursive Processing:** Can process a single file or all Python (`.py`) files in a directory recursively.
- **Heuristic Reflowing:** Attempts to intelligently reflow comments, splitting at spaces or punctuation where appropriate.

## Requirements

- **C Compiler:** GNU GCC or Clang (tested with `gcc -O2`).
- **Black:** The Python code formatter must be installed and available in your PATH (install via `pip install black`).

## Installation

1. **Clone or download the source code.**
2. **Compile the tool:**

   ```bash
   gcc -O2 -g -o reformat_print reflow_comments.c

