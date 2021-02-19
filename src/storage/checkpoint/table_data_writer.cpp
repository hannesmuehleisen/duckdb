#include "duckdb/storage/checkpoint/table_data_writer.hpp"

#include "duckdb/common/vector_operations/vector_operations.hpp"
#include "duckdb/common/types/null_value.hpp"

#include "duckdb/catalog/catalog_entry/table_catalog_entry.hpp"
#include "duckdb/common/serializer/buffered_serializer.hpp"

#include "duckdb/storage/string_segment.hpp"
#include "duckdb/storage/table/column_segment.hpp"
#include "duckdb/storage/table/persistent_segment.hpp"
#include "duckdb/storage/table/transient_segment.hpp"
#include "duckdb/storage/compressed_segment.hpp"

#include "duckdb/storage/column_data.hpp"
#include "duckdb/storage/table/morsel_info.hpp"
#include "duckdb/parser/expression/function_expression.hpp"

namespace duckdb {

class WriteOverflowStringsToDisk : public OverflowStringWriter {
public:
	explicit WriteOverflowStringsToDisk(DatabaseInstance &db);
	~WriteOverflowStringsToDisk() override;

	//! The checkpoint manager
	DatabaseInstance &db;

	//! Temporary buffer
	unique_ptr<BufferHandle> handle;
	//! The block on-disk to which we are writing
	block_id_t block_id;
	//! The offset within the current block
	idx_t offset;

	static constexpr idx_t STRING_SPACE = Storage::BLOCK_SIZE - sizeof(block_id_t);

public:
	void WriteString(string_t string, block_id_t &result_block, int32_t &result_offset) override;

private:
	void AllocateNewBlock(block_id_t new_block_id);
};

TableDataWriter::TableDataWriter(DatabaseInstance &db, TableCatalogEntry &table, MetaBlockWriter &meta_writer)
    : db(db), table(table), meta_writer(meta_writer) {
}

TableDataWriter::~TableDataWriter() {
}

void TableDataWriter::WriteTableData() {
	// allocate the initial segments
	data_pointers.resize(table.columns.size());
	stats.reserve(table.columns.size());
	column_stats.reserve(table.columns.size());
	for (idx_t i = 0; i < table.columns.size(); i++) {
		auto type_id = table.columns[i].type.InternalType();
		stats.push_back(make_unique<SegmentStatistics>(table.columns[i].type, GetTypeIdSize(type_id)));
		column_stats.push_back(BaseStatistics::CreateEmpty(table.columns[i].type));
	}

	// now start scanning the table and append the data to the uncompressed segments
	table.storage->Checkpoint(*this);

	VerifyDataPointers();
	WriteDataPointers();

	// then we checkpoint the deleted tuples
	table.storage->CheckpointDeletes(*this);
}

void TableDataWriter::CreateSegment(idx_t col_idx) {
	D_ASSERT(0);
}

void TableDataWriter::CheckpointColumn(ColumnData &col_data, idx_t col_idx) {
	if (!col_data.data.root_node) {
		return;
	}

	// scan the segments of the column data
	// we create a new segment tree with all the new segments
	SegmentTree new_tree;

	auto owned_segment = move(col_data.data.root_node);

	vector<unique_ptr<ColumnSegment>> scan_segments;
	scan_segments.reserve(10);

	auto segment = (ColumnSegment *)owned_segment.get();
	while (segment) {
		// leave segment alone case
		if (segment->segment_type == ColumnSegmentType::PERSISTENT) {
			auto &persistent = (PersistentSegment &)*segment;
			// persistent segment; check if there were changes made to the segment
			if (!persistent.HasChanges()) {
				// unchanged persistent segment: no need to write the data

				// flush any segments preceding this persistent segment
				FlushSegmentList(col_data, new_tree, col_idx, scan_segments);
				/*if (segments[col_idx]->tuple_count > 0) {
				    FlushSegment(new_tree, col_idx);
				    CreateSegment(col_idx);
				}*/

				// set up the data pointer directly using the data from the persistent segment
				DataPointer pointer;
				pointer.block_id = persistent.block_id;
				pointer.offset = 0;
				pointer.row_start = segment->start;
				pointer.tuple_count = persistent.count;
				pointer.statistics = persistent.stats.statistics->Copy();
				pointer.decompress = ((CompressedSegment *)persistent.data.get())->decompress->Copy();
				pointer.aux_offset = 0;

				// merge the persistent stats into the global column stats
				column_stats[col_idx]->Merge(*persistent.stats.statistics);

				// directly append the current segment to the new tree
				new_tree.AppendSegment(move(owned_segment));

				data_pointers[col_idx].push_back(move(pointer));

				// move to the next segment in the list
				owned_segment = move(segment->next);
				segment = (ColumnSegment *)owned_segment.get();
				continue;
			}
		}
		// not persisted yet: scan the segment and write it to disk
		scan_segments.push_back(unique_ptr_cast<SegmentBase, ColumnSegment>(move(owned_segment)));
		if (scan_segments.size() > 10) {
			FlushSegmentList(col_data, new_tree, col_idx, scan_segments);
		}
		// move to the next segment in the list
		owned_segment = move(segment->next);
		segment = (ColumnSegment *)owned_segment.get();
	}
	// flush the final segments
	FlushSegmentList(col_data, new_tree, col_idx, scan_segments);
	// replace the old tree with the new one
	col_data.data.Replace(new_tree);
}

struct AnalyzeState {
	virtual ~AnalyzeState() {
	}
};

struct RLEAnalyzeState : public AnalyzeState {
	RLEAnalyzeState() : seen_count(0), rle_count(0), last_seen_count(0) {
	}

	idx_t seen_count;
	idx_t rle_count;
	idx_t last_seen_count;
	Value last_value;
};

struct CompressionState {
	virtual ~CompressionState() {
	}
};

struct RLECompressionState : public CompressionState {
	RLECompressionState() : seen_value(false), last_seen_count(0) {
	}

	bool seen_value;
	idx_t last_seen_count;
	Value last_value;
};

// expression rewriter on complex filter pushdown
// 42 + X >= 43

// separate tasks (in arbitrary order)

// task 1
// handling half-empty blocks (half-empty blocks in free list, etc)

// task 2
// handling null values as separate boolean column

// task 3
// move updates from in-place to out of place somehow

// task 4
// scan handles not vector aligned blocks on disk

// end to end compression with rle or something
// handling strings that are bigger than a block
// expression rewrite thing

typedef unique_ptr<AnalyzeState> (*compression_function_init_t)(ColumnData &col_data);
typedef bool (*compression_function_analyze_t)(Vector &intermediate, idx_t count, AnalyzeState &state);
typedef idx_t (*compression_function_final_analyze_t)(ColumnData &col_data, AnalyzeState &state,
                                                      BaseStatistics &statistics);
typedef unique_ptr<CompressionState> (*compression_function_initialize_compression_t)(ColumnData &col_data,
                                                                                      AnalyzeState &state);
typedef idx_t (*compression_function_compress_data_t)(BufferHandle &block, idx_t &data_written, Vector &intermediate,
                                                      idx_t count, CompressionState &state);
typedef void (*compression_function_flush_state_t)(BufferHandle &block, idx_t &data_written, CompressionState &state);

struct CompressionMethod {
	CompressionMethod()
	    : initialize_state(nullptr), analyze(nullptr), final_analyze(nullptr), initialize_compression(nullptr),
	      compress_data(nullptr), flush_state(nullptr) {
	}
	virtual ~CompressionMethod() {
	}
	compression_function_init_t initialize_state;
	compression_function_analyze_t analyze;
	compression_function_final_analyze_t final_analyze;
	compression_function_initialize_compression_t initialize_compression;
	compression_function_compress_data_t compress_data;
	compression_function_flush_state_t flush_state;
};

struct RLECompression : public CompressionMethod {

	RLECompression() {
		initialize_state = InitializeRLE;
		analyze = RLEAnalyze;
		final_analyze = RLEFinalize;
		initialize_compression = InitializeRLECompression;
		compress_data = RLECompress;
		flush_state = RLECompressFinalize;
	}

	static unique_ptr<AnalyzeState> InitializeRLE(ColumnData &col_data) {
		return make_unique<RLEAnalyzeState>();
	}

	static bool RLEAnalyze(Vector &input, idx_t count, AnalyzeState &state_p) {
		auto &state = (RLEAnalyzeState &)state_p;
		idx_t i = 0;
		if (state.seen_count == 0) {
			state.seen_count = 1;
			state.rle_count = 1;
			state.last_seen_count = 1;
			state.last_value = input.GetValue(0);
			i = 1;
		}
		for (; i < count; i++) {
			auto new_value = input.GetValue(i);
			if (state.last_value != new_value || state.last_seen_count >= NumericLimits<uint16_t>::Maximum()) {
				state.rle_count++;
				state.last_seen_count = 1;
				state.last_value = new_value;
			} else {
				state.last_seen_count++;
			}
		}
		state.seen_count += count;
		// abort if RLE is probably not going to work out
		if (state.seen_count > 10000 && state.rle_count > 0.6 * state.seen_count) {
			return false;
		}
		return true;
	}

	static idx_t RLEFinalize(ColumnData &col_data, AnalyzeState &state_p, BaseStatistics &statistics) {
		auto &state = (RLEAnalyzeState &)state_p;
		return (GetTypeIdSize(col_data.type.InternalType()) + sizeof(uint16_t)) * state.rle_count;
	}

	static unique_ptr<CompressionState> InitializeRLECompression(ColumnData &col_data, AnalyzeState &state_p) {
		return make_unique<RLECompressionState>();
	}

	static idx_t RLECompress(BufferHandle &block, idx_t &offset_in_block, Vector &input, idx_t count,
	                         CompressionState &state_p) {
		auto &state = (RLECompressionState &)state_p;

		idx_t i = 0;
		if (!state.seen_value) {
			state.last_seen_count = 1;
			state.last_value = input.GetValue(0);
			i = 1;
		}
		idx_t RLE_SIZE = sizeof(uint16_t) + sizeof(int32_t);
		auto write_pointer = block.Ptr();
		for (; i < count; i++) {
			auto new_value = input.GetValue(i);
			if (state.last_value != new_value || state.last_seen_count >= NumericLimits<uint16_t>::Maximum()) {
				// write the value
				Store<uint16_t>(state.last_seen_count, write_pointer + offset_in_block);
				offset_in_block += sizeof(uint16_t);
				Store<int32_t>(state.last_value.GetValue<int32_t>(), write_pointer + offset_in_block);
				offset_in_block += sizeof(int32_t);

				// reset
				state.last_seen_count = 1;
				state.last_value = new_value;

				// check if we have space for the last value or not
				// if not, abort
				if (offset_in_block + RLE_SIZE > block.node->size) {
					return i - 1;
				}
			} else {
				state.last_seen_count++;
			}
		}

		return count;
	}

	static void RLECompressFinalize(BufferHandle &block, idx_t &data_written, CompressionState &state_p) {
		// nop
		auto &state = (RLECompressionState &)state_p;
		auto write_pointer = block.Ptr();

		Store<uint16_t>(state.last_seen_count, write_pointer + data_written);
		data_written += sizeof(uint16_t);
		Store<int32_t>(state.last_value.GetValue<int32_t>(), write_pointer + data_written);
		data_written += sizeof(int32_t);
	}
};

void TableDataWriter::FlushSegmentList(ColumnData &col_data, SegmentTree &new_tree, idx_t col_idx,
                                       vector<unique_ptr<ColumnSegment>> &segment_list) {
	if (segment_list.size() == 0) {
		return;
	}

	Vector intermediate(col_data.type);

	// set up candidate compression methods
	vector<unique_ptr<CompressionMethod>> candidates;

	candidates.push_back(make_unique<RLECompression>());

	vector<unique_ptr<AnalyzeState>> candidate_states;
	candidate_states.reserve(candidates.size());
	for (auto &compression_method : candidates) {
		candidate_states.push_back(compression_method->initialize_state(col_data));
	}

	// call analyze on all of the compression methods
	for (auto &segment : segment_list) {
		ColumnScanState state;
		segment->InitializeScan(state);
		for (idx_t vector_index = 0; vector_index * STANDARD_VECTOR_SIZE < segment->count; vector_index++) {
			idx_t count = MinValue<idx_t>(segment->count - vector_index * STANDARD_VECTOR_SIZE, STANDARD_VECTOR_SIZE);
			segment->ScanCommitted(state, vector_index, intermediate);
			stats[col_idx]->statistics->Update(intermediate, count);

			for (idx_t i = 0; i < candidates.size(); i++) {
				auto &compression_method = candidates[i];
				auto &compression_state = candidate_states[i];
				if (!compression_method) {
					continue;
				}
				//

				bool keep_candidate = compression_method->analyze(intermediate, count, *compression_state);
				if (!keep_candidate) {
					compression_method.reset();
					compression_state.reset();
				}
			}
		}
	}

	idx_t best_compression = INVALID_INDEX;
	idx_t smallest_size = NumericLimits<idx_t>::Maximum();
	for (idx_t i = 0; i < candidates.size(); i++) {
		auto &compression_method = candidates[i];
		auto &compression_state = candidate_states[i];
		if (!compression_method) {
			continue;
		}
		idx_t result_size =
		    compression_method->final_analyze(col_data, *compression_state, *stats[col_idx]->statistics);
		if (result_size < smallest_size) {
			smallest_size = result_size;
			best_compression = i;
		}
	}

	auto &buffer_manager = BufferManager::GetBufferManager(db);
	auto block = buffer_manager.Allocate(Storage::BLOCK_ALLOC_SIZE);
	idx_t data_written = 0;

	// SSSSSDDDDD
	//

	idx_t compressed_rows = 0;

	auto &best_compression_method = candidates[best_compression];
	auto compression_state =
	    best_compression_method->initialize_compression(col_data, *candidate_states[best_compression]);
	for (auto &segment : segment_list) {
		ColumnScanState state;
		segment->InitializeScan(state);
		for (idx_t vector_index = 0; vector_index * STANDARD_VECTOR_SIZE < segment->count; vector_index++) {
			idx_t count = MinValue<idx_t>(segment->count - vector_index * STANDARD_VECTOR_SIZE, STANDARD_VECTOR_SIZE);
			segment->ScanCommitted(state, vector_index, intermediate);

			idx_t remaining = count;
			while (remaining > 0) {
				idx_t compress_count = best_compression_method->compress_data(*block, data_written, intermediate, count,
				                                                              *compression_state);
				remaining -= compress_count;
				compressed_rows += compress_count;
				if (remaining > 0) {
					FlushCompressionState(*best_compression_method, *compression_state, *block, data_written, new_tree,
					                      col_idx, compressed_rows);
					data_written = 0;
					compressed_rows = 0;
				}
			}
		}
	}
	if (data_written > 0) {
		FlushCompressionState(*best_compression_method, *compression_state, *block, data_written, new_tree, col_idx,
		                      compressed_rows);
	}
}

void TableDataWriter::FlushCompressionState(CompressionMethod &best_compression_method,
                                            CompressionState &compression_state, BufferHandle &block,
                                            idx_t data_written, SegmentTree &new_tree, idx_t col_idx,
                                            idx_t compressed_rows) {
	DataPointer pointer;
	pointer.offset = 0;
	pointer.aux_offset = data_written;
	pointer.tuple_count = compressed_rows;

	if (compressed_rows == 0) {
		return;
	}

	vector<unique_ptr<ParsedExpression>> children;
	pointer.decompress = make_unique<FunctionExpression>("rle", children);
	best_compression_method.flush_state(block, data_written, compression_state);
	FlushBlock(pointer, block, data_written, new_tree, col_idx);
	data_written = 0;
}

void TableDataWriter::FlushBlock(DataPointer &pointer, BufferHandle &block, idx_t data_written, SegmentTree &new_tree,
                                 idx_t col_idx) {

	// get the buffer of the segment and pin it
	auto &block_manager = BlockManager::GetBlockManager(db);

	// get a free block id to write to
	auto block_id = block_manager.GetFreeBlockId();

	// construct the data pointer
	uint32_t offset_in_block = 0;

	pointer.block_id = block_id;
	pointer.offset = offset_in_block;
	pointer.row_start = 0;
	if (!data_pointers[col_idx].empty()) {
		auto &last_pointer = data_pointers[col_idx].back();
		pointer.row_start = last_pointer.row_start + last_pointer.tuple_count;
	}
	pointer.statistics = stats[col_idx]->statistics->Copy();

	// construct a persistent segment that points to this block, and append it to the new segment tree
	auto persistent_segment = make_unique<PersistentSegment>(db, table.columns[col_idx].type, pointer);
	new_tree.AppendSegment(move(persistent_segment));

	data_pointers[col_idx].push_back(move(pointer));
	// write the block to disk
	block_manager.Write(*block.node, block_id);

	column_stats[col_idx]->Merge(*stats[col_idx]->statistics);
	stats[col_idx] = make_unique<SegmentStatistics>(table.columns[col_idx].type,
	                                                GetTypeIdSize(table.columns[col_idx].type.InternalType()));
}

void TableDataWriter::CheckpointDeletes(MorselInfo *morsel_info) {
	// deletes! write them after the data pointers
	while (morsel_info) {
		if (morsel_info->root) {
			// first count how many ChunkInfo's we need to deserialize
			idx_t chunk_info_count = 0;
			for (idx_t vector_idx = 0; vector_idx < MorselInfo::MORSEL_VECTOR_COUNT; vector_idx++) {
				auto chunk_info = morsel_info->root->info[vector_idx].get();
				if (!chunk_info) {
					continue;
				}
				chunk_info_count++;
			}
			meta_writer.Write<idx_t>(chunk_info_count);
			for (idx_t vector_idx = 0; vector_idx < MorselInfo::MORSEL_VECTOR_COUNT; vector_idx++) {
				auto chunk_info = morsel_info->root->info[vector_idx].get();
				if (!chunk_info) {
					continue;
				}
				meta_writer.Write<idx_t>(vector_idx);
				chunk_info->Serialize(meta_writer);
			}
		} else {
			meta_writer.Write<idx_t>(0);
		}
		morsel_info = (MorselInfo *)morsel_info->next.get();
	}
}

void TableDataWriter::AppendData(SegmentTree &new_tree, idx_t col_idx, Vector &data, idx_t count) {
	D_ASSERT(0);
}

void TableDataWriter::FlushSegment(SegmentTree &new_tree, idx_t col_idx) {
	D_ASSERT(0);
}

void TableDataWriter::VerifyDataPointers() {
	// verify the data pointers
	idx_t table_count = 0;
	for (idx_t i = 0; i < data_pointers.size(); i++) {
		auto &data_pointer_list = data_pointers[i];
		idx_t column_count = 0;
		// then write the data pointers themselves
		for (idx_t k = 0; k < data_pointer_list.size(); k++) {
			auto &data_pointer = data_pointer_list[k];
			column_count += data_pointer.tuple_count;
		}
		if (i == 0) {
			table_count = column_count;
		} else {
			if (table_count != column_count) {
				throw Exception("Column count mismatch in data write!");
			}
		}
	}
}

void TableDataWriter::WriteDataPointers() {
	for (auto &stats : column_stats) {
		stats->Serialize(meta_writer);
	}

	for (idx_t i = 0; i < data_pointers.size(); i++) {
		// get a reference to the data column
		auto &data_pointer_list = data_pointers[i];
		meta_writer.Write<idx_t>(data_pointer_list.size());
		// then write the data pointers themselves
		for (idx_t k = 0; k < data_pointer_list.size(); k++) {
			auto &data_pointer = data_pointer_list[k];
			meta_writer.Write<idx_t>(data_pointer.row_start);
			meta_writer.Write<idx_t>(data_pointer.tuple_count);
			meta_writer.Write<block_id_t>(data_pointer.block_id);
			meta_writer.Write<uint32_t>(data_pointer.offset);
			data_pointer.decompress->Serialize(meta_writer);
			data_pointer.statistics->Serialize(meta_writer);
		}
	}
}

WriteOverflowStringsToDisk::WriteOverflowStringsToDisk(DatabaseInstance &db)
    : db(db), block_id(INVALID_BLOCK), offset(0) {
}

WriteOverflowStringsToDisk::~WriteOverflowStringsToDisk() {
	auto &block_manager = BlockManager::GetBlockManager(db);
	if (offset > 0) {
		block_manager.Write(*handle->node, block_id);
	}
}

void WriteOverflowStringsToDisk::WriteString(string_t string, block_id_t &result_block, int32_t &result_offset) {
	auto &buffer_manager = BufferManager::GetBufferManager(db);
	auto &block_manager = BlockManager::GetBlockManager(db);
	if (!handle) {
		handle = buffer_manager.Allocate(Storage::BLOCK_ALLOC_SIZE);
	}
	// first write the length of the string
	if (block_id == INVALID_BLOCK || offset + sizeof(uint32_t) >= STRING_SPACE) {
		AllocateNewBlock(block_manager.GetFreeBlockId());
	}
	result_block = block_id;
	result_offset = offset;

	// write the length field
	auto string_length = string.GetSize();
	Store<uint32_t>(string_length, handle->node->buffer + offset);
	offset += sizeof(uint32_t);
	// now write the remainder of the string
	auto strptr = string.GetDataUnsafe();
	uint32_t remaining = string_length;
	while (remaining > 0) {
		uint32_t to_write = MinValue<uint32_t>(remaining, STRING_SPACE - offset);
		if (to_write > 0) {
			memcpy(handle->node->buffer + offset, strptr, to_write);

			remaining -= to_write;
			offset += to_write;
			strptr += to_write;
		}
		if (remaining > 0) {
			// there is still remaining stuff to write
			// first get the new block id and write it to the end of the previous block
			auto new_block_id = block_manager.GetFreeBlockId();
			Store<block_id_t>(new_block_id, handle->node->buffer + offset);
			// now write the current block to disk and allocate a new block
			AllocateNewBlock(new_block_id);
		}
	}
}

void WriteOverflowStringsToDisk::AllocateNewBlock(block_id_t new_block_id) {
	auto &block_manager = BlockManager::GetBlockManager(db);
	if (block_id != INVALID_BLOCK) {
		// there is an old block, write it first
		block_manager.Write(*handle->node, block_id);
	}
	offset = 0;
	block_id = new_block_id;
}

} // namespace duckdb
