//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/storage/base_segment.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

namespace duckdb {

class BaseSegment {
public:
	BaseSegment() : tuple_count(0), block(nullptr) {
	}
	virtual ~BaseSegment();

	virtual void InitializeScan(ColumnScanState &state) {
	}
	//! Fetch the vector at index "vector_index" from the segment, storing it in the result vector
	virtual void Scan(Transaction &transaction, ColumnScanState &state, idx_t vector_index, Vector &result,
	                  bool get_lock = true) = 0;

	//! Scan the next vector from the column and apply a selection vector to filter the data
	virtual void FilterScan(Transaction &transaction, ColumnScanState &state, Vector &result, SelectionVector &sel,
	                        idx_t &approved_tuple_count) = 0;
	//! Fetch the vector at index "vector_index" from the segment, throwing an exception if there are any
	//! outstanding updates
	virtual void IndexScan(ColumnScanState &state, idx_t vector_index, Vector &result) = 0;

	//! Executes the filters directly in the table's data
	virtual void Select(Transaction &transaction, Vector &result, vector<TableFilter> &tableFilters,
	                    SelectionVector &sel, idx_t &approved_tuple_count, ColumnScanState &state) = 0;
	//! Fetch a single vector from the base table
	virtual void Fetch(ColumnScanState &state, idx_t vector_index, Vector &result) = 0;
	//! Fetch a single value and append it to the vector
	virtual void FetchRow(ColumnFetchState &state, Transaction &transaction, row_t row_id, Vector &result,
	                      idx_t result_idx) = 0;

	//! Append a part of a vector to the segment with the given append state, updating the provided stats
	//! in the process. Returns the amount of tuples appended. If this is less than `count`, the segment is
	//! full.
	virtual idx_t Append(SegmentStatistics &stats, Vector &data, idx_t offset, idx_t count) = 0;
	virtual void Flush() {
	}
	//! The current amount of tuples that are stored in this segment
	idx_t tuple_count;
	//! The block that this segment relates to
	shared_ptr<BlockHandle> block;
};
} // namespace duckdb
