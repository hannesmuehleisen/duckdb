#include "duckdb/storage/compressed_segment.hpp"

namespace duckdb {

NumericCompressedSegment::NumericCompressedSegment(BufferManager &manager, PhysicalType type, idx_t row_start,
                                                   block_id_t block_id)
    : block_offset(sizeof(idx_t)) {
	// TODO this is duplicated from NumericSegment
	if (block_id == INVALID_BLOCK) {
		// no block id specified: allocate a buffer for the uncompressed segment
		this->block = manager.RegisterMemory(Storage::BLOCK_ALLOC_SIZE, false);
		auto handle = manager.Pin(block);
	} else {
		this->block = manager.RegisterBlock(block_id);
	}
	this->handle = manager.Pin(block);
}

//! Fetch the vector at index "vector_index" from the uncompressed segment, storing it in the result vector
void NumericCompressedSegment::Scan(Transaction &transaction, ColumnScanState &state, idx_t vector_index,
                                    Vector &result, bool get_lock) {

	auto result_ptr = FlatVector::GetData<int32_t>(result);
	auto block_ptr = handle->Ptr() + block_offset;
	for (idx_t i = 0; i < 3; i++) { // FIXME lol 3
		auto val = Load<int32_t>(block_ptr);
		block_ptr += sizeof(int32_t);
		result_ptr[i] = val;
		block_offset += sizeof(int32_t);
	}
}
//! Scan the next vector from the column and apply a selection vector to filter the data
void NumericCompressedSegment::FilterScan(Transaction &transaction, ColumnScanState &state, Vector &result,
                                          SelectionVector &sel, idx_t &approved_tuple_count) {

	throw NotImplementedException("FilterScan");
}
//! Fetch the vector at index "vector_index" from the uncompressed segment, throwing an exception if there are any
//! outstanding updates
void NumericCompressedSegment::IndexScan(ColumnScanState &state, idx_t vector_index, Vector &result) {
	throw NotImplementedException("IndexScan");
}

//! Executes the filters directly in the table's data
void NumericCompressedSegment::Select(Transaction &transaction, Vector &result, vector<TableFilter> &tableFilters,
                                      SelectionVector &sel, idx_t &approved_tuple_count, ColumnScanState &state) {
	throw NotImplementedException("Select");
}
//! Fetch a single vector from the base table
void NumericCompressedSegment::Fetch(ColumnScanState &state, idx_t vector_index, Vector &result) {
	throw NotImplementedException("Fetch");
}

void NumericCompressedSegment::FetchRow(ColumnFetchState &state, Transaction &transaction, row_t row_id, Vector &result,
                                        idx_t result_idx) {
	throw NotImplementedException("FetchRow");
}

idx_t NumericCompressedSegment::Append(SegmentStatistics &stats, Vector &data, idx_t offset, idx_t count) {

	switch (data.type.id()) {
	case LogicalTypeId::INTEGER: {
		D_ASSERT(count * sizeof(int32_t) < handle->node->size - block_offset);
		auto data_ptr = FlatVector::GetData<int32_t>(data);
		for (idx_t i = 0; i < count; i++) {
			Store<int32_t>(data_ptr[i], handle->Ptr() + block_offset);
			block_offset += sizeof(int32_t);
		}
	} break;
	default:
		throw new NotImplementedException("eek");
	}
	tuple_count += count;
	return count;
}

void NumericCompressedSegment::Flush() {
	Store<idx_t>(block_offset, handle->Ptr());
}

} // namespace duckdb
