#include "helper.h"
#include "readCnfLib.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

////////////////////////////////////////////////////////////////////////
///                           GLOBALS                                ///
////////////////////////////////////////////////////////////////////////
vector<int> varsSInv;
vector<int> varsXF, varsXS;
vector<int> varsYF, varsYS; // to be eliminated
int numOrigInputs = 0, numX = 0, numY = 0;
vector<string> varsNameX, varsNameY;
Abc_Frame_t* pAbc = NULL;
sat_solver* m_pSat = NULL;
Cnf_Dat_t* m_FCnf = NULL;
lit m_f = 0;
double sat_solving_time = 0;
double verify_sat_solving_time = 0;
double reverse_sub_time = 0;
chrono_steady_time helper_time_measure_start = TIME_NOW;
chrono_steady_time main_time_start = TIME_NOW;

static int trailingNumber(const string& s) {
	int i = static_cast<int>(s.size()) - 1;
	while (i >= 0 && isdigit(static_cast<unsigned char>(s[i])))
		--i;
	if (i == static_cast<int>(s.size()) - 1)
		return -1;
	return stoi(s.substr(i + 1));
}

static void writeVarDetails(const string& outFile,
		const vector<int>& posUnates,
		const vector<int>& negUnates) {
	ofstream out(outFile);
	if (!out.is_open()) {
		cerr << "Could not write " << outFile << endl;
		return;
	}

	out << "Posunate:";
	for (auto v : posUnates)
		out << " " << v;
	out << "\n";

	out << "Negunate:";
	for (auto v : negUnates)
		out << " " << v;
	out << "\n";
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		cerr << "Usage: unate <file.qdimacs>" << endl;
		return 1;
	}

	const string qdFileName = argv[1];

	string baseFileName = qdFileName;
	size_t slash = baseFileName.find_last_of("/");
	if (slash != string::npos)
		baseFileName = baseFileName.substr(slash + 1);
	size_t qpos = baseFileName.find(".qdimacs");
	if (qpos == string::npos) {
		cerr << "Input must have .qdimacs extension" << endl;
		return 1;
	}
	baseFileName.erase(qpos);

	const string verilogFile = baseFileName + ".v";
	const string varsFile = baseFileName + "_var.txt";
	const string varsOrderFile = baseFileName + "_varstoelim.txt";

	// Step 1: readCnf (qdimacs -> verilog + vars)
	if (runReadCnf(qdFileName) != 0) {
		cerr << "readCnf failed" << endl;
		return 1;
	}

	// Step 3: unate checks via verilog
	Abc_Ntk_t* FNtk = getNtk(verilogFile, true);
	if (FNtk == NULL)
		FNtk = getNtk(verilogFile, false);
	if (FNtk == NULL) {
		cerr << "Could not read verilog file: " << verilogFile << endl;
		return 1;
	}
	Aig_Man_t* FAig = Abc_NtkToDar(FNtk, 0, 0);

	varsXF.clear();
	varsYF.clear();
	map<string,int> name2IdF;
	map<int,string> id2NameF;
	vector<string> varOrder;
	populateVars(FNtk, varsFile, varOrder,
				 varsXF, varsYF,
				 name2IdF, id2NameF);

	numX = static_cast<int>(varsXF.size());
	numY = static_cast<int>(varsYF.size());
	numOrigInputs = numX + numY;

	auto rankAll = calculateLeastOccurence(FAig);
	vector<pair<int, string> > rankPair;
	for (int i = 0; i < numY; i++)
		rankPair.push_back(make_pair(rankAll[i], varOrder[i]));
	sort(rankPair.begin(), rankPair.end());

	vector<string> orderedVarNames;
	for (auto &it : rankPair)
		orderedVarNames.push_back(it.second);

	vector<int> varsYFOrdered;
	varsYFOrdered.reserve(numY);
	for (auto &name : orderedVarNames) {
		auto it = name2IdF.find(name);
		if (it != name2IdF.end())
			varsYFOrdered.push_back(it->second);
	}
	varsYF = varsYFOrdered;
	numY = static_cast<int>(varsYF.size());
	numOrigInputs = numX + numY;

	// write varstoelim file to preserve pipeline artifacts
	{
		ofstream out(varsOrderFile);
		for (auto &name : orderedVarNames)
			out << name << "\n";
	}

	options.noSyntacticUnate = false;
	options.noSemanticUnate = false;
	options.unateTimeout = UNATE_TIMEOUT;
	main_time_start = TIME_NOW;

	vector<int> unate(numY, -1);
	int n, numSynUnates = 0;
	while ((n = checkUnateSyntacticAll(FAig, unate)) > 0) {
		substituteUnates(FAig, unate);
		numSynUnates += n;
	}
	int numSemUnates = checkUnateSemAll(FAig, unate);
	substituteUnates(FAig, unate);

	(void)numSynUnates;
	(void)numSemUnates;

	vector<int> posUnates;
	vector<int> negUnates;
	for (int i = 0; i < numY; ++i) {
		int varNum = trailingNumber(id2NameF[varsYF[i]]);
		if (varNum < 0)
			continue;
		if (unate[i] == 1)
			posUnates.push_back(varNum);
		else if (unate[i] == 0)
			negUnates.push_back(varNum);
	}

	string outFile = qdFileName + "_vardetails";
	writeVarDetails(outFile, posUnates, negUnates);
	cout << "Wrote " << outFile << endl;

	return 0;
}
