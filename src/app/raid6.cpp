#include <iostream>
#include <algorithm>
#include <assert.h> 
#include <docopt/docopt.h>

#include <random>
#include "string_utils.h"
#include "types.h"
#include <chrono>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include "perm_dir_mongo.hpp"
#include "perm_dir_source.hpp"
#include "block_dir_device.hpp"
#include "block_dir_fs.hpp"
#include "test_pattern_source.hpp"
#include <limits>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include "libraid6.h"
using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace raid6;
using namespace std::chrono;
namespace io = boost::iostreams;

#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
mongocxx::instance instance{};
mongocxx::pool* g_dbpool = new mongocxx::pool(mongocxx::uri("mongodb://localhost/?maxPoolSize=100"));

#pragma comment(lib, "libraid6.lib")

static const char USAGE[] =
R"(raid6, software defined data redundancy tool.

    Usage:
      raid6 calculate [--config FILE]
      raid6 recover MISSING... [--config FILE]
      raid6 init [--config FILE]
      raid6 test-files (gen|check) --number NUMBER [--config FILE]
      raid6 (-h | --help)
      raid6 --version

    Options:
      -h --help     Show this screen.
      --version     Show version.
      -c --config FILE  Path to config file [default: ./config/raid6.json].
      -n --number NUMBER  Number of files to generate for test.
)";

json config;

void gen_test_files(fs::path const& root, size_t N, int seed);
void check_test_files(fs::path const& root, size_t N, int seed);
void init(std::string_view name, fs::path const& root);
void calculate();
void recover(std::vector<std::string> const& missing);

int main(int argc, const char** argv)
{
    std::map<std::string, docopt::value> args
        = docopt::docopt(USAGE,
            { argv + 1, argv + argc },
            true,               // show help if requested
            "raid6 1.0");     // version string

#ifdef _DEBUG
    for (auto const& arg : args) {
        std::cout << arg.first << ": " << arg.second << std::endl;
    }
#endif

    std::ifstream conf(args["--config"].asString());
    if (!conf.is_open()) {
        std::cerr << "Cannot find config file " << args["--config"] << std::endl;
        return 1;
    }
    conf >> config;

    if (args["test-files"].asBool()) {
        int seed = 1;
        if (args["gen"].asBool())
            for (auto const& d : config["data"]) {
                std::cout << "generating " << d["name"] << ": " << d["path"] << std::endl;
                gen_test_files(d["path"].get<std::string>(), args["--number"].asLong(), (-1)*seed++);
            }
        else if (args["check"].asBool()) 
            for (auto const& d : config["data"]) {
                std::cout << "checking " << d["name"] << ": " << d["path"] << std::endl;
                check_test_files(d["path"].get<std::string>(), args["--number"].asLong(), (-1) * seed++);
            }
    }
    if (args["init"].asBool()) {
        for (auto const& d : config["data"]) {
            std::cout << "init " << d["name"] << " from " << d["path"] << std::endl;
            init(d["name"], d["path"]);
        }
    }
    if (args["calculate"].asBool()) {
        calculate();
    }
    if (args["recover"].asBool()) {
        recover(args["MISSING"].asStringList());
    }

    return 0;
}

void gen_test_files(fs::path const& root, size_t N, int seed)
{
    double interval[] = { 986, 2055, 128128, 512 * 1024, 515 * 1024, 768 * 1024, 769 * 1024 };
    double weights[] = { .1,   .01,    .04,       .8,       .01,      .2 };
    std::piecewise_constant_distribution<> dist(
        std::begin(interval),
        std::end(interval),
        std::begin(weights));
    std::default_random_engine generator(seed);

    size_t size_all = 0;

    /*char *buffer = new char [769 * 1024];
    for (size_t s = 0; s < 769 * 1024; ++s)
        *(buffer + s) = val (size_all++);*/
    try
    {
        test::pattern_source dev;
        //io::stream<test::pattern_source> pat(dev);

        auto start_all = system_clock::now();

        for (size_t i = 0; i < N; ++i)
        {
            auto start_op = system_clock::now();

            size_t size = (size_t)dist(generator);
            auto fname = root / fmt::format("{:06d}", i / 1000) / fmt::format("{:06d}.txt", i);
            fs::create_directories(fname.parent_path());
            //std::ofstream out (fname);
            io::mapped_file_params params;
            params.path = fname.make_preferred().string();
            params.new_file_size = size;
            io::mapped_file_sink ofs1(params);
            io::stream<io::mapped_file_sink> out(ofs1);

            auto elapso = duration_cast<microseconds>(system_clock::now() - start_op);

            dev.read(ofs1.data(), size); size_all += size;

            out.flush();
            out.close();

            /*auto elaps = duration_cast<microseconds>(system_clock::now() - start_op);
            auto esec = (double)elaps.count() / 1000000.0;
            std::cout << i << ". " << esec << "s " << elapso.count() <<"us " << fname << " " << thousand_separated (size)
                << "B, " << thousand_separated ((double)size / 1024.0 / 1024.0 / esec, 2) << "MB/s" << std::endl;*/
        }

        auto end_gen = system_clock::now();

        std::cout << root.string() << " " << N << " files, " << thousand_separated(size_all) << " bytes generated.";
            //<< thousand_separated(res.first) << " bytes read, " << thousand_separated(res.second) << " bytes stream size.\n";
        auto elapsg = duration_cast<milliseconds>(end_gen - start_all).count() / 1000.0;
        // auto elapsd = duration_cast<milliseconds>(end_db - end_gen).count() / 1000.0;
        std::cout << "write speed: " << thousand_separated((double)size_all / 1024.0 / 1024.0 / elapsg);
          //  << "MB/s. db load time: " << thousand_separated(elapsd) << "sec";
        std::cout << std::endl;
        return;
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
}

void check_test_files(fs::path const& root, size_t N, int seed)
{
    double interval[] = { 986, 2055, 128128, 512 * 1024, 515 * 1024, 768 * 1024, 769 * 1024 };
    double weights[] = { .1,   .01,    .04,       .8,       .01,      .2 };
    std::piecewise_constant_distribution<> dist(
        std::begin(interval),
        std::end(interval),
        std::begin(weights));
    std::default_random_engine generator(seed);

    size_t size_all = 0;

    try
    {
        test::pattern_source             dev;
        io::stream<test::pattern_source> pat(dev);

        auto start_all = system_clock::now();

        for (size_t i = 0; i < N; ++i)
        {
            auto start_op = system_clock::now();

            size_t size = (size_t)dist(generator);
            auto fname = root / fmt::format("{:06d}", i / 1000) / fmt::format("{:06d}.txt", i);
            std::error_code ec;
            auto fsize = fs::file_size(fname, ec);
            if (ec) {
                std::cerr << "check failed. file " << fname.string() << " doesn't exists." << std::endl;
                throw new std::runtime_error("file error");
            }
            if (fsize != size) {
                std::cerr << "check failed. file " << fname.string() << " size missmatch." << std::endl;
                throw new std::runtime_error("file error");
            }

            io::mapped_file_params params;
            params.path = fname.make_preferred().string();
            params.flags = io::mapped_file::mapmode::readonly;
            io::mapped_file_source infs1(params);
            io::stream<io::mapped_file_source> in(infs1);
            
            size_t pos = 0;
            char inp;
            while (in >> inp) {
                char p;
                pat >> p;
                if (inp != p) {
                    std::cerr << "check failed. file " << fname.string() << " content missmatch at " << pos << std::endl;
                    throw new std::runtime_error("file error");
                }
                ++pos;
            }

            size_all += size;
        }

        auto end_gen = system_clock::now();

        std::cout << root.string() << " " << N << " files, " << thousand_separated(size_all) << " bytes checked.";
        //<< thousand_separated(res.first) << " bytes read, " << thousand_separated(res.second) << " bytes stream size.\n";
        auto elapsg = duration_cast<milliseconds>(end_gen - start_all).count() / 1000.0;
        // auto elapsd = duration_cast<milliseconds>(end_db - end_gen).count() / 1000.0;
        std::cout << "read speed: " << thousand_separated((double)size_all / 1024.0 / 1024.0 / elapsg);
        //  << "MB/s. db load time: " << thousand_separated(elapsd) << "sec";
        std::cout << std::endl;
        return;
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
}

void init(std::string_view name, fs::path const& root)
{
    try
    {
        auto res = perm_dir_mongo<>::init(config["db_name"], std::string(name), root);
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
}

void calculate()
{
    // data volumes
    using perm_source_t = perm_dir_source<perm_dir_mongo<>>;
    std::vector<perm_source_t> srcs;

    for (auto const& d : config["data"]) 
        srcs.emplace_back(perm_source_t(config["db_name"], d["name"], d["path"]));

    size_t max_offset = 0;
    for (auto const& src : srcs)
    {
        auto s = src.provider().size();
        assert(s % BLOCK_SIZE == 0);
        max_offset = std::max(max_offset, s);
    }

    for (auto& src : srcs)
        src.pad_end_until(max_offset);

    // parity volumes
    using parity_dev_t = block_dir_device<block_dir_fs<>>;

    try {
        for (auto const& p : config["parity"])
            block_dir_fs<>::init(p["name"], p["path"]);
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what();
    }

    std::vector<parity_dev_t> pdevs;

    for (auto const& p : config["parity"])
        pdevs.emplace_back(parity_dev_t(config["db_name"], p["name"], p["path"], max_offset));

    auto ND = srcs.size();
    auto NP = pdevs.size();

    /*RS rs;
    rs.init();*/
    encoder enc;

    std::streamsize i;
    for (i = 0; i < max_offset; i += BLOCK_SIZE)
    {
        std::vector<buff_t> buffs(ND);
        std::vector<buff_t> pbuffs(NP);

        for (size_t n = 0; n < ND; ++n)
        {
            auto& src = srcs[n];
            src.read(buffs[n].data(), BLOCK_SIZE);
        }
        for (size_t n = 0; n < NP; ++n) {
            std::memset(pbuffs[n].data(), 0, pbuffs[n].size());
        }

        //rs.rs_encode(buffs, pbuffs);
        enc.calculate(buffs, pbuffs);

        pdevs[0].write(pbuffs[0].data(), BLOCK_SIZE);
        pdevs[1].write(pbuffs[1].data(), BLOCK_SIZE);

        if (i > 0 && (i / BLOCK_SIZE) % 10000 == 0)
            std::cout << ".";
        if (i > 0 && (i / BLOCK_SIZE) % 160000 == 0)
            std::cout << std::endl;
    }

    std::cout << "\nparity calculation finished" << std::endl;
}

ptrdiff_t find_index(auto const& arr, auto val) {
    auto it = std::ranges::find(arr, val);
    if (it != arr.cend())
        return std::distance(arr.cbegin(), it);
    return -1;
}

void recover(std::vector<std::string> const& missing)
{
    using data_recovery_device_t = block_dir_device<perm_dir_mongo<>>;
    using perm_source_t = perm_dir_source<perm_dir_mongo<>>;
    using parity_dev_t = block_dir_device<block_dir_fs<>>;

    auto const& data = config["data"];
    auto const& parity = config["parity"];

    size_t ND = data.size();
    size_t NP = parity.size();

    std::vector<bool> data_exists(ND);
    std::vector<bool> parity_exists(NP);
    std::list<size_t> data_missing_idxs;
    std::list<size_t> parity_missing_idxs;

    std::vector<perm_source_t> data_srcs(ND);
    std::vector<data_recovery_device_t> data_rec(ND);

    for (size_t i = 0; i < ND; ++i) {
        std::string name = data[i]["name"];
        auto idx = find_index(missing, name);
        if (idx < 0) {
            data_exists[i] = true;
        }
        else {
            data_exists[i] = false;
            data_missing_idxs.push_back(i);
            data_rec[i] = data_recovery_device_t(config["db_name"], data[i]["name"], data[i]["path"]);
        }
        // we need them even for missing devs to query size 
        data_srcs[i] = perm_source_t(config["db_name"], data[i]["name"], data[i]["path"]);
    }

    std::vector<parity_dev_t> pdevs;

    for (size_t i = 0; i < NP; ++i) {
        std::string name = parity[i]["name"];

        auto idx = find_index(missing, name);
        if (idx < 0) {
            parity_exists[i] = true;
        }
        else {
            parity_exists[i] = false;
            parity_missing_idxs.push_back(i);
            block_dir_fs<>::init(name, parity[i]["path"]);
        }

        pdevs.emplace_back(parity_dev_t(config["db_name"], name, parity[i]["path"]));
    }

    size_t max_offset = 0;
    for (auto const& src : data_srcs)
    {
        auto s = src.provider().size();
        assert(s % BLOCK_SIZE == 0);
        max_offset = std::max(max_offset, s);
    }

    for (auto& src : data_srcs)
        src.pad_end_until(max_offset);

    decoder dec;

    std::vector<int> erasures;
    for (auto n : data_missing_idxs) erasures.push_back((int)n);
    for (auto n : parity_missing_idxs) erasures.push_back((int)n + ND);
    assert(erasures.size() > 0 && erasures.size() < 3);
    erasures.push_back(-1); // required by jerasure

    std::streamsize i;
    for (i = 0; i < max_offset; i += BLOCK_SIZE)
    {
        std::vector<buff_t> buffs(ND);
        std::vector<buff_t> pbuffs(NP);

        for (size_t n = 0; n < ND; ++n)
        {
            if (!data_exists[n]) {
#ifdef _DEBUG
                std::memset(buffs[n].data(), 0, pbuffs[n].size());
#endif
                continue;
            }
            else
            {
                auto& src = data_srcs[n];
                src.read(buffs[n].data(), BLOCK_SIZE);
            }
        }
        for (size_t n = 0; n < NP; ++n) {
            if (!parity_exists[n]) {
#ifdef _DEBUG
                std::memset(pbuffs[n].data(), 0, pbuffs[n].size());
#endif
                continue;
            }
            else
            {
                auto& src = pdevs[n];
                src.read(pbuffs[n].data(), BLOCK_SIZE);
            }
        }

        //rs.rs_encode(buffs, pbuffs);
        dec.calculate(buffs, pbuffs, erasures);

        for (auto n : data_missing_idxs) {
            auto& dst = data_rec[n];
            dst.write(buffs[n].data(), BLOCK_SIZE);
        }
        for (auto n : parity_missing_idxs) {
            auto& dst = pdevs[n];
            dst.write(pbuffs[n].data(), BLOCK_SIZE);
        }

        if (i > 0 && (i / BLOCK_SIZE) % 10000 == 0)
            std::cout << ".";
        if (i > 0 && (i / BLOCK_SIZE) % 160000 == 0)
            std::cout << std::endl;
    }

    std::cout << "\nrecovery finished" << std::endl;
}