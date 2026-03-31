// Unified solution:
// - If input contains two programs (each ends with a line 'endprogram'),
//   output a similarity score in [0,1] using token k-shingling over S-expr tokens.
// - If input contains only one program, output a semantics-preserving rewrite:
//   consistent renaming of identifiers (except keywords, builtins, and 'main'),
//   remove comments, and normalize formatting.

#include <bits/stdc++.h>
using namespace std;

static vector<string> read_program_lines(istream &in, bool &ok) {
    vector<string> lines;
    string s;
    ok = false;
    while (true) {
        if (!getline(in, s)) break;
        if (s == "endprogram") { ok = true; break; }
        lines.push_back(s);
    }
    return lines;
}

// Tokenize PPCA S-expr: tokens are '(', ')' and atoms; ';' starts a comment to EOL.
static vector<string> sexpr_tokens(const vector<string> &lines) {
    vector<string> tokens;
    for (auto &ln : lines) {
        string cur;
        for (size_t i = 0; i < ln.size(); ++i) {
            char c = ln[i];
            if (c == ';') break; // comment
            if (c == '(' || c == ')') {
                if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
                tokens.emplace_back(1, c);
            } else if (isspace((unsigned char)c)) {
                if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
            } else {
                cur.push_back(c);
            }
        }
        if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
    }
    return tokens;
}

static bool is_integer_atom(const string &a) {
    if (a.empty()) return false;
    size_t i = 0;
    if (a[0] == '-') {
        if (a.size() == 1) return false;
        i = 1;
    }
    for (; i < a.size(); ++i) if (!isdigit((unsigned char)a[i])) return false;
    return true;
}

static unordered_set<string> builtin_atoms() {
    static const char* arr[] = {
        "+","-","*","/","%",
        "<",">","<=",">=","==","!=","||","&&","!",
        "scan","print",
        "array.create","array.scan","array.print","array.get","array.set"
    };
    unordered_set<string> s;
    for (auto &x: arr) s.insert(x);
    return s;
}

static unordered_set<string> keyword_atoms(){
    static const char* arr[] = {"function","block","if","for","return","set"};
    unordered_set<string> s;
    for (auto &x: arr) s.insert(x);
    return s;
}

// Determine if an atom is an identifier we can rename
static bool is_identifier_atom(const string &a, const unordered_set<string>& builtins, const unordered_set<string>& keywords) {
    if (a == "(" || a == ")") return false;
    if (keywords.count(a)) return false;
    if (builtins.count(a)) return false;
    if (is_integer_atom(a)) return false;
    // variable/function names cannot start with digit; '-' start cannot be pure number; we skip checking and assume atom is id here
    return true;
}

static vector<string> rename_identifiers(const vector<string>& tokens) {
    unordered_set<string> builtins = builtin_atoms();
    unordered_set<string> keywords = keyword_atoms();
    unordered_map<string,string> mp;
    vector<string> out;
    out.reserve(tokens.size());
    int ctr = 0;
    auto gen = [&](){ return string("id") + to_string(++ctr); };
    for (size_t i=0;i<tokens.size();++i){
        const string &t = tokens[i];
        if (t == "(" || t == ")") { out.push_back(t); continue; }
        if (!is_identifier_atom(t, builtins, keywords)) { out.push_back(t); continue; }
        if (t == "main") { out.push_back(t); continue; }
        auto it = mp.find(t);
        if (it == mp.end()) it = mp.emplace(t, gen()).first;
        out.push_back(it->second);
    }
    return out;
}

static string format_program(const vector<string>& tokens) {
    // Simple pretty printer: tokens separated by spaces, newline after ')'
    string out;
    int col = 0;
    for (size_t i=0;i<tokens.size();++i){
        const string &t = tokens[i];
        out += t;
        out += (t == ")" ? '\n' : ' ');
        // limit line length a bit
        if (t != ")") {
            col += (int)t.size() + 1;
            if (col > 80) { out += '\n'; col = 0; }
        } else {
            col = 0;
        }
    }
    return out;
}

template<class T>
static unordered_set<string> k_shingles(const vector<T>& toks, int k=5){
    unordered_set<string> s; s.reserve(toks.size()*2+1);
    if ((int)toks.size() < k) return s;
    for (int i=0;i + k <= (int)toks.size();++i){
        string key;
        key.reserve(k*4);
        for (int j=0;j<k;++j){
            key += toks[i+j];
            key.push_back('\x1f');
        }
        s.insert(move(key));
    }
    return s;
}

static double jaccard(const unordered_set<string>& A, const unordered_set<string>& B){
    if (A.empty() && B.empty()) return 0.0; // treat empty as dissimilar
    size_t inter=0;
    if (A.size() < B.size()){
        for (auto &x: A) if (B.find(x)!=B.end()) ++inter;
    } else {
        for (auto &x: B) if (A.find(x)!=A.end()) ++inter;
    }
    size_t uni = A.size() + B.size() - inter;
    if (uni==0) return 1.0;
    return (double)inter / (double)uni;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    bool ok1=false, ok2=false;
    auto p1 = read_program_lines(cin, ok1);
    if (!ok1) {
        // Single program without explicit endprogram: act as cheat, normalize formatting only
        auto toks = sexpr_tokens(p1);
        auto renamed = rename_identifiers(toks);
        cout << format_program(renamed);
        return 0;
    }
    auto p2 = read_program_lines(cin, ok2);
    if (!ok2) {
        // Cheat mode: one program provided
        auto toks = sexpr_tokens(p1);
        auto renamed = rename_identifiers(toks);
        cout << format_program(renamed);
        return 0;
    }

    // Anticheat mode: two programs provided; ignore further input (reference input)
    auto t1 = sexpr_tokens(p1);
    auto t2 = sexpr_tokens(p2);
    // remove parentheses from tokens for similarity comparison
    vector<string> a1, a2;
    a1.reserve(t1.size()); a2.reserve(t2.size());
    for (auto &x: t1) if (x != "(" && x != ")") a1.push_back(x);
    for (auto &x: t2) if (x != "(" && x != ")") a2.push_back(x);
    auto s1 = k_shingles(a1, 5);
    auto s2 = k_shingles(a2, 5);
    double sim = jaccard(s1, s2);
    if (sim < 0) sim = 0; if (sim > 1) sim = 1;
    cout.setf(std::ios::fixed); cout<<setprecision(6)<<sim<<"\n";
    return 0;
}
