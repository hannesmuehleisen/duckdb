//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/storage/statistics/numeric_statistics.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/storage/statistics/base_statistics.hpp"
#include "duckdb/common/serializer.hpp"
#include "duckdb/common/limits.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/types/value.hpp"

namespace duckdb {

class NumericStatistics : public BaseStatistics {
public:
	explicit NumericStatistics(LogicalType type);
	NumericStatistics(LogicalType type, Value min, Value max);

	//! The minimum value of the segment
	Value min;
	//! The maximum value of the segment
	Value max;

public:
	void Merge(const BaseStatistics &other) override;
	bool CheckZonemap(ExpressionType comparison_type, const Value &constant);

	unique_ptr<BaseStatistics> Copy() override;
	void Serialize(Serializer &serializer) override;
	static unique_ptr<BaseStatistics> Deserialize(Deserializer &source, LogicalType type);
	void Verify(Vector &vector, idx_t count) override;

	string ToString() override;

	void Update(Vector &vector, idx_t count) override;

private:
	template <class T>
	void TemplatedVerify(Vector &vector, idx_t count);
	template <class T>
	void TemplatedUpdate(Vector &vector, idx_t count);
};

} // namespace duckdb
