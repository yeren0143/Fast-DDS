// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

/**
 * @file TypeIntrospectionSubscriber.cpp
 *
 */

#include <csignal>
#include <functional>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.h>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <fastdds/rtps/transport/UDPv6TransportDescriptor.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastrtps/types/DynamicDataHelper.hpp>
#include <fastrtps/types/DynamicDataFactory.h>
#include <fastrtps/types/TypeObjectFactory.h>
#include <fastrtps/types/TypesBase.h>

#include "TypeIntrospectionSubscriber.h"
#include "types.hpp"

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
using namespace eprosima::fastrtps::rtps;
using namespace eprosima::fastrtps;

std::atomic<bool> TypeIntrospectionSubscriber::type_discovered_(false);
std::atomic<bool> TypeIntrospectionSubscriber::type_registered_(false);
std::mutex TypeIntrospectionSubscriber::type_discovered_cv_mtx_;
std::condition_variable TypeIntrospectionSubscriber::type_discovered_cv_;
std::atomic<bool> TypeIntrospectionSubscriber::stop_(false);
std::mutex TypeIntrospectionSubscriber::terminate_cv_mtx_;
std::condition_variable TypeIntrospectionSubscriber::terminate_cv_;

TypeIntrospectionSubscriber::TypeIntrospectionSubscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
{
}

bool TypeIntrospectionSubscriber::is_stopped()
{
    return stop_;
}

void TypeIntrospectionSubscriber::stop()
{
    stop_ = true;

    type_discovered_cv_.notify_all();
    terminate_cv_.notify_all();
}

bool TypeIntrospectionSubscriber::init(
        const std::string& topic_name,
        uint32_t max_messages,
        uint32_t domain)
{
    DomainParticipantQos pqos;
    pqos.name("TypeIntrospectionExample_Participant_Subscriber");

    // Set to be used as a client type lookup
    pqos.wire_protocol().builtin.discovery_config.discoveryProtocol = SIMPLE;
    pqos.wire_protocol().builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;
    pqos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter = true;
    pqos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationWriterANDSubscriptionReader = true;
    pqos.wire_protocol().builtin.typelookup_config.use_client = true;
    pqos.wire_protocol().builtin.use_WriterLivelinessProtocol = false;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = c_TimeInfinite;

    // Create listener mask so the data do not go to on_data_on_readers from subscriber
    StatusMask mask;
    mask.any();
    mask << StatusMask::data_available();
    mask << StatusMask::subscription_matched();
    // No mask for type_information_received

    // CREATE THE PARTICIPANT
    participant_ = DomainParticipantFactory::get_instance()->create_participant(domain, pqos, this, mask);

    if (participant_ == nullptr)
    {
        return false;
    }

    // CREATE THE SUBSCRIBER
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

    if (subscriber_ == nullptr)
    {
        return false;
    }

    max_messages_ = max_messages;
    topic_name_ = topic_name;

    return true;
}

TypeIntrospectionSubscriber::~TypeIntrospectionSubscriber()
{
    if (participant_ != nullptr)
    {
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        if (subscriber_ != nullptr)
        {
            if (reader_ != nullptr)
            {
                subscriber_->delete_datareader(reader_);
            }
            participant_->delete_subscriber(subscriber_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void TypeIntrospectionSubscriber::on_type_information_received(
        eprosima::fastdds::dds::DomainParticipant*,
        const eprosima::fastrtps::string_255 topic_name,
        const eprosima::fastrtps::string_255 type_name,
        const eprosima::fastrtps::types::TypeInformation& type_information)
{
    // only one type at a time could be registered
    std::unique_lock<std::mutex> d_lck(type_discovered_cv_mtx_);

    // First, if type has already been created, do not create a new one:
    if (type_discovered_)
    {
        return;
    }

    // If it is not the topic set, skip it
    if (topic_name != topic_name_)
    {
        return;
    }

    // TOPIC FOUND
    // Create DDS Entities associated
    // Create registration callback to what happens when type has been registered
    std::function<void(const std::string&, const eprosima::fastrtps::types::DynamicType_ptr)> callback(
        [this]
        (const std::string& name, const eprosima::fastrtps::types::DynamicType_ptr type)
        {
            // std::unique_lock<std::mutex> d_lck(type_discovered_cv_mtx_);
            // NOTE: locking this mutex here creates a double block between on_type_information_received and this
            // "asynchronous" callback

            // Print type information
            std::cout << "Type for topic " << topic_name_ << " found to be < " << name << " > registered." << std::endl;
            std::cout << type << std::endl;

            // Create topic
            topic_ = participant_->create_topic(
                    topic_name_,
                    name,
                    TOPIC_QOS_DEFAULT);

            if (topic_ == nullptr)
            {
                return;
            }

            // Create DataReader
            reader_ = subscriber_->create_datareader(
                    topic_,
                    DATAREADER_QOS_DEFAULT,
                    this);

            // Store dynamic type so can be used to read data
            if (type == nullptr)
            {
                const types::TypeIdentifier* ident =
                        types::TypeObjectFactory::get_instance()->get_type_identifier_trying_complete(name);

                if (nullptr != ident)
                {
                    const types::TypeObject* obj =
                            types::TypeObjectFactory::get_instance()->get_type_object(ident);

                    this->dyn_type_ =
                            types::TypeObjectFactory::get_instance()->build_dynamic_type(name, ident, obj);

                    if (nullptr == dyn_type_)
                    {
                        std::cout << "ERROR: DynamicType cannot be created for type: " << name << std::endl;
                    }
                }
                else
                {
                    std::cout << "ERROR: TypeIdentifier cannot be retrieved for type: " << name << std::endl;
                }
            }
            else
            {
                this->dyn_type_ = type;
            }

            type_registered_.store(true);
            type_discovered_cv_.notify_all();
        });

    // Register type
    participant_->register_remote_type(
        type_information,
        type_name.to_string(),
        callback);

    type_name_ = type_name.to_string();

    type_discovered_.store(true);
    type_discovered_cv_.notify_all();
}

void TypeIntrospectionSubscriber::on_subscription_matched(
        DataReader*,
        const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = info.current_count;
        std::cout << "Subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.current_count;
        std::cout << "Subscriber unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void TypeIntrospectionSubscriber::on_data_available(
        DataReader* reader)
{
    // Dynamic DataType
    types::DynamicData_ptr new_data;
    new_data = types::DynamicDataFactory::get_instance()->create_data(dyn_type_);

    SampleInfo info;

    while ((reader->take_next_sample(new_data.get(), &info) == ReturnCode_t::RETCODE_OK) && !is_stopped())
    {
        if (info.instance_state == ALIVE_INSTANCE_STATE)
        {
            samples_++;

            std::cout << "Message number " << samples_ << " RECEIVED:\n";

            // Print structure data
            print_dynamic_data(new_data);

            std::cout << std::endl;

            // Stop if max messages has already been read
            if (max_messages_ > 0 && (samples_ >= max_messages_))
            {
                stop();
            }
        }
    }
}

void TypeIntrospectionSubscriber::on_type_discovery(
        eprosima::fastdds::dds::DomainParticipant* participant,
        const eprosima::fastrtps::rtps::SampleIdentity& request_sample_id,
        const eprosima::fastrtps::string_255& topic,
        const eprosima::fastrtps::types::TypeIdentifier* identifier,
        const eprosima::fastrtps::types::TypeObject* object,
        eprosima::fastrtps::types::DynamicType_ptr dyn_type)
{
    if (topic.to_string() != topic_name_)
    {
        std::cout << "Discovered Topic " << topic.to_string() << " . Skipping." << std::endl;
        return;
    }
    else
    {
        std::cout << "Received type from topic " << topic.to_string() << " with type: " << dyn_type << std::endl;
    }

    // Print type information
    std::cout << "Type for topic " << topic_name_ << " found to be < " << dyn_type->get_name() << " >." << std::endl;
    std::cout << "Registering type: " << dyn_type << std::endl;

    // Register type
    TypeSupport m_type(new eprosima::fastrtps::types::DynamicPubSubType(dyn_type));
    m_type.register_type(participant_);

    // Create topic
    topic_ = participant_->create_topic(
            topic_name_,
            dyn_type->get_name(),
            TOPIC_QOS_DEFAULT);

    if (topic_ == nullptr)
    {
        return;
    }

    // Create DataReader
    reader_ = subscriber_->create_datareader(
            topic_,
            DATAREADER_QOS_DEFAULT,
            this);

    type_discovered_.store(true);
    type_registered_.store(true);
    type_discovered_cv_.notify_all();
}

void TypeIntrospectionSubscriber::run(
        uint32_t samples)
{
    stop_ = false;

    signal(SIGINT, [](int signum)
            {
                std::cout << "SIGINT received, stopping Subscriber execution." << std::endl;
                static_cast<void>(signum);
                TypeIntrospectionSubscriber::stop();
            });

    // WAIT FOR TYPE DISCOVERY
    std::cout << "Subscriber waiting to discover type for topic " << topic_name_
        << " in domain " << participant_->get_domain_id()
        << " . Please press CTRL+C to stop the Subscriber." << std::endl;

    // Wait for type discovered
    {
        std::unique_lock<std::mutex> lck(type_discovered_cv_mtx_);
        type_discovered_cv_.wait(lck, []
                {
                    return is_stopped() || (type_discovered_.load() && type_registered_.load());
                });
    }

    if (is_stopped())
    {
        return;
    }

    std::cout << "Subscriber " << reader_->guid() << " listening for data in topic " << topic_name_
        << " with data type " << type_name_
        << " in domain " << participant_->get_domain_id() << ". ";

    // WAIT FOR SAMPLES READ
    if (samples > 0)
    {
        std::cout << "Running until " << samples <<
            " samples have been received. Please press CTRL+C to stop the Subscriber at any time." << std::endl;
    }
    else
    {
        std::cout << "Please press CTRL+C to stop the Subscriber." << std::endl;
    }

    // Wait for signal or thread max samples received
    {
        std::unique_lock<std::mutex> lck(terminate_cv_mtx_);
        terminate_cv_.wait(lck, []
                {
                    return is_stopped();
                });
    }
}
