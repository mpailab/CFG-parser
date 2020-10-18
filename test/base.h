#pragma once
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include "Parser.h"
using namespace std;

bool addRule(GrammarState& gr, const string& s, SemanticAction act = SemanticAction(), int id = -1);
bool addRule(GrammarState& gr, const string& s, int id);
string loadfile(const string& fn);

std::string read_whole_file(const std::string& fn);

#include <chrono>  // for high_resolution_clock

struct Timer {
	decltype(std::chrono::high_resolution_clock::now()) _t0;
	bool _started = false;
	string name;
	Timer(string nm = "") :name(nm) {}
	void start() {
		_t0 = std::chrono::high_resolution_clock::now();
		_started = true;
	}
	double stop() {
		_started = false;
		auto _t1 = std::chrono::high_resolution_clock::now();
		return chrono::duration<double>(_t1 - _t0).count();
	}
	double stop_pr() {
		double t = stop();
		if (name.empty())cout << "Time: " << t << " s\n";
		else cout << name << " time: " << t << " s\n";
		return t;
	}
	~Timer() {
		if (_started && name.size()) stop_pr();
	}
};

enum SynType {
	Or = 1,
	Maybe,
	Concat,
	Plus,
	SynTypeLast=Plus
};
template<class T>
vector<T>& operator +=(vector<T>& x, const vector<T>& y) {
	x.insert(x.end(), y.begin(), y.end());
	return x;
}
template<class T>
vector<T> operator +(vector<T> x, const vector<T>& y) {
	return x += y;
}
vector<vector<vector<string>>> getVariants(ParseNode* n);

void init_base_grammar(GrammarState& st);
