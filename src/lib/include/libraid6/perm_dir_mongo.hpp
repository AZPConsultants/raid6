#pragma once
#include "perm_dir.h"
#include "../bson_util.h"
#include <string>

#include <bsoncxx/builder/stream/document.hpp>
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;

#include <mongocxx/pool.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/cursor.hpp>
#include <mongocxx/options/index.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/exception/exception.hpp>

extern mongocxx::pool *g_dbpool;

namespace raid6 {

template<size_t BLOCK_SIZE = 4 * 1024>
class perm_dir_mongo
{
public:
	perm_dir_mongo ()
	{}

	perm_dir_mongo (std::string_view db_name, std::string const& id, fs::path const& root) 
	{
		open (db_name, id, root);
	}

	void open (std::string_view db_name, std::string const& id, fs::path const& root)
	{
		db_name_ = db_name;
		id_ = id;
		root_ = root;
		open_ = true;
	}

	static auto init (std::string const& db_name, std::string const& id, fs::path const& root)
	{
		auto conn = g_dbpool->acquire ();
		auto db = (*conn)[db_name];
		auto col = db[id];

		mongocxx::options::index unique;
#ifdef _DEBUG
		unique.unique (true);
#else
		unique.unique (false);
#endif
		col.create_index (document () << "start" << 1 << finalize, unique);

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
				if (fs::is_regular_file (p))
				{
					auto size = p.file_size ();
					if (size == 0)
						continue;
					std::string rel_path = p.path ().string ().substr (root.string ().length () + 1);
					col.insert_one (document () << "_id" << rel_path << "start" << start << "size" << (int64_t)size << finalize);
					start += size;
					start = round_up_pow2 (start, BLOCK_SIZE);
					all += size;
					all_round += round_up_pow2 (size, BLOCK_SIZE);
				}
			}
		}
		catch (std::exception ex)
		{
			std::cerr << p.path().string() << ex.what () << std::endl;
			return std::pair(static_cast<std::streamsize>(-1), static_cast<std::streamsize> (-1));
		}
		return std::pair(all, all_round);
	}

	static int64_t update (std::string const& id, fs::path const& root)
	{
		return 0;
	}

	file_record at (std::streamsize offset)
	{
		assert (open_);

		if (!file_record_cache_.empty()) {
			if (file_record_cache_.start <= offset && file_record_cache_.pad_end > offset)
				return file_record_cache_;
		}

		auto conn = g_dbpool->acquire ();
		auto db = (*conn)[db_name_];
		auto col = db[id_];

		mongocxx::options::find opts;
		opts.sort (document () << "start" << 1 << finalize);

		auto doc = col.find_one (document () << "start" << open_document << "$gte" << offset << close_document << finalize, opts);
		if (!doc)
			return file_record ();

		auto dv = doc->view ();
		std::streamsize start   = (std::streamsize)get_int (dv["start"]);
		std::streamsize size    = (std::streamsize)get_int (dv["size"]);
		std::streamsize end     = start + size;
		std::streamsize pad_end = (size_t)round_up_pow2(end, BLOCK_SIZE);
		if (offset > pad_end)
			return file_record ();

		file_record_cache_.path = root_ / get_str(dv["_id"]);
		file_record_cache_.start = start;
		file_record_cache_.end = end;
		file_record_cache_.pad_end = pad_end;
		
		return file_record_cache_;
	}

	file_record at (fs::path const& path) const
	{
		assert (open_);

		auto rel_path = path.string ().substr (root_.string ().size ());
		assert (rel_path[0] != '\\' && rel_path[0] != '/');

		auto conn = g_dbpool->acquire ();
		auto db = (*conn)[db_name_];
		auto col = db[id_];

		auto doc = col.find_one (document () << "_id" << rel_path << finalize);
		if (!doc)
			return file_record ();

		auto dv = doc->view ();
		std::streamsize start = (std::streamsize)get_int (dv["start"]);
		std::streamsize size = (std::streamsize)get_int (dv["size"]);
		std::streamsize end = start + size;
		std::streamsize pad_end = (size_t)round_up_pow2 (end, BLOCK_SIZE);

		return file_record{ root_ / get_str (dv["_id"]), start, end, pad_end };
	}

	size_t size () const
	{
		// assert (open_);

		auto conn = g_dbpool->acquire ();
		auto db = (*conn)[db_name_];
		auto col = db[id_];

		mongocxx::options::find opts;
		opts.sort (document () << "start" << -1 << finalize);

		auto doc = col.find_one (document () << finalize, opts);
		if (!doc)
			return 0;

		auto dv = doc->view ();
		size_t start = (size_t)get_int (dv["start"]);
		size_t size = (size_t)get_int (dv["size"]);
		size_t end = start + size;
		size_t pad_end = (size_t)round_up_pow2 (end, BLOCK_SIZE);

		return pad_end;
	}
private:
	bool					open_{ false };
	fs::path				root_;
	std::string				id_;
	std::string				db_name_;
	file_record				file_record_cache_;
};

}