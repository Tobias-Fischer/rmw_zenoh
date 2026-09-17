// Copyright 2024 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DETAIL__RMW_CONTEXT_IMPL_S_HPP_
#define DETAIL__RMW_CONTEXT_IMPL_S_HPP_

#include <cstddef>
#include <memory>
#include <string>

#include <zenoh.hxx>

#include "buffer_backend_context.hpp"
#include "graph_cache.hpp"
#include "rmw_node_data.hpp"
#include "zenoh_utils.hpp"

#include "rmw/ret_types.h"
#include "rmw/types.h"

///=============================================================================
struct rmw_context_impl_s final
{
public:
  // Constructor that internally initializes the Zenoh session and other artifacts.
  // Throws an std::runtime_error if any of the initializations fail.
  // The construction will block until a Zenoh router is detected.
  // TODO(Yadunund): Make this a non-blocking call by checking for the Zenoh
  // router in a separate thread. Instead block when creating a node if router
  // check has not succeeded.
  // On wasm32 (no real threads), the constructor does not block at all: it
  // only starts opening the session. create_node_data() drives the rest of
  // the (non-blocking) session setup to completion, one poll at a time.
  rmw_context_impl_s(
    const std::size_t domain_id,
    const std::string & enclave);

  ~rmw_context_impl_s();

  // Get a copy of the enclave.
  std::string enclave() const;

  // Loan the Zenoh session.
  const std::shared_ptr<zenoh::Session> session() const;

  // Get an shm subsystem.
  const std::shared_ptr<rmw_zenoh_cpp::ShmContext> shm() const;

  // Get the graph guard condition.
  rmw_guard_condition_t * graph_guard_condition();

  // Get a unique id for a new entity.
  std::size_t get_next_entity_id();

  // Shutdown the Zenoh session.
  rmw_ret_t shutdown();

  // Check if the Zenoh session is shutdown.
  bool is_shutdown() const;

  // Returns true if the Zenoh session is valid.
  bool session_is_valid() const;

  /// Return a shared_ptr to the GraphCache stored in this context.
  std::shared_ptr<rmw_zenoh_cpp::GraphCache> graph_cache();

  /// Return a shared_ptr to the Serialization buffer pool stored in this context.
  std::shared_ptr<rmw_zenoh_cpp::BufferPool> serialization_buffer_pool();

  /// Create a NodeData and store it within this context. The NodeData can be
  /// retrieved using get_node().
  /// Returns false if parameters are invalid.
  bool create_node_data(
    const rmw_node_t * const node,
    const std::string & ns,
    const std::string & node_name);

  /// Retrieve the NodeData for a given rmw_node_t if present.
  std::shared_ptr<rmw_zenoh_cpp::NodeData> get_node_data(
    const rmw_node_t * const node);

  /// Delete the NodeData for a given rmw_node_t if present.
  void delete_node_data(const rmw_node_t * const node);

  /// Return a pointer to the per-context BufferBackendContext.
  rmw_zenoh_cpp::BufferBackendContext * buffer_backend_context();

#if defined(__wasm32__)
  /// Drive the zenoh runtime forward by one non-blocking step. Must be called
  /// repeatedly (e.g. from rmw_wait()'s polling loop) for any zenoh I/O --
  /// publishing, receiving, discovery -- to make progress. A no-op until the
  /// session has finished opening.
  void wasm_pump_once();
#endif

  // Forward declaration
  class Data;

private:
  std::shared_ptr<Data> data_{nullptr};
};

#endif  // DETAIL__RMW_CONTEXT_IMPL_S_HPP_
