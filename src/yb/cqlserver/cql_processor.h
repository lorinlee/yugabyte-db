//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// This module is to define CQL processor. Each processor will be handling one and only one request
// at a time. As a result all processing code in this module and the modules that it is calling
// does not need to be thread safe.
//--------------------------------------------------------------------------------------------------

#ifndef YB_CQLSERVER_CQL_PROCESSOR_H_
#define YB_CQLSERVER_CQL_PROCESSOR_H_

#include "yb/client/client.h"
#include "yb/cqlserver/cql_message.h"
#include "yb/sql/sql_processor.h"
#include "yb/rpc/inbound_call.h"

namespace yb {
namespace cqlserver {

class CQLMetrics : public sql::YbSqlMetrics {
 public:
  explicit CQLMetrics(const scoped_refptr<yb::MetricEntity>& metric_entity);

  scoped_refptr<yb::Histogram> time_to_process_request_;
  scoped_refptr<yb::Histogram> time_to_get_cql_processor_;
  scoped_refptr<yb::Histogram> time_to_parse_cql_wrapper_;
  scoped_refptr<yb::Histogram> time_to_execute_cql_request_;

  scoped_refptr<yb::Histogram> time_to_queue_cql_response_;
  scoped_refptr<yb::Counter> num_errors_parsing_cql_;
};

class CQLProcessor : public sql::SqlProcessor {
 public:
  // Public types.
  typedef std::unique_ptr<CQLProcessor> UniPtr;
  typedef std::unique_ptr<const CQLProcessor> UniPtrConst;

  // Constructor and destructor.
  CQLProcessor(std::shared_ptr<client::YBClient> client, std::shared_ptr<CQLMetrics> metrics);
  ~CQLProcessor();

  // Processing an inbound call.
  void ProcessCall(const Slice& msg, std::unique_ptr<CQLResponse> *response);

  // Process SQL statement.
  CQLResponse *ProcessQuery(const QueryRequest& req);

 private:
  std::shared_ptr<CQLMetrics> cql_metrics_;
};

}  // namespace cqlserver
}  // namespace yb

#endif  // YB_CQLSERVER_CQL_PROCESSOR_H_
