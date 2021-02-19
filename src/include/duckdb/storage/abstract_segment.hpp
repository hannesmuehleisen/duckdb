//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/storage/uncompressed_segment.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/storage/table/column_segment.hpp"
#include "duckdb/storage/block.hpp"
#include "duckdb/storage/storage_lock.hpp"
#include "duckdb/storage/table/scan_state.hpp"

namespace duckdb {
class BlockHandle;
class ColumnData;
class Transaction;
class StorageManager;

struct ColumnAppendState;
struct UpdateInfo;

class AbstractSegment {
public:
	AbstractSegment(DatabaseInstance &db_p, PhysicalType type_p, idx_t row_start_p)
	    : db(db_p), type(type_p), tuple_count(0), row_start(row_start_p) {
	}
	virtual ~AbstractSegment() {
	}

	//! The storage manager
	DatabaseInstance &db;
	//! Type of the uncompressed segment
	PhysicalType type;
	//! The block that this segment relates to
	shared_ptr<BlockHandle> block;

	//! The current amount of tuples that are stored in this segment
	idx_t tuple_count;
	//! The starting row of this segment
	idx_t row_start;

public:
	virtual void InitializeScan(ColumnScanState &state) {
	}

	//! Fetch the vector at index "vector_index" from the uncompressed segment, storing it in the result vector
	virtual void Scan(Transaction &transaction, ColumnScanState &state, idx_t vector_index, Vector &result,
	                  bool get_lock = true) = 0;

	virtual void ScanCommitted(ColumnScanState &state, idx_t vector_index, Vector &result) = 0;

	//! Scan the next vector from the column and apply a selection vector to filter the data
	virtual void FilterScan(Transaction &transaction, ColumnScanState &state, Vector &result, SelectionVector &sel,
	                        idx_t &approved_tuple_count) = 0;
	//! Fetch the vector at index "vector_index" from the uncompressed segment, throwing an exception if there are any
	//! outstanding updates
	virtual void IndexScan(ColumnScanState &state, idx_t vector_index, Vector &result) = 0;

	//! Executes the filters directly in the table's data
	virtual void Select(Transaction &transaction, Vector &result, vector<TableFilter> &table_filters,
	                    SelectionVector &sel, idx_t &approved_tuple_count, ColumnScanState &state) = 0;
	//! Fetch a single vector from the base table
	virtual void Fetch(ColumnScanState &state, idx_t vector_index, Vector &result) = 0;
	//! Fetch a single value and append it to the vector
	virtual void FetchRow(ColumnFetchState &state, Transaction &transaction, row_t row_id, Vector &result,
	                      idx_t result_idx) = 0;

	virtual void Verify(Transaction &transaction) {
	}
};

} // namespace duckdb
