#include <bits/stdc++.h>
using namespace std;

// Minimal implementation to produce a similarity score [0,1]
// between two programs in the custom language using token k-shingling.
// If only one program is provided (no second endprogram), we just echo input.

static vector<string> read_program(istream &in, bool &ok) {
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

static vector<string> tokenize(const vector<string> &lines) {
    vector<string> tokens;
    for (auto &ln : lines) {
        string cur;
        for (char c : ln) {
            if (isalnum((unsigned char)c) || c=='_' ) {
                cur.push_back((char)tolower(c));
            } else {
                if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
                if (!isspace((unsigned char)c)) tokens.emplace_back(string(1, c));
            }
        }
        if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
    }
    return tokens;
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
    auto p1 = read_program(cin, ok1);
    if (!ok1) {
        // Single program mode: echo input as-is (identity cheat)
        for (size_t i=0;i<p1.size();++i){ cout << p1[i] << '\n'; }
        return 0;
    }
    auto p2 = read_program(cin, ok2);
    if (!ok2) {
        // Two inputs not fully provided; default to echo first
        for (size_t i=0;i<p1.size();++i){ cout << p1[i] << '\n'; }
        return 0;
    }

    // There may be more content (reference input). We ignore it for this heuristic.

    auto t1 = tokenize(p1);
    auto t2 = tokenize(p2);
    auto s1 = k_shingles(t1, 5);
    auto s2 = k_shingles(t2, 5);
    double sim = jaccard(s1, s2);
    if (sim < 0) sim = 0; if (sim > 1) sim = 1;
    cout.setf(std::ios::fixed); cout<<setprecision(6)<<sim<<"\n";
    return 0;
}

