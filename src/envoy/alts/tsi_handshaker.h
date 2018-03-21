/* Copyright 2018 Istio Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <memory>
#include <mutex>

#include "envoy/buffer/buffer.h"
#include "envoy/event/dispatcher.h"

#include "src/core/tsi/transport_security_interface.h"

namespace Envoy {
namespace Security {

// An interface to get callback from TsiHandshaker
class TsiHandshakerCallbacks {
 public:
  virtual ~TsiHandshakerCallbacks() {}

  struct NextResult {
    // A enum of the result
    tsi_result status_;

    // The buffer to be sent to the peer
    Buffer::InstancePtr to_send_;

    // A pointer to tsi_handshaker_result struct. Owned by instance.
    // TODO(lizan): consider add a C++ wrapper.
    tsi_handshaker_result* result_;
  };

  typedef std::unique_ptr<NextResult> NextResultPtr;

  /**
   * Called when `next` is done, this may be called in line in `next` if the
   * handshaker is not
   * asynchronous.
   * @param result
   */
  virtual void onNextDone(NextResultPtr&& result) PURE;
};

// A C++ wrapper for tsi_handshaker interface
class TsiHandshaker : Event::DeferredDeletable {
 public:
  explicit TsiHandshaker(tsi_handshaker* handshaker,
                         Event::Dispatcher& dispatcher);
  virtual ~TsiHandshaker();

  /**
   * Conduct next step of handshake, see
   * https://github.com/grpc/grpc/blob/v1.10.0/src/core/tsi/transport_security_interface.h#L416
   * @param received the buffer received from peer.
   */
  tsi_result next(Buffer::Instance& received);

  /**
   * Set handshaker callbacks, this must be called before calling next.
   * @param callbacks supplies the callback instance.
   */
  void setHandshakerCallbacks(TsiHandshakerCallbacks& callbacks) {
    callbacks_ = &callbacks;
  }

  /**
   * Delete the handshaker when it is ready. This must be called after releasing
   * from a smart
   * pointer. The actual delete happens after ongoing next call are processed.
   */
  void deferredDelete();

 private:
  static void onNextDone(tsi_result status, void* user_data,
                         const unsigned char* bytes_to_send,
                         size_t bytes_to_send_size,
                         tsi_handshaker_result* handshaker_result);

  tsi_handshaker* handshaker_{nullptr};
  TsiHandshakerCallbacks* callbacks_{nullptr};
  bool calling_{false};
  bool delete_on_done_{false};
  Event::Dispatcher& dispatcher_;
};

typedef std::unique_ptr<TsiHandshaker> TsiHandshakerPtr;
}
}
