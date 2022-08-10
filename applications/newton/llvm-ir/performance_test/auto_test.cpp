//
// Created by pei on 30/07/22.
//

#include <algorithm>
#include <assert.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <vector>

uint64_t getCount(std::string string, size_t position) {
    std::string substring;
    substring = string.substr(0, position);
    substring.erase(
            std::remove_if(substring.begin(),
                           substring.end(),
                           static_cast<int(*)(int)>(&ispunct)),
            substring.end());
    substring.erase(
            std::remove_if(substring.begin(),
                           substring.end(),
                           static_cast<int(*)(int)>(&isspace)),
            substring.end());
    return std::stoi(substring);
}

void processData(std::string test_case,
                 uint64_t* inst_count,
                 uint64_t* time_consumption) {
    std::string line;
    size_t position;

    // perf command
    std::string cmd = "sudo make " + test_case + " 2>&1 | tee test.log";
    system(cmd.data());
    std::ifstream ifs("test.log");
    if (!ifs.is_open()) {
        std::cout << "error opening test.log";
        assert(false);
    }

    // process
    while (getline(ifs, line)) {
        position = line.find("instructions");
        if (position != std::string::npos) {
            *inst_count = getCount(line, position);
        }
        position = line.find("seconds time elapsed");
        if (position != std::string::npos) {
            *time_consumption = getCount(line, position);
            continue;
        }
    }

    ifs.close();

    return;
}

int main() {
    const std::vector<std::string> test_cases{
            "perf_exp", "perf_log",
            "perf_acosh", "perf_j0",
            "perf_y0", "perf_sincosf",
            "perf_float64_add", "perf_float64_div",
            "perf_float64_mul", "perf_float64_sin"};

    const size_t iteration_num = 2;

    std::ofstream ofs("perf.log");
    if (!ofs.is_open()) {
        std::cout << "error opening perf.log";
        return -1;
    }

    std::vector<uint64_t> inst_count_avg(test_cases.size());
    std::vector<uint64_t> time_consumption_avg(test_cases.size());
    std::vector<uint64_t> opt_inst_count_avg(test_cases.size());
    std::vector<uint64_t> opt_time_consumption_avg(test_cases.size());

    for (size_t case_id = 0; case_id < test_cases.size(); case_id++) {
        uint64_t inst_count = 0;
        uint64_t time_consumption = 0;

        // non-opt
        for (size_t idx = 0; idx < iteration_num; idx++) {
            processData(test_cases[case_id], &inst_count, &time_consumption);
            inst_count_avg[case_id] += (inst_count/1000);
            time_consumption_avg[case_id] += (time_consumption/1000);
        }
        inst_count_avg[case_id] /= iteration_num;
        time_consumption_avg[case_id] /= iteration_num;

        ofs << test_cases[case_id] << ":\tnone opt instruction count: " << inst_count_avg[case_id]
            << " \ttime consumption: " << time_consumption_avg[case_id] << " um" << std::endl;

        // opt
        for (size_t idx = 0; idx < iteration_num; idx++) {
            processData(test_cases[case_id] + "_opt", &inst_count, &time_consumption);
            opt_inst_count_avg[case_id] += (inst_count/1000);
            opt_time_consumption_avg[case_id] += (time_consumption/1000);
        }
        opt_inst_count_avg[case_id] /= iteration_num;
        opt_time_consumption_avg[case_id] /= iteration_num;

        float inst_speedup = (inst_count_avg[case_id] - opt_inst_count_avg[case_id]) * 100 / inst_count_avg[case_id];
        float time_speedup = (time_consumption_avg[case_id] - opt_time_consumption_avg[case_id]) * 100 / time_consumption_avg[case_id];
        ofs << test_cases[case_id] << "_opt:\t instruction count: " << opt_inst_count_avg[case_id]
            << ", speed up " << inst_speedup << "%\n"
            << " \ttime consumption: " << opt_time_consumption_avg[case_id] << " um"
            << " speed up " << time_speedup << "%" << std::endl;
    }

    ofs.close();

    return 0;
}