/*
	Authored 2022. Pei Mu.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.

	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.

	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written
		permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#include "newton-irPass-LLVMIR-rangeAnalysis.h"

using namespace llvm;

#define BOOST_NO_EXCEPTIONS
#include <boost/throw_exception.hpp>
void boost::throw_exception(std::exception const&, boost::source_location const&){
//do nothing
}

extern "C"
{

const bool valueRangeDebug = false;

std::pair<double, double>
getGEPArrayRange(State* N, GetElementPtrInst* llvmIrGetElePtrInstruction,
                 std::map<llvm::Value *, std::pair<double, double>> virtualRegisterRange)
{
    /*
     * if it's a constant
     * */
    if (auto * constVar = dyn_cast<llvm::Constant>(llvmIrGetElePtrInstruction->getOperand(0))) {
        auto ptrIndexValue = llvmIrGetElePtrInstruction->getOperand(1);
        int ptrIndex = 0;
        if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(ptrIndexValue)) {
            ptrIndex = constInt->getZExtValue();
        } else {
            assert(false && "gep pointer index is not constant int");
        }
        auto realVar = constVar->getOperand(ptrIndex);
        if (auto *constArr = dyn_cast<ConstantDataArray>(realVar)) {
            auto arrIndexValue = llvmIrGetElePtrInstruction->getOperand(2);
            auto dynRangeRes = [&](std::vector <uint32_t> idxVec) {
                auto arrType = constArr->getElementType();
                double minRes = 0, maxRes = 0;
                if (arrType->isDoubleTy()) {
                    std::vector<double> dbResVec;
                    for (auto idx: idxVec) {
                        dbResVec.emplace_back(constArr->getElementAsDouble(idx));
                    }
                    minRes = *std::min_element(std::begin(dbResVec), std::end(dbResVec));
                    maxRes = *std::max_element(std::begin(dbResVec), std::end(dbResVec));
                } else if (arrType->isFloatTy()) {
                    std::vector<float> ftResVec;
                    for (auto idx: idxVec) {
                        ftResVec.emplace_back(constArr->getElementAsFloat(idx));
                    }
                    minRes = *std::min_element(std::begin(ftResVec), std::end(ftResVec));
                    maxRes = *std::max_element(std::begin(ftResVec), std::end(ftResVec));
                } else if (arrType->isIntegerTy()) {
                    std::vector <uint64_t> intResVec;
                    for (auto idx: idxVec) {
                        intResVec.emplace_back(constArr->getElementAsInteger(idx));
                    }
                    minRes = static_cast<double>(
                            *std::min_element(std::begin(intResVec), std::end(intResVec)));
                    maxRes = static_cast<double>(
                            *std::max_element(std::begin(intResVec), std::end(intResVec)));
                } else if (arrType->isPointerTy()) {
                    assert(!valueRangeDebug && "pointer: implement when meet");
                } else {
                    assert(!valueRangeDebug && "other type: implement when meet");
                }
                return std::make_pair(minRes, maxRes);
            };
            if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(arrIndexValue)) {
                /*
                 * this also should be done in "--instsimplify"
                 * */
                int arrIndex = constInt->getZExtValue();
                auto resVec = dynRangeRes({arrIndex});
                return resVec;
            } else {
                std::vector <uint32_t> dynIdx;
                auto vrRangeIt = virtualRegisterRange.find(arrIndexValue);
                if (vrRangeIt != virtualRegisterRange.end()) {
                    // todo: if we need assert or other check here?
                    uint32_t min = vrRangeIt->second.first < 0 ? 0 : vrRangeIt->second.first;
                    uint32_t max = vrRangeIt->second.second < 0 ? 0 : vrRangeIt->second.second;
                    for (size_t idx = min; idx <= max; idx++) {
                        dynIdx.emplace_back(idx);
                    }
                } else {
                    /*the range is [min(arr_elements), max(arr_elements)]*/
                    for (size_t idx = 0; idx < constArr->getNumElements(); idx++) {
                        dynIdx.emplace_back(idx);
                    }
                }
                auto resVec = dynRangeRes(dynIdx);
                return resVec;
            }
        } else {
            // todo: get range other type of array
            assert(!valueRangeDebug && "implement when meet");
            return std::make_pair(0, 0);
        }
    } else {
        // todo: get range from variable array
        assert(!valueRangeDebug && "implement when meet");
        return std::make_pair(0, 0);
    }
}

/*
 * infer the result range of instruction based on the incoming value of Phi node.
 * e.g.
 *  %x = phi double [%a, %bb0], [5, %bb1], [undef, %bb2]
 *  %y = sub double %x, 10
 * if we know the range of %a, then the range of %x should be [min(min_a, 5), max(max_a, 5)]
 *
 * but it could be more accuracy for IntSet.
 * */
bool checkPhiRange(State * N, PHINode * phiNode, BoundInfo * boundInfo) {
    std::vector<double> minValueVec, maxValueVec;
	std::vector<std::vector<std::pair<double, double>>> minPHIValueVectors, maxPHIValueVectors;
	for (size_t idx = 0; idx < phiNode->getNumIncomingValues(); idx++) {
		auto phiValue = phiNode->getIncomingValue(idx);
			if (isa<llvm::Constant>(phiValue)) {
            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(phiValue)) {
                if (phiValue->getType()->isFloatTy()) {
                    float constValue = constFp->getValueAPF().convertToFloat();
                    minValueVec.emplace_back(constValue);
                    maxValueVec.emplace_back(constValue);
                } else if (phiValue->getType()->isDoubleTy()) {
                    double constValue = constFp->getValueAPF().convertToDouble();
                    minValueVec.emplace_back(constValue);
                    maxValueVec.emplace_back(constValue);
                }
            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(phiValue)) {
                auto constValue = constInt->getSExtValue();
                minValueVec.emplace_back(static_cast<double>(constValue));
                maxValueVec.emplace_back(static_cast<double>(constValue));
            } else if (auto pointerPhi = dyn_cast<GEPOperator>(phiValue)) {
                // todo: get the constant GEP from PhiNode by create a loadInst then remove it.
				auto pointerOperand = pointerPhi->getPointerOperand();
				auto vrRangeIt = boundInfo->virtualRegisterVectorRange.find(pointerOperand);
				if (vrRangeIt != boundInfo->virtualRegisterVectorRange.end())
				{
					minPHIValueVectors.emplace_back(vrRangeIt->second);
					maxPHIValueVectors.emplace_back(vrRangeIt->second);
				}
            } else if (isa<UndefValue>(phiValue) || isa<PoisonValue>(phiValue)) {
                /*do nothing*/
            } else {
                assert(!valueRangeDebug && "implement when meet");
            }
        } else {
            auto vrRangeIt = boundInfo->virtualRegisterRange.find(phiValue);
            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
            {
                minValueVec.emplace_back(vrRangeIt->second.first);
                maxValueVec.emplace_back(vrRangeIt->second.second);
            } else {
                assert(!valueRangeDebug && "failed to get range");
                return false;
            }
        }
    }

	/*
	 * Single Values
	 */
	if (!minValueVec.empty()) {
		assert(!maxValueVec.empty() && "Vector of max values empty!");
		double minRes = *std::min_element(minValueVec.begin(), minValueVec.end());
		double maxRes = *std::max_element(maxValueVec.begin(), maxValueVec.end());
		boundInfo->virtualRegisterRange.emplace(phiNode, std::make_pair(minRes, maxRes));
	}

	/*
	 * Vectors of values (for PHI nodes with GEP instructions), e.g.:
	 *
	 * %.0 = phi double* [ getelementptr inbounds ([5 x double], [5 x double]* @pR2, i64 0, i64 0), %3 ], [ getelementptr inbounds ([5 x double], [5 x double]* @pS2, i64 0, i64 0), %4 ], !dbg !35
	 *
	 */
	if (!minPHIValueVectors.empty()) {

		assert(!maxPHIValueVectors.empty() && "Vector of max PHI values empty!");

		std::vector<double> vectorWithMinValues(minPHIValueVectors.begin()->size(), DBL_MAX);
		std::vector<double> vectorWithMaxValues(minPHIValueVectors.begin()->size(), DBL_MIN);

		/*
		 * Find the min of all phi operands at every location in GEP.
		 */
		for (auto& vec : minPHIValueVectors) {
			auto itA = vec.begin();
			auto itB = vectorWithMinValues.begin();
			for (; (itA != vec.end()) && (itB != vectorWithMinValues.end()); (++itA, ++itB)) {
				*itB = (itA->first < *itB)? (itA->first) : (*itB);
			}
		}
		/*
		 * Find the max of all phi operands at every location in GEP.
		 */
		for (auto& vec : maxPHIValueVectors) {
			auto itA = vec.begin();
			auto itB = vectorWithMaxValues.begin();
			for (; (itA != vec.end()) && (itB != vectorWithMaxValues.end()); (++itA, ++itB)) {
				*itB = (itA->second > *itB)? (itA->second) : (*itB);
			}
		}
//		/*
//		 * Pair the (min, max) for every location in GEP.
//		 */
//		auto itMin = vectorWithMinValues.begin();
//		auto itMax = vectorWithMaxValues.begin();
//		std::vector<std::pair<double, double>> vectorOfPairs;
//		for (; (itMin != vectorWithMinValues.end()) && (itMax != vectorWithMaxValues.end()); (++itMin, ++itMax)) {
//			vectorOfPairs.emplace_back(std::make_pair(*itMin, *itMax));
//		}
        vectorWithMinValues.insert(vectorWithMinValues.end(),
                                   vectorWithMaxValues.begin(),
                                   vectorWithMaxValues.end());
        auto minValue = std::min_element(vectorWithMinValues.begin(), vectorWithMinValues.end());
        auto maxValue = std::max_element(vectorWithMinValues.begin(), vectorWithMinValues.end());
		boundInfo->virtualRegisterRange.emplace(phiNode, std::make_pair(*minValue, *maxValue));
	}

	if (minValueVec.empty() && minPHIValueVectors.empty()) {
		flexprint(N->Fe, N->Fm, N->Fperr, "Error: min value vectors are both empty.");
	}

	return true;
}

/*
 * stand-alone algorithm to get the range of remainder operation
 * */
std::pair<int64_t, int64_t>
modInterval(const int64_t lhs_low, const int64_t lhs_high, const int64_t rhs_low, const int64_t rhs_high);

std::pair<int64_t, int64_t>
modConstDivisorInterval(const int64_t lhs_low, const int64_t lhs_high, const int64_t m)
{
    // empty interval
    if (lhs_low > lhs_high || m == 0)
        return std::make_pair(0, 0);
        // compute modulo with positive interval and negate
    else if (lhs_high < 0)
        return std::make_pair(-modConstDivisorInterval(-lhs_high, -lhs_low, m).second,
                              -modConstDivisorInterval(-lhs_high, -lhs_low, m).first);
        // split into negative and non-negative interval, compute and join
    else if (lhs_low < 0)
    {
        auto negative_part = modConstDivisorInterval(lhs_low, -1, m);
        auto positive_part = modConstDivisorInterval(0, lhs_high, m);
        return std::make_pair(min(negative_part.first, positive_part.first),
                              max(negative_part.second, positive_part.second));
    }
        // there is no k > 0 such that a < k*m <= b
    else if ((lhs_high - lhs_low) < std::abs(m) && (lhs_low % m <= lhs_high % m))
        return std::make_pair(lhs_low % m, lhs_high % m);
    else
        return std::make_pair(0, std::abs(m) - 1);
}

std::pair<int64_t, int64_t>
modConstDividendInterval(const int64_t lhs, const int64_t rhs_low, const int64_t rhs_high)
{
    // empty interval
    if (rhs_low > rhs_high || lhs == 0)
        return std::make_pair(0, 0);
        // compute modulo with positive interval and negate
    else if (lhs < 0)
        return std::make_pair(-modConstDividendInterval(-lhs, rhs_low, rhs_high).second,
                              -modConstDividendInterval(-lhs, rhs_low, rhs_high).first);
        // use only non-negative m and n
    else if (rhs_high <= 0)
        return modConstDividendInterval(lhs, -rhs_high, -rhs_low);
        // split into negative and non-negative interval, compute and join
    else if (rhs_low <= 0)
        return modConstDividendInterval(lhs, 1, max(-rhs_low, rhs_high));
        // modulo has no effect
    else if (rhs_low > lhs)
        return std::make_pair(lhs, lhs);
        // there is some overlapping of [a,b] and [n,m]
    else if (rhs_high > lhs)
        return std::make_pair(0, lhs);
        // max value appears at [a/2] + 1
    else if (lhs / 2 + 1 <= rhs_low)
        return std::make_pair(lhs % rhs_high, lhs % rhs_low);
    else if (lhs / 2 + 1 <= rhs_high) {
        int64_t min = lhs;
        if (rhs_high != lhs) {
            for (int64_t min_idx = rhs_low; min_idx < lhs / 2 + 1; min_idx++) {
                min = lhs % min_idx < min ? lhs % min_idx : min;
            }
        } else {
            min = 0;
        }
        return std::make_pair(min, lhs % (lhs / 2 + 1));
    }
        // either compute all possibilities and join, or be imprecise
    else
    {
        int64_t min = lhs;
        int64_t max = 0;
        for (int64_t min_idx = rhs_low; min_idx <= rhs_high; min_idx++) {
            min = lhs % min_idx < min ? lhs % min_idx : min;
        }
        for (int64_t max_idx = rhs_low; max_idx <= rhs_high; max_idx++) {
            max = lhs % max_idx > max ? lhs % max_idx : max;
        }
        return std::make_pair(min, max);
    }
}

std::pair<int64_t, int64_t>
modInterval(const int64_t lhs_low, const int64_t lhs_high, const int64_t rhs_low, const int64_t rhs_high)
{
    // empty interval
    if (lhs_low > lhs_high || rhs_low > rhs_high)
        return std::make_pair(0, 0);
        // compute modulo with positive interval and negate
    else if (lhs_high < 0)
        return std::make_pair(-modInterval(-lhs_high, -lhs_low, rhs_low, rhs_high).second,
                              -modInterval(-lhs_high, -lhs_low, rhs_low, rhs_high).first);
        // split into negative and non-negative interval, compute, and join
    else if (lhs_low < 0)
    {
        auto negative_part = modInterval(lhs_low, -1, rhs_low, rhs_high);
        auto positive_part = modInterval(0, lhs_high, rhs_low, rhs_high);
        return std::make_pair(min(negative_part.first, positive_part.first),
                              max(negative_part.second, positive_part.second));
    }
        // use the simpler function from before
    else if (lhs_low == lhs_high)
        return modConstDividendInterval(lhs_low, rhs_low, rhs_high);
        // use the simpler function from before
    else if (rhs_low == rhs_high)
        return modConstDivisorInterval(lhs_low, lhs_high, rhs_low);
        // use only non-negative m and n
    else if (rhs_high <= 0)
        return modInterval(lhs_low, lhs_high, -rhs_high, -rhs_low);
        // make modulus non-negative
    else if (rhs_low <= 0)
        return modInterval(lhs_low, lhs_high, 1, max(-rhs_low, rhs_high));
        // check b-a < |modulus|
    else if (lhs_high - lhs_low >= rhs_high)
        return std::make_pair(0, rhs_high - 1);
        // split interval, compute, and join
    else if (lhs_high - lhs_low >= rhs_low)
    {
        auto part = modInterval(lhs_low, lhs_high, lhs_high-lhs_low+1, rhs_high);
        return std::make_pair(min((int64_t)0, part.first), (lhs_high-lhs_low-1, part.second));
    }
        // modulo has no effect
    else if (rhs_low > lhs_high)
        return std::make_pair(lhs_low, lhs_high);
        // there is some overlapping of [a,b] and [n,m]
    else if (rhs_high > lhs_high)
        return std::make_pair(0, lhs_high);
        // either compute all possibilities and join, or be imprecise
    else {
        auto dist_lhs = lhs_high - lhs_low + 1;
        auto dist_rhs = rhs_high - rhs_low + 1;
        std::vector<std::pair<int64_t, int64_t>> res;
        if (dist_lhs < dist_rhs) {
            for (int64_t lhs = lhs_low; lhs <= lhs_high; lhs++) {
                res.emplace_back(modConstDividendInterval(lhs, rhs_low, rhs_high));
            }
        } else {
            for (int64_t rhs = rhs_low; rhs <= rhs_high; rhs++) {
                res.emplace_back(modConstDivisorInterval(lhs_low, lhs_high, rhs));
            }
        }
        auto min_res = std::min_element(res.begin(), res.end(), [](auto a, auto b) {
            return a.first < b.first;
        });
        auto max_res = std::max_element(res.begin(), res.end(), [](auto a, auto b) {
            return a.second < b.second;
        });
        return std::make_pair(min_res->first, max_res->second);
    }

}

std::vector<std::string> getDisjointUnionSet(const boost::dynamic_bitset<>& lower_bin_set,
                                             const boost::dynamic_bitset<>& upper_bin_set) {
    std::string lower_bin, upper_bin;
    to_string(lower_bin_set, lower_bin);
    to_string(upper_bin_set, upper_bin);
    const uint8_t bit_size = lower_bin.length();
    std::vector<std::string> disjoint_union_set;
    std::string bin_str_ele;

    /*
     * the turning point is where lower_bin different from upper_bin
     * */
    std::string xor_str;
    to_string((lower_bin_set ^ upper_bin_set), xor_str);
    if (xor_str.find_first_of('1') == std::string::npos) {
        /*
         * lower bound == higher bound, return with itself
         * */
        disjoint_union_set.emplace_back(lower_bin);
        return disjoint_union_set;
    }
    const auto turning_point = xor_str.find_first_of('1');
    /*
     * for the right-most continuously bit of lower bound,
     *  if they're '0', then change them to 'x'
     *  if it's '1', then emplace '1'
     */
    const auto lower_start_pos = lower_bin.substr(1).find_last_of('1') + 1;
    if (lower_start_pos == bit_size-1) {
        disjoint_union_set.emplace_back(lower_bin);
    } else {
        /* substitute all of the right most '0' in lower_bin to 'x'
         *  e.g.
         *  ([010100], [011101]) -> {[0101xx], ...}
         * but if the turning point is righter than 'lower_start_pos'
         * only substitute bits right after the turning point
         *  e.g.
         *  ([01000], [01110]) -> {[010xx], ...}
         * */
        const auto substitute_bit = lower_start_pos > turning_point ? lower_start_pos : turning_point;
        bin_str_ele = lower_bin.substr(0, substitute_bit + 1);
        bin_str_ele.append(bit_size-substitute_bit - 1, 'x');
        disjoint_union_set.emplace_back(bin_str_ele);
        bin_str_ele.clear();
    }
    /*
     * when splitting the lower bound, iterate from right to left
     * */
    for (int idx = lower_start_pos-1; idx > turning_point; idx--) {
        if (idx < 0)
            break;
        /*
         * substitute the '0' to '1' and append with 'x' to get the large number
         * */
        if (lower_bin[idx] == '0') {
            bin_str_ele = lower_bin.substr(0, idx) + '1';
            bin_str_ele.append(bit_size-idx-1, 'x');
            disjoint_union_set.emplace_back(bin_str_ele);
            bin_str_ele.clear();
        }
    }

    /*
     * for the right-most continuously bit of upper bound,
     *  if they're '1', then change them to 'x'
     *  if it's '0', then emplace '0'
     * */
    const auto upper_end_pos = upper_bin.substr(1).find_last_of('0') + 1;
    if (upper_end_pos == bit_size-1) {
        disjoint_union_set.emplace_back(upper_bin);
    } else {
        /* substitute all of the right most '1' in upper_bin to 'x'.
         *  e.g.
         *  ([010001], [011011]) -> {[0110xx], ...}
         * but if the turning point is righter than 'upper_end_pos'
         * only substitute bits right after the turning point
         *  e.g.
         *  ([010001], [011111]) -> {[011xxx], ...}
         * */
        const auto substitute_bit = upper_end_pos > turning_point ? upper_end_pos : turning_point;
        bin_str_ele = upper_bin.substr(0, substitute_bit + 1);
        bin_str_ele.append(bit_size-substitute_bit - 1, 'x');
        disjoint_union_set.emplace_back(bin_str_ele);
        bin_str_ele.clear();
    }
    /*
     * when splitting the upper bound, iterate from left to right
     * */
    for (int idx = turning_point+1; idx < upper_end_pos; idx++) {
        /*
         * substitute the '1' to '0' and append with 'x' to get the smaller number
         * */
        if (upper_bin[idx] == '1') {
            bin_str_ele = upper_bin.substr(0, idx) + '0';
            bin_str_ele.append(bit_size-idx-1, 'x');
            disjoint_union_set.emplace_back(bin_str_ele);
            bin_str_ele.clear();
        }
    }

    return disjoint_union_set;
}

enum BitSetType {
    HAS_BIT = 1,
    ALL_SAME = 2,
    ALL_DIFF = 3,
};

using bitSets = std::vector<std::vector<size_t>>;
using signedBit = std::pair<bool, std::vector<size_t>>;
struct hashFunc {
    size_t operator()(const signedBit& x) const {
        size_t second_xor;
        for (const auto& s : x.second) {
            if (s == SIZE_MAX)
                return x.first;
            second_xor ^= s;
        }
        return x.first ^ second_xor;
    }
};
using signedBitSets = std::unordered_set<signedBit, hashFunc>;

signedBitSets getBitPos(bitSets lhs_vecs, bitSets rhs_vecs,
                        std::vector<std::string> lhs_dus,
                        std::vector<std::string> rhs_dus,
                        const std::string& bitwise_func,
                        BitSetType bsType) {
    signedBitSets res;
    const std::vector<size_t> non_exit(1, SIZE_MAX);
    for (size_t lhs_idx = 0; lhs_idx < lhs_vecs.size(); lhs_idx++) {
        for (size_t rhs_idx = 0; rhs_idx < rhs_vecs.size(); rhs_idx++) {
            std::vector<size_t> tmp_same_pos;
            if (bsType == BitSetType::HAS_BIT) {
                std::set_union(lhs_vecs[lhs_idx].begin(), lhs_vecs[lhs_idx].end(),
                               rhs_vecs[rhs_idx].begin(), rhs_vecs[rhs_idx].end(),
                               std::back_inserter(tmp_same_pos));
            } else if (bsType == BitSetType::ALL_SAME) {
                std::set_intersection(lhs_vecs[lhs_idx].begin(), lhs_vecs[lhs_idx].end(),
                                      rhs_vecs[rhs_idx].begin(), rhs_vecs[rhs_idx].end(),
                                      std::back_inserter(tmp_same_pos));
            } else if (bsType == BitSetType::ALL_DIFF) {
                std::set_difference(lhs_vecs[lhs_idx].begin(), lhs_vecs[lhs_idx].end(),
                                    rhs_vecs[rhs_idx].begin(), rhs_vecs[rhs_idx].end(),
                                    std::back_inserter(tmp_same_pos));
            } else
            assert(false && "unknown enum of BitSetType");
            tmp_same_pos = tmp_same_pos.empty() ? non_exit : tmp_same_pos;
            if (bitwise_func == "and") {
                res.emplace(std::make_pair(lhs_dus[lhs_idx][0]-48 & rhs_dus[rhs_idx][0]-48,
                                           tmp_same_pos));
            } else if (bitwise_func == "or") {
                res.emplace(std::make_pair(lhs_dus[lhs_idx][0]-48 | rhs_dus[rhs_idx][0]-48,
                                           tmp_same_pos));
            } else {
                res.emplace(std::make_pair(lhs_dus[lhs_idx][0]-48 ^ rhs_dus[rhs_idx][0]-48,
                                           tmp_same_pos));
            }
        }
    }
    return res;
}

int64_t convertBin2Dec(const std::string& min_res_str, const uint8_t bit_size) {
    int64_t res;
    if (min_res_str[0] == '1') {
        auto tmp_res = min_res_str;
        /*
         * minus 1
         * */
        if (tmp_res.back() == '1') {
            tmp_res.back() = '0';
        } else {
            const size_t last_one = tmp_res.find_last_of('1');
            std::string append_one(bit_size-last_one-1, '1');
            tmp_res[last_one] = '0';
            tmp_res = tmp_res.substr(0, last_one+1) + append_one;
        }
        /*
         * negate
         * */
        boost::dynamic_bitset<> min_res_db(tmp_res);
        res = -min_res_db.flip().to_ulong();
    } else {
        boost::dynamic_bitset<> min_res_db(min_res_str);
        res = min_res_db.to_ulong();
    }
    return res;
}

std::pair<int64_t, int64_t>
bitwiseInterval(const int64_t lhs_low, const int64_t lhs_high,
                const int64_t rhs_low, const int64_t rhs_high,
                const std::string& bitwise_func)
{
    /*
     * 1. get the max bit size
     *  for negative number, it has the same or smaller remaining bit size with its positive,
     *  like -8="1...1000", 8="0...1000"; -14="1...0010", 14="0...1110"
     *  so when meet negative number, we get its max bit size with its absolute value.
     *  PS: the first bit is a sign bit.
     * */
    std::vector<int64_t> ranges{std::abs(lhs_low), std::abs(lhs_high),
                                std::abs(rhs_low), std::abs(rhs_high)};
    auto max_abs_value = std::max_element(ranges.begin(), ranges.end());
    uint8_t bit_size = 64 - std::bitset<64>(*max_abs_value).to_string().find('1') + 1;

    /*
     * 2. convert to binary number
     * */
    boost::dynamic_bitset<> ll_bin(bit_size, lhs_low);
    boost::dynamic_bitset<> lh_bin(bit_size, lhs_high);
    boost::dynamic_bitset<> rl_bin(bit_size, rhs_low);
    boost::dynamic_bitset<> rh_bin(bit_size, rhs_high);

    /*
     * 3. construct disjoint union set of each binary number
     * */
    auto lhs_dus = getDisjointUnionSet(ll_bin, lh_bin);
    auto rhs_dus = getDisjointUnionSet(rl_bin, rh_bin);

    /*
     * 4. for different bit_wise operation, we have different formulas and purpose
     * */
    /*collect all bits index with '0' and '1'*/
    bitSets lhs_zero_vecs, lhs_one_vecs;
    for (const auto& lhs: lhs_dus) {
        std::vector<size_t> match_zero, match_one;
        /*no need to check the sign bit*/
        std::vector<size_t> index_vec(bit_size-1);
        std::iota(index_vec.begin(), index_vec.end(), 1);
        std::copy_if(index_vec.begin(), index_vec.end(), std::back_inserter(match_zero), [lhs](size_t v){
            return lhs[v] == '0';
        });
        lhs_zero_vecs.emplace_back(match_zero);
        std::copy_if(index_vec.begin(), index_vec.end(), std::back_inserter(match_one), [lhs](size_t v){
            return lhs[v] == '1';
        });
        lhs_one_vecs.emplace_back(match_one);
    }
    bitSets rhs_zero_vecs, rhs_one_vecs;
    for (const auto& rhs: rhs_dus) {
        std::vector<size_t> match_zero, match_one;
        /*no need to check the sign bit*/
        std::vector<size_t> index_vec(bit_size-1);
        std::iota(index_vec.begin(), index_vec.end(), 1);
        std::copy_if(index_vec.begin(), index_vec.end(), std::back_inserter(match_zero), [rhs](size_t v){
            return rhs[v] == '0';
        });
        rhs_zero_vecs.emplace_back(match_zero);
        std::copy_if(index_vec.begin(), index_vec.end(), std::back_inserter(match_one), [rhs](size_t v){
            return rhs[v] == '1';
        });
        rhs_one_vecs.emplace_back(match_one);
    }

    signedBitSets same_one_pos = getBitPos(lhs_one_vecs, rhs_one_vecs, lhs_dus, rhs_dus,
                                           bitwise_func, BitSetType::ALL_SAME);
    signedBitSets same_zero_pos = getBitPos(lhs_zero_vecs, rhs_zero_vecs, lhs_dus, rhs_dus,
                                            bitwise_func, BitSetType::ALL_SAME);
    signedBitSets exist_one_pos = getBitPos(lhs_one_vecs, rhs_one_vecs, lhs_dus, rhs_dus,
                                            bitwise_func, BitSetType::HAS_BIT);
    signedBitSets exist_zero_pos = getBitPos(lhs_zero_vecs, rhs_zero_vecs, lhs_dus, rhs_dus,
                                             bitwise_func, BitSetType::HAS_BIT);
    signedBitSets diff_first_pos = getBitPos(lhs_one_vecs, rhs_zero_vecs, lhs_dus, rhs_dus,
                                             bitwise_func, BitSetType::ALL_SAME);
    signedBitSets diff_second_pos = getBitPos(lhs_zero_vecs, rhs_one_vecs, lhs_dus, rhs_dus,
                                              bitwise_func, BitSetType::ALL_SAME);
    signedBitSets union_min, union_max;
    // todo: there's a bug of xor: [-5, -4, 0, 1]
    std::set_union(diff_first_pos.begin(), diff_first_pos.end(),
                   diff_second_pos.begin(), diff_second_pos.end(),
                   std::inserter(union_min, union_min.begin()));
    std::set_union(same_zero_pos.begin(), same_zero_pos.end(),
                   same_one_pos.begin(), same_one_pos.end(),
                   std::inserter(union_max, union_max.begin()));

    signedBitSets::iterator min_res_it, max_res_it;
    auto compSingBit = [](const signedBit& a, const signedBit& b) {
        auto it_a = a.second.begin();
        auto it_b = b.second.begin();
        while (it_a != a.second.end() && it_b != b.second.end()) {
            if ((*it_a) < (*it_b))
                return true;
            else if ((*it_a) > (*it_b)){
                return false;
            } else {
                it_a++;
                it_b++;
            }
        }
        return it_a!=a.second.end();
    };
    if (bitwise_func == "and") {
        /*
         * and:
         *  0 & 0 = 0, 0 & 1 = 0, 0 & x = 0
         *  1 & 1 = 1, 1 & x = x, x & x = x
         *   min: find the same '1', the larger the better, others are '0'
         *   max: find '0' exist, the larger the better, others are '1'
         * */
        min_res_it = std::max_element(same_one_pos.begin(), same_one_pos.end(),
                                      [compSingBit](const signedBit& a, const signedBit& b) {
                                          if (a.first < b.first)
                                              return true;
                                          else if (a.first > b.first)
                                              return false;
                                          else {
                                              return compSingBit(a, b);
                                          }
                                      });
        max_res_it = std::max_element(exist_zero_pos.begin(), exist_zero_pos.end(),
                                      [compSingBit](const signedBit& a, const signedBit& b) {
                                          if (a.first > b.first)
                                              return true;
                                          else if (a.first < b.first)
                                              return false;
                                          else {
                                              return compSingBit(a, b);
                                          }
                                      });
    } else if (bitwise_func == "or") {
        /*
         *  or:
         *  1 | 1 = 1, 1 | 0 = 1, 1 | x = 1
         *  0 | 0 = 0, 0 | x = x, x | x = x
         *   min: find '1' exist, the larger the better, others are '0'
         *   max: find the same '0', the larger the better, others are '1'
         * */
        min_res_it = std::max_element(exist_one_pos.begin(), exist_one_pos.end(),
                                      [compSingBit](const signedBit& a, const signedBit& b) {
                                          if (a.first < b.first)
                                              return true;
                                          else if (a.first > b.first)
                                              return false;
                                          else {
                                              return compSingBit(a, b);
                                          }
                                      });
        max_res_it = std::max_element(same_zero_pos.begin(), same_zero_pos.end(),
                                      [compSingBit](const signedBit& a, const signedBit& b) {
                                          if (a.first > b.first)
                                              return true;
                                          else if (a.first < b.first)
                                              return false;
                                          else {
                                              return compSingBit(a, b);
                                          }
                                      });
    } else if (bitwise_func == "xor") {
        /*
         *  xor:
         *  0 ^ 0 = 0, 0 ^ 1 = 1, 1 ^ 1 = 0
         *  x ^ 0 = x, x ^ 1 = x, x ^ x = x
         *   min: find the different '0' + different '1', the larger the better, others are '0'
         *   max: find the same '0' + same '1', the larger the better, others are '1'
         * */
        min_res_it = std::max_element(union_min.begin(), union_min.end(),
                                      [compSingBit](const signedBit& a, const signedBit& b) {
                                          if (a.first < b.first)
                                              return true;
                                          else if (a.first > b.first)
                                              return false;
                                          else {
                                              return compSingBit(a, b);
                                          }
                                      });
        max_res_it = std::max_element(union_max.begin(), union_max.end(),
                                      [compSingBit](const signedBit& a, const signedBit& b) {
                                          if (a.first > b.first)
                                              return true;
                                          else if (a.first < b.first)
                                              return false;
                                          else {
                                              return compSingBit(a, b);
                                          }
                                      });
    } else {
        assert(false && "unknown bit_wise operation");
    }

    std::string min_res_str(bit_size, '0'), max_res_str(bit_size, '1');
    min_res_str[0] = min_res_it->first ? '1' : '0';
    for (const auto& bit : min_res_it->second) {
        if (bit != SIZE_MAX) {
            min_res_str[bit] = '1';
        }
    }
    max_res_str[0] = max_res_it->first ? '1' : '0';
    for (const auto& bit : max_res_it->second) {
        if (bit != SIZE_MAX) {
            max_res_str[bit] = '0';
        }
    }

    /*
     * 5. convert binary string to decimal int64_t
     * */
    int64_t min_res = convertBin2Dec(min_res_str, bit_size);
    int64_t max_res = convertBin2Dec(max_res_str, bit_size);
    return std::make_pair(min_res, max_res);
}

std::pair<Value *, std::pair<double, double>>
rangeAnalysis(State * N, BoundInfo * boundInfo, Function & llvmIrFunction, bool standaloneFunc)
{
    /*
     * We only search the function with Newton Information
     * */
    bool nonNewtonInfo = standaloneFunc;
    DISubprogram *irFuncSubProgram = llvmIrFunction.getSubprogram();
    if (irFuncSubProgram != nullptr) {
        auto funcType = irFuncSubProgram->getType();
        if (funcType != nullptr) {
            DITypeRefArray typeArray = irFuncSubProgram->getType()->getTypeArray();
            for (size_t typeIdx = 1; typeIdx < typeArray.size(); typeIdx++) {
                StringRef paramTypeName = typeArray[typeIdx]->getName();
                if (boundInfo->typeRange.find(paramTypeName.str()) != boundInfo->typeRange.end()) {
                    nonNewtonInfo = false;
                    break;
                }
            }
        }
    }
    if (nonNewtonInfo) {
        return std::make_pair(nullptr, std::make_pair(DBL_MIN, DBL_MAX));
    }

    /*
     * information for the union data structure
     * */
    std::map<Value *, Value *> unionAddress;
    /*
     * function arguments
     * */
    std::map<Value *, Value *> storeParamMap;
    for (BasicBlock & llvmIrBasicBlock : llvmIrFunction)
    {
        for (Instruction & llvmIrInstruction : llvmIrBasicBlock)
        {
            switch (llvmIrInstruction.getOpcode())
            {
                case Instruction::Call:
                    if (auto llvmIrCallInstruction = dyn_cast<CallInst>(&llvmIrInstruction))
                    {
                        Function * calledFunction = llvmIrCallInstruction->getCalledFunction();
                        if (calledFunction == nullptr || !calledFunction->hasName() || calledFunction->getName().empty())
                            break;
                        if (calledFunction->getName().startswith("llvm.dbg.value") ||
                            calledFunction->getName().startswith("llvm.dbg.declare"))
                        {
                            if (!isa<MetadataAsValue>(llvmIrCallInstruction->getOperand(0)))
                                break;
                            auto firstOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
                            if (!isa<ValueAsMetadata>(firstOperator->getMetadata()))
                                break;
                            auto localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
                            auto localVariableAddress = localVariableAddressAsMetadata->getValue();

                            auto variableMetadata = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(1));
                            if (!isa<DIVariable>(variableMetadata->getMetadata()))
                                break;
                            auto debugInfoVariable = cast<DIVariable>(variableMetadata->getMetadata());
                            const DIType * variableType = debugInfoVariable->getType();

                            auto recordType = [&](const DIType *variableType) {
                                if (const auto *derivedVariableType = dyn_cast<DIDerivedType>(variableType))
                                {
                                    std::string baseTypeName;
                                    /*
                                    *	if we find such type in boundInfo->typeRange,
                                    *	we record it in the boundInfo->virtualRegisterRange
                                    */
                                    if (derivedVariableType->getTag() == llvm::dwarf::DW_TAG_pointer_type) {
                                        baseTypeName = derivedVariableType->getBaseType()->getName().str();
                                    } else {
                                        baseTypeName = derivedVariableType->getName().str();
                                    }
                                    auto typeRangeIt = boundInfo->typeRange.find(baseTypeName);
                                    if (typeRangeIt != boundInfo->typeRange.end())
                                    {
                                        boundInfo->virtualRegisterRange.emplace(localVariableAddress, typeRangeIt->second);
                                        auto spIt = storeParamMap.find(localVariableAddress);
                                        if (spIt != storeParamMap.end()) {
                                            boundInfo->virtualRegisterRange.emplace(spIt->second, typeRangeIt->second);
                                        }
                                    }
                                    else {
                                        //todo: other DIDerivedType, like size_t
                                    }
                                }
                                else if (const auto *basicVariableType = dyn_cast<DIBasicType>(variableType))
                                {
                                    /*
                                     * if it's a basic type, insert the basic, todo
                                     * */
//                                boundInfo->virtualRegisterRange.emplace(localVariableAddress, size);
                                }
                            };

                            if (const auto *compositeVariableType = dyn_cast<DICompositeType>(variableType))
                            {
                                /*
                                 * It's a composite type, including structure, union, array, and enumeration
                                 * Extract from composite type
                                 * */
                                auto typeTag = compositeVariableType->getTag();
                                if (typeTag == dwarf::DW_TAG_union_type)
                                {
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: DW_TAG_union_type\n");
                                }
                                else if (typeTag == dwarf::DW_TAG_structure_type)
                                {
                                    // todo
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: DW_TAG_structure_type\n");
//                                    auto unionTypeArr = compositeVariableType->getElements();
//                                    for (size_t i = 0; i < unionTypeArr.size(); i++)
//                                    {
//                                        auto typeRangeIt = boundInfo->typeRange.find(unionTypeArr[i]->getName().str());
//                                        if (typeRangeIt != boundInfo->typeRange.end())
//                                        {
//                                            boundInfo->virtualRegisterRange.emplace(localVariableAddress, typeRangeIt->second);
//                                        }
//                                    }
                                }
                                else if (typeTag == dwarf::DW_TAG_array_type)
                                {
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: DW_TAG_array_type\n");
                                    const DIType *ElemType = compositeVariableType->getBaseType();
                                    recordType(ElemType);
                                }
                                else if (typeTag == dwarf::DW_TAG_enumeration_type)
                                {
                                    // todo
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: DW_TAG_enumeration_type\n");
                                }
                            }
                            recordType(variableType);

                        }
                        /*
                         * It's function defined by programmer, eg. %17 = call i32 @abstop12(float %16), !dbg !83
                         * */
                        else
                        {
                            if (!calledFunction)
                            {
                                flexprint(N->Fe, N->Fm, N->Fperr, "\tCall: CalledFunction %s is nullptr or undeclared.\n",
                                          calledFunction->getName().str().c_str());
                                continue;
                            } else if (calledFunction->isDeclaration()) {
                                /*
                                 * the primary definition of this global value is outside the current translation unit.
                                 * */
                                std::string funcName = calledFunction->getName().str();
//                                std::vector<std::string> monotonicLibFunc = {"log", "sqrt", "log1p", "exp", "scalbn"};
//                                std::vector<std::string> trigonometircFunc = {"sin", "cos"};
//                                std::vector<std::string> monotonicLLVMFunc = {"llvm.floor", "llvm.ceil"};
                                std::map<uint32_t, std::pair<double, double>> argRanges;
                                for (size_t idx = 0; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++)
                                {
                                    auto vrRangeIt = boundInfo->virtualRegisterRange.find(
                                            llvmIrCallInstruction->getOperand(idx));
                                    if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                                    {
                                        argRanges.emplace(idx, vrRangeIt->second);
                                    } else {
                                        assert(!valueRangeDebug && "failed to get range");
                                        break;
                                    }
                                }
                                double lowRange, highRange;
                                // todo: reconstruct by MACRO or template
                                if (funcName == "log") {
                                    lowRange = log(argRanges[0].first);
                                    highRange = log(argRanges[0].second);
                                } else if (funcName == "exp") {
                                    lowRange = exp(argRanges[0].first);
                                    highRange = exp(argRanges[0].second);
                                } else if (funcName == "sqrt") {
                                    lowRange = sqrt(argRanges[0].first);
                                    highRange = sqrt(argRanges[0].second);
                                } else if (funcName == "log1p") {
                                    lowRange = log1p(argRanges[0].first);
                                    highRange = log1p(argRanges[0].second);
                                } else if (funcName == "scalbn") {
                                    lowRange = scalbn(argRanges[0].first, argRanges[1].first);
                                    highRange = scalbn(argRanges[0].second, argRanges[1].second);
                                } else if (funcName == "sin" || funcName == "cos") {
                                    lowRange = -1;
                                    highRange = 1;
                                } else if (calledFunction->getName().startswith("llvm.fabs")) {
                                    lowRange = min(min(fabs(argRanges[0].first),
                                                   fabs(argRanges[0].second)), 0);
                                    highRange = max(fabs(argRanges[0].first),
                                                    fabs(argRanges[0].second));
                                } else if (calledFunction->getName().startswith("llvm.floor")) {
                                    lowRange = floor(argRanges[0].first);
                                    highRange = floor(argRanges[0].second);
                                } else if (calledFunction->getName().startswith("llvm.ceil")) {
                                    lowRange = ceil(argRanges[0].first);
                                    highRange = ceil(argRanges[0].second);
                                } else {
                                    assert(!valueRangeDebug && "didn't support such function yet");
                                    break;
                                }
                                boundInfo->virtualRegisterRange.emplace(llvmIrCallInstruction,
                                                                        std::make_pair(lowRange, highRange));
                            } else {
                                /*
                                 * Algorithm to infer the range of CallInst's result:
                                 * 1. find the CallInst (caller).
                                 * 2. check if the CallInst's operands is a variable with range.
                                 * 3. infer the range of the operands (if needed).
                                 * 4. look into the called function (callee), and get its operands with range in step 3.
                                 * 5. if there's a CallInst in the body of called function, go to step 1.
                                 *    else infer the range of the return value.
                                 * 6. set the range of the result of the CallInst.
                                 * */
                                flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: detect CalledFunction %s.\n",
                                          calledFunction->getName().str().c_str());
                                auto innerBoundInfo = new BoundInfo();
								innerBoundInfo->virtualRegisterVectorRange = boundInfo->virtualRegisterVectorRange;
                                for (size_t idx = 0; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++)
                                {
                                    /*
                                     * First, we check if it's a constant value
                                     * */
                                    if (ConstantInt* cInt = dyn_cast<ConstantInt>(llvmIrCallInstruction->getOperand(idx)))
                                    {
                                        int64_t constIntValue = cInt->getSExtValue();
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: It's a constant int value: %d.\n", constIntValue);
                                        innerBoundInfo->virtualRegisterRange.emplace(calledFunction->getArg(idx),
                                                std::make_pair(static_cast<double>(constIntValue), static_cast<double>(constIntValue)));
                                    }
                                    else if (ConstantFP * constFp = dyn_cast<ConstantFP>(llvmIrCallInstruction->getOperand(idx)))
                                    {
                                        double constDoubleValue = (constFp->getValueAPF()).convertToDouble();
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: It's a constant double value: %f.\n", constDoubleValue);
                                        innerBoundInfo->virtualRegisterRange.emplace(calledFunction->getArg(idx),
                                                std::make_pair(constDoubleValue, constDoubleValue));
                                    }
                                    else
                                    {
                                        /*
                                        *	if we find the operand in boundInfo->virtualRegisterRange,
                                        *	we know it's a variable with range.
                                        */
                                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrCallInstruction->getOperand(idx));
                                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                                        {
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: the range of the operand is: %f - %f.\n",
                                            vrRangeIt->second.first, vrRangeIt->second.second);
                                            innerBoundInfo->virtualRegisterRange.emplace(calledFunction->getArg(idx), vrRangeIt->second);
                                        } else {
                                            assert(!valueRangeDebug && "failed to get range");
                                        }
                                    }
                                }
                                auto returnRange = rangeAnalysis(N, innerBoundInfo, *calledFunction, false);
                                if (returnRange.first != nullptr)
                                {
                                    boundInfo->virtualRegisterRange.emplace(llvmIrCallInstruction, returnRange.second);
                                }
                                /*
                                 * if variables of innerBoundInfo has been stored in boundInfo,
                                 * we get the union set of them
                                 * */
                                for (const auto& vrRange : innerBoundInfo->virtualRegisterRange) {
                                    auto ibIt = boundInfo->virtualRegisterRange.find(vrRange.first);
                                    if (ibIt != boundInfo->virtualRegisterRange.end()) {
                                    auto innerLowerBound = vrRange.second.first < ibIt->second.first ?
                                            vrRange.second.first : ibIt->second.first;
                                    auto innerUpperBound = vrRange.second.second > ibIt->second.second ?
                                            vrRange.second.second : ibIt->second.second;
                                    boundInfo->virtualRegisterRange[ibIt->first] = std::make_pair(innerLowerBound,
                                                                                                  innerUpperBound);
                                    } else {
                                        boundInfo->virtualRegisterRange.emplace(vrRange.first, vrRange.second);
                                    }
                                }
								/*
								 * Check the return type of the function,
								 * if it's a physical type that records in `boundInfo.typeRange`
								 * but didn't match the range we inferred from `rangeAnalysis` algorithm,
								 * we give a warning to the programmer.
								 * But we still believe in the range we inferred from the function body.
								 */
                                DISubprogram *subProgram = calledFunction->getSubprogram();
                                DITypeRefArray typeArray = subProgram->getType()->getTypeArray();
                                if (typeArray[0] != nullptr) {
                                    StringRef returnTypeName = typeArray[0]->getName();
                                    auto vrRangeIt = boundInfo->typeRange.find(returnTypeName.str());
                                    if (vrRangeIt != boundInfo->typeRange.end() &&
                                        (vrRangeIt->second.first != returnRange.second.first || vrRangeIt->second.second != returnRange.second.second))
                                    {
                                        flexprint(N->Fe, N->Fm, N->Fperr, "\tCall: the range of the function's return type is: %f - %f, but we inferred as: %f - %f\n",
                                        vrRangeIt->second.first, vrRangeIt->second.second, returnRange.second.first, returnRange.second.second);
                                    }
                                }
                            }
                        }
                    }
                    break;

                case Instruction::Add:
                case Instruction::FAdd:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tAdd: swap left and right\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tAdd: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1+x2
                             * 	btw, I don't think we should check type here, which should be done in other pass like dimension-check
                             * 	find left operand from the boundInfo->virtualRegisterRange
                             * 	range: [x1_min+x2_min, x1_max+x2_max]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            /*
                             * 	find right operand from the boundInfo->virtualRegisterRange
                             */
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound += vrRangeIt->second.first;
                                upperBound += vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. x+2
                             */
                            double constValue = 0.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
                            {
                                /*
                                 * 	both "float" and "double" type can use "convertToDouble"
                                 */
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(vrRangeIt->second.first + constValue,
                                                                                       vrRangeIt->second.second + constValue));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::Sub:
                case Instruction::FSub:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tSub: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1-x2
                             * 	btw, I don't think we should check type here, which should be done in other pass like dimension-check
                             * 	find left operand from the boundInfo->virtualRegisterRange
                             * 	range: [x1_min-x2_max, x1_max-x2_min]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound -= vrRangeIt->second.second;
                                upperBound -= vrRangeIt->second.first;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. x-2
                             */
                            double constValue = 0.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
                            {
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(vrRangeIt->second.first - constValue,
                                                                                       vrRangeIt->second.second - constValue));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. 2-x
                             */
                            double constValue = 0.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(leftOperand))
                            {
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(constValue - vrRangeIt->second.second,
                                                                                       constValue - vrRangeIt->second.first));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tSub: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::Mul:
                case Instruction::FMul:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tMul: swap left and right\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tMul: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1*x2
                             * 	range: [min(x1_min*x2_min, x1_min*x2_max, x1_max*x2_min, x1_max*x2_max),
                             * 	        max(x1_min*x2_min, x1_min*x2_max, x1_max*x2_min, x1_max*x2_max)]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto leftMin = lowerBound;
                                auto leftMax = upperBound;
                                auto rightMin = vrRangeIt->second.first;
                                auto rightMax = vrRangeIt->second.second;
                                lowerBound = min(min(min(leftMin * rightMin,
                                                         leftMin * rightMax),
                                                     leftMax * rightMin),
                                                 leftMax * rightMax);
                                upperBound = max(max(max(leftMin * rightMin,
                                                         leftMin * rightMax),
                                                     leftMax * rightMin),
                                                 leftMax * rightMax);
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                        } else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. x*2
                             */
                            double constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
                            {
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(vrRangeIt->second.first * constValue,
                                                                                       vrRangeIt->second.second * constValue));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        } else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tMul: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::SDiv:
                case Instruction::FDiv:
                case Instruction::UDiv:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tDiv: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1/x2
                             * 	range: [min(x1_min/x2_min, x1_min/x2_max, x1_max/x2_min, x1_max/x2_max),
                             * 	        max(x1_min/x2_min, x1_min/x2_max, x1_max/x2_min, x1_max/x2_max)]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto leftMin = lowerBound;
                                auto leftMax = upperBound;
                                auto rightMin = vrRangeIt->second.first;
                                auto rightMax = vrRangeIt->second.second;
                                lowerBound = min(min(min(leftMin / rightMin,
                                                         leftMin / rightMax),
                                                     leftMax / rightMin),
                                                 leftMax / rightMax);
                                upperBound = max(max(max(leftMin / rightMin,
                                                         leftMin / rightMax),
                                                     leftMax / rightMin),
                                                 leftMax / rightMax);
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. 2/x
                             */
                            double constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(leftOperand))
                            {
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(leftOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto rightMin = vrRangeIt->second.first;
                                auto rightMax = vrRangeIt->second.second;
                                double lowerBound = min(constValue / rightMin, constValue / rightMax);
                                double upperBound = max(constValue / rightMin, constValue / rightMax);
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(lowerBound, upperBound));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. x/2
                             */
                            double constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
                            {
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(vrRangeIt->second.first / constValue,
                                                                                       vrRangeIt->second.second / constValue));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tDiv: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::URem:
                case Instruction::SRem:
                case Instruction::FRem:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tRem: swap left and right\n");
                        }
                        else if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tRem: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                            int64_t leftMin = lowerBound;
                            int64_t leftMax = upperBound;
                            int64_t rightMin = vrRangeIt->second.first;
                            int64_t rightMax = vrRangeIt->second.second;
                            auto res = modInterval(leftMin, leftMax, rightMin, rightMax);
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                    std::make_pair(res.first, res.second));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                            {
                            /*
                             * 	eg. x%2
                             */
                            double constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
                            {
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto res = modInterval(vrRangeIt->second.first, vrRangeIt->second.second,
                                                       constValue, constValue);
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(res.first, res.second));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tDiv: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }

                    }
                    break;

                case Instruction::Shl:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShl: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1 << x2
                             * 	range: [min(x1_min<<x2_min, x1_min<<x2_max, x1_max<<x2_min, x1_max<<x2_max),
                             * 	        max(x1_min<<x2_min, x1_min<<x2_max, x1_max<<x2_min, x1_max<<x2_max)]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto leftMin = lowerBound;
                                auto leftMax = upperBound;
                                auto rightMin = vrRangeIt->second.first;
                                auto rightMax = vrRangeIt->second.second;
                                lowerBound = min(min(min((int)leftMin << (int)rightMin,
                                                         (int)leftMin << (int)rightMax),
                                                     (int)leftMax << (int)rightMin),
                                                 (int)leftMax << (int)rightMax);
                                upperBound = max(max(max((int)leftMin << (int)rightMin,
                                                         (int)leftMin << (int)rightMax),
                                                     (int)leftMax << (int)rightMin),
                                                 (int)leftMax << (int)rightMax);
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	e.g. 2 << x
                             * 	range: [min(2<<x2_min, 2<<x2_max),
                             * 	        max(2<<x2_min, 2<<x2_max)]
                             */
                            uint64_t constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(leftOperand))
                            {
                                constValue = static_cast<uint64_t>((constFp->getValueAPF()).convertToDouble());
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(leftOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                // todo: if we need assert or other check here?
                                uint64_t rightMin = vrRangeIt->second.first < 0 ? 0 : vrRangeIt->second.first;
                                uint64_t rightMax = vrRangeIt->second.second < 0 ? 0 : vrRangeIt->second.second;
                                double lowerBound = min(constValue << rightMin, constValue << rightMax);
                                double upperBound = max(constValue << rightMin, constValue << rightMax);
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(lowerBound, upperBound));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. x<<2
                             */
                            int constValue = 1.0;
                            if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getZExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair((int)vrRangeIt->second.first << constValue,
                                                                                       (int)vrRangeIt->second.second << constValue));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShl: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::LShr:
                case Instruction::AShr:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShr: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1 >> x2
                             * 	range: [min(x1_min>>x2_min, x1_min>>x2_max, x1_max>>x2_min, x1_max>>x2_max),
                             * 	        max(x1_min>>x2_min, x1_min>>x2_max, x1_max>>x2_min, x1_max>>x2_max)]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto leftMin = lowerBound;
                                auto leftMax = upperBound;
                                auto rightMin = vrRangeIt->second.first;
                                auto rightMax = vrRangeIt->second.second;
                                lowerBound = min(min(min((int)leftMin >> (int)rightMin,
                                                         (int)leftMin >> (int)rightMax),
                                                     (int)leftMax >> (int)rightMin),
                                                 (int)leftMax >> (int)rightMax);
                                upperBound = max(max(max((int)leftMin >> (int)rightMin,
                                                         (int)leftMin >> (int)rightMax),
                                                     (int)leftMax >> (int)rightMin),
                                                 (int)leftMax >> (int)rightMax);
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	e.g. 2 >> x
                             * 	range: [min(2>>x2_min, 2>>x2_max),
                             * 	        max(2>>x2_min, 2>>x2_max)]
                             */
                            uint64_t constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(leftOperand))
                            {
                                constValue = static_cast<uint64_t>((constFp->getValueAPF()).convertToDouble());
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(leftOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                // todo: if we need assert or other check here?
                                uint64_t rightMin = vrRangeIt->second.first < 0 ? 0 : vrRangeIt->second.first;
                                uint64_t rightMax = vrRangeIt->second.second < 0 ? 0 : vrRangeIt->second.second;
                                double lowerBound = min(constValue >> rightMin, constValue >> rightMax);
                                double upperBound = max(constValue >> rightMin, constValue >> rightMax);
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(lowerBound, upperBound));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             *	eg. x>>2
                             */
                            int constValue = 1.0;
                            if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getZExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair((int)vrRangeIt->second.first >> constValue,
                                                                                       (int)vrRangeIt->second.second >> constValue));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShr: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::And:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tAnd: swap left and right\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tAnd: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            /*
                             * 	find right operand from the boundInfo->virtualRegisterRange
                             */
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                std::pair<int64_t, int64_t> and_res = bitwiseInterval(lowerBound, upperBound,
                                                                                      vrRangeIt->second.first,
                                                                                      vrRangeIt->second.second,
                                                                                      "and");
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(and_res.first, and_res.second));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. x&2
                             */
                            double constValue = 0.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
                            {
                                /*
                                 * 	both "float" and "double" type can use "convertToDouble"
                                 */
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                std::pair<int64_t, int64_t> and_res = bitwiseInterval(constValue, constValue,
                                                                                      vrRangeIt->second.first,
                                                                                      vrRangeIt->second.second,
                                                                                      "and");
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(and_res.first, and_res.second));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::Or:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tOr: swap left and right\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tOr: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            /*
                             * 	find right operand from the boundInfo->virtualRegisterRange
                             */
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                std::pair<int64_t, int64_t> and_res = bitwiseInterval(lowerBound, upperBound,
                                                                                      vrRangeIt->second.first,
                                                                                      vrRangeIt->second.second,
                                                                                      "or");
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(and_res.first, and_res.second));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. x|2
                             */
                            double constValue = 0.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
                            {
                                /*
                                 * 	both "float" and "double" type can use "convertToDouble"
                                 */
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                std::pair<int64_t, int64_t> and_res = bitwiseInterval(constValue, constValue,
                                                                                      vrRangeIt->second.first,
                                                                                      vrRangeIt->second.second,
                                                                                      "or");
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(and_res.first, and_res.second));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::Xor:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tXor: swap left and right\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tXor: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            /*
                             * 	find right operand from the boundInfo->virtualRegisterRange
                             */
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                std::pair<int64_t, int64_t> and_res = bitwiseInterval(lowerBound, upperBound,
                                                                                      vrRangeIt->second.first,
                                                                                      vrRangeIt->second.second,
                                                                                      "xor");
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(and_res.first, and_res.second));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. x^2
                             */
                            double constValue = 0.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
                            {
                                /*
                                 * 	both "float" and "double" type can use "convertToDouble"
                                 */
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                std::pair<int64_t, int64_t> and_res = bitwiseInterval(constValue, constValue,
                                                                                      vrRangeIt->second.first,
                                                                                      vrRangeIt->second.second,
                                                                                      "xor");
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(and_res.first, and_res.second));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::FPToUI:
                case Instruction::FPToSI:
                case Instruction::SIToFP:
                case Instruction::UIToFP:
                case Instruction::ZExt:
                case Instruction::SExt:
                case Instruction::FPExt:
                case Instruction::Trunc:
                case Instruction::FPTrunc:
                {
                    Value * operand = llvmIrInstruction.getOperand(0);
                    auto	vrRangeIt = boundInfo->virtualRegisterRange.find(operand);
                    if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                    {
                        boundInfo->virtualRegisterRange.emplace(&llvmIrInstruction,
                                                                vrRangeIt->second);
                    } else {
                        assert(!valueRangeDebug && "failed to get range");
                    }
                }
                    break;

                case Instruction::Load:
                    if (auto llvmIrLoadInstruction = dyn_cast<LoadInst>(&llvmIrInstruction))
                    {
                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrLoadInstruction->getOperand(0));
                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                        {
                            boundInfo->virtualRegisterRange.emplace(llvmIrLoadInstruction, vrRangeIt->second);

                        } else {
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::Store:
                    if (auto llvmIrStoreInstruction = dyn_cast<StoreInst>(&llvmIrInstruction))
                    {
                        if (isa<llvm::Constant>(llvmIrStoreInstruction->getOperand(0)))
                        {
                            /*
                             * 	eg. store double 5.000000e+00, double* %2, align 8, !dbg !27
                             */
                            double constValue = 0.0;
                            if (ConstantFP *constFp = llvm::dyn_cast<llvm::ConstantFP>(llvmIrStoreInstruction->getOperand(0)))
                            {
                                /*
                                 * 	both "float" and "double" type can use "convertToDouble"
                                 */
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            }
                            else if (ConstantInt *constInt = llvm::dyn_cast<llvm::ConstantInt>(llvmIrStoreInstruction->getOperand(0)))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto rhsIt = boundInfo->virtualRegisterRange.find(llvmIrStoreInstruction->getOperand(1));
                            if (rhsIt != boundInfo->virtualRegisterRange.end()) {
                                rhsIt->second = std::make_pair(constValue, constValue);
                            } else {
                                boundInfo->virtualRegisterRange.emplace(llvmIrStoreInstruction->getOperand(1), std::make_pair(constValue, constValue));
                            }
                        }
                        else
                        {
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrStoreInstruction->getOperand(0));
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto rhsIt = boundInfo->virtualRegisterRange.find(llvmIrStoreInstruction->getOperand(1));
                                if (rhsIt != boundInfo->virtualRegisterRange.end()) {
                                    rhsIt->second = vrRangeIt->second;
                                } else {
                                    boundInfo->virtualRegisterRange.emplace(llvmIrStoreInstruction->getOperand(1), vrRangeIt->second);
                                }
                                boundInfo->virtualRegisterRange.emplace(llvmIrStoreInstruction->getOperand(1), vrRangeIt->second);
                            } else {
                                /*
                                 * llvmIrStoreInstruction->getOperand(0) is the param of function
                                 * */
                                if (isa<Argument>(llvmIrStoreInstruction->getOperand(0))) {
                                    storeParamMap.emplace(llvmIrStoreInstruction->getOperand(1),
                                                          llvmIrStoreInstruction->getOperand(0));
                                } else {
                                    assert(!valueRangeDebug && "failed to get range");
                                }
                            }
                            /*
                             * Each time if there's a StorInst assign to the unionAddress, it updates the value of union.
                             * */
                            auto uaIt = unionAddress.find(llvmIrStoreInstruction->getOperand(1));
                            if (uaIt != unionAddress.end())
                            {
                                flexprint(N->Fe, N->Fm, N->Fpinfo, "\tStore Union: %f - %f\n",  vrRangeIt->second.first, vrRangeIt->second.second);
                                boundInfo->virtualRegisterRange.emplace(uaIt->second, vrRangeIt->second);
                            }
                        }
                    }
                    break;

//                case Instruction::Trunc:
//                    if (auto llvmIrTruncInstruction = dyn_cast<TruncInst>(&llvmIrInstruction))
//                    {
//                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrTruncInstruction->getOperand(0));
//                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
//                        {
//                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFPTrunc: %f - %f\n",  vrRangeIt->second.first, vrRangeIt->second.second);
//                            double originLow = vrRangeIt->second.first;
//                            double originHigh = vrRangeIt->second.second;
//                            double lowRange = 0, highRange = 0;
//                            auto DestEleType = llvmIrTruncInstruction->getDestTy();
//                            switch (DestEleType->getTypeID())
//                            {
//                                case Type::IntegerTyID:
//                                    switch (DestEleType->getIntegerBitWidth())
//                                    {
//                                        case 8:
//                                            lowRange = static_cast<double>(static_cast<int8_t>(originLow));
//                                            highRange = static_cast<double>(static_cast<int8_t>(originHigh));
//                                        break;
//                                        case 16:
//                                            lowRange = static_cast<double>(static_cast<int16_t>(originLow));
//                                            highRange = static_cast<double>(static_cast<int16_t>(originHigh));
//                                        break;
//                                        case 32:
//                                            lowRange = static_cast<double>(static_cast<int32_t>(originLow));
//                                            highRange = static_cast<double>(static_cast<int32_t>(originHigh));
//                                        break;
//                                        case 64:
//                                            lowRange = static_cast<double>(static_cast<int64_t>(originLow));
//                                            highRange = static_cast<double>(static_cast<int64_t>(originHigh));
//                                        break;
//                                        default:
//                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tTrunc: Type::SignedInteger, don't support such bit width yet.");
//                                    }
//                                break;
//                            }
//                            boundInfo->virtualRegisterRange.emplace(llvmIrTruncInstruction, std::make_pair(lowRange, highRange));
//                        }
//                    }
//                break;
//
//                case Instruction::FPTrunc:
//                    if (auto llvmIrFPTruncInstruction = dyn_cast<FPTruncInst>(&llvmIrInstruction))
//                    {
//                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrFPTruncInstruction->getOperand(0));
//                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
//                        {
//                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFPTrunc: %f - %f\n",  vrRangeIt->second.first, vrRangeIt->second.second);
//                            boundInfo->virtualRegisterRange.emplace(llvmIrFPTruncInstruction,
//                                                                    std::make_pair(static_cast<double>(static_cast<float>(vrRangeIt->second.first)),
//                                                                                   static_cast<double>(static_cast<float>(vrRangeIt->second.second))));
//                        }
//                    }
//                    break;

                case Instruction::BitCast:
                    if (auto llvmIrBitCastInstruction = dyn_cast<BitCastInst>(&llvmIrInstruction))
                    {
                        /*
                        * for the union type, LLVM IR uses a store intrinsic to link the variables, e.g.
                         * union {
                         *   float f;
                         *   uint32_t i;
                         * } u = {f};
                         * The IR is:
                         *   %4 = bitcast %union.anon* %3 to double*
                         *   %5 = load double, double* %2, align 8
                         *   store double %5, double* %4, align 8
                         *   %6 = bitcast %union.anon* %3 to i32*
                         *
                         * So the Algorithm to infer the range of Union type is:
                         * 1. record the first bitcast instruction info to a map,
                         *    as we didn't have the actual variable information
                         * 2. check with the store instruction with the records in the map
                         *    and store the actual variable to the %union.anon
                         * 3. get the variable info from the %union.anon by the second bitcast instruction,
                         *    and reinterpret it if necessary
                         * */
                        unionAddress.emplace(llvmIrBitCastInstruction, llvmIrBitCastInstruction->getOperand(0));
                        assert(llvmIrBitCastInstruction->getDestTy()->getTypeID() == Type::PointerTyID);
                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrBitCastInstruction->getOperand(0));
                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                        {
                            /*
                             * In our current test cases, there's only double->uint64_t or float->uint32_t.
                             * But theoretically there will be uint64_t->double, uint32_t->float or others.
                             * We should rewrite it to
                             * `auto originLow = static_cast<uint64_t>vrRangeIt->second.first;` when it appears.
                             * */
                            double originLow = vrRangeIt->second.first;
                            double originHigh = vrRangeIt->second.second;
                            double lowRange, highRange;
                            auto DestEleType = llvmIrBitCastInstruction->getDestTy()->getPointerElementType();
                            /*
                             * if it's a structure type, we use reinterpret_cast
                             * todo: not very sure, need further check
                             * */
                            if (llvmIrBitCastInstruction->getSrcTy()->isStructTy()) {
                                switch (DestEleType->getTypeID())
                                {
                                    case Type::FloatTyID:
                                        lowRange = static_cast<double>(*reinterpret_cast<float *>(&originLow));
                                        highRange = static_cast<double>(*reinterpret_cast<float *>(&originHigh));
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::FloatTyID, %f - %f to %f - %f\n",
                                        vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                        boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, std::make_pair(lowRange, highRange));
                                        break;
                                    case Type::DoubleTyID:
                                        lowRange = *reinterpret_cast<double *>(&originLow);
                                        highRange = *reinterpret_cast<double *>(&originHigh);
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::DoubleTyID, %f - %f to %f - %f\n",
                                        vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                        boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, std::make_pair(lowRange, highRange));
                                        break;
                                    case Type::IntegerTyID:
                                        switch (DestEleType->getIntegerBitWidth())
                                        {
                                            case 8:
                                                lowRange = static_cast<double>(*reinterpret_cast<int8_t *>(&originLow));
                                                highRange = static_cast<double>(*reinterpret_cast<int8_t *>(&originHigh));
                                                break;
                                            case 16:
                                                lowRange = static_cast<double>(*reinterpret_cast<int16_t *>(&originLow));
                                                highRange = static_cast<double>(*reinterpret_cast<int16_t *>(&originHigh));
                                                break;
                                            case 32:
                                                lowRange = static_cast<double>(*reinterpret_cast<int32_t *>(&originLow));
                                                highRange = static_cast<double>(*reinterpret_cast<int32_t *>(&originHigh));
                                                break;
                                            case 64:
                                                lowRange = static_cast<double>(*reinterpret_cast<int64_t *>(&originLow));
                                                highRange = static_cast<double>(*reinterpret_cast<int64_t *>(&originHigh));
                                                break;
                                            default:
                                                flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::SignedInteger, don't support such bit width yet.");
                                        }

                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::IntegerTyID, %f - %f to %f - %f\n",
                                        vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                        boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, std::make_pair(lowRange, highRange));
                                        break;
                                    case Type::StructTyID:
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::StructTyID, %f - %f to %f - %f\n",
                                        vrRangeIt->second.first, vrRangeIt->second.second, originLow, originHigh);
                                        break;
                                    default:
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Do not support other type yet.\n");
                                        boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, vrRangeIt->second);
                                        continue;
                                }
                            } else {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, vrRangeIt->second);
                            }
                        } else {
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::GetElementPtr:
                    if (auto llvmIrGetElePtrInstruction = dyn_cast<GetElementPtrInst>(&llvmIrInstruction))
                    {
                        /*
                         * If there's a union type like:
                         * typedef union
                         * {
                         *   double value;
                         *   struct
                         *   {
                         *     __uint32_t lsw;
                         *     __uint32_t msw;
                         *   } parts;
                         * } ieee_double_shape_type;
                         *
                         * ieee_double_shape_type gh_u;
                         * gh_u.value = (d);
                         * (i) = gh_u.parts.msw;
                         *
                         * It's IR is:
                         * %9 = bitcast %union.ieee_double_shape_type* %2 to double*, !dbg !66
                         * store double %0, double* %9, align 8, !dbg !66
                         * %10 = bitcast %union.ieee_double_shape_type* %2 to %struct.anon*, !dbg !66
                         * %11 = getelementptr inbounds %struct.anon, %struct.anon* %10, i32 0, i32 1, !dbg !66
                         *
                         * Our algorithm is:
                         * 1. check if the pointer operand has been recorded in the map that contains bitcast information
                         * 2. get the value-holder bitcast instruction from the map
                         * 3. check if the range of the value-holder has been inferred
                         * 4. get the variable info from the value-holder, and cast it if necessary
                         * */
                        auto uaIt = unionAddress.find(llvmIrGetElePtrInstruction->getPointerOperand());
                        if (uaIt != unionAddress.end())
                        {
                            /*
                             * The pointer operand has been recorded in the unionAddress,
                             * so the value of this elementPtr must be gotten from the nearest bitcasts of union.
                             * */
                            auto it = std::find_if(unionAddress.rbegin(), unionAddress.rend(), [uaIt](const auto & ua){
                                auto valueHolderBitcast = dyn_cast<BitCastInst>(ua.first);
                                assert(valueHolderBitcast != nullptr);
                                auto resTypeId = valueHolderBitcast->getDestTy()->getTypeID();
                                return (ua.second == uaIt->second) && (resTypeId != Type::StructTyID);
                            });
                            if (it != unionAddress.rend())
                            {
                                auto vrRangeIt = boundInfo->virtualRegisterRange.find(it->second);
                                if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                                {
                                    double originLow = vrRangeIt->second.first;
                                    double originHigh = vrRangeIt->second.second;
                                    uint64_t originLowWord = *reinterpret_cast<uint64_t*>(&originLow);
                                    uint64_t originHighWord = *reinterpret_cast<uint64_t*>(&originHigh);
                                    double lowRange, highRange;
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tGetElementPtr: find the value holder.");
                                    auto valueHolderBitcast = dyn_cast<BitCastInst>(it->first);
                                    auto DestEleType = valueHolderBitcast->getDestTy()->getPointerElementType();
                                    // todo: is it necessary to check? or does it have other usage?
                                    unsigned dataBitWidth = DestEleType->getPrimitiveSizeInBits();
                                    /*
                                     * re-interpret the value in structure, e.g. for the example we showed below,
                                     * uint32_t lsw = static_cast<uint32_t>(long_word);
                                     * uint32_t msw = static_cast<uint32_t>(long_word >> 32);
                                     * */
                                    int element_offset = 0, pointer_offset = 0;
                                    if (llvmIrGetElePtrInstruction->getNumIndices() == 1)
                                    {
                                        /*
                                         * It's reference
                                         * */
                                        element_offset = dyn_cast<ConstantInt>(llvmIrGetElePtrInstruction->getOperand(1))->getZExtValue();
                                    }
                                    else
                                    {
                                        /*
                                         * It's array or structure
                                         * */
                                        pointer_offset = dyn_cast<ConstantInt>(llvmIrGetElePtrInstruction->getOperand(1))->getZExtValue();
                                        element_offset = dyn_cast<ConstantInt>(llvmIrGetElePtrInstruction->getOperand(2))->getZExtValue();
                                    }
                                    auto resEleTy = llvmIrGetElePtrInstruction->getResultElementType();
                                    switch (resEleTy->getTypeID())
                                    {
                                        case Type::FloatTyID:
                                            lowRange = static_cast<double>(static_cast<float>(originLowWord >> (32 * element_offset)));
                                            highRange = static_cast<double>(static_cast<float>(originHighWord >> (32 * element_offset)));
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::FloatTyID, %f - %f to %f - %f\n",
                                                      vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange));
                                            break;
                                        case Type::DoubleTyID:
                                            lowRange = static_cast<double>(originLowWord >> (32 * element_offset));
                                            highRange = static_cast<double>(originHighWord >> (32 * element_offset));
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::DoubleTyID, %f - %f to %f - %f\n",
                                                      vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange));
                                            break;
                                        case Type::IntegerTyID:
                                            switch (resEleTy->getPrimitiveSizeInBits())
                                            {
                                                case 32:
                                                    lowRange = static_cast<double>(static_cast<int32_t>(originLowWord >> (32 * element_offset)));
                                                    highRange = static_cast<double>(static_cast<int32_t>(originHighWord >> (32 * element_offset)));
                                                    break;
                                                case 64:
                                                    lowRange = static_cast<double>(static_cast<int64_t>(originLowWord));
                                                    highRange = static_cast<double>(static_cast<int64_t>(originHighWord));
                                                    break;
                                                default:
                                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::SignedInteger, don't support such bit width yet.");
                                            }

                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::IntegerTyID, %f - %f to %f - %f\n",
                                                      vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange));
                                            break;
                                        default:
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tGetElePtr: Do not support other type yet.\n");
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, vrRangeIt->second);
                                            continue;
                                    }
                                } else {
                                    assert(!valueRangeDebug && "failed to get range");
                                }
                            }
                        }
                        /*
                         * infer the range from structure or array
                         * */
                        else if (llvmIrGetElePtrInstruction->getPointerOperandType()
                                    ->getPointerElementType()->isArrayTy()) {
                            auto resVec = getGEPArrayRange(N, llvmIrGetElePtrInstruction,
                                                           boundInfo->virtualRegisterRange);
                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction,
                                                                    std::make_pair(resVec.first, resVec.second));
                        } else if (llvmIrGetElePtrInstruction->getPointerOperandType()
                                    ->getPointerElementType()->isStructTy()) {
                            // todo: get range from structure
                            assert(!valueRangeDebug && "implement when meet");
                        } else {
                            /*
                             * E.g.,
                             * %8 = getelementptr inbounds double, double* %.0, i64 2, !dbg !39
                             */
							if (auto llvmIrPHIOperand = dyn_cast<PHINode>(llvmIrGetElePtrInstruction->getPointerOperand())) {
								/*
								 * E.g.,
								 * %.0 = phi double* [ getelementptr inbounds ([5 x double], [5 x double]* @pR2, i64 0, i64 0), %3 ], [ getelementptr inbounds ([5 x double], [5 x double]* @pS2, i64 0, i64 0), %4 ], !dbg !35
								 */
								auto it = boundInfo->virtualRegisterVectorRange.find(llvmIrPHIOperand);
								if (it != boundInfo->virtualRegisterVectorRange.end())
								{
									if (auto index = dyn_cast<ConstantInt>(llvmIrGetElePtrInstruction->getOperand(1))) {
										boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, (it->second)[index->getZExtValue()]);
										break;
									}
								}
							}
							auto vrRangeIt = boundInfo->virtualRegisterRange.find(
                                    llvmIrGetElePtrInstruction->getOperand(0));
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction,
                                                                        vrRangeIt->second);
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                    }

                case Instruction::Ret:
                    if (auto llvmIrReturnInstruction = dyn_cast<ReturnInst>(&llvmIrInstruction))
                    {
                        if (llvmIrReturnInstruction->getNumOperands() != 0)
                        {
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrReturnInstruction->getOperand(0));
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                return {llvmIrReturnInstruction, vrRangeIt->second};
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                    }
                    break;
                case Instruction::FNeg:
                    if (auto llvmIrFNegInstruction = dyn_cast<UnaryOperator>(&llvmIrInstruction))
                    {
                        if (llvmIrFNegInstruction->getNumOperands() != 0)
                        {
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrFNegInstruction->getOperand(0));
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(
                                        llvmIrFNegInstruction, std::make_pair(-vrRangeIt->second.first,
                                                                              -vrRangeIt->second.second));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                    }
                    break;
                case Instruction::Br:
                    if (auto llvmIrBrInstruction = dyn_cast<BranchInst>(&llvmIrInstruction))
                    {

                    }
                    break;

                case Instruction::PHI:
                    if (auto llvmIRPhiNode = dyn_cast<PHINode>(&llvmIrInstruction)) {
                        checkPhiRange(N, llvmIRPhiNode, boundInfo);
                    }
                    break;

                case Instruction::Select:
                    break;

                case Instruction::Switch:
                    break;

                case Instruction::IndirectBr:
                case Instruction::Invoke:
                case Instruction::Resume:
                case Instruction::Unreachable:
                case Instruction::CleanupRet:
                case Instruction::CatchRet:
                case Instruction::CatchSwitch:
                case Instruction::CallBr:
                case Instruction::Fence:
                case Instruction::AtomicCmpXchg:
                case Instruction::AtomicRMW:
                case Instruction::PtrToInt:
                case Instruction::IntToPtr:
                case Instruction::AddrSpaceCast:
                case Instruction::CleanupPad:
                case Instruction::CatchPad:
                case Instruction::UserOp1:
                case Instruction::UserOp2:
                case Instruction::VAArg:
                case Instruction::ExtractElement:
                case Instruction::InsertElement:
                case Instruction::ShuffleVector:
                case Instruction::ExtractValue:
                case Instruction::InsertValue:
                case Instruction::LandingPad:
                case Instruction::Freeze:
                default:
                    continue;
            }
        }
    }
    return {nullptr, {}};
}
}
