// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef JUBATUS_CORE_FV_CONVERTER_WEIGHT_MANAGER_HPP_
#define JUBATUS_CORE_FV_CONVERTER_WEIGHT_MANAGER_HPP_

#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <msgpack.hpp>
#include "jubatus/util/data/unordered_map.h"
#include "../framework/model.hpp"
#include "../common/type.hpp"
#include "../common/version.hpp"
#include "counter.hpp"
#include "datum.hpp"
#include "keyword_weights.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

struct versioned_weight_diff {
  versioned_weight_diff();
  explicit versioned_weight_diff(const fv_converter::keyword_weights& w);
  versioned_weight_diff(const fv_converter::keyword_weights& w,
                        const storage::version& v);
  versioned_weight_diff& merge(const versioned_weight_diff& target);

  MSGPACK_DEFINE(weights_, version_);

  fv_converter::keyword_weights weights_;
  storage::version version_;
};

class weight_manager : public framework::model {
 public:
  weight_manager();

  void update_weight(const common::sfv_t& fv);
  void get_weight(common::sfv_t& fv) const;

  void add_weight(const std::string& key, float weight);

  void get_diff(versioned_weight_diff& diff) const {
    diff = versioned_weight_diff(diff_weights_, version_);
  }

  bool put_diff(const versioned_weight_diff& diff) {
    if (diff.version_ == version_) {
      master_weights_.merge(diff.weights_);
      diff_weights_.clear();
      version_.increment();
      return true;
    } else {
      return false;
    }
  }

  void mix(
      const versioned_weight_diff& lhs,
      versioned_weight_diff& acc) const {
    if (lhs.version_ == acc.version_) {
      acc.weights_.merge(lhs.weights_);
    } else if (lhs.version_ > acc.version_) {
      acc = lhs;
    }
  }

  void clear() {
    diff_weights_.clear();
    master_weights_.clear();
  }

  storage::version get_version() const {
    return version_;
  }

  MSGPACK_DEFINE(version_, diff_weights_, master_weights_);

  void pack(framework::packer& pk) const {
    pk.pack(*this);
  }

  void unpack(msgpack::object o) {
    o.convert(this);
  }

  std::string to_string() const {
    std::stringstream ss;
    ss << "version:" << version_
       << " diff_weights:" << diff_weights_.to_string()
       << " master_weights:" << master_weights_.to_string();
    return ss.str();
  }

 private:
  uint64_t get_document_count() const {
    return diff_weights_.get_document_count() +
        master_weights_.get_document_count();
  }

  size_t get_document_frequency(const std::string& key) const {
    return diff_weights_.get_document_frequency(key) +
        master_weights_.get_document_frequency(key);
  }

  double get_user_weight(const std::string& key) const {
    return diff_weights_.get_user_weight(key) +
        master_weights_.get_user_weight(key);
  }

  double get_global_weight(const std::string& key) const;

  storage::version version_;
  keyword_weights diff_weights_;
  keyword_weights master_weights_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_WEIGHT_MANAGER_HPP_
