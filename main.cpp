
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

using namespace std;

// ============================================================
// MINI QUERY LANGUAGE COMPILER - PHASE 2
// C++ implementation for Visual Studio Code
// Stages: Lexer -> Parser/AST -> Symbol Table -> Semantic Analysis -> TAC
// ============================================================

enum class TokenType {
    SELECT, FROM, WHERE, AND, OR,
    IDENTIFIER, INTEGER, FLOAT, STRING,
    EQ, NEQ, LT, GT, LE, GE,
    COMMA, SEMICOLON, LPAREN, RPAREN,
    END, INVALID
};

struct Token {
    TokenType type;
    string lexeme;
    int line;
};

string tokenName(TokenType t) {
    switch (t) {
        case TokenType::SELECT: return "SELECT";
        case TokenType::FROM: return "FROM";
        case TokenType::WHERE: return "WHERE";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::STRING: return "STRING";
        case TokenType::EQ: return "EQ";
        case TokenType::NEQ: return "NEQ";
        case TokenType::LT: return "LT";
        case TokenType::GT: return "GT";
        case TokenType::LE: return "LE";
        case TokenType::GE: return "GE";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::END: return "EOF";
        case TokenType::INVALID: return "INVALID";
    }
    return "UNKNOWN";
}

class Lexer {
    string source;
    size_t pos = 0;
    int line = 1;
    vector<string> errors;

    static string upper(string s) {
        for (char &c : s) c = (char)toupper((unsigned char)c);
        return s;
    }

    void addError(const string& msg) {
        errors.push_back("Lexical error at line " + to_string(line) + ": " + msg);
    }

public:
    explicit Lexer(string src) : source(move(src)) {}

    vector<Token> tokenize() {
        vector<Token> tokens;

        while (pos < source.size()) {
            char c = source[pos];

            if (c == ' ' || c == '\t' || c == '\r') {
                ++pos;
                continue;
            }
            if (c == '\n') {
                ++line;
                ++pos;
                continue;
            }

            if (c == '-' && pos + 1 < source.size() && source[pos + 1] == '-') {
                while (pos < source.size() && source[pos] != '\n') ++pos;
                continue;
            }

            if (isalpha((unsigned char)c) || c == '_') {
                int startLine = line;
                size_t start = pos++;
                while (pos < source.size() &&
                       (isalnum((unsigned char)source[pos]) || source[pos] == '_')) {
                    ++pos;
                }
                string lex = source.substr(start, pos - start);
                string u = upper(lex);
                TokenType type = TokenType::IDENTIFIER;
                if (u == "SELECT") type = TokenType::SELECT;
                else if (u == "FROM") type = TokenType::FROM;
                else if (u == "WHERE") type = TokenType::WHERE;
                else if (u == "AND") type = TokenType::AND;
                else if (u == "OR") type = TokenType::OR;
                tokens.push_back({type, lex, startLine});
                continue;
            }

            if (isdigit((unsigned char)c) || (c == '.' && pos + 1 < source.size() &&
                                                isdigit((unsigned char)source[pos + 1]))) {
                int startLine = line;
                size_t start = pos;
                bool dot = false;
                if (source[pos] == '.') {
                    dot = true;
                    ++pos;
                }
                while (pos < source.size() && isdigit((unsigned char)source[pos])) ++pos;
                if (pos < source.size() && source[pos] == '.') {
                    if (dot) {
                        addError("invalid numeric literal '" + source.substr(start, pos - start + 1) + "'");
                        ++pos;
                    } else {
                        dot = true;
                        ++pos;
                        while (pos < source.size() && isdigit((unsigned char)source[pos])) ++pos;
                    }
                }
                string lex = source.substr(start, pos - start);
                tokens.push_back({dot ? TokenType::FLOAT : TokenType::INTEGER, lex, startLine});
                continue;
            }

            if (c == '"' || c == '\'') {
                int startLine = line;
                char quote = c;
                ++pos;
                size_t start = pos;
                bool closed = false;
                while (pos < source.size()) {
                    if (source[pos] == quote) {
                        closed = true;
                        break;
                    }
                    if (source[pos] == '\n') ++line;
                    ++pos;
                }
                if (!closed) {
                    addError("unterminated string literal");
                    tokens.push_back({TokenType::INVALID, source.substr(start - 1), startLine});
                } else {
                    string lex = source.substr(start, pos - start);
                    ++pos;
                    tokens.push_back({TokenType::STRING, lex, startLine});
                }
                continue;
            }

            TokenType type = TokenType::INVALID;
            string lex(1, c);

            if (c == '=' ) {
                type = TokenType::EQ;
            } else if (c == '!') {
                if (pos + 1 < source.size() && source[pos + 1] == '=') {
                    type = TokenType::NEQ; lex = "!="; ++pos;
                } else {
                    addError("unexpected '!'; use '!='");
                }
            } else if (c == '<') {
                if (pos + 1 < source.size() && source[pos + 1] == '=') {
                    type = TokenType::LE; lex = "<="; ++pos;
                } else type = TokenType::LT;
            } else if (c == '>') {
                if (pos + 1 < source.size() && source[pos + 1] == '=') {
                    type = TokenType::GE; lex = ">="; ++pos;
                } else type = TokenType::GT;
            } else if (c == ',') type = TokenType::COMMA;
            else if (c == ';') type = TokenType::SEMICOLON;
            else if (c == '(') type = TokenType::LPAREN;
            else if (c == ')') type = TokenType::RPAREN;

            if (type == TokenType::INVALID) {
                addError("invalid character '" + lex + "'");
            } else {
                tokens.push_back({type, lex, line});
            }
            ++pos;
        }

        tokens.push_back({TokenType::END, "", line});
        return tokens;
    }

    const vector<string>& getErrors() const { return errors; }
};

enum class DataType { INT, FLOAT, STRING, UNKNOWN };

string typeName(DataType t) {
    switch (t) {
        case DataType::INT: return "INT";
        case DataType::FLOAT: return "FLOAT";
        case DataType::STRING: return "STRING";
        default: return "UNKNOWN";
    }
}

struct FieldInfo {
    DataType type;
};

struct TableInfo {
    unordered_map<string, FieldInfo> fields;
};

class SymbolTable {
    unordered_map<string, TableInfo> tables;

public:
    SymbolTable() {
        tables["students"] = {{
            {"name",  {DataType::STRING}},
            {"marks", {DataType::INT}},
            {"age",   {DataType::INT}},
            {"cgpa",  {DataType::FLOAT}}
        }};
        tables["employees"] = {{
            {"name",   {DataType::STRING}},
            {"salary", {DataType::FLOAT}},
            {"age",    {DataType::INT}},
            {"dept",   {DataType::STRING}}
        }};
    }

    bool hasTable(const string& table) const {
        return tables.find(table) != tables.end();
    }

    bool hasField(const string& table, const string& field) const {
        auto it = tables.find(table);
        if (it == tables.end()) return false;
        return it->second.fields.find(field) != it->second.fields.end();
    }

    DataType fieldType(const string& table, const string& field) const {
        auto it = tables.find(table);
        if (it == tables.end()) return DataType::UNKNOWN;
        auto f = it->second.fields.find(field);
        if (f == it->second.fields.end()) return DataType::UNKNOWN;
        return f->second.type;
    }

    void print() const {
        cout << "\nSYMBOL TABLE\n";
        cout << "---------------------------------\n";
        for (const auto& [table, info] : tables) {
            cout << "Table: " << table << "\n";
            for (const auto& [field, f] : info.fields)
                cout << "  " << left << setw(10) << field << " : " << typeName(f.type) << "\n";
        }
    }
};

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual void print(int depth = 0) const = 0;
};

using AST = unique_ptr<ASTNode>;

string indent(int n) { return string(n * 2, ' '); }

struct LiteralNode : ASTNode {
    string value;
    DataType type;
    LiteralNode(string v, DataType t) : value(move(v)), type(t) {}
    void print(int d) const override {
        cout << indent(d) << "Literal(" << value << ") [" << typeName(type) << "]\n";
    }
};

struct IdentifierNode : ASTNode {
    string name;
    explicit IdentifierNode(string n) : name(move(n)) {}
    void print(int d) const override {
        cout << indent(d) << "Identifier(" << name << ")\n";
    }
};

struct ConditionNode : ASTNode {
    string op;
    unique_ptr<IdentifierNode> field;
    unique_ptr<LiteralNode> literal;
    ConditionNode(string o, unique_ptr<IdentifierNode> f, unique_ptr<LiteralNode> l)
        : op(move(o)), field(move(f)), literal(move(l)) {}
    void print(int d) const override {
        cout << indent(d) << "Condition(" << op << ")\n";
        field->print(d + 1);
        literal->print(d + 1);
    }
};

struct LogicalNode : ASTNode {
    string op;
    AST left, right;
    LogicalNode(string o, AST l, AST r) : op(move(o)), left(move(l)), right(move(r)) {}
    void print(int d) const override {
        cout << indent(d) << "Logical(" << op << ")\n";
        left->print(d + 1);
        right->print(d + 1);
    }
};

struct QueryNode : ASTNode {
    vector<string> fields;
    string table;
    AST condition;

    QueryNode(vector<string> f, string t, AST c)
        : fields(move(f)), table(move(t)), condition(move(c)) {}

    void print(int d) const override {
        cout << indent(d) << "QUERY\n";
        cout << indent(d + 1) << "SELECT_LIST\n";
        for (const auto& f : fields)
            cout << indent(d + 2) << f << "\n";
        cout << indent(d + 1) << "FROM\n";
        cout << indent(d + 2) << table << "\n";
        if (condition) {
            cout << indent(d + 1) << "WHERE\n";
            condition->print(d + 2);
        }
    }
};

class Parser {
    const vector<Token>& tokens;
    size_t pos = 0;
    vector<string> errors;

    const Token& current() const { return tokens[pos]; }

    bool match(TokenType t) {
        if (current().type == t) {
            ++pos;
            return true;
        }
        return false;
    }

    bool expect(TokenType t, const string& what) {
        if (current().type == t) {
            ++pos;
            return true;
        }
        errors.push_back("Syntax error at line " + to_string(current().line) +
                         ": expected " + what + ", found '" + current().lexeme + "'");
        return false;
    }

    unique_ptr<LiteralNode> parseLiteral() {
        Token tok = current();
        if (tok.type == TokenType::INTEGER) {
            ++pos;
            return make_unique<LiteralNode>(tok.lexeme, DataType::INT);
        }
        if (tok.type == TokenType::FLOAT) {
            ++pos;
            return make_unique<LiteralNode>(tok.lexeme, DataType::FLOAT);
        }
        if (tok.type == TokenType::STRING) {
            ++pos;
            return make_unique<LiteralNode>(tok.lexeme, DataType::STRING);
        }
        errors.push_back("Syntax error at line " + to_string(tok.line) +
                         ": expected integer, float, or string literal");
        return nullptr;
    }

    string parseRelOp() {
        Token tok = current();
        switch (tok.type) {
            case TokenType::EQ: ++pos; return "=";
            case TokenType::NEQ: ++pos; return "!=";
            case TokenType::LT: ++pos; return "<";
            case TokenType::GT: ++pos; return ">";
            case TokenType::LE: ++pos; return "<=";
            case TokenType::GE: ++pos; return ">=";
            default:
                errors.push_back("Syntax error at line " + to_string(tok.line) +
                                 ": expected relational operator");
                return "";
        }
    }

    AST parseSimpleCondition() {
        if (current().type != TokenType::IDENTIFIER) {
            errors.push_back("Syntax error at line " + to_string(current().line) +
                             ": expected field name in WHERE condition");
            return nullptr;
        }
        auto field = make_unique<IdentifierNode>(current().lexeme);
        ++pos;

        string op = parseRelOp();
        if (op.empty()) return nullptr;

        auto lit = parseLiteral();
        if (!lit) return nullptr;

        return make_unique<ConditionNode>(op, move(field), move(lit));
    }

    AST parsePrimaryCondition() {
        if (match(TokenType::LPAREN)) {
            AST node = parseOr();
            expect(TokenType::RPAREN, "')'");
            return node;
        }
        return parseSimpleCondition();
    }

    AST parseAnd() {
        AST left = parsePrimaryCondition();
        if (!left) return nullptr;
        while (match(TokenType::AND)) {
            AST right = parsePrimaryCondition();
            if (!right) return nullptr;
            left = make_unique<LogicalNode>("AND", move(left), move(right));
        }
        return left;
    }

    AST parseOr() {
        AST left = parseAnd();
        if (!left) return nullptr;
        while (match(TokenType::OR)) {
            AST right = parseAnd();
            if (!right) return nullptr;
            left = make_unique<LogicalNode>("OR", move(left), move(right));
        }
        return left;
    }

public:
    explicit Parser(const vector<Token>& t) : tokens(t) {}

    AST parse() {
        if (!expect(TokenType::SELECT, "SELECT")) return nullptr;

        vector<string> fields;
        if (current().type != TokenType::IDENTIFIER) {
            errors.push_back("Syntax error at line " + to_string(current().line) +
                             ": expected field after SELECT");
            return nullptr;
        }

        fields.push_back(current().lexeme);
        ++pos;

        while (match(TokenType::COMMA)) {
            if (current().type != TokenType::IDENTIFIER) {
                errors.push_back("Syntax error at line " + to_string(current().line) +
                                 ": expected field after comma");
                return nullptr;
            }
            fields.push_back(current().lexeme);
            ++pos;
        }

        if (!expect(TokenType::FROM, "FROM")) return nullptr;

        if (current().type != TokenType::IDENTIFIER) {
            errors.push_back("Syntax error at line " + to_string(current().line) +
                             ": expected table name after FROM");
            return nullptr;
        }
        string table = current().lexeme;
        ++pos;

        AST condition = nullptr;
        if (match(TokenType::WHERE))
            condition = parseOr();

        if (!expect(TokenType::SEMICOLON, "';'")) return nullptr;

        if (current().type != TokenType::END) {
            errors.push_back("Syntax error at line " + to_string(current().line) +
                             ": unexpected token '" + current().lexeme + "'");
            return nullptr;
        }

        return make_unique<QueryNode>(move(fields), move(table), move(condition));
    }

    const vector<string>& getErrors() const { return errors; }
};

class SemanticAnalyzer {
    const SymbolTable& symbols;
    vector<string> errors;

    bool compatible(DataType field, DataType lit, const string& op) {
        if (field == DataType::UNKNOWN || lit == DataType::UNKNOWN) return false;
        if (field == DataType::STRING || lit == DataType::STRING)
            return field == DataType::STRING && lit == DataType::STRING &&
                   (op == "=" || op == "!=");
        return true; // INT/FLOAT comparisons are allowed
    }

    void checkCondition(const ASTNode* node, const string& table) {
        if (!node) return;

        if (auto c = dynamic_cast<const ConditionNode*>(node)) {
            string field = c->field->name;
            if (!symbols.hasField(table, field)) {
                errors.push_back("Semantic error: field '" + field +
                                 "' does not exist in table '" + table + "'");
                return;
            }

            DataType ft = symbols.fieldType(table, field);
            if (!compatible(ft, c->literal->type, c->op)) {
                errors.push_back("Semantic error: incompatible comparison '" +
                                 field + " " + c->op + " " + c->literal->value +
                                 "' (" + typeName(ft) + " vs " +
                                 typeName(c->literal->type) + ")");
            }
        } else if (auto l = dynamic_cast<const LogicalNode*>(node)) {
            checkCondition(l->left.get(), table);
            checkCondition(l->right.get(), table);
        }
    }

public:
    explicit SemanticAnalyzer(const SymbolTable& s) : symbols(s) {}

    bool analyze(const QueryNode* query) {
        if (!symbols.hasTable(query->table)) {
            errors.push_back("Semantic error: unknown table '" + query->table + "'");
            return false;
        }

        for (const auto& field : query->fields) {
            if (!symbols.hasField(query->table, field))
                errors.push_back("Semantic error: field '" + field +
                                 "' is not present in table '" + query->table + "'");
        }

        checkCondition(query->condition.get(), query->table);
        return errors.empty();
    }

    const vector<string>& getErrors() const { return errors; }
};

struct TACInstruction {
    string op, arg1, arg2, result;
};

class TACGenerator {
    vector<TACInstruction> code;
    int tempCount = 0;

    string newTemp() { return "t" + to_string(++tempCount); }

    string generateCondition(const ASTNode* node) {
        if (auto c = dynamic_cast<const ConditionNode*>(node)) {
            string t = newTemp();
            code.push_back({c->op, c->field->name, c->literal->value, t});
            return t;
        }

        auto l = dynamic_cast<const LogicalNode*>(node);
        string left = generateCondition(l->left.get());
        string right = generateCondition(l->right.get());
        string t = newTemp();
        code.push_back({l->op, left, right, t});
        return t;
    }

public:
    vector<TACInstruction> generate(const QueryNode* query) {
        code.clear();
        tempCount = 0;

        string condTemp;
        if (query->condition)
            condTemp = generateCondition(query->condition.get());

        if (!condTemp.empty())
            code.push_back({"FILTER", query->table, condTemp, ""});

        string selectList;
        for (size_t i = 0; i < query->fields.size(); ++i) {
            if (i) selectList += ", ";
            selectList += query->fields[i];
        }
        code.push_back({"SELECT", selectList, "", ""});

        return code;
    }
};

void printTokens(const vector<Token>& tokens) {
    cout << "\nTOKEN STREAM\n";
    cout << "-------------------------------------------------------------\n";
    for (const auto& t : tokens) {
        if (t.type == TokenType::END) {
            cout << "EOF\n";
            break;
        }
        cout << left << setw(12) << tokenName(t.type)
             << " Lexeme: " << t.lexeme
             << "  Line: " << t.line << "\n";
    }
}

void printTAC(const vector<TACInstruction>& code) {
    cout << "\nTHREE-ADDRESS CODE (TAC)\n";
    cout << "-------------------------------------------------------------\n";
    int n = 1;
    for (const auto& q : code) {
        if (q.op == "FILTER")
            cout << n++ << ". FILTER " << q.arg1 << " USING " << q.arg2 << "\n";
        else if (q.op == "SELECT")
            cout << n++ << ". SELECT " << q.arg1 << "\n";
        else
            cout << n++ << ". " << q.result << " = " << q.arg1
                 << " " << q.op << " " << q.arg2 << "\n";
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "=============================================================\n";
    cout << "        MINI QUERY LANGUAGE COMPILER - PHASE 2\n";
    cout << "=============================================================\n";
    cout << "Supported syntax:\n";
    cout << "SELECT field[, field...] FROM table [WHERE condition];\n";
    cout << "Conditions: = != < > <= >= with AND / OR and parentheses.\n";
    cout << "Example:\n";
    cout << "SELECT name, marks FROM students WHERE marks > 80 AND age >= 18;\n\n";

    cout << "Enter query:\n> ";
    string query, line;
    while (getline(cin, line)) {
        query += line;
        if (line.find(';') != string::npos) break;
        query += '\n';
    }

    Lexer lexer(query);
    vector<Token> tokens = lexer.tokenize();

    printTokens(tokens);

    if (!lexer.getErrors().empty()) {
        cout << "\nLEXICAL ERRORS\n";
        for (const auto& e : lexer.getErrors()) cout << "- " << e << "\n";
        return 1;
    }

    Parser parser(tokens);
    AST root = parser.parse();

    if (!parser.getErrors().empty()) {
        cout << "\nSYNTAX ERRORS\n";
        for (const auto& e : parser.getErrors()) cout << "- " << e << "\n";
        return 1;
    }

    auto* queryAst = dynamic_cast<QueryNode*>(root.get());

    cout << "\nABSTRACT SYNTAX TREE (AST)\n";
    cout << "-------------------------------------------------------------\n";
    queryAst->print(0);

    SymbolTable symbols;
    symbols.print();

    SemanticAnalyzer semantic(symbols);
    if (!semantic.analyze(queryAst)) {
        cout << "\nSEMANTIC ERRORS\n";
        for (const auto& e : semantic.getErrors()) cout << "- " << e << "\n";
        return 1;
    }

    cout << "\nSEMANTIC ANALYSIS: PASSED\n";
    cout << "All referenced tables, fields and condition types are valid.\n";

    TACGenerator generator;
    vector<TACInstruction> tac = generator.generate(queryAst);
    printTAC(tac);

    cout << "\n=============================================================\n";
    cout << "Compilation completed successfully.\n";
    cout << "Pipeline: Source Query -> Lexer -> Parser -> AST ->\n";
    cout << "          Symbol Table + Semantic Analysis -> TAC\n";
    cout << "=============================================================\n";

    return 0;
}
