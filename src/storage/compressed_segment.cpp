#include "duckdb/storage/data_table.hpp"
#include "duckdb/common/operator/comparison_operators.hpp"

#include "duckdb/storage/compressed_segment.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/types/vector.hpp"
#include "duckdb/transaction/update_info.hpp"
#include "duckdb/planner/table_filter.hpp"
#include "duckdb/storage/buffer/block_handle.hpp"
#include "duckdb/transaction/transaction.hpp"

namespace duckdb {

CompressedSegment::CompressedSegment(DatabaseInstance &db_p, PhysicalType type_p, idx_t row_start_p,
                                     unique_ptr<ParsedExpression> decompress_p, block_id_t block_id_p,
                                     idx_t tuple_count_p)
    : AbstractSegment(db_p, type_p, row_start_p), decompress(move(decompress_p)) {
	auto &buffer_manager = BufferManager::GetBufferManager(db);
	tuple_count = tuple_count_p;
	this->block = buffer_manager.RegisterBlock(block_id_p);
}

CompressedSegment::~CompressedSegment() {
}

//! Fetch the vector at index "vector_index" from the uncompressed segment, storing it in the result vector
void CompressedSegment::Scan(Transaction &transaction, ColumnScanState &state, idx_t vector_index, Vector &result,
                             bool get_lock) {
	if (decompress->GetName() != "rle()") {
		throw NotImplementedException("Unknown compression method");
	}

	auto &buffer_manager = BufferManager::GetBufferManager(db);
	state.primary_handle = buffer_manager.Pin(block);

	D_ASSERT(vector_index == 0);
	D_ASSERT(tuple_count < STANDARD_VECTOR_SIZE);
	D_ASSERT(type == PhysicalType::INT32);
	auto block_ptr = state.primary_handle->Ptr();
	auto vector_ptr = FlatVector::GetData<int32_t>(result);
	for (idx_t i = 0; i < tuple_count;) {
		auto count = Load<uint16_t>(block_ptr);
		block_ptr += sizeof(uint16_t);
		auto val = Load<int32_t>(block_ptr);
		block_ptr += sizeof(int32_t);
		for (idx_t next = i + count; i < next; i++) {
			vector_ptr[i] = val;
		}
	}
}

void CompressedSegment::ScanCommitted(ColumnScanState &state, idx_t vector_index, Vector &result) {
	throw NotImplementedException("ScanCommitted");
}

//! Scan the next vector from the column and apply a selection vector to filter the data
void CompressedSegment::FilterScan(Transaction &transaction, ColumnScanState &state, Vector &result,
                                   SelectionVector &sel, idx_t &approved_tuple_count) {

	throw NotImplementedException("FilterScan");
}
//! Fetch the vector at index "vector_index" from the uncompressed segment, throwing an exception if there are any
//! outstanding updates
void CompressedSegment::IndexScan(ColumnScanState &state, idx_t vector_index, Vector &result) {

	throw NotImplementedException("IndexScan");
}

//! Executes the filters directly in the table's data
void CompressedSegment::Select(Transaction &transaction, Vector &result, vector<TableFilter> &table_filters,
                               SelectionVector &sel, idx_t &approved_tuple_count, ColumnScanState &state) {
	throw NotImplementedException("Select");
}
//! Fetch a single vector from the base table
void CompressedSegment::Fetch(ColumnScanState &state, idx_t vector_index, Vector &result) {
	throw NotImplementedException("Fetch");
}
//! Fetch a single value and append it to the vector
void CompressedSegment::FetchRow(ColumnFetchState &state, Transaction &transaction, row_t row_id, Vector &result,
                                 idx_t result_idx) {
	throw NotImplementedException("FetchRow");
}

void CompressedSegment::Verify(Transaction &transaction) {
	throw NotImplementedException("Verify");
}

} // namespace duckdb
