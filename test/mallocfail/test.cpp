#include <duckdb.hpp>
#include "mallocfail.h"

int main(int argc, const char** argv) {
	reset_malloc_limit(10000);
	try {
		duckdb::DuckDB db(nullptr);
		duckdb::Connection conn(db);
		auto res = conn.Query("SELECT 42");
		res->Print();
	} catch(...) {
        printf("uuh eeeh\n");
		return 1;
	}
	return 0;
}