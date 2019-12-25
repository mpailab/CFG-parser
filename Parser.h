#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include <cctype>
#include <algorithm>
#include <memory>
#include <string>
#include "Exception.h"
#include "Lexer.h"

using namespace std;
/*
struct Position {
	int line;
	int col;
};*/

struct ParseNode {
	int nt;               // ����� ��������� ��� �����������
	int B = -1;           // ����� �������������� ����������� � ������ ������ nt <- B <- rule
	int rule=-1;          // ����� ������� (-1 => ��������)
	string term;          // ��������� �������� (���� ��������)
	ParseNode* p = 0;     // ������ �� ������������ ����
	vector<ParseNode> ch; // �������� ����
	Location loc;         // ���������� ��������� � ������
	bool isTerminal()const {
		return rule < 0;
	}
};


struct NTSetS { // ��������� ������������
	unordered_set<int> s;
	typedef unordered_set<int>::const_iterator iterator;
	typedef int value_type;
	bool intersects(const NTSetS&y) const{
		for (int x : y.s)
			if (s.count(x))return true;
		return false;
	}
	int intersects(const NTSetS&y, int *B) const {
		int res = 0;
		for (int x : y.s)
			if (s.count(x)) {
				res++;
				*B = x;
				if (res >= 2)return res;
			}
		return res;
	}
	/// ���������� ��������� ������������
	bool filter(NTSetS &y)const {
		for (auto it = y.s.begin(); it != y.s.end();) {
			if (s.count(*it))++it;
			else it = y.s.erase(it);
		}
		return !y.s.empty();
	}
	bool has(int x)const {
		return s.count(x)>0;
	}
	bool add(int x) {
		return s.insert(x).second;
	}
	bool empty()const { return s.empty(); }
	NTSetS&clear() { s.clear(); return *this; }
	NTSetS() = default;
	NTSetS(std::initializer_list<int> &l) :s(l) {}
	NTSetS& operator|=(const NTSetS &s) {
		for (int i : s.s)
			add(i);
		return *this;
	}
	NTSetS& operator&=(const NTSetS &ns) {
		for (auto it = s.begin(); it != s.end(); )
			if (!ns.has(*it))it = s.erase(it);
			else ++it;
		return *this;
	}
	NTSetS operator|(const NTSetS &x) const {
		NTSetS r(*this);
		return r |= x;
	}
	NTSetS operator&(const NTSetS &x) const {
		NTSetS r(*this);
		return r &= x;
	}
	NTSetS& operator=(const std::initializer_list<int> &l) {
		s = l;
		return *this;
	}
	NTSetS& operator=(const vector<int> &l) {
		s.clear();
		s.insert(l.begin(), l.end());
		return *this;
	}
	iterator begin()const { return s.begin(); }
	iterator end()const { return s.end(); }
};
typedef uint64_t u64;
#if 0
struct NTSetV {
	vector<u64> mask;
	struct iterator {
		u64 curr;
		int pos=0;
		const u64 *ptr, *end;
		iterator(const u64*p, size_t l):curr(0),ptr(p),end(p+l) {
			next();
		}
		iterator& next() {
			while (ptr < end && !*ptr) {
				pos += 64;
				++ptr;
			}
			if (ptr != end)
				curr = *ptr;
			return *this;
		}
		iterator& operator++() {
			if(curr &= (curr - 1))return *this;
			++ptr;
			return next();
		}
		int operator*() const{
			return pos + _lzcnt_u64(curr);
		}
		bool operator==(const iterator& i)const {
			return ptr == i.ptr && curr == i.curr;
		}
		bool operator!=(const iterator& i)const {
			return ptr != i.ptr || curr != i.curr;
		}
	};
	bool intersects(const NTSetV&y) const {
		for (int i = 0, sz = (int)mask.size(); i < sz; i++)
			if (mask[i] & y.mask[i])return true;
		return false;
	}
	/// ���������� ��������� ������������
	bool filter(NTSetV &y)const {
		//y &= *this;
		int sz = (int)min(y.mask.size(),mask.size());
		y.mask.resize(sz);
		u64 r = 0;
		for (int i = 0; i < sz; i++)
			r |= y.mask[i] &= mask[i];
		return r!=0;
	}
	bool has(int x)const {
		return (x>>6)<mask.size() && ((mask[x >> 6] >> (x & 63)) & 1)!=0;
	}
	void add(int x) {
		if (x >= mask.size() * 64)
			mask.resize((x-1) >> 6 + 1);
		mask[x >> 6] |= 1ULL << (x & 63);// s.insert(x).second;
	}
	bool empty()const {
		for (auto x : mask)
			if (x)return false;
		return true;
	}
	NTSetV&clear() { fill(mask.begin(), mask.end(), 0); return *this; }
	NTSetV() = default;
	NTSetV(std::initializer_list<int> &l) {}
	NTSetV& operator|=(const NTSetV &s) {
		mask.resize(max(mask.size(), s.mask.size()),0ULL);
		for (int i = 0, sz = (int)s.mask.size(); i < sz; i++)
			mask[i] |= s.mask[i];
		return *this;
	}
	NTSetV& operator&=(const NTSetV &ns) {
		ns.filter(*this);
		return *this;
	}
	NTSetV operator|(const NTSetV &x) const {
		NTSetV r(*this);
		return r |= x;
	}
	NTSetV operator&(const NTSetV &x) const {
		NTSetV r(*this);
		return r &= x;
	}
	NTSetV& operator=(const std::initializer_list<int> &l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	NTSetV& operator=(const vector<int> &l) {
		clear();
		for (int i : l)
			add(i);
		return *this;
	}
	iterator begin()const { return iterator(mask.data(), mask.size()); }
	iterator end()const { return iterator(mask.data()+mask.size(),0); }
};
#endif
typedef NTSetS NTSet;

//struct NTTreeNode;
template<class Node>
struct NTTreeEdge {
	Node end;  // ����� �����
	//NTSet filter;    // ������ �� �����
};
template<class T>
struct Ref : unique_ptr<T> {
	Ref() :unique_ptr<T>(new T) {}
};
struct NTTreeNode {
	//typedef NTTreeEdge<NTTreeNode> Edge;
	//unordered_map<string, Edge> charedges; // ��������� �� ����������� ����������
	unordered_map<int, Ref<NTTreeNode>> termEdges;    // ��������� �� ������������� ����������
	unordered_map<int, Ref<NTTreeNode>> ntEdges;      // ��������� �� ������������
	NTSet phi;                                   // �����������, �� ������� ����� ������ �� ������� �������
	NTSet finalNT;                               // �����������, ��� ������� ������� ������� -- ���������
	NTSet next;                                // ��������� ������������, �� ������� ����� ������ ������ �� ������-���� �����
	NTSet nextnt;                              // ��������� ������������, �� ������� ����� ������ ������ �� ��������������� �����
	unordered_map<int, int> rules;             // ������� �� ������� ������������
	int pos = 0;                               // ���������� �� ����� ������
	int _frule = -1;
	const NTTreeNode* nextN(int A)const {
		auto it = ntEdges.find(A);
		if (it == ntEdges.end())return 0;
		return it->second.get();
	}
	int rule(int A)const {
		return rules.find(A)->second;
	}
};

struct TF {
	vector<NTSet> T;     // �� ����������� ���������� ��� �����������, ������� ����������� �� ������� ����������� (A :-> {B | B => A})
	vector<NTSet> inv;   // �� ����������� ���������� ��� �����������, �� ������� ����������� ������ ����������   (A :-> {B | A => B})
	vector<NTSet> fst;   // �� ����������� ���������� ��� �����������, � ������� ����� ���������� ������ ����������
	vector<NTSet> ifst;  // �� ����������� ���������� ��� �����������, ������� ����� ���������� � ������� �����������
	void addRuleBeg(int /*pos*/, int A, int rhs0, int len) { // ��������� � ��������� ������� A -> rhs0 ...; pos -- ������� � ������; len -- ����� ������ ����� �������
		int mx = max(A, rhs0);
		checkSize(mx);
		//if (T.size() <= mx)T.resize(mx + 1), inv.resize(mx + 1);
		if (len == 1) {
			//T[rhs0].add(A);
			for(int x : inv[rhs0])
				T[x] |= T[A]; // B -> A ..., A -> rhs0 ... ==> B -> rhs0 ...

			//inv[A].add(rhs0);
			for(int x : T[A])
				inv[x] |= inv[rhs0]; // A -> rhs0 ..., rhs0 -> B ... ==> A -> B ...
		}
		for(int x : ifst[A])
			fst[x] |= fst[rhs0];
		for(int x : fst[rhs0])
			ifst[x] |= ifst[A];
		// TODO: ���-�� ���������� ������� � ���������, ����� ����� ���� ����� �������� �����.
	}
	void checkSize(int A) {
		int n0 = (int)T.size();
		if (n0 <= A) {
			T.resize(A + 1), inv.resize(A + 1), fst.resize(A + 1), ifst.resize(A + 1);
			for (int i = n0; i <= A; i++) {
				T[i].add(i);
				inv[i].add(i);
				fst[i].add(i);
				ifst[i].add(i);
			}
		}
	}
};


struct NTTree {
	unordered_map<int, vector<int>> ntFirstMap; // �� �������� ����������� ���������� ������ ���� ������������, � ������� ������ ���������� ����� ����������
	unordered_map<int, vector<int>> tFirstMap;  // �� �������� ����������� ���������� ������ ���� ����������, � ������� ������ ���������� ����� ����������
	NTTreeNode root;
	int start;
};

struct RuleElem {
	int num;
	bool term;
	bool save;
};

struct CFGRule {
	int A;
	vector<RuleElem> rhs; 
	int used;
};

struct GrammarState {
	//unordered_map<int, vector<int>> ntFirstMap; // �� �������� ����������� ���������� ������ ���� ������������, � ������� ������ ���������� ����� ����������
	//unordered_map<int, vector<int>> tFirstMap;  // �� �������� ����������� ���������� ������ ���� ����������, � ������� ������ ���������� ����� ����������
	unordered_map<int, NTSet> tFirstMap;   // �� ��������� ����������, ����� ����������� ����� ���������� � ������� ���������
	//vector<NTSet> ntFirstMap;              // �� ����������� ���������� ��������� ������������, � ������� ������ ���������� ����� ����������
	vector<vector<NTTreeNode*>> ntRules;   // ������� ����������� �������������� ������ ��������� ������, ��������������� �������� ��� ������� �����������
	NTTreeNode root;        // �������� ������� ������ ������
	Enumerator<string, unordered_map> nts; // ��������� ������������
	Enumerator<string, unordered_map> ts;  // ��������� ����������
	vector<CFGRule> rules;
	int start;
	bool finish = false;
	TF tf;
	Lexer lex;
	int syntaxDefNT=0;
	int tokenNT=0;
	vector<pair<Pos, string>> _err;
	void error(const string &err);

	void addToken(const string& term, const string& re);
	bool addRule(const string &lhs, const vector<string> &rhs);
	bool addRule(const CFGRule &r);

	bool addToken(const ParseNode *tokenDef);
	bool addRule(const ParseNode *ruleDef);
	GrammarState() {
		ts[""];
		nts[""];
	}
	ParseNode reduce(const ParseNode *pn, const NTTreeNode *v, int nt, int nt1) {
		int r = v->rule(nt), sz = len(rules[r].rhs);
		ParseNode res;
		res.rule = r;
		res.B = nt;
		res.nt = nt1;
		for (int i = 0; i < sz; i++)
			if (rules[r].rhs[i].save)
				res.ch.emplace_back(move(pn[i]));
		res.loc.beg = pn[0].loc.beg; 
		res.loc.end = pn[sz - 1].loc.end;
		if (nt == tokenNT)addToken(&res);
		else if (nt == syntaxDefNT)addRule(&res);
		return std::move(res);
	}
	void setNtNames(const string&start, const string& token, const string &syntax) {
		int S0 = nts[""];
		this->start = nts[start];
		CFGRule r;
		r.A = S0;
		r.rhs.resize(2);
		r.rhs[0].num = this->start;
		r.rhs[0].save = true;
		r.rhs[0].term = false;
		r.rhs[1].term = true;
		r.rhs[1].save = false;
		r.rhs[1].num = 0;
		addRule(r);
		if(!token.empty())
			tokenNT = nts[token];
		if (!syntax.empty())
			syntaxDefNT = nts[syntax];
	}
	void print_rules(ostream &s) const{
		for (auto &r : rules) {
			s << nts[r.A] << " -> ";
			for (auto& rr : r.rhs) {
				if (rr.term)s << ts[rr.num] << " ";
				else s << nts[rr.num] << " ";
			}
			s << "\n";
		}
	}
};


typedef unordered_map<int, TrieM<int>> FollowR_T;

struct RulePos {
	mutable const NTTreeNode* sh = 0;
	const NTTreeNode *v = 0; // ������� ������� � ������ ������
	NTSet M;                 // ��������� ������������ � ����� ������
};

struct LR0State {
	vector<RulePos> v;
};


struct SStack {
	vector<LR0State> s;
};

struct PStack {
	vector<ParseNode> s;
};
template<class T>
struct dvector {
	vector<T> v;
	const T& operator[](size_t i)const {
		return v[i];
	}
	T& operator[](size_t i) {
		if (i >= v.size())v.resize(i + 1);
		return v[i];
	}
	int size()const { return (int)v.size(); }
};

#define Assert(cond) if(!(cond))throw Exception(#cond " assertion failed ")

ParseNode parse(GrammarState & g, const std::string& text);

struct StackFrame {
	const NTTreeNode *v = 0; // ������� ������� � ������ ������
	NTSetS nts;         // ������� ��������� ������������
	NTSetS leftrec;     // �����������, ��� ������� �� ����� ��������� ���������� ���������� (��, ���� ������� ���� ��������� ���� �� �����)
	FollowR_T fr;      // ��������� ��������� ����������� ����� ������ � ����������� �� �����������
	vector<ParseNode> parsed; // ������ ����������� �����������, ��������������� ����������� ������������ �������� �������
};

typedef vector<StackFrame> ParseStack;

struct ParseState {
	ParseStack st;
	FollowR_T followR;
	StackFrame&operator[](size_t x) { return st[x]; }
	StackFrame& top() { return st.back(); }
};

struct ParserError {
	Location loc;
	string err;
};
#if 0
class Parser {
public:
	NTTree gr;
	Lexer lex;
	void error(const std::string &s);
	bool reduce();        // ������ �� ������������, ��� ������� ������ ������� �������� ���������
	bool reduceN(int nt); // ������ �� ������� �����������
	void initFrame(StackFrame& f, vector<int> nts);
	void parse(ParseState&st, const char *word);
};
#endif
