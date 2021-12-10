#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>

using namespace std;

const int sample_count = 5;

vector<int> countnameslong(const set<string>& storage, const vector<string>& candidates) {
	vector<int> output;
	for (auto& name : candidates) {
		output.push_back(count(storage.begin(), storage.end(), name));
	}
	return output;
}

vector<int> countnames(const set<string>& storage, const vector<string>& candidates) {
	vector<int> ret;
	for (const auto i : candidates) {
		if (storage.count(i)) {
			ret.push_back(1);
		}
		else {
			ret.push_back(0);
		}
	}
	return ret;
}

int main() {
	set<string> s;
	vector<string> v;
	string stra = ""s;
	string strb = ""s;

	for (int j = 0; j < 10000; ++j) {
		s.insert(stra);
		stra += "a"s;
		if (j % 2 == 0) {
			v.push_back(strb);
			strb += "b"s;
		}
		else {
			v.push_back(stra);
		}
	}
	cout << "Testing slow version" << endl;
	for (int i = 0; i < SAMPLE_COUNT; ++i) {
		auto begin = chrono::steady_clock::now();
		CountNames(s, v);
		auto end = chrono::steady_clock::now();
		cout << "Time difference Long = "s << chrono::duration_cast<chrono::microseconds>(end - begin).count() << "[µs]"s << endl;
	}
}