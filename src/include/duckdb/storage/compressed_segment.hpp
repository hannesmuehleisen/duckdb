//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/storage/compressed_segment.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/storage/base_segment.hpp"

namespace duckdb {

class CompressedSegment : public BaseSegment {};

class BufferManager;

class NumericCompressedSegment : public CompressedSegment {
public:
	NumericCompressedSegment(BufferManager &manager, PhysicalType type, idx_t row_start,
	                         block_id_t block_id = INVALID_BLOCK);

	//! Fetch the vector at index "vector_index" from the uncompressed segment, storing it in the result vector
	void Scan(Transaction &transaction, ColumnScanState &state, idx_t vector_index, Vector &result,
	          bool get_lock = true) override;
	//! Scan the next vector from the column and apply a selection vector to filter the data
	void FilterScan(Transaction &transaction, ColumnScanState &state, Vector &result, SelectionVector &sel,
	                idx_t &approved_tuple_count) override;
	//! Fetch the vector at index "vector_index" from the uncompressed segment, throwing an exception if there are any
	//! outstanding updates
	void IndexScan(ColumnScanState &state, idx_t vector_index, Vector &result) override;

	//! Executes the filters directly in the table's data
	void Select(Transaction &transaction, Vector &result, vector<TableFilter> &tableFilters, SelectionVector &sel,
	            idx_t &approved_tuple_count, ColumnScanState &state) override;
	//! Fetch a single vector from the base table
	void Fetch(ColumnScanState &state, idx_t vector_index, Vector &result) override;

	void FetchRow(ColumnFetchState &state, Transaction &transaction, row_t row_id, Vector &result,
	              idx_t result_idx) override;

	idx_t Append(SegmentStatistics &stats, Vector &data, idx_t offset, idx_t count) override;
	void Flush() override;

private:
	idx_t block_offset;
	unique_ptr<BufferHandle> handle;
};

} // namespace duckdb
