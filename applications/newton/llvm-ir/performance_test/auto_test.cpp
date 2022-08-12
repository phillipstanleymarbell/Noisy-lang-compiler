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

std::pair<uint64_t, uint64_t> processData(std::string test_case, std::string params) {
    std::string line;
    size_t position;
    uint64_t inst_count, time_consumption;

    // perf command
    std::string cmd = "make " + test_case;
    system(cmd.data());
    cmd.clear();
    cmd = "cset shield --exec -- perf stat -B ./main_out " + params;

    cmd += "if=/dev/zero of=/dev/null count=1000000";
    cmd += " 2>&1 | tee tmp.log";
    system(cmd.data());
    std::ifstream ifs("tmp.log");
    if (!ifs.is_open()) {
        std::cout << "error opening tmp.log";
        assert(false);
    }

    // process
    while (getline(ifs, line)) {
        position = line.find("instructions");
        if (position != std::string::npos) {
            inst_count = getCount(line, position);
        }
        position = line.find("seconds time elapsed");
        if (position != std::string::npos) {
            time_consumption = getCount(line, position);
            continue;
        }
    }

    printf("%lu\t%lu\n", inst_count, time_consumption);

    ifs.close();

    return std::make_pair(inst_count, time_consumption);
}

std::string change_nt_range(std::string cmd1, std::string cmd2, std::vector<double> params) {
    std::string param_str;
    std::string change_nt_cmd;
    param_str.clear();
    change_nt_cmd.clear();
    change_nt_cmd = cmd1;
    // prepare parameters
    for (auto pp : params) {
        param_str += std::to_string(pp) + " ";
        change_nt_cmd += std::to_string(pp) + " mjf, ";
    }

    change_nt_cmd.erase(change_nt_cmd.end() - 2);
    change_nt_cmd += cmd2;
    system(change_nt_cmd.data());

    return param_str;
}

uint64_t exactNumber() {
    std::ifstream ifs("tmp.log");
    if (!ifs.is_open()) {
        std::cout << "error opening tmp.log";
        assert(false);
    }

    // process
    std::string line;
    std::getline(ifs, line);
    auto it = std::remove_if(line.begin(), line.end(), [](const char &c){
        return !std::isdigit(c);
    });

    line.erase(it, line.end());

    ifs.close();

    char* pEnd;

    return std::strtol(line.data(), &pEnd, 10);
}

uint64_t getIrLines() {
    std::string cmd = "wc -l out.ll 2>&1 | tee tmp.log";
    system(cmd.data());

    return exactNumber();
}

uint64_t getLibSize() {
    std::string cmd = "wc -c libout.a 2>&1 | tee tmp.log";
    system(cmd.data());

    return exactNumber();
}

struct perfData {
    uint64_t inst_count_avg;
    uint64_t time_consumption_avg;
    uint64_t ir_lines;
    uint64_t library_size;
};

struct perfData recordData(std::string test_cases, std::string param_str, std::ofstream& ofs) {
    const size_t iteration_num = 5;

    perfData perf_data = {0, 0, 0, 0};

    for (size_t idx = 0; idx < iteration_num; idx++) {
        const std::pair<uint64_t, uint64_t> inst_time_data = processData(test_cases, param_str);
        perf_data.inst_count_avg += (inst_time_data.first/1000);
        perf_data.time_consumption_avg += (inst_time_data.second/1000);
    }
    perf_data.inst_count_avg /= iteration_num;
    perf_data.time_consumption_avg /= iteration_num;

    perf_data.ir_lines = getIrLines();
    perf_data.library_size = getLibSize();

    ofs << test_cases << "\t" << param_str << "\t" << perf_data.inst_count_avg
        << "\t" << perf_data.time_consumption_avg << "\t" << perf_data.ir_lines << "\t" << perf_data.library_size << std::endl;

    return perf_data;
}

int main(int argc, char** argv) {
    std::vector<std::string> test_cases{
            "perf_exp", "perf_log",
            "perf_acosh", "perf_j0",
            "perf_y0", "perf_sincosf",
            "perf_float64_add", "perf_float64_div",
            "perf_float64_mul", "perf_float64_sin"};

    if (argc == 2) {
        test_cases.clear();
        test_cases.emplace_back(argv[1]);
    }

    std::ofstream ofs("perf.log");
    if (!ofs.is_open()) {
        std::cout << "error opening perf.log";
        return -1;
    }

    const std::vector<std::vector<double>> parameters{
            {-1000.3, -999.2},
            {-134.5, -133.8},
            {-23.9, -23.1},
            {-5.4, -4.5},
            {-1.1, -0.6},
            {0.2, 0.9},
            {9.7, 10.5},
            {35.75, 36.03},
            {476.84, 477.21},
            {999.8, 1000.9}
    };

    ofs << "test case\tparam\tinstruction count\ttime consumption\tir lines\tlibrary size" << std::endl;

    for (size_t case_id = 0; case_id < test_cases.size(); case_id++) {
        for (auto p : parameters) {
            const std::string param_str = change_nt_range("sed -i 's/3 mjf, 10 mjf/", "/g' ../../sensors/test.nt", p);
            const double p1 = p.front() + 3.2;
            const double p2 = p.back() + 2.9;
            change_nt_range("sed -i 's/1 mjf, 16 mjf/", "/g' ../../sensors/test.nt", {p1, p2});

            perfData ori_perf_data = recordData(test_cases[case_id], param_str, ofs);
            perfData opt_perf_data = recordData(test_cases[case_id] + "_opt", param_str, ofs);

            float inst_speedup = (ori_perf_data.inst_count_avg - opt_perf_data.inst_count_avg) * 100 / opt_perf_data.inst_count_avg;
            float time_speedup = (ori_perf_data.time_consumption_avg - opt_perf_data.time_consumption_avg) * 100 / opt_perf_data.time_consumption_avg;
            float ir_reduce = (ori_perf_data.ir_lines - opt_perf_data.ir_lines) * 100 / opt_perf_data.ir_lines;
            float lib_size_reduce = (ori_perf_data.library_size - opt_perf_data.library_size) * 100 / opt_perf_data.library_size;
            ofs << "speed up after optimization\t" << inst_speedup << "%\t" << time_speedup << "%\t"
                << ir_reduce << "%\t" << lib_size_reduce << "%" << std::endl;

            // reset test.nt
            change_nt_range("sed -i 's/", "/3 mjf, 10 mjf/g' ../../sensors/test.nt", p);
            change_nt_range("sed -i 's/", "/1 mjf, 16 mjf/g' ../../sensors/test.nt", {p1, p2});
        }
    }

    ofs.close();

    return 0;
}
