// Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "carla/Debug.h"
#include "carla/sensor/data/Array.h"
#include "carla/sensor/s11n/RadioSerializer.h"
#include "carla/sensor/data/RadioData.h"

namespace carla {
namespace sensor {
namespace data {

  /// Measurement produced by a Radar. Consists of an array of RadioDetection.
  /// A RadioDetection contains 4 floats: velocity, azimuth, altitude and depth
  class RadioMeasurement : public Array<data::RadioDetection> {
    using Super = Array<data::RadioDetection>;
  protected:

    using Serializer = s11n::RadioSerializer;

    friend Serializer;

    explicit RadioMeasurement(RawData &&data)
      : Super(0u, std::move(data)) {}

  public:

    Super::size_type GetDetectionAmount() const {
      return Super::size();
    }
  };

} // namespace data
} // namespace sensor
} // namespace carla
