#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <set>
#include <vector>
#include <cctype>
#include <algorithm>
#include <memory>
#include <string>
#include "Exception.h"
#include "PEGLexer.h"
#include "NTSet.h"
#include "Hash.h"
#include "Vector.h"
#include "Alloc.h"
//#include "BinAlloc.h"

using namespace std;

struct ParseNode;
using PParseNode = unique_ptr<ParseNode>;
struct ParseTree;

struct ParseNode {
	int nt;               // ����� ��������� ��� �����������
	int B = -1;           // ����� �������������� ����������� � ������ ������ nt <- B <- rule
	int rule=-1;          // ����� ������� (-1 => ��������)
	int rule_id = -1;     // ������� ����� �������
	string term;          // ��������� �������� (���� ��������)
	//ParseTree* tree = 0;  // ������ �� ������ �������
	//ParseNode* p = 0;     // ������ �� ������������ ����
	int size = 1;
	unsigned lpr = -1, rpr = -1; // ����� � ������ ���������� (���� unsigned(-1), �� ���������� �� ������)
	vector<ParseNode* /*,BinAlloc<ParseNode>*/> ch; // �������� ����
	Location loc;         // ���������� ��������� � ������
	bool isTerminal()const {
		return rule < 0;
	}
	bool haslpr()const { return (int)lpr != -1; }
	bool hasrpr()const { return (int)rpr != -1; }
	ParseNode& operator[](size_t i) { return *ch[int(i)<0 ? i+ch.size() : i]; }
	const ParseNode& operator[](size_t i)const { return *ch[int(i)<0 ? i + ch.size() : i]; }
	ParseNode() = default;
	ParseNode(const ParseNode&) = delete;
	ParseNode& operator=(const ParseNode&) = delete;

	ParseNode(ParseNode&&) = default;
	void updateSize() {
		size = 1;
		for (auto *n : ch)
			size += n->size;
	}
	ParseNode* balancePr() {		// TODO: ������� ��������, ��� ��� ��������������� ������ ������ ������������, ������� �� ������������ �� ����������� 						                            
		if (haslpr() && nt == ch[0]->nt)					  //        this                           r
			lpr = min(lpr, ch[0]->lpr);						  //       / .. \ 			              / \ 
		if (hasrpr()) {										  //      x...   r			             .   .  
			ParseNode *pn = ch.back(), *pp;					  //            / \ 		            .     .
			if (pn->nt == nt && pn->lpr < rpr) {			  //           .   .     ==>           .........
				do {										  //          .     .		          /
					pn->lpr = min(pn->lpr, lpr);			  //         .........		         pp 
					pp = pn; pn = pn->ch[0];				  //        /				       / .. \ 
				} while (pn->nt == nt && pn->lpr < rpr);      //      pp				   this   ...y
				ParseNode *l = pp->ch[0], *r = ch.back();     //    / .. \ 				  / .. \ 
				pp->ch[0] = this;                             //   pn  ...y				 x...  pn
				ch.back() = l;  
				return r; // ������ ���������� r
			}
		}
		return this; // ������ ������� �������
	}
	ParseNode& operator=(ParseNode&&) = default;
	//void del();
	~ParseNode() {}
};

struct ParseTree {
	Alloc<ParseNode> _alloc;
	ParseNode* root = 0;
	template<class ... Args>
	ParseNode *newnode(Args && ... args) {
		ParseNode*res = _alloc.allocate(args...);
		return res;
	}
	~ParseTree() { del(root); }
	ParseTree() {}
	ParseTree(ParseTree && t):root(t.root),_alloc(move(t._alloc)) { t.root = 0; }
	ParseTree& operator=(ParseTree && t) {
		if (root != t.root)del(root);
		_alloc = std::move(t._alloc);
		root = t.root; t.root = 0;
		return *this;
	}
	void del(ParseNode *r) { 
		for (ParseNode *curr = r, *next = 0; curr; curr = next) {
			next = 0;
			for (auto *n : curr->ch)
				if (n->size > curr->size / 2)
					next = n;
				else del(n);
			_alloc.deallocate(curr);
		}
	}
};

template<class T>
struct Ref : unique_ptr<T> {
	Ref() :unique_ptr<T>(new T) {}
};
struct NTTreeNode {
	unordered_map<int, Ref<NTTreeNode>> termEdges;    // ��������� �� ������������� ����������
	unordered_map<int, Ref<NTTreeNode>> ntEdges;      // ��������� �� ������������
	NTSet phi;                                   // �����������, �� ������� ����� ������ �� ������� �������
	NTSet finalNT;                               // �����������, ��� ������� ������� ������� -- ���������
	NTSet next;                                // ��������� ������������, �� ������� ����� ������ ������ �� ������-���� �����
	NTSet nextnt;                              // ��������� ������������, �� ������� ����� ������ ������ �� ��������������� �����
	unordered_map<int, int> rules;             // ������� �� ������� ������������
	int pos = 0;                               // ���������� �� ����� ������
	int _frule = -1;

	///////////////// ��� ���������� ��������� ���������� ���������� ////////////////////////////
	unordered_map<int, NTSet> next_mt;           // ������������ ����������� A ��������� ����������, �� ������� ����� ������ �� ������ �������, ����� ������� ��� A
	unordered_map<int, NTSet> next_mnt;          // ������������ ����������� A ��������� ������������, �� ������� ����� ������ �� ������ �������, ����� ������� ��� A
	/////////////////////////////////////////////////////////////////////////////////////////////
	
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
	vector<NTSet> T;      // �� ����������� ���������� ��� �����������, ������� ����������� �� ������� ����������� (A :-> {B | B => A})
	vector<NTSet> inv;    // �� ����������� ���������� ��� �����������, �� ������� ����������� ������ ����������   (A :-> {B | A => B})
	vector<NTSet> fst;    // �� ����������� ���������� ��� �����������, � ������� ����� ���������� ������ ����������
	vector<NTSet> ifst;   // �� ����������� ���������� ��� �����������, ������� ����� ���������� � ������� �����������

	vector<NTSet> fst_t;  // �� ����������� ���������� ��� ���������, � ������� ����� ���������� ������ ����������
	vector<NTSet> ifst_t; // �� ��������� ���������� ��� �����������, ������� ����� ���������� � ������� ���������

	void addRuleBeg(int /*pos*/, int A, int rhs0, int len) { // ��������� � ��������� ������� A -> rhs0 ...; pos -- ������� � ������; len -- ����� ������ ����� �������
		int mx = max(A, rhs0);
		checkSize(mx);
		if (len == 1) {
			//T[rhs0].add(A);
			for(int x : inv[rhs0])
				T[x] |= T[A]; // B -> A ..., A -> rhs0 ... ==> B -> rhs0 ...

			//inv[A].add(rhs0);
			for(int x : T[A])
				inv[x] |= inv[rhs0]; // A -> rhs0 ..., rhs0 -> B ... ==> A -> B ...
		}
		for (int x : ifst[A]) {
			fst[x] |= fst[rhs0];
			fst_t[x] |= fst_t[rhs0];
		}
		for (int x : fst_t[rhs0])
			ifst_t[x] |= ifst[A];
		for(int x : fst[rhs0])
			ifst[x] |= ifst[A];
		// TODO: ���-�� ���������� ������� � ���������, ����� ����� ���� ����� �������� �����.
	}
	void addRuleBeg_t(int /*pos*/, int A, int rhs0) { // ��������� � ��������� ������� A -> rhs0 ..., ������ ����� rhs0 -- ��������; pos -- ������� � ������;
		//int mx = max(A, rhs0);
		checkSize(A);
		checkSize_t(rhs0);
		for (int x : ifst[A])
			fst_t[x].add(rhs0);
		ifst_t[rhs0] |= ifst[A];
		// TODO: ���-�� ���������� ������� � ���������, ����� ����� ���� ����� �������� �����.
	}
	void checkSize(int A) {
		int n0 = (int)T.size();
		if (n0 <= A) {
			T.resize(A + 1), inv.resize(A + 1), fst.resize(A + 1), ifst.resize(A + 1), fst_t.resize(A + 1);
			for (int i = n0; i <= A; i++) {
				T[i].add(i);
				inv[i].add(i);
				fst[i].add(i);
				ifst[i].add(i);
			}
		}
	}
	void checkSize_t(int t) {
		int n0 = (int)ifst_t.size();
		if (n0 <= t) {
			ifst_t.resize(t + 1);
		}
	}
};

// ������� ������ ����� ������� (�������� ��� ����������)
struct RuleElem {
	int num; // �����
	bool cterm; // ��� ���������: �������� �� �� �����������
	bool term;  // true => ��������, false => ����������
	bool save;  // ������� �� ������ ������� ��������� � ������ �������; �� ��������� save = !cterm, ��������� ������ ������������� �������� ����� ����� ���������
};
struct GrammarState;
typedef function<void(GrammarState *g, ParseNode&)> SemanticAction;

struct CFGRule {
	int A; // ���������� � ����� ����� �������
	vector<RuleElem> rhs; // ������ ����� ������� 
	int used;
	SemanticAction action;
	int ext_id = -1;
	int lpr = -1, rpr = -1; // ����� � ������ ���������� �������; � ��������� �������� �������� ���, � ���������� -- ������ ������, � � ����������� -- ������ �����
};

template<class T>
struct dvector {
	vector<T> v;
	int mx = -1;
	const T& operator[](size_t i)const {
		return v[i];
	}
	T& operator[](size_t i) {
		mx = max(mx, (int)i);
		if (i >= v.size())v.resize(i + 1);
		return v[i];
	}
	int size()const { return (int)v.size(); }
	void clear() {
		for (int i = 0; i <= mx; i++)v[i].clear();
		mx = -1;
	}
};

void setDebug(int b);
struct GrammarState {
	unordered_map<int, NTSet> tFirstMap;   // �� ��������� ����������, ����� ����������� ����� ���������� � ������� ���������
	vector<vector<NTTreeNode*>> ntRules;   // ������� ����������� �������������� ������ ��������� ������, ��������������� �������� ��� ������� �����������
	NTTreeNode root;        // �������� ������� ������ ������
	Enumerator<string, unordered_map> nts; // ��������� ������������
	Enumerator<string, unordered_map> ts;  // ��������� ����������
	vector<CFGRule> rules;
	struct Temp {
		struct BElem {
			int i;
			const NTTreeNode* v;
			NTSet M;
		};
		struct PV {
			const NTTreeNode* v;
			int A;
			int B; // reduction: B -> A -> rule
		};
		dvector<vector<BElem>> B;
		dvector<NTSet> F;
		vector<PV> path;
		void clear() { 
			B.clear(); 
			F.clear(); 
			path.clear();
		}
	};
	Temp tmp;
	int start = -1;
	bool finish = false;
	TF tf;
	PEGLexer lex;
	//int syntaxDefNT=0;
	//int tokenNT=0;
	vector<pair<Pos, string>> _err;
	void error(const string &err);

	void addLexerRule(const string& term, const string& re, bool tok=false, bool to_begin = false);
	void addToken(const string& term, const string& re) { addLexerRule(term, re, true); }
	bool addRule(const string &lhs, const vector<vector<string>> &rhs, SemanticAction act = SemanticAction(), int id = -1, unsigned lpr = -1, unsigned rpr = -1);
	bool addRule(const string &lhs, const vector<vector<string>> &rhs, int id, unsigned lpr = -1, unsigned rpr = -1) {
		return addRule(lhs, rhs, SemanticAction(), id, lpr, rpr);
	}
	bool addRuleAssoc(const string &lhs, const vector<vector<string>> &rhs, int id, unsigned pr, int assoc = 0) { // assoc>0 ==> left-to-right, assoc<0 ==> right-to-left, assoc=0 ==> not assotiative or unary
		return addRule(lhs, rhs, SemanticAction(), id, pr * 2 + assoc > 0, pr * 2 + assoc < 0);
	}
	bool addRule(const string &lhs, const vector<string> &rhs, SemanticAction act = SemanticAction(), int id = -1, unsigned lpr = -1, unsigned rpr = -1) {
		vector<vector<string>> vrhs(rhs.size());
		for (int i = 0; i < (int)rhs.size(); i++)
			vrhs[i] = { rhs[i] };
		return addRule(lhs, vrhs, act, id, lpr, rpr);
	}
	bool addRule(const string &lhs, const vector<string> &rhs, int id, unsigned lpr, unsigned rpr) {
		return addRule(lhs, rhs, SemanticAction(), id, lpr, rpr);
	}
	bool addRuleAssoc(const string &lhs, const vector<string> &rhs, int id, unsigned pr, int assoc = 0) { // assoc>0 ==> left-to-right, assoc<0 ==> right-to-left, assoc=0 ==> not assotiative or unary
		return addRule(lhs, rhs, SemanticAction(), id, pr*2+assoc>0, pr*2+assoc<0);
	}
	bool addRule(const string &lhs, const vector<string> &rhs, int id) {
		return addRule(lhs, rhs, SemanticAction(), id);
	}
	bool addRule(const CFGRule &r);

	bool addLexerRule(const ParseNode *tokenDef, bool tok, bool to_begin=false);
	//bool addRule(const ParseNode *ruleDef);
	GrammarState() {
		ts[""];  // ����������� ������� ����� ���������, ����� ��� ��������� ����� ��������� �����.
		nts[""]; // ����������� ������� ����� �����������, ����� ��� ����������� ����� ��������� �����.
	}
	ParseNode* reduce(ParseNode **pn, const NTTreeNode *v, int nt, int nt1, ParseTree &pt) {
		int r = v->rule(nt), sz = len(rules[r].rhs);
		ParseNode* res = pt.newnode();
		res->rule = r;
		res->B = nt;
		res->nt = nt1;
		res->rule_id = rules[r].ext_id;
		int szp = 0;
		for (int i = 0; i < sz; i++)
			szp += rules[r].rhs[i].save;
		res->ch.resize(szp);
		res->lpr = rules[r].lpr;
		res->rpr = rules[r].rpr;
		res->loc.beg = pn[0]->loc.beg;
		res->loc.end = pn[sz - 1]->loc.end;
		for (int i = 0, j = 0; i < sz; i++)
			if (rules[r].rhs[i].save)
				res->ch[j++] = pn[i];// ? pn[i] : termnode(;
			else if(pn[i])pt.del(pn[i]);
		//res->loc.beg = res->ch[0].
		res->updateSize();
		if (rules[r].action) // ��� ����� � ����������� � ������� ������ �� ����������� ������������� �������� (������ ����������� ��� ���������� ������)
			rules[r].action(this, *res);
		return res->balancePr();
	}
	void setStart(const string&start){ //, const string& token, const string &syntax) {
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
		//if(!token.empty())
		//	tokenNT = nts[token];
		//if (!syntax.empty())
		//	syntaxDefNT = nts[syntax];
	}
	void setWsToken(const string& ws) {
		lex.setWsToken(ws);
	}
	void setIndentToken(const std::string &nm) {
		lex.declareIndentToken(nm, ts[nm]);
	}
	void setDedentToken(const std::string &nm) {
		lex.declareDedentToken(nm, ts[nm]);
	}
	void setCheckIndentToken(const std::string &nm) {
		lex.declareCheckIndentToken(nm, ts[nm]);
	}
	void setEOLToken(const std::string &nm) {
		lex.declareEOLToken(nm, ts[nm]);
	}
	void setEOFToken(const std::string &nm) {
		lex.declareEOFToken(nm, ts[nm]);
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

struct LAInfo {
	NTSet t, nt;
	LAInfo& operator |=(const LAInfo &i) { 
		t |= i.t;   nt |= i.nt;
		return *this;
	}
};
typedef PosHash<int, LAInfo> LAMap;

struct RulePos {
	mutable const NTTreeNode* sh = 0;
	const NTTreeNode *v = 0; // ������� ������� � ������ ������
	NTSet M;                 // ��������� ������������ � ����� ������
};

struct LR0State {
	VectorF<RulePos,4> v;
	LAMap la; // ���������� � ������������� ��� ������ �� ������������ �� ������� ������: �� ����������� A ����������, ����� ������� ����� ���� ����� ������ �� A
};


struct SStack {
	vector<LR0State> s;
};

struct PStack {
	vector<ParseNode*> s;
};

ParseTree parse(GrammarState & g, const std::string& text);

struct ParserError {
	Location loc;
	string err;
};
