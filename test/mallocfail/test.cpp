#include <duckdb.hpp>
#include "mallocfail.h"

int main() {
	reset_malloc_limit(100000);
	try {
		duckdb::DuckDB db(nullptr);
		duckdb::Connection conn(db);
		auto res = conn.Query("SELECT 42");
		res->Print();
	} catch(...) {

	}
}