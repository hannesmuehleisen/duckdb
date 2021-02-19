#include "duckdb/storage/table/persistent_segment.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/types/vector.hpp"
#include "duckdb/storage/checkpoint/table_data_writer.hpp"
#include "duckdb/storage/storage_manager.hpp"

#include "duckdb/storage/compressed_segment.hpp"

namespace duckdb {

PersistentSegment::PersistentSegment(DatabaseInstance &db, const LogicalType &type_p, DataPointer &pointer)
    : ColumnSegment(type_p, ColumnSegmentType::PERSISTENT, pointer.row_start, pointer.tuple_count,
                    pointer.statistics->Copy()),
      db(db), block_id(pointer.block_id), offset(pointer.offset) {
	D_ASSERT(offset == 0);

	data = make_unique<CompressedSegment>(db, type.InternalType(), pointer.row_start, pointer.decompress->Copy(),
	                                      pointer.block_id, pointer.tuple_count);
}

void PersistentSegment::InitializeScan(ColumnScanState &state) {
	data->InitializeScan(state);
}

void PersistentSegment::Scan(Transaction &transaction, ColumnScanState &state, idx_t vector_index, Vector &result) {
	data->Scan(transaction, state, vector_index, result);
}

void PersistentSegment::ScanCommitted(ColumnScanState &state, idx_t vector_index, Vector &result) {
	data->ScanCommitted(state, vector_index, result);
}

void PersistentSegment::FilterScan(Transaction &transaction, ColumnScanState &state, Vector &result,
                                   SelectionVector &sel, idx_t &approved_tuple_count) {
	data->FilterScan(transaction, state, result, sel, approved_tuple_count);
}

void PersistentSegment::IndexScan(ColumnScanState &state, Vector &result) {
	data->IndexScan(state, state.vector_index, result);
}

void PersistentSegment::Select(Transaction &transaction, ColumnScanState &state, Vector &result, SelectionVector &sel,
                               idx_t &approved_tuple_count, vector<TableFilter> &table_filter) {
	data->Select(transaction, result, table_filter, sel, approved_tuple_count, state);
}

void PersistentSegment::Fetch(ColumnScanState &state, idx_t vector_index, Vector &result) {
	data->Fetch(state, vector_index, result);
}

void PersistentSegment::FetchRow(ColumnFetchState &state, Transaction &transaction, row_t row_id, Vector &result,
                                 idx_t result_idx) {
	data->FetchRow(state, transaction, row_id - this->start, result, result_idx);
}

void PersistentSegment::Update(ColumnData &column_data, Transaction &transaction, Vector &updates, row_t *ids,
                               idx_t count) {

	throw InternalException("Can't update persistent segments");
}

bool PersistentSegment::HasChanges() {
	return block_id != data->block->BlockId();
}

} // namespace duckdb
