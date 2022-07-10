#pragma once
#include "perm_dir.h"
#include <string>
#include <sqlite_modern_cpp.h>
using namespace  sqlite;

namespace raid6 {

	template<size_t BLOCK_SIZE = 4 * 1024>
	class perm_dir_sqlite
	{
	public:
		perm_dir_sqlite ()
		{}

		perm_dir_sqlite (std::string const& id, fs::path const& root)
		{
			open (id, root);
		}

		/*perm_dir_sqlite(perm_dir_sqlite const& p)
		{
			id_ = p.id_;
			root_ = p.id_;
			open_ = p.open_;
		}*/

		void open (std::string const& id, fs::path const& root)
		{
			id_ = id;
			root_ = root;
			db_.reset (new database ((root / (id + ".db")).string ()));
			open_ = true;
		}

		static auto init (std::string const& id, fs::path const& root)
		{
			auto db_path = root / (id + ".db");
			if (fs::exists (db_path))
				throw std::runtime_error ("db already exists " + db_path.string ());

			database db (db_path.string ());

			db << R"(create table folder (
						_id text primary key not null,
						start int,
						size int);
					)";

			std::string unique;
#ifdef _DEBUG
			unique = "unique ";
#endif
			db << "create " + unique + "index IDX_FOLDER_START on folder (start);";

			auto insert = db << "insert into folder values (?,?,?)";
			std::streamsize all = 0;
			std::streamsize all_round = 0;
			fs::directory_entry p;
			try
			{
				fs::recursive_directory_iterator dir (root);
				int64_t start = 0;
				for (; dir != fs::recursive_directory_iterator (); ++dir)
				{
					p = *dir;
					if (fs::is_regular_file (p) && p.path() != db_path)
					{
						auto size = p.file_size ();
						if (size == 0)
							continue;
						std::string rel_path = p.path ().string ().substr (root.string ().length () + 1);
						insert << rel_path << start << size;
						insert++;
						start += size;
						start = round_up_pow2 (start, BLOCK_SIZE);
						all += size;
						all_round += round_up_pow2 (size, BLOCK_SIZE);
					}
				}
			}
			catch (std::exception ex)
			{
				std::cerr << p.path ().string () << ex.what () << std::endl;
				return std::pair (static_cast<std::streamsize>(-1), static_cast<std::streamsize> (-1));
			}
			return std::pair (all, all_round);
		}

		static int64_t update (std::string const& id, fs::path const& root)
		{
			return 0;
		}

		file_record at (std::streamsize offset) const
		{
			assert (open_);

			std::string _id;
			std::streamsize start;
			std::streamsize size;

			*db_ << R"( select _id, start, size from folder
							where start >= ?
							order by start limit 1;
					)" << offset
				>> std::tie(_id, start, size);

			if (_id.empty ())
				return file_record ();

			std::streamsize end = start + size;
			std::streamsize pad_end = (size_t)round_up_pow2 (end, BLOCK_SIZE);
			if (offset > pad_end)
				return file_record ();

			return file_record{ root_ / _id, start, end, pad_end };
		}

		file_record at (fs::path const& path) const
		{
			assert (open_);

			auto rel_path = path.string ().substr (root_.string ().size ());
			assert (rel_path[0] != '\\' && rel_path[0] != '/');

			std::string _id;
			std::streamsize start;
			std::streamsize size;

			*db_ << R"(  select _id, start, size from folder
							where _id like ? limit 1;
					)" << path
				>> std::tie(_id, start, size);

			/*
			auto conn = g_dbpool->acquire ();
			auto db = (*conn)[DBNAME];
			auto col = db[id_];

			auto doc = col.find_one (document () << "_id" << rel_path << finalize);
			
			if (!doc)
				return file_record ();
			*/
			if (_id.empty())
				return file_record();

			std::streamsize end = start + size;
			std::streamsize pad_end = (size_t)round_up_pow2(end, BLOCK_SIZE);

			return file_record{ root_ / _id, start, end, pad_end };
		}

		size_t size () const
		{
			assert (open_);
			std::string _id;
			std::streamsize start;
			std::streamsize size;

			*db_ << R"(select _id, start, size from folder
						order by start desc limit 1)"
				>> std::tie(_id, start, size);
			if (_id.empty())
				return 0;

			
			std::streamsize end = start + size;
			std::streamsize pad_end = (size_t)round_up_pow2 (end, BLOCK_SIZE);

			return (size_t)pad_end;
		}
	private:
		std::shared_ptr<database>	db_;
		bool					open_{ false };
		fs::path				root_;
		std::string				id_;
	};

}