# Mini Query Language Compiler - Phase 2

**Student:** PRANAV GOSWAMI  
**Registration No.:** 24BCB0046  
**Project:** Mini Query Language Compiler: Parsing, Semantic Validation and Three-Address Code Generation

## Core implementation
- Lexical analysis
- Recursive-descent syntax analysis
- Abstract Syntax Tree (AST)
- Symbol table with predefined schemas
- Semantic validation
- Three-address code generation
- Lexical, syntax and semantic error reporting

## Run in VS Code

Open this folder in VS Code.

### Compile
```bash
g++ -std=c++17 -Wall -Wextra main.cpp -o compiler
```

### Run
```bash
./compiler
```

Windows PowerShell:
```powershell
g++ -std=c++17 -Wall -Wextra main.cpp -o compiler.exe
.\compiler.exe
```

### Demonstration input
```text
SELECT name, marks FROM students WHERE marks > 80 AND age >= 18;
```

The program displays:
1. Token stream
2. AST
3. Symbol table
4. Semantic analysis result
5. Three-address code

The implementation intentionally does not connect to a real database, matching the Phase 1 scope.
