#!/usr/bin/env python

# Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma de
# Barcelona (UAB).
#
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT>.

import glob
import os
import sys

try:
    sys.path.append(glob.glob('../carla/dist/carla-*%d.%d-%s.egg' % (
        sys.version_info.major,
        sys.version_info.minor,
        'win-amd64' if os.name == 'nt' else 'linux-x86_64'))[0])
except IndexError:
    pass

import carla
import random
import time

def callback(data):
    set_of_actors = set()
    for detection in data:
        set_of_actors.add(detection.hitted_actor_id)
    print(set_of_actors)


def main():
    actor_list = []
    try:        
        client = carla.Client('localhost', 2000)
        client.set_timeout(2.0)

        world = client.get_world()
        blueprint_library = world.get_blueprint_library()

        bp = blueprint_library.find('vehicle.tesla.model3')
        
        if bp.has_attribute('color'):
            color = random.choice(bp.get_attribute('color').recommended_values)
            bp.set_attribute('color', color)

        transform = random.choice(world.get_map().get_spawn_points())

        vehicle = world.spawn_actor(bp, transform)

        actor_list.append(vehicle)
        print('created %s' % vehicle.type_id)

        vehicle.set_autopilot(True)

        radio_bp = blueprint_library.find('sensor.other.radio')
        radio_transform = carla.Transform(carla.Location(x=1.5, z=2.4))
        radio = world.spawn_actor(radio_bp, radio_transform, attach_to=vehicle)
        actor_list.append(radio)
        print('created %s' % radio.type_id)

        cc = carla.ColorConverter.LogarithmicDepth
        
        radio.listen(lambda radio_detection: callback(radio_detection))

        location = vehicle.get_location()
        location.x += 40
        vehicle.set_location(location)
        print('moved vehicle to %s' % location)

        time.sleep(50)

    finally:

        print('destroying actors')
        radio.destroy()
        client.apply_batch([carla.command.DestroyActor(x) for x in actor_list])
        print('done.')


if __name__ == '__main__':

    main()
