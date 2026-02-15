
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
#include "helper.h"

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

////////////////////////////////////////////////////////////////////////
///                            MAIN                                  ///
////////////////////////////////////////////////////////////////////////
int main(int argc, char * argv[]) {
	string varsFile, benchmarkName;
	map<string, int> name2IdF;
	map<int, string> id2NameF;
	vector<string> varOrder;

	parseOptionsOrdering(argc, argv);
	benchmarkName = options.benchmark;
	varsFile      = options.varsOrder;

	Abc_Ntk_t* FNtk = getNtk(benchmarkName,true);
	if (FNtk == NULL) {
		FNtk = getNtk(benchmarkName,false);
	}
	if (FNtk == NULL) {
		cerr << "Error: could not read benchmark " << benchmarkName << endl;
		return 1;
	}
	Aig_Man_t* FAig = Abc_NtkToDar(FNtk, 0, 0);
	if (FAig == NULL) {
		cerr << "Error: could not create AIG from " << benchmarkName << endl;
		return 1;
	}

	populateVars(FNtk, varsFile, varOrder,
					varsXF, varsYF,
					name2IdF, id2NameF);
	if (numY <= 0) {
		cerr << "Error: no Y variables found in " << varsFile << endl;
		return 1;
	}

	auto rankAll =  calculateLeastOccurence(FAig);

	vector<pair<int, string> > rankPair;
	for(int i = 0; i < numY; i++)
		rankPair.push_back(make_pair(rankAll[i],varOrder[i]));

	sort(rankPair.begin(), rankPair.end());

	for(auto it: rankPair)
		cout << it.second << endl;

	// Stop ABC
	Abc_Stop();
	return 0;
}
