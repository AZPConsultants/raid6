#include <filesystem>
#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <future>
#include <numeric>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include "test_pattern_source.hpp"
#include "string_utils.h"
#include <fmt/format.h>
#include "table_printer.h"
#define NOMINMAX
#include <Windows.h>
#include "perm_dir_source.hpp"
#include "perm_dir_mongo.hpp"
#include "perm_dir_sqlite.hpp"
#include "block_dir_device.hpp"
#include "block_dir_fs.hpp"
#include "types.h"
#include "rs.h"

#include "schifra_galois_field.hpp"
#include "schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra_reed_solomon_encoder.hpp"
#include "schifra_reed_solomon_file_encoder.hpp"

#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
/*
mongocxx::instance instance{};
mongocxx::pool* g_dbpool = new mongocxx::pool (mongocxx::uri ("mongodb://localhost/?maxPoolSize=100"));
*/

#define NW(w) (1 << w) /* NW equals 2 to the w-th power */

using namespace ds;
using namespace std::chrono;
namespace fs = std::filesystem;
namespace io = boost::iostreams;

void test_write_speed ();
void test_read ();
void test_block_dir ();

void test_schifra ();

int main (int argc, const char** argv)
{
	if (false)
	{
		test_write_speed ();

		auto load = [](std::string const& id, std::string const& root)
		{
			auto start_db = system_clock::now ();
			auto res = ds::perm_dir_sqlite<>::init (id, root);
			auto end_db = system_clock::now ();

			std::cout << "c:/test " << thousand_separated (res.first) << " bytes read, " << thousand_separated (res.second) << " bytes stream size.\n";
			auto elapsd = duration_cast<milliseconds>(end_db - start_db).count () / 1000.0;
			std::cout << "db load time: " << thousand_separated (elapsd) << "sec" << std::endl;
		};

		load ("test0", "c:/test0");
		load ("test1", "c:/test1");
		load ("test2", "c:/test2");
		load ("test3", "c:/test3");
	}
	
	//test_read ();

	//test_block_dir ();

	test_schifra ();

	return 0;
}


void test_block_dir ()
{
	

	// data volumes
	using perm_source_t = perm_dir_source<perm_dir_sqlite<>>;
	std::vector<perm_source_t> srcs;

	srcs.emplace_back (perm_source_t ("test0", "c:/test0"));
	srcs.emplace_back (perm_source_t ("test1", "c:/test1"));
	srcs.emplace_back (perm_source_t ("test2", "c:/test2"));
	srcs.emplace_back (perm_source_t ("test3", "c:/test3"));

	size_t max_offset = 0;
	for (auto const& src : srcs)
	{
		auto s = src.provider ().size ();
		assert (s % BLOCK_SIZE == 0);
		max_offset = std::max (max_offset, s);
	}

	for (auto& src : srcs)
		src.pad_end_until (max_offset);

	// parity volumes
	using parity_dev_t = block_dir_device<block_dir_fs<>>;

	try {
		block_dir_fs<>::init ("testp", "c:/testp");
		block_dir_fs<>::init ("testq", "c:/testq");
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what ();
	}

	std::vector<parity_dev_t> pdevs;


	pdevs.emplace_back (parity_dev_t ("testp", "c:/testp", max_offset));
	pdevs.emplace_back (parity_dev_t ("testq", "c:/testq", max_offset));

	auto ND = srcs.size();
	auto NP = pdevs.size();

	RS rs;
	rs.init();

	std::streamsize i;
	for (i = 0; i < max_offset; i += BLOCK_SIZE)
	{
		std::vector<buff_t> buffs (ND);
		std::vector<buff_t> pbuffs (NP);


		for (size_t n = 0; n < ND; ++n)
		{
			auto& src = srcs[n];
			src.read (buffs[n].data (), BLOCK_SIZE);
		}
		for (size_t n = 0; n < NP; ++n) {
			std::memset(pbuffs[n].data(), 0, pbuffs[n].size());
		}

		rs.rs_encode(buffs, pbuffs);
		

		pdevs[0].write (pbuffs[0].data (), BLOCK_SIZE);
		pdevs[1].write (pbuffs[1].data (), BLOCK_SIZE);

		if (i > 0 && (i / BLOCK_SIZE) % 10000 == 0)
			std::cout << ".";
		if (i > 0 && (i / BLOCK_SIZE) % 160000 == 0)
			std::cout << std::endl;
	}

	std::cout << "\nparity calculation finished" << std::endl;
}

void test_read ()
{
	const size_t BLOCK_SIZE = 4096;

	using perm_source_t = perm_dir_source<perm_dir_sqlite<>>;
	std::vector<perm_source_t> srcs;

	srcs.emplace_back (perm_source_t ("test0", "c:/test0"));
	srcs.emplace_back (perm_source_t ("test1", "c:/test1"));
	srcs.emplace_back (perm_source_t ("test2", "c:/test2"));
	srcs.emplace_back (perm_source_t ("test3", "c:/test3"));

	size_t max_offset = 0;
	for (auto const& src : srcs)
	{
		auto s = src.provider ().size ();
		assert (s % BLOCK_SIZE == 0);
		max_offset = std::max (max_offset, s);
	}

	for (auto& src : srcs)
		src.pad_end_until (max_offset);

	test::pattern_source pat;

	std::vector<size_t> pos;
	for (size_t n = 0; n < srcs.size (); ++n)
		pos.emplace_back (0);

	std::streamsize i;
	for (i = 0; i < max_offset; i+=BLOCK_SIZE)
	{
		for (size_t n = 0; n < srcs.size (); ++n)
		{
			/// every file starts and ends on a mod BLOCK_SIZE address,
			/// therefore one BLOCK_SIZE read (starting from 0) can only  
			/// contain data from one file.
			auto& src = srcs[n];
			auto& p = pos[n];
			std::array<char, BLOCK_SIZE> sbuf;
			std::array<char, BLOCK_SIZE> pbuf;

			auto read = src.read (sbuf.data (), BLOCK_SIZE);
			assert (read == BLOCK_SIZE);
			auto curr_file = src.curr_file ();
			assert (curr_file != file_record ());

			pat.seek (p, std::ios_base::beg);
			pat.read (pbuf.data (), BLOCK_SIZE);

			// match read data with generated pattern
			std::streamsize ins = 0; std::streamsize inp = 0;
			size_t to_check = BLOCK_SIZE;
			while (to_check > 0)
			{
				std::streamsize len = std::min ((std::streamsize)BLOCK_SIZE - ins, curr_file.end - i - ins);
				if (len > 0)
				{
					auto res = std::memcmp (sbuf.data () + ins, pbuf.data () + inp, len);
					assert (res == 0);
					ins += len; inp += len;
					to_check -= len; p += len;
				}
				else
				{
					len = std::min ((std::streamsize)BLOCK_SIZE - ins, curr_file.pad_end - i);
					for (size_t pad = 0; pad < len; ++pad) {
						assert (sbuf[ins + pad] == 0);
					}
					ins += len; 
					to_check -= len; 
				}
			}
		}
		if (i>0 && (i / BLOCK_SIZE) % 10000 == 0)
			std::cout << ".";
		if (i>0 && (i / BLOCK_SIZE) % 160000 == 0)
			std::cout << std::endl;
	}

	std::cout << "\nread test finished" << std::endl;
}

/// write speed tests and test file generators
struct sptask
{
	fs::path path;
	size_t	 size;
};

struct spres
{
	double subop1_sec{ 0 };
	double all_sec{ 0 };
	double mbs{ 0 };
};

struct sptestres
{
	std::string comment;
	double avg_speed{ 0 };
	double dev_speed{ 0 };
	double avg_subop1{ 0 };
	double dev_subop1{ 0 };
	double tot_speed{ 0 };
};

std::vector<sptask> tasks;
std::vector<spres>  speeds;

void setup_write_test (size_t N, fs::path const& root, int seed)
{
	double interval[] = { 986, 2055, 128128, 512 * 1024, 515 * 1024, 768 * 1024, 769 * 1024 };
	double weights[] = { .1,   .01,    .04,       .8,       .01,      .2 };
	std::piecewise_constant_distribution<> dist (
		std::begin (interval),
		std::end (interval),
		std::begin (weights));
	std::default_random_engine generator (seed);

	tasks.clear ();
	for (size_t i = 0; i < N; ++i)
	{
		size_t size = (size_t)dist (generator);
		auto fname = root / fmt::format ("{:06d}", i / 1000) / fmt::format ("{:06d}.txt", i);
		fs::create_directories (fname.parent_path ());
		tasks.emplace_back (sptask{ fname, size });
	}

	speeds.clear ();
	speeds.resize (N);
}

sptestres analize_test (double all_bytes, double secs)
{
	sptestres res;

	for (auto const& sp : speeds)
	{
		res.avg_subop1 += sp.subop1_sec;
		res.avg_speed += sp.mbs;
	}

	res.avg_subop1 /= (double)speeds.size ();
	res.avg_speed /= (double)speeds.size ();

	std::vector<double> diff_subop (speeds.size ());
	double avg = res.avg_subop1;
	std::transform (speeds.begin (), speeds.end (), diff_subop.begin (), [avg](auto const& sp) { return sp.subop1_sec - avg; });
	double sq_sum = std::inner_product (diff_subop.begin (), diff_subop.end (), diff_subop.begin (), 0.0);
	res.dev_subop1 = std::sqrt (sq_sum / speeds.size ());

	std::vector<double> diff_sp (speeds.size ());
	avg = res.avg_speed;
	std::transform (speeds.begin (), speeds.end (), diff_sp.begin (), [avg](auto const& sp) { return sp.mbs - avg; });
	sq_sum = std::inner_product (diff_sp.begin (), diff_sp.end (), diff_sp.begin (), 0.0);
	res.dev_speed= std::sqrt (sq_sum / speeds.size ());

	res.tot_speed = all_bytes / 1024.0 / 1024.0 / secs;

	return res;
}

size_t test_windows_mapped (bool use_pattern)
{
	size_t N = tasks.size ();
	size_t size_all = 0;

	test::pattern_source	dev;

	for (size_t i = 0; i < N; ++i)
	{
		auto start = system_clock::now ();
		auto size = tasks[i].size;

		HANDLE hFile = CreateFile (
			tasks[i].path.string ().c_str (),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			std::cerr << "File open error " << __LINE__ << std::endl;
			return 0;
		}

		LARGE_INTEGER lis;
		lis.QuadPart = size;

		HANDLE hMap = CreateFileMapping (
			hFile,
			NULL,
			PAGE_READWRITE,
			lis.HighPart,
			lis.LowPart,
			NULL);

		if (hMap == NULL)
		{
			CloseHandle (hFile);
			std::cerr << "File map error " << __LINE__ << std::endl;
			return 0;
		}

		void *vpMapped = MapViewOfFile (
			hMap,
			FILE_MAP_ALL_ACCESS,
			0,
			0,
			0);

		if (vpMapped == NULL)
		{
			CloseHandle (hMap);
			CloseHandle (hFile);
			std::cerr << "File map view error " << __LINE__ << std::endl;
			return 0;
		}

		speeds[i].subop1_sec = (double)duration_cast<microseconds>(system_clock::now () - start).count () / 1'000'000.0;

		// fill the file
		if (!use_pattern)
			std::memset (vpMapped, '0', size);
		else
			dev.read ((char*)vpMapped, size);
		
		// close handles
		//FlushViewOfFile (vpMapped, 0);
		UnmapViewOfFile (vpMapped);
		vpMapped = NULL;
		CloseHandle (hMap);
		hMap = INVALID_HANDLE_VALUE;
		FlushFileBuffers (hFile);
		CloseHandle (hFile);
		hFile = INVALID_HANDLE_VALUE;

		speeds[i].all_sec = (double)duration_cast<microseconds>(system_clock::now () - start).count () / 1'000'000.0;
		speeds[i].mbs = size / 1024.0 / 1024.0 / speeds[i].all_sec;

		size_all += size;
	}
	
	return size_all;
}

size_t test_windows_seq ()
{
	size_t N = tasks.size ();
	size_t size_all = 0;

	test::pattern_source	dev;
	const std::streamsize BUFF_SIZE = 4096;
	char *buffer = new char[BUFF_SIZE];

	for (size_t i = 0; i < N; ++i)
	{
		auto start = system_clock::now ();
		auto size = tasks[i].size;

		HANDLE hFile = CreateFile (
			tasks[i].path.string ().c_str (),
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			std::cerr << "File open error " << __LINE__ << std::endl;
			return 0;
		}

		speeds[i].subop1_sec = (double)duration_cast<microseconds>(system_clock::now () - start).count () / 1'000'000.0;

		// fill the file
		std::streamsize done = 0;
		std::streamsize left = (std::streamsize)size;
		while (done < (std::streamsize)size)
		{
			DWORD l = std::min (left, BUFF_SIZE);
			dev.read (buffer, l);
			done += l;
			left -= l;

			DWORD written;
			WriteFile (hFile, buffer, l, &written, NULL);
		}

		// close handles
		FlushFileBuffers (hFile);
		CloseHandle (hFile);
		hFile = INVALID_HANDLE_VALUE;

		speeds[i].all_sec = (double)duration_cast<microseconds>(system_clock::now () - start).count () / 1'000'000.0;
		speeds[i].mbs = size / 1024.0 / 1024.0 / speeds[i].all_sec;

		size_all += size;
	}

	delete [] buffer;
	return size_all;
}

size_t test_mapped_sink ()
{
	size_t N = tasks.size ();
	size_t size_all = 0;

	test::pattern_source	dev;

	for (size_t i = 0; i < N; ++i)
	{
		auto start = system_clock::now ();
		auto size = tasks[i].size;

		io::mapped_file_params params;
		params.path = tasks[i].path.string ().c_str ();
		params.new_file_size = size;
		io::mapped_file_sink ofs1 (params);

		speeds[i].subop1_sec = (double)duration_cast<microseconds>(system_clock::now () - start).count () / 1'000'000.0;

		// fill the file
		dev.read (ofs1.data (), size);

		// close handles
		ofs1.close ();

		speeds[i].all_sec = (double)duration_cast<microseconds>(system_clock::now () - start).count () / 1'000'000.0;
		speeds[i].mbs = size / 1024.0 / 1024.0 / speeds[i].all_sec;

		size_all += size;
	}

	return size_all;
}

io::mapped_file_sink open_mapped_file (size_t i)
{
	io::mapped_file_params params;
	params.path = tasks[i].path.string ().c_str ();
	params.new_file_size = tasks[i].size;
	return io::mapped_file_sink (params);
}

size_t test_mapped_sink_async ()
{
	size_t N = tasks.size ();
	size_t size_all = 0;

	try
	{
		test::pattern_source	dev;

		auto ofs1 = open_mapped_file (0);

		for (size_t i = 0; i < N; ++i)
		{
			auto start = system_clock::now ();

			// async opening next file
			std::future<io::mapped_file_sink> fut;
			if (i + 1 < N)
				fut = std::async (std::launch::async, open_mapped_file, i + 1);

			auto size = tasks[i].size;

			speeds[i].subop1_sec = (double)duration_cast<microseconds>(system_clock::now () - start).count () / 1'000'000.0;

			// fill the file
			dev.read (ofs1.data (), size);

			// close handles
			
			ofs1.close ();

			size_all += size;

			if (i + 1 < N)
			{
				ofs1 = fut.get ();
				if (!ofs1.is_open ())
				{
					std::cerr << "file open error" << std::endl;
					return 0;
				}
			}

			speeds[i].all_sec = (double)duration_cast<microseconds>(system_clock::now () - start).count () / 1'000'000.0;
			speeds[i].mbs = size / 1024.0 / 1024.0 / speeds[i].all_sec;
		}
	}
	catch (std::exception& ex) {
		std::cerr << ex.what () << std::endl;
		return 0;
	}

	return size_all;
}

size_t test_stream ()
{
	size_t N = tasks.size ();
	size_t size_all = 0;

	test::pattern_source	dev;
	io::stream<test::pattern_source> pat (dev);

	for (size_t i = 0; i < N; ++i)
	{
		auto start = system_clock::now ();
		auto size = tasks[i].size;

		std::ofstream out (tasks[i].path.string ());
		
		speeds[i].subop1_sec = (double)duration_cast<microseconds>(system_clock::now () - start).count () / 1'000'000.0;

		// fill the file
		pat->next_eof (size);
		out << pat.rdbuf (); 

		// close handles
		out.flush ();
		out.close ();

		speeds[i].all_sec = (double)duration_cast<microseconds>(system_clock::now () - start).count () / 1'000'000.0;
		speeds[i].mbs = size / 1024.0 / 1024.0 / speeds[i].all_sec;

		size_all += size;
	}

	return size_all;
}



void test_write_speed ()
{
	using namespace bprinter;

	const size_t N = 2000;
	const size_t seed_incr = 36000;
	size_t seed = 42;

	std::list<sptestres> results;

	std::string root ("c:");
	HANDLE hVolume = CreateFile (
		("\\\\.\\"+root).c_str (),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		std::cerr << "admin privileges required" << std::endl;
		return;
	}
	
	if (!FlushFileBuffers (hVolume))
	{
		std::cerr << "file buffers flush failed" << std::endl;
		return;
	}

	auto test = [&](std::string const& dir, std::string const& msg, std::function<size_t(void)>&& func, std::string const& comment) -> void {
		//system_clock::time_point start_all;
		//size_t all;
		//double all_sec;
		//double flush_sec;

		seed += seed_incr;
		setup_write_test (N, root + dir, seed);
		std::cout << msg << "... ";
		auto start_all = system_clock::now ();
		auto all = func ();
		auto all_sec = (double)duration_cast<microseconds>(system_clock::now () - start_all).count () / 1'000'000.0;
		FlushFileBuffers (hVolume);
		auto flush_sec = (double)duration_cast<microseconds>(system_clock::now () - start_all).count () / 1'000'000.0 - all_sec;
		if (all > 0)
		{
			std::cout << thousand_separated (all / 1024 / 1024) << "MB in " << thousand_separated (all_sec) << "sec, flush "
				<< thousand_separated (flush_sec) << "sec" << std::endl;
			auto res = analize_test ((double)all, all_sec + flush_sec);
			res.comment = comment;
			results.push_back (res);
		}

		std::this_thread::sleep_for (5s);
	};

	test ("/test00", 
		"Windows mapped file write w/o pattern", 
		std::bind (&test_windows_mapped, false), 
		"Win. mapped w/o pattern");

	test ("/test0",
		"Windows mapped file write w/ pattern",
		std::bind (&test_windows_mapped, true),
		"Win. mapped w/ pattern");

	test ("/test1",
		"Windows seq file write w/ pattern",
		&test_windows_seq,
		"Windows seq");

	test ("/test2",
		"Boost mapped dev w/ pattern",
		&test_mapped_sink,
		"Boost mapped");

	test ("/test3",
		"Boost mapped dev async open w/ pattern",
		&test_mapped_sink_async,
		"Boost mapped async");

	/*test ("/test4",
		"Stream rdbuf w/ pattern",
		&test_stream,
		"Stream rdbuf");*/

	CloseHandle (hVolume);

	// print results
	TablePrinter tp (&std::cout);
	tp.AddColumn ("test", 25);
	tp.AddColumn ("tot.speed(MB/s)", 16);
	tp.AddColumn ("avg.speed", 10);
	tp.AddColumn ("dev.speed", 10);
	tp.AddColumn ("avg.open(us)", 12);
	tp.AddColumn ("dev.open", 10);

	tp.PrintHeader ();
	for (auto const& res : results)
		tp << res.comment << thousand_separated (res.tot_speed) << thousand_separated (res.avg_speed) << thousand_separated (res.dev_speed)
		<< thousand_separated (res.avg_subop1 * 1'000'000.0) << thousand_separated (res.dev_subop1 * 1'000'000.0);
	tp.PrintFooter ();
}

void test_schifra ()
{
	const std::size_t field_descriptor = 8;
	const std::size_t gen_poly_index = 120;
	const std::size_t gen_poly_root_count = 6;
	const std::size_t code_length = 255;
	const std::size_t fec_length = 6;
	const std::string input_file_name = "D:\\test0\\000000\\000000.txt";
	const std::string output_file_name = "D:\\test0\\000000\\output.txt";

	typedef schifra::reed_solomon::encoder<code_length, fec_length> encoder_t;
	typedef schifra::reed_solomon::file_encoder<code_length, fec_length> file_encoder_t;

	const schifra::galois::field field (field_descriptor,
		schifra::galois::primitive_polynomial_size06,
		schifra::galois::primitive_polynomial06);

	schifra::galois::field_polynomial generator_polynomial (field);

	if (
		!schifra::make_sequential_root_generator_polynomial (field,
			gen_poly_index,
			gen_poly_root_count,
			generator_polynomial)
		)
	{
		std::cout << "Error - Failed to create sequential root generator!" << std::endl;
		return;
	}

	const encoder_t rs_encoder (field, generator_polynomial);

	file_encoder_t (rs_encoder, input_file_name, output_file_name);

}