//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//
//--------------------------------------------------------------------------------------------------

#include "yb/yql/pgsql/pbgen/pg_coder.h"

namespace yb {
namespace pgsql {

using std::shared_ptr;

//--------------------------------------------------------------------------------------------------

CHECKED_STATUS PgCoder::ColumnRefsToPB(const PgTDmlStmt *tstmt,
                                       PgsqlColumnRefsPB *columns_pb) {
  // Write a list of columns to be read before executing the statement.
  const MCSet<int32>& column_refs = tstmt->column_refs();
  for (auto column_ref : column_refs) {
    columns_pb->add_ids(column_ref);
  }
  return Status::OK();
}

CHECKED_STATUS PgCoder::ColumnArgsToPB(const shared_ptr<client::YBTable>& table,
                                       const PgTDmlStmt *tstmt,
                                       PgsqlWriteRequestPB *req) {
  const MCVector<ColumnArg>& column_args = tstmt->column_args();
  for (const ColumnArg& col : column_args) {
    if (!col.IsInitialized()) {
      // This column is not assigned a value, ignore it. We don't support default value yet.
      continue;
    }

    const ColumnDesc *col_desc = col.desc();
    PgsqlExpressionPB *expr_pb;

    VLOG(3) << "WRITE request, column id = " << col_desc->id();
    if (col_desc->is_hash()) {
      expr_pb = req->add_partition_column_values();
    } else if (col_desc->is_primary()) {
      expr_pb = req->add_range_column_values();
    } else {
      PgsqlColumnValuePB* col_pb = req->add_column_values();
      col_pb->set_column_id(col_desc->id());
      expr_pb = col_pb->mutable_expr();
    }

    RETURN_NOT_OK(TExprToPB(col.expr(), expr_pb));

    // null values not allowed for primary key: checking here catches nulls introduced by bind
    if (col_desc->is_primary() && expr_pb->has_value() && IsNull(expr_pb->value())) {
      LOG(INFO) << "Unexpected null value. Current request: " << req->DebugString();
      return compile_context_->Error(col.expr(), ErrorCode::NULL_ARGUMENT_FOR_PRIMARY_KEY);
    }
  }

  return Status::OK();
}

}  // namespace pgsql
}  // namespace yb
