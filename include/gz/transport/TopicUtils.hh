/*
 * Copyright (C) 2014 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef GZ_TRANSPORT_TOPICUTILS_HH_
#define GZ_TRANSPORT_TOPICUTILS_HH_

#include <cstdint>
#include <string>
#include <vector>

#include "gz/transport/config.hh"
#include "gz/transport/Export.hh"

namespace gz::transport
{
  // Inline bracket to help doxygen filtering.
  inline namespace GZ_TRANSPORT_VERSION_NAMESPACE {
  //
  /// \class TopicUtils TopicUtils.hh gz/transport/TopicUtils.hh
  /// \brief This class provides different utilities related with topics.
  class GZ_TRANSPORT_VISIBLE TopicUtils
  {
    /// \brief Determines if a namespace is valid. A namespace's length must
    /// not exceed kMaxNameLength.
    /// The following symbols are not allowed as part of the
    /// namespace:  '@', ':=', '~'.
    /// \param[in] _ns Namespace to be checked.
    /// \return true if the namespace is valid.
    public: static bool IsValidNamespace(const std::string &_ns);

    /// \brief Determines if a partition is valid.
    /// The same rules to validate a topic name applies to a partition with
    /// the addition of the empty string, which is a valid partition (meaning
    /// no partition is used). A partition name's length must not exceed
    /// kMaxNameLength.
    /// \param[in] _partition Partition to be checked.
    /// \return true if the partition is valid.
    public: static bool IsValidPartition(const std::string &_partition);

    /// \brief Determines if a topic name is valid. A topic name is any
    /// non-empty alphanumeric string. The symbol '/' is also allowed as part
    /// of a topic name.
    /// The following symbols are not allowed as part of the
    /// topic name:  '@', ':=', '~'.
    /// A topic name's length must not exceed kMaxNameLength.
    /// Examples of valid topics: abc, /abc, /abc/de, /abc/de/
    /// \param[in] _topic Topic name to be checked.
    /// \return true if the topic name is valid.
    public: static bool IsValidTopic(const std::string &_topic);

    /// \brief Get the full topic path given a namespace and a topic name.
    /// A fully qualified topic name's length must not exceed kMaxNameLength.
    /// The fully qualified name follows the next syntax:
    /// \@\<PARTITION\>\@\<NAMESPACE\>/\<TOPIC\>
    /// where:
    /// \<PARTITION\>: The name of the partition or empty string.
    ///              A "/" will be prefixed to the partition name unless is
    ///              empty or it already starts with slash. A trailing slash
    ///              will always be removed.
    /// \<NAMESPACE\>: The namespace or empty string. A namespace is a prefix
    ///              applied to the topic name. If not empty, it will always
    ///              start with a "/". A trailing slash will always be
    ///              removed
    /// \<TOPIC\>: The topic name. A trailing slash will always be removed.
    ///
    /// Note: Intuitively, you can imagine the fully qualified name as a
    /// UNIX absolute path, where the partition is always surrounded by "@".
    /// A namespace, if present, corresponds with the directories of the
    /// path, and you can imagine the topic as the filename.
    ///
    /// E.g.:
    ///   Only topic:                @@/topic
    ///   No partition:              @@/namespace/topic1
    ///   No namespace:              @/partition@/topic1
    ///   Partition+namespace+topic: @/my_partition@/name/space/topic
    ///
    /// \param[in] _partition Partition name.
    /// \param[in] _ns Namespace.
    /// \param[in] _topic Topic name.
    /// \param[out] _name Fully qualified topic name.
    /// \return True if the fully qualified name is valid
    /// (if partition, namespace and topic are correct).
    /// \sa IsValidPartition
    /// \sa IsValidNamespace
    /// \sa IsValidTopic
    /// \sa DecomposeFullyQualifiedTopic
    public: static bool FullyQualifiedName(const std::string &_partition,
                                           const std::string &_ns,
                                           const std::string &_topic,
                                           std::string &_name);

    /// \brief Decompose a fully qualified topic name into its partition and
    /// topic strings. Note that if the topic is preceded by a namespace, then
    /// the namespace and topic name will remain together as one string.
    ///
    /// Given a fully qualified topic name with the following syntax:
    ///
    /// \@\<PARTITION\>\@\<NAMESPACE\>/\<TOPIC\>
    ///
    /// The _partition output argument will be set to \<PARTITION\>, and the
    /// _namespaceAndTopic output argument will be set to
    /// \<NAMESPACE\>/\<TOPIC\>.
    ///
    /// \param[in] _fullyQualifiedName The fully qualified topic name.
    /// \param[out] _partition The partition component of the fully qualified
    /// topic name.
    /// \param[out] _namespaceAndTopic The namespace and topic name component.
    /// Note that there is no way to distinguish between where a namespace
    /// ends and a topic name begins, since topic names may contain slashes.
    /// \return True if the topic and partition were set.
    /// \sa FullyQualifiedName
    public: static bool DecomposeFullyQualifiedTopic(
      const std::string &_fullyQualifiedName,
      std::string &_partition,
      std::string &_namespaceAndTopic);

    /// \brief Partially decompose a Zenoh liveliness token into its components.
    ///
    /// Given a Zenoh liveliness token with the following syntax:
    ///
    /// \<prefix\>/\<partition\>/\<session_id\>/\<node_id\>/\<entity_id\>/
    /// \<entity_kind\>/\<mangled_enclave\>/\<mangled_namespace\>/\<node_name\>/
    /// \<mangled_qualified_name\>/\<type_name\>/\<type_hash\>/\<qos\>
    ///
    /// The _prefix output argument will be set from \<prefix\>, the _partition
    /// output argument will be set from \<partition\>, the
    /// _namespaceAndTopic output argument will be set and unmangled from
    /// \<mangled_namespace\>/\<mangled_qualified_name\>, the _pUUID output
    /// argument will be set from \<session_id\>, the _nUUID output argument
    /// will be set from \<node_id\>, the _entityType output argument will be
    /// set from \<entity_kind\>, the _remainingToken output argument will be
    /// set from the remaining token to be processed afterwards.
    ///
    /// We're using the ROS 2 liveliness token convention.
    /// https://github.com/ros2/rmw_zenoh/blob/rolling/docs/design.md#graph-cache
    ///
    /// \param[in] _token The Zenoh liveliness token.
    /// \param[out] _prefix The prefix component.
    /// \param[out] _partition The partition component.
    /// \param[out] _namespaceAndTopic The namespace and topic name component.
    /// Note that there is no way to distinguish between where a namespace
    /// ends and a topic name begins, since topic names may contain slashes.
    /// \param[out] _pUUID The process UUID component.
    /// \param[out] _nUUID The node UUID component.
    /// \param[out] _entityType The entity type (pub, sub) component.
    /// \param[out] _remainingToken The part of the token unprocessed.
    /// \return True if all the components were set.
    public: static bool DecomposeLivelinessTokenHelper(
      const std::string &_token,
      std::string &_prefix,
      std::string &_partition,
      std::string &_namespaceAndTopic,
      std::string &_pUUID,
      std::string &_nUUID,
      std::string &_entityType,
      std::string &_remainingToken);

    /// \brief Decompose a Zenoh liveliness token into its components.
    ///
    /// Given a Zenoh liveliness token with the following syntax:
    ///
    /// \<prefix\>/\<partition\>/\<session_id\>/\<node_id\>/\<entity_id\>/
    /// \<entity_kind\>/\<mangled_enclave\>/\<mangled_namespace\>/\<node_name\>/
    /// \<mangled_qualified_name\>/\<type_name\>/\<type_hash\>/\<qos\>
    ///
    /// The _prefix output argument will be set from \<prefix\>, the _partition
    /// output argument will be set from \<partition\>, the
    /// _namespaceAndTopic output argument will be set and unmangled from
    /// \<mangled_namespace\>/\<mangled_qualified_name\>, the _pUUID output
    /// argument will be set from \<session_id\>, the _nUUID output argument
    /// will be set from \<node_id\>, the _entityType output argument will be
    /// set from \<entity_kind\>, the _msgType output argument will be set from
    /// \<type_name\>.
    ///
    /// We're using the ROS 2 liveliness token convention.
    /// https://github.com/ros2/rmw_zenoh/blob/rolling/docs/design.md#graph-cache
    ///
    /// \param[in] _token The Zenoh liveliness token.
    /// \param[out] _prefix The prefix component.
    /// \param[out] _partition The partition component.
    /// \param[out] _namespaceAndTopic The namespace and topic name component.
    /// Note that there is no way to distinguish between where a namespace
    /// ends and a topic name begins, since topic names may contain slashes.
    /// \param[out] _pUUID The process UUID component.
    /// \param[out] _nUUID The node UUID component.
    /// \param[out] _entityType The entity type.
    ///   * MP for a message publisher.
    ///   * MS for a message subscription.
    ///   * SS for a service server.
    /// \param[out] _TypeName The message type component.
    /// \return True if all the components were set.
    public: static bool DecomposeLivelinessToken(
      const std::string &_token,
      std::string &_prefix,
      std::string &_partition,
      std::string &_namespaceAndTopic,
      std::string &_pUUID,
      std::string &_nUUID,
      std::string &_entityType,
      std::string &_TypeName);

    /// \brief Decompose a Zenoh liveliness token into its components.
    ///
    /// Given a Zenoh liveliness token with the following syntax:
    ///
    /// \<prefix\>/\<partition\>/\<session_id\>/\<node_id\>/\<entity_id\>/
    /// \<entity_kind\>/\<mangled_enclave\>/\<mangled_namespace\>/\<node_name\>/
    /// \<mangled_qualified_name\>/\<type_name\>/\<type_hash\>/\<qos\>
    ///
    /// The _prefix output argument will be set from \<prefix\>, the _partition
    /// output argument will be set from \<partition\>, the
    /// _namespaceAndTopic output argument will be set and unmangled from
    /// \<mangled_namespace\>/\<mangled_qualified_name\>, the _pUUID output
    /// argument will be set from \<session_id\>, the _nUUID output argument
    /// will be set from \<node_id\>, the _entityType output argument will be
    /// set from \<entity_kind\>, the _reqType and _reptype output arguments
    /// will be set from unmangling \<type_name\>.
    ///
    /// We're using the ROS 2 liveliness token convention.
    /// https://github.com/ros2/rmw_zenoh/blob/rolling/docs/design.md#graph-cache
    ///
    /// \param[in] _token The Zenoh liveliness token.
    /// \param[out] _prefix The prefix component.
    /// \param[out] _partition The partition component.
    /// \param[out] _namespaceAndTopic The namespace and topic name component.
    /// Note that there is no way to distinguish between where a namespace
    /// ends and a topic name begins, since topic names may contain slashes.
    /// \param[out] _pUUID The process UUID component.
    /// \param[out] _nUUID The node UUID component.
    /// \param[out] _entityType The entity type.
    ///   * MP for a message publisher.
    ///   * MS for a message subscription.
    ///   * SS for a service server.
    /// \param[out] _reqType The service request message type.
    /// \param[out] _repType The service response message type.
    /// \return True if all the components were set.
    public: static bool DecomposeLivelinessToken(
      const std::string &_token,
      std::string &_prefix,
      std::string &_partition,
      std::string &_namespaceAndTopic,
      std::string &_pUUID,
      std::string &_nUUID,
      std::string &_entityType,
      std::string &_reqType,
      std::string &_repType);

    /// \brief Create a partial liveliness token.
    /// \param[in] _fullyQualifiedTopic The fully qualified topic.
    /// \param[in] _pUuid The process UUID.
    /// \param[in] _nUuid The node UUID.
    /// \param[in] _entityType The entity type.
    ///   * MP for a message publisher.
    ///   * MS for a message subscription.
    ///   * SS for a service server.
    /// \return A partial liveliness token.
    public: static std::string CreateLivelinessTokenHelper(
      const std::string &_fullyQualifiedTopic,
      const std::string &_pUuid,
      const std::string &_nUuid,
      const std::string &_entityType);

    /// \brief Create a liveliness token.
    /// \param[in] _fullyQualifiedTopic The fully qualified topic.
    /// \param[in] _pUuid The process UUID.
    /// \param[in] _nUuid The node UUID.
    /// \param[in] _entityType The entity type.
    ///   * MP for a message publisher.
    ///   * MS for a message subscription.
    ///   * SS for a service server.
    ///
    /// We're using the ROS 2 liveliness token convention.
    /// https://github.com/ros2/rmw_zenoh/blob/rolling/docs/design.md#graph-cache
    ///
    /// \param[in] _msgTypeName The message type.
    /// \return The liveliness token.
    public: static std::string CreateLivelinessToken(
      const std::string &_fullyQualifiedTopic,
      const std::string &_pUuid,
      const std::string &_nUuid,
      const std::string &_entityType,
      const std::string &_typeName);

    /// \brief Create a liveliness token.
    /// \param[in] _fullyQualifiedTopic The fully qualified topic.
    /// \param[in] _pUuid The process UUID.
    /// \param[in] _nUuid The node UUID.
    /// \param[in] _entityType The entity type.
    ///   * MP for a message publisher.
    ///   * MS for a message subscription.
    ///   * SS for a service server.
    /// \param[in] _reqTypeName The service request type.
    /// \param[in] _repTypeName The service response type.
    ///
    /// We're using the ROS 2 liveliness token convention.
    /// https://github.com/ros2/rmw_zenoh/blob/rolling/docs/design.md#graph-cache
    ///
    /// \return The liveliness token.
    public: static std::string CreateLivelinessToken(
      const std::string &_fullyQualifiedTopic,
      const std::string &_pUuid,
      const std::string &_nUuid,
      const std::string &_entityType,
      const std::string &_reqTypeName,
      const std::string &_repTypeName);

    /// \brief Convert a topic name to a valid topic. The input topic is
    /// modified by:
    /// * turning white space into `_`.
    /// * removing special characters and combinations.
    /// \param[in] _topic Input topic, which may be invalid.
    /// \return A valid topic, or empty string if not possible to convert.
    public: static std::string AsValidTopic(const std::string &_topic);

    /// \brief Replace "/" instances with "%".
    /// \param[in] _input Input name.
    /// \return The mangled name.
    public: static std::string MangleName(const std::string &_input);

    /// \brief Recompose a previously mangled name.
    /// \param[in] _input Input mangled name.
    /// \return The unmangled name.
    public: static std::string DemangleName(const std::string &_input);

    /// \brief Mangle multiple types into a single string using "&" as delimiter
    /// \param[in] _input Input vector with types.
    /// \param[out] _output The mangled string.
    /// \return True if the mangled worked succesfully.
    public: static bool MangleType(const std::vector<std::string> &_input,
                                   std::string &_output);

    /// \brief Recompose a previously mangled type.
    /// \param[in] _input Input mangled type.
    /// \param[out] _output The unmangled vector with types.
    /// \return True if the demanged worked succesfully.
    public: static bool DemangleType(const std::string &_input,
                                     std::vector<std::string> &_output);

    /// \brief The kMaxNameLength specifies the maximum number of characters
    /// allowed in a namespace, a partition name, a topic name, and a fully
    /// qualified topic name.
    public: static const uint16_t kMaxNameLength = 65535;

    /// \brief The separator used within the liveliness token.
    public: static const char kTokenSeparator[];

    /// \brief The separator used to concatenate type names.
    public: static const char kTypeSeparator[];

    /// \brief A common prefix for all liveliness tokens.
    public: static const char kTokenPrefix[];

    /// \brief A replacement for the slash when mangling names.
    public: static const char kSlashReplacement;
  };
  }
}

#endif
