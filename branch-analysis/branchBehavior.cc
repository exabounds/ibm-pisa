/*******************************************************************************
 * (C) Copyright IBM Corporation 2017
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Algorithms & Machines team
 *******************************************************************************/

#include<stdlib.h>
#include<iostream>
#include<fstream>
#include<map>
#include<cmath>
#include<vector>
#include<queue>
#include<algorithm>
#include<cassert>

using namespace std;

typedef enum {
    BASIC = 0,
    SEEN_ONCE_05,
    SEEN_FIRST_05,
    SEEN_ONCE_DISCARDED,
    SEEN_FIRST_DISCARDED
} PredictionRateType;

/* Branch class */
class Branch {
public:
    unsigned int func_id;
    unsigned int bb_id;
    unsigned int instr_id;
    unsigned int cond_id;

    Branch(unsigned int f, unsigned int b, unsigned int i, unsigned int c);
    Branch();
    virtual ~Branch();
    bool operator< (const Branch& b) const;
};

ostream& operator<<(ostream& out, const Branch& b) {
    out << "(" << b.func_id << "," << b.bb_id << "," << b.instr_id << "," << b.cond_id << ")";
    return out;
}

istream& operator>>(istream& in, Branch& b) {
    in >> b.func_id >> b.bb_id >> b.instr_id >> b.cond_id;
    return in;
}

Branch::Branch(unsigned int f, unsigned int b, unsigned int i, unsigned int c)
: func_id(f), bb_id(b), instr_id(i), cond_id(c) {}

Branch::Branch()
:func_id(0), bb_id(0), instr_id(0), cond_id(0) {}

Branch::~Branch(){}

bool Branch::operator< (const Branch& b) const {
    if (func_id < b.func_id) return true;
    else if (func_id > b.func_id) return false;

    if (bb_id < b.bb_id) return true;
    else if (bb_id > b.bb_id) return false;

    if (instr_id < b.instr_id) return true;
    else if (instr_id > b.instr_id) return false;

    if (cond_id < b.cond_id) return true;
    else if (cond_id > b.cond_id) return false;

    return false;
}

/* Build a table with discrete values for the misprediction rates */
void buildInverseEntropyTable(map<double,double>& table, unsigned int maxi=10000) {
    table[0] = 0;
    for (unsigned int i=1;i<maxi;++i) {
        double p = (1.0*i)/maxi/2;
        double entropy = -(p*log2(p)+(1-p)*log2(1-p));
        table[entropy] = p;
    }
    table[1] = 0.5;
}

/* Given a branch entropy value, calculate the misprediction rate */
double inverseEntropy(const map<double,double>& table, double entropy) {
    if (!(entropy >= 0 && entropy <= 1)) return -1;
    
    auto itlow = table.lower_bound(entropy);
    // auto ithigh = table.lower_bound(entropy);
    // if (itlow == table.end()) 
    //    return 0.5;
    assert(itlow != table.end());
    return itlow->second;
}

/* Branch history pattern class */
class HistoryPattern {
public:
    unsigned long long count0;
    unsigned long long count1;
    
    HistoryPattern(unsigned long long count0, unsigned long long count1):count0(count0), count1(count1) {}
    bool operator<(const HistoryPattern& other) const {return count0+count1>other.count0+other.count1;}
    
    double max() const {
        bool firstOption = false;
        if (count0+count1 == 0) return 0;
        if (firstOption) {
            if (count0+count1 == 1) return 0.5;
            else if (count0 > count1) return count0;
            else return count1;
        } else {
            if (count0 > count1) return count0-0.5;
            else return count1-0.5;
        }
    }
};

/* 
    Used in the max outcome branch behavior method to estimate predictions 
*/
class PredictionRateComputer {
public:
    PredictionRateComputer() {}
    virtual ~PredictionRateComputer() {}
    
    void registerPattern(const unsigned long long count0, const unsigned long long count1) {
        unsigned long long mx = (count0 > count1 ? count0 : count1);
        unsigned long long tt = count0 + count1;
        
        if (tt == 0) return;
        
        count[BASIC] += mx;
        total[BASIC] += tt;
        
        count[SEEN_ONCE_05] += (mx == 1 ? 0.5 : mx);
        total[SEEN_ONCE_05] += tt;
        
        count[SEEN_FIRST_05] += mx + 0.5 - ((double)mx)/tt;
        total[SEEN_FIRST_05] += tt;
        
        count[SEEN_ONCE_DISCARDED] += (mx == 1 ? 0 : mx);
        total[SEEN_ONCE_DISCARDED] += (mx == 1 ? 0 : tt);
        
        count[SEEN_FIRST_DISCARDED] += ((double)mx)*(tt-1)/tt;
        total[SEEN_FIRST_DISCARDED] += tt - 1;
    }
    
    double predictionRate(const PredictionRateType& predictionRateType) {
        if (total[predictionRateType] == 0) return -1;
        return count[predictionRateType] / total[predictionRateType];
    }  
private:
    map<PredictionRateType, double> total, count;
};

/* 
   Used in the max outcome branch behavior method to store predictions for branch 
   pattern tables of limited sizes. 
*/
class LimitedPredictionRate {
public:
    unsigned long long nEntries;
    double predictionRate;
    double fractionStored;
    LimitedPredictionRate(unsigned long long nEntries, double predictionRate, double fractionStored)
    : nEntries(nEntries), predictionRate(predictionRate), fractionStored(fractionStored) {}
};

/*
    Data structure for storing all the results for branch entropy and 
    max-outcome metrics.
*/
typedef struct {
    unsigned int windowSize;
    double entropy;
    double difference;
    double predictionRate_BASIC;
    double predictionRate_SEEN_ONCE_05;
    double predictionRate_SEEN_FIRST_05;
    double predictionRate_SEEN_ONCE_DISCARDED;
    double predictionRate_SEEN_FIRST_DISCARDED;
    vector<LimitedPredictionRate> limitedPredictionRates;
    vector<double> limitedEntropies;
    unsigned long long nHistoryPatterns;
    unsigned long long nBranches0;
    unsigned long long nBranches1;
    unsigned long long nBranchesMore;
} WindowInfo;

double singleEntropyTerm(unsigned long long count, unsigned long long total) {
    double p = ((double)count) / total;
    return -p*log(p)/log(2);
}

/*
    Computes all the necessary metrics by traversing the trace using a 
    sliding window of size equal to the history pattern size (window size).
*/
void computePredictionRate(const vector<char>& branchResults, 
                           vector<WindowInfo>& result, 
                           const unsigned int historyBufferSizeMax, 
                           vector<unsigned long long>& nTableEntries, 
                           map<double,double> table) {
                               
    unsigned long long nEvents = branchResults.size();
    double old_entropy = 0;
    map<unsigned long long,unsigned long long> twoBranchPatterns[2];

    result.resize(historyBufferSizeMax);

    sort(nTableEntries.begin(), nTableEntries.end());
    const unsigned long long maxSize = nTableEntries.back();
    
    bool useSlidingWindow = true;
    bool optimizeKeyComputation = true;
    
    for(unsigned int windowSize = 1; windowSize <= historyBufferSizeMax+1; windowSize++) {
        if (windowSize > 1) cerr << "\tProcessing for window size " << (windowSize-1) << " ";
        map<unsigned long long,unsigned long long>& branchPatterns = twoBranchPatterns[windowSize%2];
        map<unsigned long long,unsigned long long>& prevBranchPatterns = twoBranchPatterns[1-(windowSize%2)];
        
        unsigned long long count = 0;
        unsigned int increment = (useSlidingWindow?1:windowSize);
        branchPatterns.clear();

        unsigned long long key = 0;
        unsigned long long mask = ~(((unsigned long long)1) << (windowSize-1));
        int nPointsShown = 0;
        int nPointsTotal = 20;
        
        for(unsigned long long windowStart = 0; windowStart < nEvents-windowSize+1; windowStart +=increment) {
            if (windowSize > 1) {
                while ((windowStart+1)*nPointsTotal/(nEvents-windowSize+1) > nPointsShown) {
                    cerr << ".";
                    ++nPointsShown;
                    if (nPointsShown == nPointsTotal) cerr << " DONE " << endl;
                }
            }
            if (windowStart == 0 || !optimizeKeyComputation || !useSlidingWindow) {
                key = 0;
                for(unsigned int withinWindowIndex = 0; withinWindowIndex < windowSize; withinWindowIndex++) {
                    key += ((unsigned long long)(branchResults[windowStart+withinWindowIndex]-'0')) << (windowSize-withinWindowIndex-1);
                }
            } else {
                // unsigned long long oldKey = key;
                key = ((key & mask) << 1) + (branchResults[windowStart+windowSize-1]-'0');
                // if (windowSize == 1) {
                //    cout << "DEBUG: windowSize=" << windowSize << " mask=" << mask << " oldKey=" << oldKey 
                //         << " outcome=" << branchResults[windowStart+windowSize-1] << " newKey=" << key << endl;
                // }
            }
            ++branchPatterns[key];
            ++count;
        }
        
        if (windowSize <= historyBufferSizeMax) {
            double entropy = 0;
            // cout << "DEBUG: Computing entropy for windowSize=" << windowSize 
            //      << " count=" << count << " nBranches=" << branchPatterns.size() << endl;
            if (branchPatterns.size() > (((unsigned long long)1) << windowSize)) {
                cerr << "ERROR: Too many patterns (" << branchPatterns.size() 
                     << "). Maximum expected was 2^" << windowSize << "=" 
                     << (((unsigned long long)1) << windowSize) 
                     << "." << endl;
                exit(-1);
            }
            for (auto it=branchPatterns.begin(); it!=branchPatterns.end(); ++it) {
                entropy += singleEntropyTerm(it->second, count);
                // cout << "DEBUG: \tvalue=" << it->second << " crtEntropy=" << entropy << endl;
            }
            
            /*  BEGIN: new form of entropy calculation that takes 
                into acount the limited pattern table size 
            */
            
            vector<unsigned long long> valuesFromMap;
            for (auto it=branchPatterns.begin(); it!=branchPatterns.end(); ++it) {
                valuesFromMap.push_back(it->second);
            }
            sort(valuesFromMap.begin(),valuesFromMap.end());
            unsigned long long sizeValuesFromMap = valuesFromMap.size();
            for (int i=nTableEntries.size()-1;i>=0;--i) {
                double newEntropy = 0.0;
                unsigned long long newCount = 0;
                unsigned long long val = 0;
                for(int j=1; j<nTableEntries[i]; j++) {
                    if (sizeValuesFromMap<j) break; 
                    val = valuesFromMap[sizeValuesFromMap-j];
                    newEntropy += singleEntropyTerm(val, count);
                    newCount += val;
                }
                val = count - newCount;
                if (val>0) {
                    double p = val;
                    p /= count;
                    newEntropy -= p*log(p/((((unsigned long long)1)<<windowSize)-nTableEntries[i]+1))/log(2);
                }
                result[windowSize-1].limitedEntropies.push_back(newEntropy);
                double diff = newEntropy;
                if (windowSize > 1) {
                    diff -= result[windowSize-2].limitedEntropies[result[windowSize-1].limitedEntropies.size()-1];
                }
                cout << "DEBUG: i=" << i << " window=" << windowSize << " newEntropy=" << newEntropy << " diff=" << diff << "\n";
            }
            cout << "DEBUG: window=" << windowSize << " entropy=" << entropy << " diff=" << (entropy - old_entropy) << "\n";
        
            /*  END: new form of entropy calculation that takes 
                into acount the limited pattern table size 
            */
            
            result[windowSize-1].windowSize = windowSize;
            result[windowSize-1].entropy = entropy;
            result[windowSize-1].difference = entropy - old_entropy;

            // corner case: when branch entropy becomes negative we set it to 0
            if (result[windowSize-1].difference < 0) result[windowSize-1].difference = 0;
            old_entropy = entropy;
        }
        
        PredictionRateComputer prc;
        unsigned long long countPred = 0;
        double aggCount0 = 0;
        double aggCount1 = 0;
        
        unsigned long long nBranches0=0, nBranches1=0, nBranchesMore=0;

        if (windowSize > 1) {
            priority_queue<HistoryPattern> q;
            for (auto kv:prevBranchPatterns) {
                unsigned long long key0 = kv.first << 1;
                unsigned long long key1 = key0 + 1;
                unsigned long long count0 = 0;
                if (branchPatterns.find(key0) != branchPatterns.end()) count0=branchPatterns[key0];
                unsigned long long count1 = 0;
                if (branchPatterns.find(key1) != branchPatterns.end()) count1=branchPatterns[key1];
                
                HistoryPattern hp(count0, count1);
                prc.registerPattern(count0, count1);
                countPred += (count0+count1);
                
                if (count0+count1 == 0) ++nBranches0;
                else if (count0+count1 == 1) ++nBranches1;
                else ++nBranchesMore;
                
                q.push(hp);
                if (q.size() > maxSize) {
                    hp = q.top(); q.pop();
                    aggCount0 += hp.count0;
                    aggCount1 += hp.count1;
                }
                /*
                cout << "WS=" << windowSize << " key=" << kv.first
                     << " key0=" << key0 << " count0=" << count0
                     << " key1=" << key1 << " count1=" << count1
                     << " predictionRate=" << predictionRate << " countPred=" << countPred
                     << endl;
                */
            }
            
            double limitedPredictionRate = 0;
            priority_queue<HistoryPattern> q2;
            while (!q.empty()) {
                HistoryPattern hp = q.top(); q.pop(); q2.push(hp);
                limitedPredictionRate += hp.max();
            }
            for (int i=nTableEntries.size()-1;i>=0;--i) {
                while (q2.size() > nTableEntries[i]) {
                    HistoryPattern hp = q2.top(); q2.pop();
                    aggCount0 += hp.count0;
                    aggCount1 += hp.count1;
                    limitedPredictionRate -= hp.max();
                }
                HistoryPattern agghp(aggCount0,aggCount1);
                result[windowSize-2].limitedPredictionRates.push_back(
                    LimitedPredictionRate(nTableEntries[i],
                                         (limitedPredictionRate+agghp.max())/countPred,
                                         1-1.0*(aggCount0+aggCount1)/countPred));
                // cerr << "windowSize=" << (windowSize-1) << " aggCount=" << (aggCount0+aggCount1) << " total=" << countPred << " frac=" << result[windowSize-3].limitedPredictionRates.back().fractionStored << endl;
            }
            result[windowSize-2].predictionRate_BASIC = prc.predictionRate(BASIC);
            result[windowSize-2].predictionRate_SEEN_ONCE_05 = prc.predictionRate(SEEN_ONCE_05);
            result[windowSize-2].predictionRate_SEEN_FIRST_05 = prc.predictionRate(SEEN_FIRST_05);
            result[windowSize-2].predictionRate_SEEN_ONCE_DISCARDED = prc.predictionRate(SEEN_ONCE_DISCARDED);
            result[windowSize-2].predictionRate_SEEN_FIRST_DISCARDED = prc.predictionRate(SEEN_FIRST_DISCARDED);
            result[windowSize-2].nHistoryPatterns = prevBranchPatterns.size();
            result[windowSize-2].nBranches0 = nBranches0;
            result[windowSize-2].nBranches1 = nBranches1;
            result[windowSize-2].nBranchesMore = nBranchesMore;
        }
    }
}

int main(int argc, char*argv[]) {
    unsigned long long nEvents          = 0;
    unsigned int historyBufferSizeMax   = 0;

    ifstream ifp;

    if (argc < 4) {
        cerr << "Program usage: ./entropy <input file> <history buffer length> <global/local 0/1>\n";
        return -1;
    }
    
    bool isGlobal = true;
    if (atoi(argv[3]) == 1) isGlobal = false;
    historyBufferSizeMax = atoi(argv[2]);
    ifp.open(argv[1]);

    if (!ifp.is_open()) {
        cerr << "Can't open input file\n";
        return -1;
    }
    
    /* Change this vector when different table sizes are necessary for analysis */
    vector<unsigned long long> nTableEntries = {8,32,64,256,512,1024,2*1024,4*1024,8*1024,16*1024,32*1024};

    ifp >> nEvents;
    cerr << "Number of conditional branch events is " << nEvents << "\n";

    map<unsigned long long, unsigned long long> branchPatterns;
    vector<char> branchResults(nEvents);
    map<Branch, vector<char> > branches;

    cerr << "Start reading input file..." << "\n";
    for(unsigned long long i = 0; i < nEvents; ++i) {
        Branch tmp;
        char outcome;
        ifp >> tmp.func_id >> tmp.bb_id >> tmp.instr_id >> outcome;
        if (!(outcome == '0' || outcome == '1')) {
            cerr << "ERROR: Found outcome=`" << outcome << "`. Was expecting 0/1." << endl;
            exit(-1);
        }
        if (isGlobal) {tmp.func_id = tmp.bb_id = tmp.instr_id = 0;}
        branches[tmp].push_back(outcome);
    }

    ifp.close();
    cerr << "Reading DONE" << "\n";
    cerr << "Detected a total of " << branches.size() << " branches" << endl;

    map<Branch, vector<WindowInfo> > result;

    // for(auto it = branches.begin(); it != branches.end(); ++it)
    //    cout << it->first << " " << it->second.size() << "\n";
    // cout << "Size branches " << branches.size() << "\n";

    cerr << "Finished processing" << endl;
    map<double,double> table;
    buildInverseEntropyTable(table);
    
    int counter = 0;
    for(auto it = branches.begin(); it != branches.end(); ++it) {
        cerr << "Starting processing for branch " << (++counter) << "/" << branches.size() << endl;
        // cout << "Branch " << it->first << " " << it->second.size() << "\n";
        if (it->second.size() > historyBufferSizeMax) {
            computePredictionRate(it->second, result[it->first], historyBufferSizeMax, nTableEntries, table);
        }
    }

/*
    for(auto it = result.begin(); it != result.end(); ++it) {
        for(unsigned int win = 0; win < historyBufferSizeMax - 1; ++win) 
            cout << it->first.func_id << " " << it->first.bb_id << " " << it->first.instr_id << " "
                 << it->second[win].windowSize << " " << it->second[win].entropy << " " << it->second[win].difference <<"\n";
    }
*/
    
    cout << "windowSize,entropy,difference,approxInverse,inverse,mispredictionRate_BASIC,mispredictionRate_SEEN_ONCE_05,mispredictionRate_SEEN_FIRST_05,mispredictionRate_SEEN_ONCE_DISCARDED,mispredictionRate_SEEN_FIRST_DISCARDED";
    for (int i=nTableEntries.size()-1;i>=0;--i) {
        cout << ",limitedMisprediction" << nTableEntries[i];
        cout << ",limitedFractionNotStored" << nTableEntries[i];
    }
    cout << ",nHistoryPatterns,nBranches0,nBranches1,nBranchesMore" << endl;
    
    for(unsigned int win = 0; win < historyBufferSizeMax; ++win) {
        unsigned long long total = 0;
        double sumEntropy = 0;
        double sumDifference = 0;
        map<PredictionRateType, double> sumPredictionRate;
        sumPredictionRate[BASIC] = 0;
        sumPredictionRate[SEEN_ONCE_05] = 0;
        sumPredictionRate[SEEN_FIRST_05] = 0;
        sumPredictionRate[SEEN_ONCE_DISCARDED] = 0;
        sumPredictionRate[SEEN_FIRST_DISCARDED] = 0;
        double sumInverse = 0;
        double sumPatterns = 0;
        double sumBranches0 = 0;
        double sumBranches1 = 0;
        double sumBranchesMore = 0;
        vector<LimitedPredictionRate> limitedPredictionRates;
        
        for(auto it = result.begin(); it != result.end(); ++it) {
            sumEntropy = sumEntropy + it->second[win].entropy*branches[it->first].size();
            sumDifference = sumDifference + it->second[win].difference*branches[it->first].size();
            sumInverse = sumInverse + inverseEntropy(table,it->second[win].difference)*branches[it->first].size();
            sumPredictionRate[BASIC] += it->second[win].predictionRate_BASIC*branches[it->first].size();
            sumPredictionRate[SEEN_ONCE_05] += it->second[win].predictionRate_SEEN_ONCE_05*branches[it->first].size();
            sumPredictionRate[SEEN_FIRST_05] += it->second[win].predictionRate_SEEN_FIRST_05*branches[it->first].size();
            sumPredictionRate[SEEN_ONCE_DISCARDED] += it->second[win].predictionRate_SEEN_ONCE_DISCARDED*branches[it->first].size();
            sumPredictionRate[SEEN_FIRST_DISCARDED] += it->second[win].predictionRate_SEEN_FIRST_DISCARDED*branches[it->first].size();
            sumPatterns += it->second[win].nHistoryPatterns*branches[it->first].size();
            sumBranches0 += it->second[win].nBranches0*branches[it->first].size();
            sumBranches1 += it->second[win].nBranches1*branches[it->first].size();
            sumBranchesMore += it->second[win].nBranchesMore*branches[it->first].size();
            
            if (limitedPredictionRates.empty()) {
                for (unsigned int i=0;i<it->second[win].limitedPredictionRates.size();++i) {
                    limitedPredictionRates.push_back(LimitedPredictionRate(it->second[win].limitedPredictionRates[i].nEntries,
                        it->second[win].limitedPredictionRates[i].predictionRate * branches[it->first].size(),
                        it->second[win].limitedPredictionRates[i].fractionStored * branches[it->first].size()
                    ));
                }
            } else {
                for (unsigned int i=0;i<limitedPredictionRates.size();++i) {
                    assert(limitedPredictionRates[i].nEntries == it->second[win].limitedPredictionRates[i].nEntries);
                    limitedPredictionRates[i].predictionRate += it->second[win].limitedPredictionRates[i].predictionRate*branches[it->first].size();
                    limitedPredictionRates[i].fractionStored += it->second[win].limitedPredictionRates[i].fractionStored*branches[it->first].size();
                }
            }
            total += branches[it->first].size();
        }

        cout << win + 1 << "," << sumEntropy/total << "," << sumDifference/total
             << "," << inverseEntropy(table,sumDifference/total)
             << "," << sumInverse/total
             << "," << 1 - sumPredictionRate[BASIC]/total
             << "," << 1 - sumPredictionRate[SEEN_ONCE_05]/total
             << "," << 1 - sumPredictionRate[SEEN_FIRST_05]/total
             << "," << 1 - sumPredictionRate[SEEN_ONCE_DISCARDED]/total
             << "," << 1 - sumPredictionRate[SEEN_FIRST_DISCARDED]/total;

        for (unsigned int i=0;i<limitedPredictionRates.size();++i) {
            cout << "," << (1-limitedPredictionRates[i].predictionRate/total);
            cout << "," << (1-limitedPredictionRates[i].fractionStored/total);
        }
        cout << "," << sumPatterns/total
             << "," << sumBranches0/total
             << "," << sumBranches1/total
             << "," << sumBranchesMore/total
             << endl;
    }

    // for (unsigned int index = 0; index < result.size(); ++index)
    //    cout << result[index].windowSize << "," << result[index].entropy << "," << result[index].difference << "\n";

    return 0;
}
