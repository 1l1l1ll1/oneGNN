/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <glog/logging.h>
#include "oneflow/core/common/data_type.h"
#include "oneflow/core/common/maybe.h"
#include "oneflow/core/common/shape.h"
#include "oneflow/core/common/shape_view.h"
#include "oneflow/core/common/stride.h"
#include "oneflow/core/framework/framework.h"
#include "oneflow/core/framework/op_generated.h"

namespace oneflow {

/*static*/ Maybe<void> TransposedBinaryOp::GetSbp(user_op::SbpContext* ctx) {
  const bool inplace = ctx->Attr<bool>("inplace");
  ctx->NewBuilder()
      .Broadcast(user_op::OpArg("lhs", 0))
      .Broadcast(user_op::OpArg("rhs", 0))
      .Broadcast(user_op::OpArg("y", 0))
      .Build();
  return Maybe<void>::Ok();
}

/*static*/ Maybe<void> TransposedBinaryOp::InferLogicalTensorDesc(user_op::InferContext* ctx) {
  const Shape& lhs = ctx->InputShape("lhs", 0);
  const Shape& rhs = ctx->InputShape("rhs", 0);
  CHECK_EQ_OR_RETURN(lhs.NumAxes(), rhs.NumAxes());
  for (int i = 0; i < lhs.NumAxes(); i++) CHECK_EQ_OR_RETURN(lhs.At(i), rhs.At(i));
  ctx->SetOutputShape("y", 0, lhs);
  // const bool inplace = ctx->Attr<bool>("inplace");
  // if (inplace)
  ctx->SetOutputStride("y", 0, ctx->InputStride("lhs", 0));
  return Maybe<void>::Ok();
}
/*static*/ Maybe<void> TransposedBinaryOp::InferPhysicalTensorDesc(user_op::InferContext* ctx) {
  return InferLogicalTensorDesc(ctx);
}
/*static*/ Maybe<void> TransposedBinaryOp::InferDataType(user_op::InferContext* ctx) {
  auto lhs = ctx->InputDType("lhs", 0);
  auto rhs = ctx->InputDType("rhs", 0);
  ctx->SetOutputDType("y", 0, GetSizeOfDataType(lhs) >= GetSizeOfDataType(rhs) ? lhs : rhs);
  return Maybe<void>::Ok();
}

/*static*/ Maybe<void> TransposedBinaryOpGrad::GetSbp(user_op::SbpContext* ctx) {
  const bool inplace = ctx->Attr<bool>("inplace");
  ctx->NewBuilder()
      .Broadcast(user_op::OpArg("lhs", 0))
      .Broadcast(user_op::OpArg("rhs", 0))
      .Broadcast(user_op::OpArg("y", 0))
      .Build();
  return Maybe<void>::Ok();
}

/*static*/ Maybe<void> TransposedBinaryOpGrad::InferLogicalTensorDesc(user_op::InferContext* ctx) {
  const Shape& lhs = ctx->InputShape("lhs", 0);
  const Shape& rhs = ctx->InputShape("rhs", 0);
  CHECK_EQ_OR_RETURN(lhs.NumAxes(), rhs.NumAxes());
  for (int i = 0; i < lhs.NumAxes(); i++) CHECK_EQ_OR_RETURN(lhs.At(i), rhs.At(i));
  ctx->SetOutputShape("y", 0, lhs);
  ctx->SetOutputStride("y", 0, ctx->InputStride("lhs", 0));
  return Maybe<void>::Ok();
}
/*static*/ Maybe<void> TransposedBinaryOpGrad::InferPhysicalTensorDesc(user_op::InferContext* ctx) {
  return InferLogicalTensorDesc(ctx);
}
/*static*/ Maybe<void> TransposedBinaryOpGrad::InferDataType(user_op::InferContext* ctx) {
  auto lhs = ctx->InputDType("lhs", 0);
  auto rhs = ctx->InputDType("rhs", 0);
  ctx->SetOutputDType("y", 0, GetSizeOfDataType(lhs) >= GetSizeOfDataType(rhs) ? lhs : rhs);
  return Maybe<void>::Ok();
}

}  // namespace oneflow
