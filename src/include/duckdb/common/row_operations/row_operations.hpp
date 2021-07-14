//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/common/row_operations/row_operations.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/enums/expression_type.hpp"
#include "duckdb/common/vector.hpp"

namespace duckdb {

struct AggregateObject;
class DataChunk;
class RowLayout;
class RowDataCollection;
struct SelectionVector;
class StringHeap;
class Vector;
struct VectorData;

// RowOperations contains a set of operations that operate on data using a RowLayout
struct RowOperations {
	//===--------------------------------------------------------------------===//
	// Aggregation Operators
	//===--------------------------------------------------------------------===//
	//! initialize - unaligned addresses
	static void InitializeStates(RowLayout &layout, Vector &addresses, const SelectionVector &sel, idx_t count);
	//! destructor - unaligned addresses, updated
	static void DestroyStates(RowLayout &layout, Vector &addresses, idx_t count);
	//! update - aligned addresses
	static void UpdateStates(AggregateObject &aggr, Vector &addresses, DataChunk &payload, idx_t arg_idx, idx_t count);
	//! filtered update - aligned addresses
	static void UpdateFilteredStates(AggregateObject &aggr, Vector &addresses, DataChunk &payload, idx_t arg_idx);
	//! combine - unaligned addresses, updated
	static void CombineStates(RowLayout &layout, Vector &sources, Vector &targets, idx_t count);
	//! finalize - unaligned addresses, updated
	static void FinalizeStates(RowLayout &layout, Vector &addresses, DataChunk &result, idx_t aggr_idx);

	//===--------------------------------------------------------------------===//
	// Read/Write Operators
	//===--------------------------------------------------------------------===//
	//! Scatter group data to the rows. Initialises the ValidityMask.
	static void Scatter(DataChunk &columns, VectorData col_data[], const RowLayout &layout, Vector &rows,
	                    RowDataCollection &string_heap, const SelectionVector &sel, idx_t count);
	//! Gather a single column
	static void Gather(Vector &rows, const SelectionVector &row_sel, Vector &col, const SelectionVector &col_sel,
	                   const idx_t count, const idx_t col_offset, const idx_t col_no);

	//===--------------------------------------------------------------------===//
	// Comparison Operators
	//===--------------------------------------------------------------------===//
	//! Compare a block of key data against the row values to produce an updated selection that matches
	//! and a second (optional) selection of non-matching values.
	//! Returns the number of matches remaining in the selection.
	using Predicates = vector<ExpressionType>;

	static idx_t Match(DataChunk &columns, VectorData col_data[], const RowLayout &layout, Vector &rows,
	                   const Predicates &predicates, SelectionVector &sel, idx_t count, SelectionVector *no_match,
	                   idx_t &no_match_count);

	//===--------------------------------------------------------------------===//
	// Out-of-Core Operators
	//===--------------------------------------------------------------------===//
	//! Swizzles blob pointers to offset within heap row
	static void SwizzleColumns(const RowLayout &layout, const data_ptr_t base_row_ptr, const idx_t count);
	//! Swizzles the base pointer of each row to offset within heap block
	static void SwizzleHeapPointer(const RowLayout &layout, data_ptr_t row_ptr, const data_ptr_t heap_base_ptr,
	                               const idx_t count);
	//! Swizzles the base offset of each row back to a pointer
	static void UnswizzleHeapPointer(const RowLayout &layout, data_ptr_t row_ptr, const data_ptr_t heap_base_ptr,
	                                 const idx_t count);
	//! Unswizzles offsets back to pointers to blobs
	static void UnswizzleColumns(const RowLayout &layout, const data_ptr_t base_row_ptr, const idx_t count);
};

} // namespace duckdb
