//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/storage/checkpoint_manager.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/common.hpp"
#include "duckdb/common/types/chunk_collection.hpp"
#include "duckdb/storage/storage_manager.hpp"
#include "duckdb/storage/meta_block_writer.hpp"
#include "duckdb/storage/statistics/base_statistics.hpp"

namespace duckdb {
class ClientContext;
class MetaBlockReader;
class SchemaCatalogEntry;
class SequenceCatalogEntry;
class TableCatalogEntry;
class ViewCatalogEntry;

enum class BlockType : uint8_t { INVALID = 0, UNCOMPRESSED = 100, COMPRESSED = 200 };

class DataPointer {
public:
	DataPointer(){};
	BlockType block_type;
	uint64_t row_start;
	uint64_t tuple_count;
	block_id_t block_id;
	uint32_t offset;
	//! Type-specific statistics of the segment
	unique_ptr<BaseStatistics> statistics;
};

//! CheckpointManager is responsible for checkpointing the database
class CheckpointManager {
public:
	CheckpointManager(StorageManager &manager);

	//! Checkpoint the current state of the WAL and flush it to the main storage. This should be called BEFORE any
	//! connction is available because right now the checkpointing cannot be done online. (TODO)
	void CreateCheckpoint();
	//! Load from a stored checkpoint
	void LoadFromStorage();

	//! The block manager to write the checkpoint to
	BlockManager &block_manager;
	//! The buffer manager
	BufferManager &buffer_manager;
	//! The database this storagemanager belongs to
	DatabaseInstance &database;
	//! The metadata writer is responsible for writing schema information
	unique_ptr<MetaBlockWriter> metadata_writer;
	//! The table data writer is responsible for writing the DataPointers used by the table chunks
	unique_ptr<MetaBlockWriter> tabledata_writer;

private:
	void WriteSchema(ClientContext &context, SchemaCatalogEntry &schema);
	void WriteTable(ClientContext &context, TableCatalogEntry &table);
	void WriteView(ViewCatalogEntry &table);
	void WriteSequence(SequenceCatalogEntry &table);
	void WriteMacro(MacroCatalogEntry &table);

	void ReadSchema(ClientContext &context, MetaBlockReader &reader);
	void ReadTable(ClientContext &context, MetaBlockReader &reader);
	void ReadView(ClientContext &context, MetaBlockReader &reader);
	void ReadSequence(ClientContext &context, MetaBlockReader &reader);
	void ReadMacro(ClientContext &context, MetaBlockReader &reader);
};

} // namespace duckdb
