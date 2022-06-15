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
 * @file TypeIntrospectionPublisher.h
 *
 */

#ifndef _EPROSIMA_FASTDDS_EXAMPLES_CPP_DDS_TYPEINTROSPECTIONEXAMPLE_TYPEINTROSPECTIONPUBLISHER_H_
#define _EPROSIMA_FASTDDS_EXAMPLES_CPP_DDS_TYPEINTROSPECTIONEXAMPLE_TYPEINTROSPECTIONPUBLISHER_H_

#include <atomic>
#include <condition_variable>
#include <mutex>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "types.hpp"

/**
 * Class used to group into a single working unit a Publisher with a DataWriter, its listener, and a TypeSupport member
 * corresponding to the HelloWorld datatype
 */
class TypeIntrospectionPublisher
{
public:

    TypeIntrospectionPublisher();

    virtual ~TypeIntrospectionPublisher();

    //! Initialize the publisher
    bool init(
            const std::string& topic_name,
            DataType data_type,
            uint32_t domain);

    /**
     * @brief Publish a sample
     *
     * This method creates each time a variable of type \c data_type_ , initializes it
     * depending on the \c msg_index and publishes it.
     */
    void publish(unsigned int msg_index);


    //! Run for number samples, publish every sleep seconds
    void run(
            uint32_t number,
            uint32_t sleep);

    //! Return the current state of execution
    static bool is_stopped();

    //! Trigger the end of execution
    static void stop();

private:

    // Write data depending on data type
    void publish_helloworld_(unsigned int msg_index);
    void publish_datapointxml_(unsigned int msg_index);

    // Register type depending on data type
    void register_helloworld_();
    void register_datapointxml_();

    DataType data_type_;

    eprosima::fastdds::dds::DomainParticipant* participant_;

    eprosima::fastdds::dds::Publisher* publisher_;

    eprosima::fastdds::dds::Topic* topic_;

    eprosima::fastdds::dds::DataWriter* writer_;

    eprosima::fastdds::dds::TypeSupport type_;

    /**
     * Class handling discovery events and dataflow
     */
    class PubListener : public eprosima::fastdds::dds::DataWriterListener
    {
    public:

        PubListener()
        {
        }

        ~PubListener() override
        {
        }

        //! Callback executed when a DataReader is matched or unmatched
        void on_publication_matched(
                eprosima::fastdds::dds::DataWriter* writer,
                const eprosima::fastdds::dds::PublicationMatchedStatus& info) override;
    }
    listener_;

    //! Run thread for number samples, publish every sleep seconds
    void runThread(
            uint32_t number,
            uint32_t sleep);

    //! Member used for control flow purposes
    static std::atomic<bool> stop_;

    // ONLY FOR DATAPOINTXML
    eprosima::fastrtps::types::DynamicType_ptr dyn_type_;
};



#endif /* _EPROSIMA_FASTDDS_EXAMPLES_CPP_DDS_TYPEINTROSPECTIONEXAMPLE_TYPEINTROSPECTIONPUBLISHER_H_ */
