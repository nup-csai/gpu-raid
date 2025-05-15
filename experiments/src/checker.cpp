#include <chrono>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <vector>

using namespace std;


int main() {

	constexpr int size = 26000000;

	ifstream f1("data/vector1.dat"), f2("data/vector2.dat");
	
	vector<float> A(size), B(size), C(size);
	for (int i = 0; i < size; ++i) {
		f1 >> A[i];
	}
	for (int i = 0; i < size; ++i) {
		f2 >> B[i];
	}

	auto start = chrono::high_resolution_clock::now();
	for (int i = 0; i < size; ++i) {
		C[i] = A[i] + B[i];
	}

	auto end = chrono::high_resolution_clock::now();
	chrono::duration<double> duration = end - start;
	cerr << "EXECUTED IN " << duration.count() << " SECONDS" << endl;

	return 0;

	//for (int i = 0; i < size; ++i) {
	//	float a, b, c;
	//	f1 >> a;
	//	f2 >> b;
	//	f3 >> c;
	//	if (fabsl(c - a - b) > 1e-4) {
	//		cerr << "ERROR (LINE " + to_string(i + 1) + "): EXPECTED " + to_string(a + b) + "; FOUND " + to_string(c) << endl;
	//		return 1;
	//	}
	//}
	//cerr << "NO ERRORS FOUND" << endl;


}
