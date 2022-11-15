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
#include <math.h>
#include <vector>

int64_t
getCount(const std::string & string, size_t position)
{
	std::string substring;
	substring = string.substr(0, position);
	substring.erase(
	    std::remove_if(substring.begin(),
			   substring.end(),
			   static_cast<int (*)(int)>(&ispunct)),
	    substring.end());
	substring.erase(
	    std::remove_if(substring.begin(),
			   substring.end(),
			   static_cast<int (*)(int)>(&isspace)),
	    substring.end());
	return std::stoi(substring);
}

std::pair<int64_t, int64_t>
processData(const std::string test_case, const std::string params)
{
	std::string line;
	size_t	    position;
	int64_t	    inst_count, time_consumption;

	// perf command
	std::string cmd = "make " + test_case;
	system(cmd.data());
	cmd.clear();
	cmd = "perf stat -B ./main_out " + params;

	cmd += "if=/dev/zero of=/dev/null count=1000000";
	cmd += " 2>&1 | tee tmp.log";
	system(cmd.data());
	std::ifstream ifs("tmp.log");
	if (!ifs.is_open())
	{
		std::cout << "error opening tmp.log";
		assert(false);
	}

	// process
	while (getline(ifs, line))
	{
		position = line.find("instructions");
		if (position != std::string::npos)
		{
			inst_count = getCount(line, position);
		}
		position = line.find("seconds time elapsed");
		if (position != std::string::npos)
		{
			time_consumption = getCount(line, position);
			continue;
		}
	}

	printf("%lu\t%lu\n", inst_count, time_consumption);

	ifs.close();

	return std::make_pair(inst_count, time_consumption);
}

std::string
change_nt_range(const std::string & cmd1, const std::string & cmd2, const std::vector<double> & params)
{
	std::string param_str;
	std::string change_nt_cmd;
	param_str.clear();
	change_nt_cmd.clear();
	change_nt_cmd = cmd1;
	// prepare parameters
	for (const auto & pp : params)
	{
		param_str += std::to_string(pp) + " ";
		change_nt_cmd += std::to_string(pp) + " mjf, ";
	}

	change_nt_cmd.erase(change_nt_cmd.end() - 2);
	change_nt_cmd += cmd2;
	system(change_nt_cmd.data());

	return param_str;
}

int64_t
exactNumber()
{
	std::ifstream ifs("tmp.log");
	if (!ifs.is_open())
	{
		std::cout << "error opening tmp.log";
		assert(false);
	}

	// process
	std::string line;
	std::getline(ifs, line);
	auto it = std::remove_if(line.begin(), line.end(), [](const char & c) {
		return !std::isdigit(c);
	});

	line.erase(it, line.end());

	ifs.close();

	char * pEnd;

	return std::strtol(line.data(), &pEnd, 10);
}

int64_t
getIrLines()
{
	std::string cmd = "wc -l out.ll 2>&1 | tee tmp.log";
	system(cmd.data());

	return exactNumber();
}

int64_t
getLibSize()
{
	std::string cmd = "wc -c libout.a 2>&1 | tee tmp.log";
	system(cmd.data());

	return exactNumber();
}

struct perfData {
	int64_t inst_count_avg;
	int64_t time_consumption_avg;
	int64_t ir_lines;
	int64_t library_size;
};

struct perfData
recordData(const std::string & test_cases, const std::string & param_str, std::ofstream & ofs)
{
#ifdef DEBUG_TS
	const size_t iteration_num = 1;
#else
	const size_t			 iteration_num = 5;
#endif

	perfData perf_data = {0, 0, 0, 0};

	for (size_t idx = 0; idx < iteration_num; idx++)
	{
		const std::pair<int64_t, int64_t> inst_time_data = processData(test_cases, param_str);
		perf_data.inst_count_avg += (inst_time_data.first / 1000);
		perf_data.time_consumption_avg += (inst_time_data.second / 1000);
	}
	perf_data.inst_count_avg /= iteration_num;
	perf_data.time_consumption_avg /= iteration_num;

	perf_data.ir_lines     = getIrLines();
	perf_data.library_size = getLibSize();

	ofs << test_cases << "\t" << param_str << "\t" << perf_data.inst_count_avg
	    << "\t" << perf_data.time_consumption_avg << "\t" << perf_data.ir_lines << "\t" << perf_data.library_size << std::endl;

	return perf_data;
}

int
main(int argc, char ** argv)
{
	std::vector<std::string> test_cases{
	    "perf_exp", "perf_log",
	    "perf_acosh", "perf_j0",
	    "perf_y0", "perf_rem_pio2", "perf_sincosf",
	    "perf_float64_add", "perf_float64_div",
	    "perf_float64_mul"};

	if (argc >= 2)
	{
		test_cases.clear();
		test_cases.emplace_back(argv[1]);
	}

	std::ofstream ofs("perf.log");
	if (!ofs.is_open())
	{
		std::cout << "error opening perf.log";
		return -1;
	}

	std::ofstream avg_speedup("average_speedup.log");
	if (!avg_speedup.is_open())
	{
		std::cout << "error opening perf.log";
		return -1;
	}

#ifdef DEBUG_TS
	std::vector<std::vector<double>> normalParameters{
	    {0.2, 0.8},
	};
	std::vector<double> range_extend{1};
#else
	std::vector<std::vector<double>> normalParameters{
	    {-1000.3, -999.2},
	    {-134.5, -133.8},
	    {-23.9, -23.1},
	    {-5.4, -4.5},
	    {-0.9, -0.4},
	    {0.2, 0.8},
	    {9.7, 10.5},
	    {35.75, 36.03},
	    {476.84, 477.21},
	    {999.8, 1000.9}};
	std::vector<double> range_extend{1, 10, 100, 1000, 10000, 100000};
#endif

	if (argc == 3)
	{
		range_extend.clear();
		range_extend.emplace_back(strtod(argv[2], nullptr));
	}

	std::vector<std::vector<double>> trigonometricParams{
	    {0, 0.17453292519943295},		       // (0, pi/18)
	    {0.6981317007977318, 0.8726646259971648},  // (2pi/9, 5pi/18)
	    {1.3962634015954636, 1.5707963267948966},  // (4pi/9, pi/2)
	    {2.0943951023931953, 2.2689280275926285},  // (2pi/3, 13pi/18)
	    {2.792526803190927, 2.9670597283903604},   // (8pi/9, 17pi/18)
	    {3.490658503988659, 3.665191429188092},    // (10pi/9, 7pi/6)
	    {4.1887902047863905, 4.363323129985824},   // (8pi/6, 25pi/18)
	    {4.886921905584122, 5.061454830783556},    // (14pi/9, 29pi/18)
	    {5.585053606381854, 5.759586531581288},    // (16pi/9, 33pi/18)
	    {5.934119456780721, 6.1086523819801535}    // (17pi/9, 35pi/18)
	};

	if (argc == 4)
	{
		normalParameters.clear();
		std::vector<double> input_param{strtod(argv[2], nullptr), strtod(argv[3], nullptr)};
		normalParameters.emplace_back(input_param);

		trigonometricParams.clear();
		trigonometricParams.emplace_back(input_param);
	}

	ofs << "test case\tparam\tinstruction count\ttime consumption\tir lines\tlibrary size" << std::endl;
	avg_speedup << "test cast\textend\tinstruction count\ttime consumption\tir lines\tlibrary size" << std::endl;

	for (size_t case_id = 0; case_id < test_cases.size(); case_id++)
	{
		int				       avg_inst_speedup	   = 0;
		int				       avg_time_speedup	   = 0;
		int				       avg_ir_reduce	   = 0;
		int				       avg_lib_size_reduce = 0;
		const std::vector<std::vector<double>> parameters =
		    test_cases[case_id] == "perf_float64_sin" ? trigonometricParams : normalParameters;
		for (const auto & extend : range_extend)
		{
			for (const auto & p : parameters)
			{
				const std::string param_str = change_nt_range("sed -i 's/3 mjf, 10 mjf/",
									      "/g' ../../sensors/test.nt",
									      {p.front(), p.back() - 1 + extend});
				const double	  p1	    = p.front() + 0.6;
				const double	  p2	    = p.back() + 0.3;
				change_nt_range("sed -i 's/1 mjf, 16 mjf/", "/g' ../../sensors/test.nt", {p1, p2 - 1 + extend});

				perfData ori_perf_data = recordData(test_cases[case_id], param_str, ofs);
				perfData opt_perf_data = recordData(test_cases[case_id] + "_opt", param_str, ofs);

				int inst_speedup    = round((ori_perf_data.inst_count_avg - opt_perf_data.inst_count_avg) * 100 / opt_perf_data.inst_count_avg);
				int time_speedup    = round((ori_perf_data.time_consumption_avg - opt_perf_data.time_consumption_avg) * 100 / opt_perf_data.time_consumption_avg);
				int ir_reduce	    = round((ori_perf_data.ir_lines - opt_perf_data.ir_lines) * 100 / opt_perf_data.ir_lines);
				int lib_size_reduce = round((ori_perf_data.library_size - opt_perf_data.library_size) * 100 / opt_perf_data.library_size);
				ofs << "speed up after optimization\t" << param_str << "\t" << inst_speedup << "%\t" << time_speedup << "%\t"
				    << ir_reduce << "%\t" << lib_size_reduce << "%" << std::endl;

				avg_inst_speedup += inst_speedup;
				avg_time_speedup += time_speedup;
				avg_ir_reduce += ir_reduce;
				avg_lib_size_reduce += lib_size_reduce;

				// reset test.nt
				change_nt_range("sed -i 's/", "/3 mjf, 10 mjf/g' ../../sensors/test.nt",
						{p.front(), p.back() - 1 + extend});
				change_nt_range("sed -i 's/", "/1 mjf, 16 mjf/g' ../../sensors/test.nt", {p1, p2 - 1 + extend});
			}
			avg_inst_speedup    = round(avg_inst_speedup / parameters.size());
			avg_time_speedup    = round(avg_time_speedup / parameters.size());
			avg_ir_reduce	    = round(avg_ir_reduce / parameters.size());
			avg_lib_size_reduce = round(avg_lib_size_reduce / parameters.size());
			avg_speedup << test_cases[case_id] << "\t" << extend << "\t" << avg_inst_speedup << "%\t"
				    << avg_time_speedup << "%\t" << avg_ir_reduce << "%\t" << avg_lib_size_reduce << "%" << std::endl;

			if (test_cases[case_id] == "perf_float64_sin")
			{
				// trigonometricParams cannot have extend
				break;
			}
		}
	}

	ofs.close();

	return 0;
}
