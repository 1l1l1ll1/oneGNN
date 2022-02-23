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
#ifndef ONEFLOW_CORE_VM_STREAM_H_
#define ONEFLOW_CORE_VM_STREAM_H_

#include "oneflow/core/vm/instruction.h"
#include "oneflow/core/device/device_context.h"
#include "oneflow/core/common/symbol.h"
#include "oneflow/core/common/stream_role.h"

namespace oneflow {

class Device;

namespace vm {

struct ThreadCtx;
struct StreamType;

class Stream final : public intrusive::Base {
 public:
  // types
  using DispatchedInstructionList =
      intrusive::List<INTRUSIVE_FIELD(Instruction, dispatched_instruction_hook_)>;

  // Getters
  const ThreadCtx& thread_ctx() const { return *thread_ctx_; }
  bool has_thread_ctx() const { return thread_ctx_ != nullptr; }
  const std::unique_ptr<DeviceCtx>& device_ctx() const { return device_ctx_; }
  const intrusive::ListHook& active_stream_hook() const { return active_stream_hook_; }
  const DispatchedInstructionList& free_instruction_list() const { return free_instruction_list_; }
  const DispatchedInstructionList& zombie_instruction_list() const {
    return zombie_instruction_list_;
  }
  const DispatchedInstructionList& running_instruction_list() const {
    return running_instruction_list_;
  }

  // Setters
  ThreadCtx* mut_thread_ctx() { return thread_ctx_; }
  void set_thread_ctx(ThreadCtx* val) { thread_ctx_ = val; }
  void clear_thread_ctx() { thread_ctx_ = nullptr; }
  std::unique_ptr<DeviceCtx>* mut_device_ctx() { return &device_ctx_; }
  DispatchedInstructionList* mut_free_instruction_list() { return &free_instruction_list_; }
  DispatchedInstructionList* mut_zombie_instruction_list() { return &zombie_instruction_list_; }
  DispatchedInstructionList* mut_running_instruction_list() { return &running_instruction_list_; }

  // methods
  void __Init__(ThreadCtx* thread_ctx, Symbol<Device> device, StreamRole stream_role);
  intrusive::shared_ptr<Instruction> NewInstruction(InstructionMsg* instr_msg);
  void DeleteInstruction(intrusive::shared_ptr<Instruction>&&);
  int64_t device_id() const;
  Symbol<Device> device() const { return device_; }
  StreamRole stream_role() const { return stream_role_; }
  const StreamType& stream_type() const;

 private:
  void MoveToFreeList(intrusive::shared_ptr<Instruction>&& instruction);
  void MoveFromZombieListToFreeList();

  friend class intrusive::Ref;
  intrusive::Ref* mut_intrusive_ref() { return &intrusive_ref_; }

  Stream()
      : intrusive_ref_(),
        thread_ctx_(),
        device_ctx_(),
        device_(),
        stream_role_(StreamRole::kInvalid),
        stream_type_(),
        free_instruction_list_(),
        zombie_instruction_list_(),
        running_instruction_list_(),
        active_stream_hook_(),
        thread_ctx_stream_hook_() {}
  intrusive::Ref intrusive_ref_;
  // fields
  ThreadCtx* thread_ctx_;
  Symbol<Device> device_;
  StreamRole stream_role_;
  const StreamType* stream_type_;
  std::unique_ptr<DeviceCtx> device_ctx_;
  // lists
  DispatchedInstructionList free_instruction_list_;
  DispatchedInstructionList zombie_instruction_list_;
  DispatchedInstructionList running_instruction_list_;

 public:
  // list hooks
  intrusive::ListHook active_stream_hook_;
  intrusive::ListHook thread_ctx_stream_hook_;
};

}  // namespace vm
}  // namespace oneflow

#endif  // ONEFLOW_CORE_VM_STREAM_H_
