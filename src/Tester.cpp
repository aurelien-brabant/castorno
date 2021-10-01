#include <iostream>
#include <iomanip>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdexcept>
#include "castorno/Tester.hpp"
#include "castorno/Timer.hpp"

using std::cout;
using std::string;
using std::setw;

Tester::Tester(const std::string& progName): _progName(progName)
{
	if (mkdir("./.castorno", 0777) && errno != EEXIST) {
		throw std::runtime_error("Could not create log directory");
	}
}

Tester::Tester(const Tester & rhs) { *this = rhs; } Tester & Tester::operator=(const Tester & rhs) {

	if (this == &rhs) {
	}

	return *this;
}

string Tester::_consumeError(void)
{
	string s(error.str());

	error.str("");
	error.clear();

	return s;
}

void Tester::registerTest(string const & suiteName, const string & testName, TestFn fn)
{
	TestSuiteMap::iterator it = _testSuites.find(suiteName);

	if (it == _testSuites.end()) {
		std::vector<Test> vfn;

		vfn.push_back(Test(testName, fn));
		TestSuiteMap::value_type p(suiteName, vfn);
		_testSuites.insert(p);
	} else {
		it->second.push_back(Test(testName, fn));
	}
}

size_t Tester::_getMaxTestNameLength(void)
{
	size_t greatestLength = 0;

	for (TestSuiteMap::const_iterator cit = _testSuites.begin(); cit != _testSuites.end(); ++cit) {
		for (std::vector<Test>::const_iterator citb = cit->second.begin(); citb != cit->second.end(); ++citb) {
			if (citb->name.size() > greatestLength) {
				greatestLength = citb->name.size();
			}
		}
	}

	return greatestLength;
}

void Tester::runAllSuites(void)
{
	size_t suitePassedN = 0;
	std::ofstream timeDumpOfs((std::string("./.castorno/") + _progName + ".time.txt").c_str());
	std::ofstream testListOfs("./.castorno/tests.txt");
	const size_t offset = _getMaxTestNameLength() + 1;

	if (!timeDumpOfs) {
		throw std::runtime_error("Could not create time dump file");
	}

	for (TestSuiteMap::const_iterator cit = _testSuites.begin(); cit != _testSuites.end(); ++cit) {
		cout << "\033[1;30m>>>>>>>>>> \033[0;36m" << cit->first << " \033[1;30m<<<<<<<<<<\033[0m\n\n";

		/* For each test */
		size_t passedN = 0;
		for (std::vector<Test>::const_iterator vcit = cit->second.begin(); vcit != cit->second.end(); ++vcit) {
			testListOfs << vcit->name << "\n";

			Timer testTimer;

			testTimer.start();
			int ret = vcit->fn(*this);
			testTimer.finish();

			cout << "[" << (ret ? "\033[0;31mX" : "\033[0;32m✔") << "\033[0m] " <<
				std::left << setw(offset + 17) << (string("\033[0;34m") + vcit->name + "\033[0m");

			passedN += (ret == 0);

			double elapsed = testTimer.getElapsed();
			cout << "\033[1;35m" << setw(8) << elapsed << "\033[0m ms";
			timeDumpOfs << elapsed << "\n";

			if (ret && !error.str().empty()) {
				cout << " (\033[0;31m" << _consumeError() << "\033[0m)";
			}

			cout << "\n";
		}

		if (passedN == cit->second.size()) {
			++suitePassedN;
		}

		cout << "\n\033[1;33mSUITE SUMMARY: \033[0m" << passedN << "/" << cit->second.size() << " passed.\n\n";
	}

	cout << "All tests suites have been processed! " << suitePassedN << "/" << _testSuites.size() << " were successful.\n";
}