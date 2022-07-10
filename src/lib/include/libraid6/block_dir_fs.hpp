#pragma once
#include "perm_dir.h"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;
#include <fmt/format.h>

namespace raid6 {

	template<size_t BLOCK_SIZE = 4 * 1024>
	class block_dir_fs
	{
		const size_t BLOCKS_PER_FILE = 32 * 1024;
		const size_t FILES_PER_DIR = 1024;
	public:
		block_dir_fs ()
		{}

		block_dir_fs (std::string_view db_name, std::string const& id, fs::path const& root)
		{
			boost::ignore_unused_variable_warning(db_name);
			open (id, root);
		}

		static auto init (std::string const& id, fs::path const& root)
		{
			auto p = root / "volumelabel.txt";
			if (fs::exists (p))
			{
				std::ifstream idf (p.string ());
				std::stringstream st;
				st << idf.rdbuf ();
				if (st.str () != id)
					throw std::ios_base::failure ("volume aleady initialized");
				idf.close ();
			}
			else
			{
				fs::create_directories (root);
				std::ofstream odf (p.string ());
				odf << id;
				odf.flush (); 
				odf.close ();
			}
		}

		void open (std::string_view db_name, std::string const& id, fs::path const& root)
		{
			boost::ignore_unused_variable_warning(db_name);

			id_ = id;
			root_ = root;
			auto p = root / "volumelabel.txt";
			if (fs::exists (p))
			{
				std::ifstream idf (p.string ());
				std::stringstream st;
				st << idf.rdbuf ();
				if (st.str () != id)
					throw std::ios_base::failure ("wrong volume");
				idf.close ();
			}
			else
				throw std::ios_base::failure ("volume not initialized");

			open_ = true;
		}

		file_record at (std::streamsize offset) const
		{
			auto block_num = (size_t)offset / BLOCK_SIZE;
			auto file_num = block_num / BLOCKS_PER_FILE;
			auto dir_num = file_num / FILES_PER_DIR;

			std::streamsize start = (std::streamsize)file_num * BLOCKS_PER_FILE * BLOCK_SIZE;
			std::streamsize end = start + BLOCKS_PER_FILE * BLOCK_SIZE;
			std::streamsize pad_end = end;
			std::string path = fmt::format ("{:08d}/{:08d}.dat", dir_num, file_num);
			return file_record{ root_ / path, start, end, pad_end };
		}

		file_record at (fs::path const& path) const
		{
			auto file_num = std::atoll(path.filename ().string ().substr(0, 8));
			std::streamsize start = (std::streamsize)file_num * BLOCKS_PER_FILE * BLOCK_SIZE;
			std::streamsize end = start + BLOCKS_PER_FILE * BLOCK_SIZE;
			std::streamsize pad_end = end;

			return file_record{ path, start, end, pad_end };
		}

	private:
		bool					open_{ false };
		fs::path				root_;
		std::string				id_;
	};

}