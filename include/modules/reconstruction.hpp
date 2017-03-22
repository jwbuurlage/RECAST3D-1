#pragma once

#include "zmq.hpp"

#include "../serialize.hpp"
#include "scene.hpp"
#include "scene_list.hpp"
#include "scene_module.hpp"

#include "graphics/components/reconstruction_component.hpp"
#include "modules/packets/reconstruction_packets.hpp"

namespace tomovis {

// for the 'one-way-communication' we have two parts
// a handler that knows how to read in a packet
// and an executor that knows how to execute a packet

class ReconstructionProtocol : public SceneModuleProtocol {
   public:
    std::unique_ptr<Packet> read_packet(packet_desc desc, memory_buffer& buffer,
                                        zmq::socket_t& socket,
                                        SceneList& scenes_) override {
        switch (desc) {
            case packet_desc::slice_data: {
                auto packet = std::make_unique<SliceDataPacket>();
                packet->deserialize(std::move(buffer));

                zmq::message_t reply(sizeof(int));

                int success = 1;
                memcpy(reply.data(), &success, sizeof(int));
                socket.send(reply);

                return std::move(packet);
            }

            case packet_desc::volume_data: {
                auto packet = std::make_unique<VolumeDataPacket>();
                packet->deserialize(std::move(buffer));

                zmq::message_t reply(sizeof(int));

                int success = 1;
                memcpy(reply.data(), &success, sizeof(int));
                socket.send(reply);

                return std::move(packet);
            }

            default: { return nullptr; }
        }
    }

    void process(SceneList& scenes,
                 std::unique_ptr<Packet> event_packet) override {
        switch (event_packet->desc) {
            case packet_desc::slice_data: {
                SliceDataPacket& packet = *(SliceDataPacket*)event_packet.get();
                auto scene = scenes.get_scene(packet.scene_id);
                if (!scene) std::cout << "Updating non-existing scene\n";
                auto& reconstruction_component =
                    (ReconstructionComponent&)scene->object().get_component(
                        "reconstruction");
                reconstruction_component.set_size(packet.slice_size,
                                                  packet.slice_id);
                reconstruction_component.set_data(packet.data, packet.slice_id);
                break;
            }

            case packet_desc::volume_data: {
                VolumeDataPacket& packet =
                    *(VolumeDataPacket*)event_packet.get();
                auto scene = scenes.get_scene(packet.scene_id);
                if (!scene) std::cout << "Updating non-existing scene\n";
                auto& reconstruction_component =
                    (ReconstructionComponent&)scene->object().get_component(
                        "reconstruction");
                reconstruction_component.set_volume_data(packet.volume_size,
                                                         packet.data);
                break;
            }

            default: { break; }
        }
    }

    std::vector<packet_desc> descriptors() override {
        return {packet_desc::slice_data, packet_desc::volume_data};
    }
};

}  // namespace tomovis