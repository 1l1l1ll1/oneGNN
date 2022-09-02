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
#include "oneflow/core/graph/exec_graph.h"
#include <sstream>
#include "oneflow/core/common/just.h"
#include "oneflow/core/graph/op_graph.h"

namespace oneflow {

void ExecNode::BindBnWithRegst(const std::string& bn, std::shared_ptr<RegstDesc> regst) {
  CHECK(bn_in_op2regst_.emplace(bn, regst).second);
}

void ExecNode::BindBnsWithRegst(const PbRpf<std::string>& (Operator::*bns_getter)() const,
                                std::shared_ptr<RegstDesc> regst) {
  for (const std::string& bn : (op_.get()->*bns_getter)()) { BindBnWithRegst(bn, regst); }
}

void ExecNode::AddBnToRegstAndBindIt(const PbRpf<std::string>& (Operator::*bns_getter)() const,
                                     std::shared_ptr<RegstDesc> regst) {
  for (const std::string& bn : (op_.get()->*bns_getter)()) { regst->AddLbi(op_->BnInOp2Lbi(bn)); }
  BindBnsWithRegst(bns_getter, regst);
}

bool ExecNode::TryBindBnWithOneOfTheRegsts(const std::string& bn,
                                           const std::list<std::shared_ptr<RegstDesc>>& regsts) {
  const LogicalBlobId& lbi = op()->BnInOp2Lbi(bn);
  bool has_binded = false;
  for (std::shared_ptr<RegstDesc> regst : regsts) {
    // TODO(strint): this is strange, try to remove this.
    if (regst->GetBlobDesc(lbi) == nullptr) { continue; }
    BindBnWithRegst(bn, regst);
    has_binded = true;
    break;
  }
  return has_binded;
}

void ExecNode::BindBnWithOneOfTheRegsts(const std::string& bn,
                                        const std::list<std::shared_ptr<RegstDesc>>& regsts) {
  CHECK(TryBindBnWithOneOfTheRegsts(bn, regsts));
}

void ExecNode::UnbindBnWithEmptyRegst() {
  EraseIf<std::string, std::shared_ptr<RegstDesc>>(
      &bn_in_op2regst_, [](HashMap<std::string, std::shared_ptr<RegstDesc>>::iterator it) {
        return it->second->regst_desc_type().has_data_regst_desc() && it->second->NumOfLbi() == 0;
      });
}

void ExecNode::ToProto(const ParallelContext* parallel_ctx, const bool need_op_attr, ExecNodeProto* ret) const {
  op_->GenKernelConf(GetRegstBlobDesc4BnInOpFunc(), parallel_ctx, need_op_attr, ret->mutable_kernel_conf());
  for (const auto& bn_regst : bn_in_op2regst_) {
    const std::string& bn_in_op = bn_regst.first;
    auto regst = bn_regst.second;
    CHECK(regst);
    PbMapPair<std::string, int64_t> pair{bn_in_op, regst->regst_desc_id()};
    CHECK(ret->mutable_bn_in_op2regst_desc_id()->insert(pair).second);
  }
}

namespace {

Maybe<void> CheckPhysicalBlobDesc(const BlobDesc& logical, const NdSbp& nd_sbp,
                                  const ParallelDesc& parallel_desc,
                                  const ParallelContext* parallel_ctx, const BlobDesc& physical) {
  CHECK_EQ_OR_RETURN(physical.shape(), *JUST(GetPhysicalShape(logical.shape(), nd_sbp,
                                                              parallel_desc, *parallel_ctx)));
  return Maybe<void>::Ok();
}

Maybe<void> CheckPhysicalBlobDesc(
    const Operator& op, const PbRpf<std::string>& bns,
    const std::function<Maybe<const BlobDesc>(const std::string&)>& GetLogicalBlobDesc,
    const NdSbpSignature* nd_sbp_signature, const ParallelContext* parallel_ctx,
    const std::function<BlobDesc*(const std::string&)>& GetPhysicalBlobDesc) {
  const std::shared_ptr<const ParallelDesc> op_parallel_desc = JUST(op.GetOpParallelDesc());
  for (const auto& bn : bns) {
    const BlobDesc* physical_blob_desc = GetPhysicalBlobDesc(bn);
    if (physical_blob_desc == nullptr) {
      // TODO(liujuncheng): remove this hotfix
      continue;
    }
    if (*JUST(op.GetParallelDesc4BnInOp(bn)) == *op_parallel_desc) {
      JUST_MSG(CheckPhysicalBlobDesc(*JUST(GetLogicalBlobDesc(bn)),
                                     nd_sbp_signature->bn_in_op2nd_sbp().at(bn), *op_parallel_desc,
                                     parallel_ctx, *physical_blob_desc),
               std::stringstream() << " check physical shape failed, op name " << op.op_loc());
    }
  }
  return Maybe<void>::Ok();
}

}  // namespace

void ExecNode::InferBlobDescs(const OpNode* op_node, const ParallelContext* parallel_ctx) {
  auto GetBlobDesc4BnInOp = GetRegstBlobDesc4BnInOpFunc();
  // NOTE(strint): bad news, lots of InferTmpSizeFn use input register TensorDesc.
  CHECK_JUST_MSG(op_->InferBlobDescsIf(GetBlobDesc4BnInOp, parallel_ctx, &GlobalJobDesc()),
                 std::stringstream() << " infer blob descs if failed, op name " << op_->op_loc());
  // NOTE(strint): Inplace infer doesn't need the real blob desc value, pass GetBlobDesc4BnInOp is just becase some
  // infer context's constructor need it. 
  CHECK_JUST_MSG(op_->InferInplaceObn2IbnIf(&mut_inplace_obn2ibn_, &con_inplace_obn2ibn_,
                                            GetBlobDesc4BnInOp, parallel_ctx),
                 std::stringstream()
                     << " infer inplace obn to ibn if failed, op name " << op_->op_loc());
}

std::function<BlobDesc*(const std::string&)> ExecNode::GetRegstBlobDesc4BnInOpFunc() const {
  return [this](const std::string& bn_in_op) -> BlobDesc* {
    auto it = bn_in_op2regst_.find(bn_in_op);
    if (it == bn_in_op2regst_.end()) { return nullptr; }
    std::shared_ptr<RegstDesc> regst = it->second;
    CHECK(regst);
    auto ret = regst->MutBlobDesc(op()->BnInOp2Lbi(bn_in_op));
    return ret;
  };
}

void ExecGraph::ToExecSequence(const ParallelContext* parallel_ctx, const bool need_op_attr, ExecSequence* ret) const {
  TopoForEachNode([&](ExecNode* node) { node->ToProto(parallel_ctx, need_op_attr, ret->add_exec_node()); });
}

}  // namespace oneflow
