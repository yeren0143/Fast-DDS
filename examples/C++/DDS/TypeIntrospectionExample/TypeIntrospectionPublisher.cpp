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
 * @file TypeIntrospectionPublisher.cpp
 *
 */

#include <csignal>
#include <thread>

#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.h>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <fastdds/rtps/transport/UDPv6TransportDescriptor.h>
#include <fastdds/rtps/common/Time_t.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastrtps/xmlparser/XMLProfileManager.h>
#include <fastrtps/types/DynamicPubSubType.h>
#include <fastrtps/types/DynamicDataPtr.h>
#include <fastrtps/types/DynamicDataFactory.h>

#include "example_types/HelloWorld/HelloWorldPubSubTypes.h"
#include "TypeIntrospectionPublisher.h"

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
using namespace eprosima::fastrtps::rtps;
using namespace eprosima::fastrtps;

std::atomic<bool> TypeIntrospectionPublisher::stop_(false);

TypeIntrospectionPublisher::TypeIntrospectionPublisher()
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
{
}

bool TypeIntrospectionPublisher::is_stopped()
{
    return stop_;
}

void TypeIntrospectionPublisher::stop()
{
    stop_ = true;
}

bool TypeIntrospectionPublisher::init(
        const std::string& topic_name,
        DataType data_type,
        uint32_t domain)
{
    DomainParticipantQos pqos;
    pqos.name("TypeIntrospectionExample_Participant_Publisher");

    // Set to be used as a type lookup server
    pqos.wire_protocol().builtin.discovery_config.discoveryProtocol = SIMPLE;
    pqos.wire_protocol().builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;
    pqos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter = true;
    pqos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationWriterANDSubscriptionReader = true;
    pqos.wire_protocol().builtin.typelookup_config.use_server = true;
    pqos.wire_protocol().builtin.use_WriterLivelinessProtocol = false;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = c_TimeInfinite;

    // CREATE THE PARTICIPANT
    participant_ = DomainParticipantFactory::get_instance()->create_participant(domain, pqos);

    if (participant_ == nullptr)
    {
        return false;
    }

    // REGISTER THE TYPE
    switch (data_type)
    {
    case DataType::HELLOWORLD :
        register_helloworld_();
        break;

    case DataType::DATA_POINT_XML :
        register_datapointxml_();
        break;

    default:
        return false;
    }
    // Store data type
    data_type_ = data_type;

    // WORKAROUND: a data of this type must be created so the dynamic type is registered
    {
        type_.create_data();
    }

    // Send information about the type
    type_.get()->auto_fill_type_information(true);
    type_.get()->auto_fill_type_object(true);

    // CREATE THE PUBLISHER
    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

    if (publisher_ == nullptr)
    {
        return false;
    }

    // CREATE THE TOPIC
    topic_ = participant_->create_topic(topic_name, type_.get_type_name(), TOPIC_QOS_DEFAULT);

    if (topic_ == nullptr)
    {
        return false;
    }

    // CREATE THE WRITER
    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;
    wqos.durability().kind = eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS;

    writer_ = publisher_->create_datawriter(topic_, wqos, &listener_);

    if (writer_ == nullptr)
    {
        return false;
    }
    return true;
}

TypeIntrospectionPublisher::~TypeIntrospectionPublisher()
{
    if (participant_ != nullptr)
    {
        if (publisher_ != nullptr)
        {
            if (writer_ != nullptr)
            {
                publisher_->delete_datawriter(writer_);
            }
            participant_->delete_publisher(publisher_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void TypeIntrospectionPublisher::PubListener::on_publication_matched(
        eprosima::fastdds::dds::DataWriter*,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        std::cout << "Publisher matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        std::cout << "Publisher unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}

void TypeIntrospectionPublisher::runThread(
        uint32_t samples,
        uint32_t sleep)
{
    unsigned int samples_sent = 0;
    while (!is_stopped() && (samples == 0 || samples_sent < samples))
    {
        publish(samples_sent);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
        ++samples_sent;
    }
}

void TypeIntrospectionPublisher::run(
        uint32_t samples,
        uint32_t sleep)
{
    stop_ = false;
    std::thread thread(&TypeIntrospectionPublisher::runThread, this, samples, sleep);
    if (samples == 0)
    {
        std::cout << "Publisher " << writer_->guid() << " running in topic " << topic_->get_name()
            << " with data type < " << type_.get_type_name() << " > in domain " << participant_->get_domain_id()
            << ". Please press CTRL+C to stop the Publisher at any time." << std::endl;
    }
    else
    {
        std::cout << "Publisher running " << samples <<
            " samples. Please press CTRL+C to stop the Publisher at any time." << std::endl;
    }
    signal(SIGINT, [](int signum)
            {
                std::cout << "SIGINT received, stopping Publisher execution." << std::endl;
                static_cast<void>(signum);
                TypeIntrospectionPublisher::stop();
            });
    thread.join();
}

void TypeIntrospectionPublisher::publish(unsigned int msg_index)
{
    switch (data_type_)
    {
    case DataType::HELLOWORLD :
        publish_helloworld_(msg_index);
        break;

    case DataType::DATA_POINT_XML :
        publish_datapointxml_(msg_index);
        break;

    default:
        break;
    }
}

void TypeIntrospectionPublisher::register_helloworld_()
{
    // Create new PubSub Type for TypeSupport
    type_ = eprosima::fastdds::dds::TypeSupport(new HelloWorldPubSubType());

    // Register type in participant
    participant_->register_type(type_);

    std::cout << "Register < " << type_.get_type_name() << " > data type, get from autogenerted files "
        << "from an IDL file" << std::endl;
}

void TypeIntrospectionPublisher::register_datapointxml_()
{
    // Load XML
    if (eprosima::fastrtps::xmlparser::XMLP_ret::XML_OK !=
        eprosima::fastrtps::xmlparser::XMLProfileManager::loadXMLFile("data_point.xml"))
    {
        std::cout << "Cannot open XML file \"data_point.xml\". Please, run the publisher from the folder "
                  << "that contains this XML file." << std::endl;
        return;
    }

    // Create Dynamic data
    dyn_type_ = xmlparser::XMLProfileManager::getDynamicTypeByName("DataPointXml")->build();
    type_ = eprosima::fastdds::dds::TypeSupport(new types::DynamicPubSubType(dyn_type_));

    // Register type in participant
    type_.register_type(participant_);

    std::cout << "Register DataPointXml Data Type, get from XML file data_point.xml" << std::endl;
}

void TypeIntrospectionPublisher::publish_helloworld_(unsigned int msg_index)
{
    // Create and initialize new data
    HelloWorld new_data;
    new_data.index(msg_index);
    memcpy(new_data.message().data(), "HelloWorld ", strlen("HelloWorld") + 1);
    new_data.message()[strlen("HelloWorld")] = 0; // Set last char as 0

    writer_->write((void*)&new_data);

    std::cout << "Message of type " << type_.get_type_name() << " sent with values:\n"
              << " - index: " << new_data.index() << "\n"
              << " - message: " << std::string(new_data.message().begin())
              << std::endl;
}

void TypeIntrospectionPublisher::publish_datapointxml_(unsigned int msg_index)
{
    // Create and initialize new data
    eprosima::fastrtps::types::DynamicData_ptr new_data;
    new_data = eprosima::fastrtps::types::DynamicDataFactory::get_instance()->create_data(dyn_type_);

    // Set index
    new_data->set_int32_value(msg_index, 0);
    // X -> Add 0.1
    new_data->set_float64_value(msg_index + 0.1, 1);
    // Y -> Multiply by 0.5
    new_data->set_float64_value(msg_index * 0.5, 2);

    writer_->write(new_data.get());

    std::cout << "Message of type " << type_.get_type_name() << " sent with values:\n"
              << " - index: " << new_data->get_int32_value(0) << "\n"
              << " - x: " << new_data->get_float64_value(1) << "\n"
              << " - y: " << new_data->get_float64_value(2)
              << std::endl;
}
