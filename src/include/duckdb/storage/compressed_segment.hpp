//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/storage/compressed_segment.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/storage/abstract_segment.hpp"

namespace duckdb {

class CompressedSegment : public AbstractSegment {

public:
	CompressedSegment(DatabaseInstance &db_p, PhysicalType type_p, idx_t row_start_p,
	                  unique_ptr<ParsedExpression> decompress_p, block_id_t block_id_p, idx_t tuple_count_p);
	virtual ~CompressedSegment();

	//! Fetch the vector at index "vector_index" from the uncompressed segment, storing it in the result vector
	void Scan(Transaction &transaction, ColumnScanState &state, idx_t vector_index, Vector &result,
	          bool get_lock = true) override;

	void ScanCommitted(ColumnScanState &state, idx_t vector_index, Vector &result) override;

	//! Scan the next vector from the column and apply a selection vector to filter the data
	void FilterScan(Transaction &transaction, ColumnScanState &state, Vector &result, SelectionVector &sel,
	                idx_t &approved_tuple_count) override;
	//! Fetch the vector at index "vector_index" from the uncompressed segment, throwing an exception if there are any
	//! outstanding updates
	void IndexScan(ColumnScanState &state, idx_t vector_index, Vector &result) override;

	//! Executes the filters directly in the table's data
	void Select(Transaction &transaction, Vector &result, vector<TableFilter> &table_filters, SelectionVector &sel,
	            idx_t &approved_tuple_count, ColumnScanState &state) override;
	//! Fetch a single vector from the base table
	void Fetch(ColumnScanState &state, idx_t vector_index, Vector &result) override;
	//! Fetch a single value and append it to the vector
	void FetchRow(ColumnFetchState &state, Transaction &transaction, row_t row_id, Vector &result,
	              idx_t result_idx) override;

	void Verify(Transaction &transaction) override;

public:
	unique_ptr<ParsedExpression> decompress;
};

} // namespace duckdb
